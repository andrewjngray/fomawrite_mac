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

#include "backend.h"
#include "systemtheme.h"
#ifdef Q_OS_MACOS
void configureMacWindowChrome(QWindow *window);
#endif

// Finder delivers documents as events, rather than command-line arguments.
class WriterApplication : public QApplication {
public:
    using QApplication::QApplication;
    std::function<void(const QUrl &)> openDocument;
    QList<QUrl> pendingDocuments;

    bool event(QEvent *event) override {
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
    };
    QList<std::shared_ptr<Session>> sessions;
    bool quitting = false;
    std::function<void()> closeNext;
    closeNext = [&] {
        if (!quitting) return;
        for (const auto &session : sessions) {
            if (session->window) {
                session->window->show();
                session->window->raise();
                session->window->requestActivate();
                session->window->close();
                return;
            }
        }
        app.quit();
    };
    const auto focusExisting = [&](const QUrl &url, Backend *except) {
        const auto path = QFileInfo(url.toLocalFile()).canonicalFilePath();
        if (path.isEmpty()) return false;
        for (const auto &session : sessions) {
            if (session->backend && session->backend != except && session->window
                && QFileInfo(session->backend->fileUrl().toLocalFile()).canonicalFilePath() == path) {
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
        backend->setDarkMode(systemTheme.darkMode());
        backend->setTextScale(systemTheme.textScale());
        backend->focusExistingDocument = [&, backend](const QUrl &target) { return focusExisting(target, backend); };
        QObject::connect(&systemTheme, &SystemTheme::darkModeChanged, backend, &Backend::setDarkMode);
        QObject::connect(&systemTheme, &SystemTheme::textScaleChanged, backend, &Backend::setTextScale);
        QObject::connect(backend, &Backend::newWindowRequested, &app, createWindow);
        QObject::connect(backend, &Backend::quitRequested, &app, [&] { quitting = true; closeNext(); });
        QObject::connect(backend, &Backend::quitCanceled, &app, [&] { quitting = false; });
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
#endif
        sessions.append(session);
        QObject::connect(backend, &Backend::windowClosed, &app, [&, session] {
            QTimer::singleShot(0, &app, [&, session] {
                sessions.removeAll(session);
                if (session->engine) session->engine->deleteLater();
                if (session->backend) session->backend->deleteLater();
                if (quitting) closeNext();
            });
        });
        if (!url.isEmpty() && !backend->modified()) backend->open(url);
    };
    QObject::connect(&systemTheme, &SystemTheme::textScaleChanged, &app, applyInterfaceFont);
    app.openDocument = createWindow;
    const QStringList args = app.arguments();
    createWindow(args.size() > 1 ? QUrl::fromLocalFile(args.at(1)) : QUrl());
    for (const QUrl &url : app.pendingDocuments) createWindow(url);
    app.pendingDocuments.clear();

    return app.exec();
}
