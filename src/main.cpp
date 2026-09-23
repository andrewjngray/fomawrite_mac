#include <QFont>
#include <QFontDatabase>
#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QUrl>
#include <QWindow>
#include <QFile>
#include <QFileOpenEvent>
#include <functional>
#include <QPointer>
#include <QTimer>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QScreen>
#include "workspace.h"

#include "backend.h"
#include "systemtheme.h"
#ifdef Q_OS_MACOS
void configureMacWindowChrome(QWindow *window);
void applyMacWindowTheme(QWindow *window, bool followSystem, bool dark);
QVariantMap macWorkspaceState(QWindow *window);
void restoreMacWorkspaceTabs(const QList<QWindow *> &windows);
#endif

// Finder delivers documents as events, rather than command-line arguments.
class WriterApplication : public QApplication {
public:
    using QApplication::QApplication;
    std::function<void(const QUrl &)> openDocument;
    QList<QUrl> pendingDocuments;
    std::function<void()> guardedQuit;
    bool allowExit = false;

    bool event(QEvent *event) override {
        if (event->type() == QEvent::Quit && guardedQuit && !allowExit) { guardedQuit(); event->ignore(); return true; }
        if (event->type() == QEvent::FileOpen) {
            const QUrl url = static_cast<QFileOpenEvent *>(event)->url();
            if (openDocument)
                openDocument(url);
            else
                pendingDocuments.append(url);
            return true;
        }
        return QApplication::event(event);
    }
};

int main(int argc, char *argv[]) {
    WriterApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("omawrite"));
    app.setDesktopFileName(QStringLiteral("omawrite"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("omawrite")));

    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/iAWriterMonoS-Regular.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/iAWriterMonoS-Italic.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/iAWriterMonoS-Bold.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/iAWriterMonoS-BoldItalic.ttf"));
    app.setOrganizationName(QStringLiteral("AndrewGray"));
    app.setOrganizationDomain(QStringLiteral("andrewjngray.github.io"));
    app.setApplicationDisplayName(QStringLiteral("Omawrite Mac"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));

    QQuickStyle::setStyle(QStringLiteral("Material"));

    const QString identity = QString::fromLatin1(QCryptographicHash::hash(
        (QDir::homePath() + QCoreApplication::applicationFilePath()).toUtf8(), QCryptographicHash::Sha256).toHex().left(20));
    const QString stateDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(stateDirectory);
    const QString workspacePath = QDir(stateDirectory).filePath("workspace-" + identity + ".json");
    QStringList launchPaths;
    for (const auto &arg : app.arguments().mid(1))
        if (!arg.startsWith('-')) launchPaths.append(QFileInfo(arg).absoluteFilePath());
    InstanceBroker broker;
    const auto ownership = broker.start(QDir::tempPath(), "omawrite-" + identity, launchPaths);
    if (ownership == InstanceBroker::Forwarded) return 0;
    if (ownership == InstanceBroker::Failed) {
        QMessageBox::critical(nullptr, "Omawrite", broker.error()); return 1;
    }
    app.setQuitOnLastWindowClosed(false);
    SystemTheme systemTheme(&app);

    // Carry the desktop's text scale into the default font, so the chrome that
    // inherits it (dialog titles, buttons) grows along with the writing area.
#ifdef Q_OS_MAC
    const QFont interfaceFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
#else
    const QFont interfaceFont(QStringLiteral("iA Writer Mono S"));
#endif
    const qreal basePointSize = interfaceFont.pointSizeF() > 0
        ? interfaceFont.pointSizeF()
        : app.font().pointSizeF();
    const auto applyInterfaceFont = [&app, interfaceFont, basePointSize](qreal textScale) {
        QFont scaled = interfaceFont;
        scaled.setPointSizeF(basePointSize * textScale);
        app.setFont(scaled);
    };
    applyInterfaceFont(systemTheme.textScale());

    struct Session {
        QPointer<Backend> backend;
        QPointer<QQmlApplicationEngine> engine;
        QPointer<QWindow> window;
        QJsonObject checkpoint;
    };
    QList<std::shared_ptr<Session>> sessions;
    QPointer<QWindow> lastActiveWindow;
    bool quitting = false;
    bool committingQuit = false;
    QMap<Backend *, int> preparedForQuit;
    QJsonArray quitWorkspace;
    const auto capture = [&] {
        QJsonArray windows;
        for (const auto &session : sessions) {
            if (!session->window || !session->backend) continue;
            auto *w = session->window.data();
            QJsonObject entry{{"url", session->backend->fileUrl().toString()},
                {"cursor", w->property("workspaceCursor").toInt()},
                {"x", w->x()}, {"y", w->y()}, {"width", w->width()}, {"height", w->height()},
                {"active", lastActiveWindow == w},
                {"root", session->backend->library()->property("rootFolder").toUrl().toString()}};
#ifdef Q_OS_MACOS
            const auto native = macWorkspaceState(w);
            entry["group"] = native.value("group").toString();
            entry["order"] = native.value("order").toInt();
#endif
            session->checkpoint = entry;
            windows.append(entry);
        }
        return windows;
    };
    const auto persist = [&] {
        if (!WorkspaceStore::write(workspacePath, quitting ? quitWorkspace : capture()))
            qWarning("Unable to save workspace arrangement");
    };
    QTimer checkpointTimer;
    checkpointTimer.setInterval(1000);
    QObject::connect(&checkpointTimer, &QTimer::timeout, &app, [&] { if (!quitting) persist(); });
    std::function<void()> closeNext;
    closeNext = [&] {
        if (!quitting) return;
        if (committingQuit) {
            if (sessions.isEmpty()) { app.allowExit = true; app.quit(); }
            return;
        }
        for (const auto &session : sessions) {
            if (!session->window || !session->backend) continue;
            if (!preparedForQuit.contains(session->backend)
                || preparedForQuit.value(session->backend) != session->backend->documentRevision()) {
                session->window->show(); session->window->raise(); session->window->requestActivate();
                QMetaObject::invokeMethod(session->window, "prepareQuit");
                return;
            }
        }
        // Every document has approved this exact revision. Close synchronously
        // only now, so Cancel at a later prompt leaves earlier windows intact.
        committingQuit = true;
        for (const auto &session : sessions)
            if (session->window) QMetaObject::invokeMethod(session->window, "commitQuit");
        if (sessions.isEmpty()) { app.allowExit = true; app.quit(); }
    };
    app.guardedQuit = [&] { if (quitting) return; preparedForQuit.clear(); committingQuit = false; quitWorkspace = capture(); quitting = true; persist(); closeNext(); };
    const auto focusExisting = [&](const QUrl &url, Backend *except) {
        const auto path = QFileInfo(url.toLocalFile()).canonicalFilePath();
        if (path.isEmpty()) return false;
        for (const auto &session : sessions) {
            if (session->backend && session->backend != except && session->window
                && QFileInfo(session->backend->fileUrl().toLocalFile()).canonicalFilePath() == path) {
                if (session->window->windowState() == Qt::WindowMinimized) session->window->showNormal();
                session->window->raise(); session->window->requestActivate(); return true;
            }
        }
        return false;
    };
    std::function<void(const QUrl &)> createWindow;
    createWindow = [&](const QUrl &url) {
        if (!url.isEmpty() && focusExisting(url, nullptr)) return;
        auto session = std::make_shared<Session>();
        session->backend = new Backend(&app);
        auto *backend = session->backend.data();
        QObject::connect(backend, &Backend::themePresetChanged, &app, [&, backend] {
            for (const auto &other : sessions)
                if (other->backend && other->backend != backend) other->backend->setThemePreset(backend->themePreset());
        });
        backend->setDarkMode(systemTheme.darkMode());
        backend->setTextScale(systemTheme.textScale());
        backend->focusExistingDocument = [&, backend](const QUrl &target) { return focusExisting(target, backend); };
        QObject::connect(&systemTheme, &SystemTheme::darkModeChanged, backend, &Backend::setDarkMode);
        QObject::connect(&systemTheme, &SystemTheme::textScaleChanged, backend, &Backend::setTextScale);
        QObject::connect(backend, &Backend::newWindowRequested, &app, createWindow);
        QObject::connect(backend, &Backend::quitReady, &app, [&, backend] {
            if (!quitting) return;
            preparedForQuit[backend] = backend->documentRevision();
            QTimer::singleShot(0, &app, closeNext);
        });
        QObject::connect(backend, &Backend::quitRequested, &app, [&] { app.guardedQuit(); });
        QObject::connect(backend, &Backend::quitCanceled, &app, [&] { quitting = false; persist(); });
        session->engine = new QQmlApplicationEngine(&app);
        session->engine->rootContext()->setContextProperty(QStringLiteral("backend"), backend);
        session->engine->load(QUrl(QStringLiteral("qrc:/Main.qml")));
        if (session->engine->rootObjects().isEmpty()) {
            session->engine->deleteLater(); backend->deleteLater(); return;
        }
        session->window = qobject_cast<QWindow *>(session->engine->rootObjects().constFirst());
        backend->setParentWindow(session->window);
#ifdef Q_OS_MACOS
        configureMacWindowChrome(session->window);
        const auto applyWindowTheme = [session] {
            if (session->window && session->backend)
                applyMacWindowTheme(session->window, session->backend->themePreset() == "system", session->backend->darkMode());
        };
        QObject::connect(backend, &Backend::themeColorsChanged, session->window, applyWindowTheme);
        applyWindowTheme();
#endif
        sessions.append(session);
        QObject::connect(session->window, &QWindow::activeChanged, &app, [&, session] {
            if (session->window && session->window->isActive()) lastActiveWindow = session->window;
        });
        if (session->window->isActive()) lastActiveWindow = session->window;
        QObject::connect(backend, &Backend::windowClosed, &app, [&, session] {
            // Save As during a quit prompt must update the persisted document URL.
            if (quitting) {
                for (int i = 0; i < quitWorkspace.size(); ++i) {
                    if (quitWorkspace[i].toObject() == session->checkpoint) {
                        auto entry = session->checkpoint;
                        entry["url"] = session->backend->fileUrl().toString();
                        quitWorkspace[i] = entry; break;
                    }
                }
            }
            QTimer::singleShot(0, &app, [&, session] {
                sessions.removeAll(session);
                if (session->engine) session->engine->deleteLater();
                if (session->backend) session->backend->deleteLater();
                persist();
                if (quitting) closeNext();
                else if (sessions.isEmpty()) { app.allowExit = true; app.quit(); }
            });
        });
        if (!url.isEmpty() && !backend->modified()) backend->open(url);
        // Never let an orphan recovery snapshot swallow an explicit open request.
        if (!url.isEmpty() && backend->modified() && backend->fileUrl() != url) createWindow(url);
    };
    QObject::connect(&systemTheme, &SystemTheme::textScaleChanged, &app, applyInterfaceFont);
    app.openDocument = createWindow;
    const auto savedWorkspace = WorkspaceStore::read(workspacePath);
    // Drain orphan recoveries first. Each backend locks its own snapshot; no dirty
    // recovered buffer is replaced by a saved-session entry.
    do { createWindow(QUrl()); } while (sessions.size() < 100 && !sessions.isEmpty() && sessions.last()->backend->modified());
    auto spare = sessions.isEmpty() ? std::shared_ptr<Session>() : sessions.last();
    if (spare && spare->backend->modified()) spare.reset();
    QMap<QString, QMap<int, QWindow *>> tabGroups;
    QPointer<QWindow> activeWindow;
    for (const auto &value : savedWorkspace) {
        const auto entry = value.toObject();
        const QUrl url(entry["url"].toString());
        if (!url.isLocalFile() || !QFileInfo(url.toLocalFile()).isFile()) continue;
        std::shared_ptr<Session> restored;
        for (const auto &session : sessions)
            if (QFileInfo(session->backend->fileUrl().toLocalFile()).canonicalFilePath() == QFileInfo(url.toLocalFile()).canonicalFilePath()) { restored = session; break; }
        if (!restored) {
            if (spare) { restored = spare; spare.reset(); restored->backend->open(url); }
            else { createWindow(url); restored = sessions.last(); }
        }
        const QUrl root(entry["root"].toString());
        if (root.isLocalFile() && QFileInfo(root.toLocalFile()).isDir()) restored->backend->library()->setProperty("rootFolder", root);
        auto *w = restored->window.data();
        const QRect available = w->screen()->availableGeometry();
        const int width = qBound(w->minimumWidth(), entry["width"].toInt(1100), qMax(w->minimumWidth(), available.width()));
        const int height = qBound(w->minimumHeight(), entry["height"].toInt(720), qMax(w->minimumHeight(), available.height()));
        w->setGeometry(qBound(available.left(), entry["x"].toInt(), qMax(available.left(), available.right()-width+1)),
                       qBound(available.top(), entry["y"].toInt(), qMax(available.top(), available.bottom()-height+1)), width, height);
        QMetaObject::invokeMethod(w, "restoreWorkspaceCursor", Q_ARG(QVariant, entry["cursor"].toInt()));
        if (entry["active"].toBool()) activeWindow = w;
        const QString group = entry["group"].toString();
        if (!group.isEmpty()) tabGroups[group][entry["order"].toInt()] = w;
    }
#ifdef Q_OS_MACOS
    for (const auto &group : tabGroups) restoreMacWorkspaceTabs(group.values());
#endif
    if (activeWindow) { lastActiveWindow = activeWindow; activeWindow->raise(); activeWindow->requestActivate(); }
    for (const auto &path : launchPaths) {
        const auto url = QUrl::fromLocalFile(path);
        if (spare && !focusExisting(url, nullptr)) { spare->backend->open(url); spare.reset(); }
        else createWindow(url);
    }
    if (spare && sessions.size() > 1) spare->window->close();
    QObject::connect(&broker, &InstanceBroker::requested, &app, [&](const QStringList &paths) {
        // A new launch is new work: stop a pending quit sequence.
        quitting = false;
        if (paths.isEmpty() && !sessions.isEmpty()) {
            auto *w = lastActiveWindow ? lastActiveWindow.data() : sessions.last()->window.data();
            if (w->windowState() == Qt::WindowMinimized) w->showNormal();
            w->raise(); w->requestActivate();
        }
        for (const auto &path : paths) createWindow(QUrl::fromLocalFile(path));
    });
    checkpointTimer.start();
    for (const QUrl &url : app.pendingDocuments) createWindow(url);
    app.pendingDocuments.clear();

    const int result = app.exec();
    // QApplication outlives these captured locals. Disconnect callbacks before
    // its child windows/backends are destroyed during application teardown.
    for (const auto &session : sessions) {
        if (session->window) QObject::disconnect(session->window, nullptr, &app, nullptr);
        if (session->backend) QObject::disconnect(session->backend, nullptr, &app, nullptr);
    }
    app.openDocument = {};
    app.guardedQuit = {};
    return result;
}
