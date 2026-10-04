#include "documentviewcheck.h"

#include "backend.h"
#include "systemtheme.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QUuid>
#include <functional>

#ifdef Q_OS_MACOS
void configureMacWindowChrome(QWindow *window);
void applyMacWindowTheme(QWindow *window, bool followSystem, bool dark);
#endif

namespace {
const QStringList viewNames{QStringLiteral("visualEditToggle"), QStringLiteral("sourceModeButton"),
                           QStringLiteral("previewSplitButton"), QStringLiteral("previewFullButton")};
const QStringList controlNames = viewNames + QStringList{QStringLiteral("sourceAppearanceButton"),
    QStringLiteral("previewTemplateButton"), QStringLiteral("footerStylesButton")};

bool waitUntil(const std::function<bool()> &condition, int timeout = 3000) {
    QElapsedTimer timer;
    timer.start();
    do {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        if (condition()) return true;
        QThread::msleep(2);
    } while (timer.elapsed() < timeout);
    return condition();
}

void settle(int milliseconds = 60) {
    QElapsedTimer timer;
    timer.start();
    waitUntil([&] { return timer.elapsed() >= milliseconds; }, milliseconds + 100);
}

QString sha256(const QByteArray &bytes) {
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

QString fileHash(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) return {};
    return QString::fromLatin1(hash.result().toHex());
}

QRectF bounds(QQuickItem *item) {
    return item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));
}

QJsonObject rectJson(const QRectF &rect) {
    return {{"x", rect.x()}, {"y", rect.y()}, {"width", rect.width()}, {"height", rect.height()}};
}

// The check must never consume real typing or pointer input from a person using
// another application. sendEvent below creates non-spontaneous Qt events.
class InputIsolation final : public QObject {
public:
    InputIsolation(QApplication &app, QQuickWindow *window) : m_app(app), m_window(window) {
        m_app.installEventFilter(this);
    }
    ~InputIsolation() override { m_app.removeEventFilter(this); }
protected:
    bool eventFilter(QObject *target, QEvent *event) override {
        const auto *item = qobject_cast<QQuickItem *>(target);
        if (target != m_window && (!item || item->window() != m_window)) return false;
        if (event->type() == QEvent::InputMethod) return true;
        if (!event->spontaneous()) return false;
        switch (event->type()) {
        case QEvent::KeyPress: case QEvent::KeyRelease: case QEvent::ShortcutOverride:
        case QEvent::InputMethod: case QEvent::MouseButtonPress: case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick: case QEvent::MouseMove: case QEvent::Wheel:
        case QEvent::TouchBegin: case QEvent::TouchUpdate: case QEvent::TouchEnd:
            return true;
        default: return false;
        }
    }
private:
    QApplication &m_app;
    QQuickWindow *m_window;
};

bool click(QQuickWindow *window, QQuickItem *button, bool allowDisabled = false) {
    settle(30); // Let RowLayout and the text labels finish their deferred polish.
    if (!button || !button->isVisible() || (!allowDisabled && !button->isEnabled())) return false;
    const QPointF point = bounds(button).center();
    if (!QRectF(QPointF(), window->size()).contains(point)) return false;
    const QPointF global = window->mapToGlobal(point.toPoint());
    QMouseEvent press(QEvent::MouseButtonPress, point, point, global,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, point, point, global,
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
    return true;
}

// Qt has no AppDataLocation setter. A unique, previously absent test-mode
// namespace points into the temporary directory so the real recovery/storage
// code runs unchanged. Only the link we created is removed, never a user folder.
class TemporaryDataLink final {
public:
    bool create(const QString &target) {
        m_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (m_path.isEmpty() || QFileInfo::exists(m_path) || QFileInfo(m_path).isSymLink()) return false;
        m_parent = QFileInfo(m_path).absolutePath();
        if (!QDir().mkpath(target) || !QDir().mkpath(m_parent)) return false;
        m_created = QFile::link(target, m_path);
        return m_created && QFileInfo(m_path).canonicalFilePath() == QFileInfo(target).canonicalFilePath();
    }
    ~TemporaryDataLink() {
        if (m_created) {
            QFile::remove(m_path);
            QDir().rmdir(m_parent); // Non-recursive: succeeds only if empty.
        }
    }
    QString path() const { return m_path; }
private:
    QString m_path;
    QString m_parent;
    bool m_created = false;
};
}

int runDocumentViewCheck(QApplication &app, const QString &outputDirectory) {
    const QDir output(QFileInfo(outputDirectory).absoluteFilePath());
    if (outputDirectory.isEmpty() || !QDir().mkpath(output.absolutePath())) {
        qCritical("Document view check needs a writable output directory.");
        return 2;
    }
    QStringList failures;
    QJsonArray scenarios;
    QJsonArray screenshots;
    QJsonArray qmlWarnings;
    QJsonObject report{{"schema", 1}, {"startedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
                       {"applicationVersion", app.applicationVersion()},
                       {"executable", QFileInfo(app.applicationFilePath()).canonicalFilePath()},
                       {"executableSha256", fileHash(app.applicationFilePath())},
                       {"platform", QGuiApplication::platformName()},
                       {"qmlEntry", "qrc:/Main.qml"},
                       {"input", "Synthetic Qt window mouse events; native input rejected"}};
    const auto check = [&](bool condition, const QString &message) {
        if (!condition) failures.append(message);
        return condition;
    };
    const auto finish = [&] {
        if (!qmlWarnings.isEmpty()) failures.append("The bundled QML emitted engine warnings; see qmlWarnings");
        report.insert("passed", failures.isEmpty());
        report.insert("failures", QJsonArray::fromStringList(failures));
        report.insert("scenarios", scenarios);
        report.insert("screenshots", screenshots);
        report.insert("qmlWarnings", qmlWarnings);
        report.insert("finishedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        QSaveFile file(output.filePath(QStringLiteral("report.json")));
        const QByteArray json = QJsonDocument(report).toJson(QJsonDocument::Indented);
        if (!file.open(QIODevice::WriteOnly) || file.write(json) != json.size() || !file.commit()) {
            qCritical("Cannot write report.json.");
            return 2;
        }
        qInfo().noquote() << "Document view check:" << (failures.isEmpty() ? "PASS" : "FAIL")
                          << scenarios.size() << "states; report:" << file.fileName();
        for (const QString &failure : failures) qWarning().noquote() << failure;
        return failures.isEmpty() ? 0 : 1;
    };
    QJsonObject resources;
    for (const QString &name : {QStringLiteral("Main.qml"), QStringLiteral("DocumentFooter.qml"),
            QStringLiteral("FooterButton.qml"), QStringLiteral("PreviewPane.qml"),
            QStringLiteral("WorkspaceLayout.qml"), QStringLiteral("WorkspaceCommands.qml"), QStringLiteral("AboutDialog.qml")}) {
        const QString hash = fileHash(":/" + name);
        check(!hash.isEmpty(), "Bundled resource missing: " + name);
        resources.insert(name, hash);
    }
    report.insert("resourceSha256", resources);

    QTemporaryDir temporary(QDir::tempPath() + "/fomawrite-view-check-XXXXXX");
    if (!check(temporary.isValid(), "Cannot create disposable fixture directory")) return finish();
    const QString identity = "FomawriteViewCheck-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QStandardPaths::setTestModeEnabled(true);
    app.setOrganizationName(identity);
    app.setOrganizationDomain(QStringLiteral("invalid.fomawrite-self-check"));
    app.setApplicationName(identity);
    app.setApplicationDisplayName(QStringLiteral("Fomawrite — document view self-check"));
    app.setQuitOnLastWindowClosed(false);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.filePath("settings"));
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, temporary.filePath("system-settings"));
    TemporaryDataLink isolatedData;
    if (!check(isolatedData.create(temporary.filePath("data")), "Cannot isolate AppDataLocation in disposable data directory"))
        return finish();
    report.insert("isolation", QJsonObject{{"temporaryRoot", temporary.path()},
        {"settingsFile", QSettings().fileName()}, {"dataLocation", isolatedData.path()},
        {"dataTarget", QFileInfo(isolatedData.path()).canonicalFilePath()},
        {"uniqueApplicationName", identity}, {"preferencesMigration", false}, {"ipc", false}});
    QQuickStyle::setStyle(QStringLiteral("Material"));
    SystemTheme systemTheme;
#ifdef Q_OS_MACOS
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
#else
    QFont font(QStringLiteral("iA Writer Mono S"));
#endif
    font.setPointSizeF((font.pointSizeF() > 0 ? font.pointSizeF() : app.font().pointSizeF()) * systemTheme.textScale());
    app.setFont(font);
    Backend backend;
    backend.setDarkMode(systemTheme.darkMode());
    backend.setTextScale(systemTheme.textScale());
    QObject::connect(&systemTheme, &SystemTheme::darkModeChanged, &backend, &Backend::setDarkMode);
    QObject::connect(&systemTheme, &SystemTheme::textScaleChanged, &backend, &Backend::setTextScale);
    qputenv("QML_DISABLE_DISK_CACHE", "1");
    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlEngine::warnings, &engine, [&](const QList<QQmlError> &warnings) {
        for (const auto &warning : warnings) qmlWarnings.append(warning.toString());
    });
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
    engine.rootContext()->setContextProperty(QStringLiteral("referenceWorkspaceUpgrade"), false);
    engine.setInitialProperties({{QStringLiteral("visible"), false}});
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    auto *window = engine.rootObjects().isEmpty() ? nullptr
        : qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (!check(window, "Shipped qrc:/Main.qml did not create a QQuickWindow")) return finish();
#ifdef Q_OS_MACOS
    // The native bridge dereferences an NSView; offscreen checks have no NSView.
    if (QGuiApplication::platformName() == "cocoa") backend.setParentWindow(window);
#else
    backend.setParentWindow(window);
#endif
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    auto *sourceScroll = window->findChild<QQuickItem *>("editorScroll");
    auto *settings = window->findChild<QObject *>("workspaceSettings");
    if (!check(footer && editor && layout && pane && sourceScroll && settings,
               "Shipped component tree is missing the common footer or writing surfaces")) return finish();
    check(!window->findChild<QObject *>("sourceModeMenu"), "Obsolete Source dropdown found in shipped component tree");
    QMap<QString, QQuickItem *> buttons;
    for (const QString &name : controlNames) {
        buttons.insert(name, footer->findChild<QQuickItem *>(name));
        check(buttons.value(name), "Missing document footer button: " + name);
    }
    if (!failures.isEmpty()) return finish();
    window->setFlag(Qt::WindowDoesNotAcceptFocus, true);
    InputIsolation input(app, window);
#ifdef Q_OS_MACOS
    if (QGuiApplication::platformName() == "cocoa") {
        configureMacWindowChrome(window);
        applyMacWindowTheme(window, false, backend.darkMode());
    }
#endif
    window->resize(1440, 800);
    window->show();
    if (!check(waitUntil([&] { return window->isExposed(); }), "Native diagnostic window was not exposed")) return finish();
    QMetaObject::invokeMethod(window, "applyReferenceWorkspace");
    QString saved = QString::fromUtf8("# A steadier place to write\n\nThis is a disposable diagnostic document — café, 你好.\n\n");
    for (int paragraph = 0; paragraph < 45; ++paragraph)
        saved += QString("Paragraph %1 keeps its **Markdown**, selection and reading position while the view changes.\n\n").arg(paragraph);
    const QString samplePath = temporary.filePath("View check.md");
    QFile sample(samplePath);
    if (!check(sample.open(QIODevice::WriteOnly) && sample.write(saved.toUtf8()) == saved.toUtf8().size(),
               "Cannot write disposable sample document")) return finish();
    sample.close();
    backend.library()->setProperty("rootFolder", QUrl::fromLocalFile(temporary.path()));
    if (!check(backend.open(QUrl::fromLocalFile(samplePath)), "Cannot open disposable sample in real Backend")) return finish();
    check(waitUntil([&] { return editor->property("text").toString() == saved; }), "Loaded document did not reach source editor");
    settle(180); // Finish documentLoaded's deferred cursor reset before selecting.
    check(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, saved.size()),
                                  Q_ARG(QString, QStringLiteral("Unsaved diagnostic addition.\n"))), "Cannot create draft fixture");
    settle(180);
    const QString draft = editor->property("text").toString();
    const int selectionStart = draft.indexOf("Paragraph 20");
    const int selectionEnd = selectionStart + 12;
    QMetaObject::invokeMethod(editor, "select", Q_ARG(int, selectionStart), Q_ARG(int, selectionEnd));
    settle(100);
    const int cursor = editor->property("cursorPosition").toInt();
    const int revision = backend.documentRevision();
    const auto ready = [&] {
        return !window->property("changingDocumentView").toBool()
            && !pane->property("viewportRefreshPending").toBool();
    };
    const auto capture = [&](const QString &name) {
        settle(100);
        const QString path = output.filePath(name + ".png");
        const QImage image = window->grabWindow();
        if (check(!image.isNull() && image.save(path), "Cannot capture native window: " + name)) screenshots.append(path);
    };
    const auto record = [&](const QString &step, const QMap<QString, QRectF> &expectedBounds) {
        QJsonObject controls;
        const int mode = layout->property("effectiveLayoutMode").toInt();
        const bool visual = mode != 0 && layout->property("visualEditEnabled").toBool();
        for (const QString &name : controlNames) {
            auto *button = buttons.value(name);
            const QRectF rect = bounds(button);
            auto entry = rectJson(rect);
            entry.insert("visible", button->isVisible());
            entry.insert("enabled", button->isEnabled());
            entry.insert("checked", button->property("checked").toBool());
            auto *label = button->findChild<QQuickItem *>("footerButtonLabel");
            entry.insert("truncated", !label || label->property("truncated").toBool());
            controls.insert(name, entry);
            check(button->isVisible() == expectedBounds.contains(name), step + ": control visibility changed: " + name);
            if (!button->isVisible()) continue;
            check(QRectF(QPointF(), window->size()).contains(rect), step + ": control is clipped: " + name);
            if (viewNames.contains(name) || name == "footerStylesButton")
                check(label && !label->property("truncated").toBool(), step + ": label is truncated: " + name);
            const QRectF baseline = expectedBounds.value(name);
            check(qAbs(rect.x() - baseline.x()) < 0.5 && qAbs(rect.y() - baseline.y()) < 0.5
                  && qAbs(rect.width() - baseline.width()) < 0.5 && qAbs(rect.height() - baseline.height()) < 0.5,
                  step + ": control moved: " + name);
            if (!viewNames.contains(name)) continue;
            const bool checked = name == "visualEditToggle" ? visual
                : name == "sourceModeButton" ? mode == 0 : name == "previewSplitButton" ? mode == 1 : mode == 2;
            check(button->property("checked").toBool() == checked, step + ": incorrect checked state: " + name);
        }
        check(editor->property("text").toString() == draft, step + ": canonical draft changed");
        check(editor->property("cursorPosition").toInt() == cursor
              && editor->property("selectionStart").toInt() == selectionStart
              && editor->property("selectionEnd").toInt() == selectionEnd, step + ": source selection changed");
        check(backend.documentRevision() == revision && backend.modified(), step + ": document revision/dirty state changed");
        auto *previewScroll = pane->findChild<QQuickItem *>("previewScroll");
        scenarios.append(QJsonObject{{"step", step}, {"windowWidth", window->width()},
            {"windowHeight", window->height()}, {"layoutMode", mode}, {"visualEditing", visual},
            {"documentFooter", rectJson(bounds(footer))}, {"controls", controls},
            {"draftSha256", sha256(editor->property("text").toString().toUtf8())},
            {"documentRevision", backend.documentRevision()}, {"cursor", editor->property("cursorPosition").toInt()},
            {"selectionStart", editor->property("selectionStart").toInt()}, {"selectionEnd", editor->property("selectionEnd").toInt()},
            {"sourceScrollY", sourceScroll->property("contentY").toDouble()},
            {"sourceContentHeight", sourceScroll->property("contentHeight").toDouble()},
            {"previewScrollY", previewScroll ? previewScroll->property("contentY").toDouble() : -1},
            {"previewContentHeight", previewScroll ? previewScroll->property("contentHeight").toDouble() : -1}});
    };
    const auto action = [&](const QString &name, int mode, bool visual) {
        check(click(window, buttons.value(name), true), "Cannot click visible control: " + name);
        check(waitUntil([&] { return layout->property("effectiveLayoutMode").toInt() == mode
            && layout->property("visualEditEnabled").toBool() == visual && ready(); }),
            QString("Click %1 did not settle in mode %2, visual %3").arg(name).arg(mode).arg(visual));
        settle(30);
    };
    for (int width : {1440, 1280, 960, 739, 720}) {
        window->resize(width, 800);
        settle(100);
        action("sourceModeButton", 0, false);
        QMap<QString, QRectF> baseline;
        for (const QString &name : controlNames)
            if (buttons.value(name)->isVisible()) baseline.insert(name, bounds(buttons.value(name)));
        const bool canSplit = width >= 803;
        check(buttons.value("previewSplitButton")->isEnabled() == canSplit, "Split availability does not match window width");
        for (int pass = 0; pass < 3; ++pass) {
            struct Step { const char *button; int mode; bool visual; };
            const QList<Step> steps{{"sourceModeButton", 0, false}, {"visualEditToggle", 2, true},
                {"visualEditToggle", 2, false}, {"previewSplitButton", canSplit ? 1 : 2, false},
                {"visualEditToggle", canSplit ? 1 : 2, true}, {"previewFullButton", 2, true},
                {"sourceModeButton", 0, false}, {"previewFullButton", 2, false}};
            for (int index = 0; index < steps.size(); ++index) {
                const auto &step = steps.at(index);
                action(step.button, step.mode, step.visual);
                const QString name = QString("width-%1-pass-%2-step-%3").arg(width).arg(pass).arg(index);
                record(name, baseline);
                if (pass == 0 && (index == 0 || index == 1 || index == 3 || index == 7)) capture(name);
            }
        }
    }
    // Explicit view clicks resolve the currently visible narrow arrangement,
    // including a remembered Split request whose restoration margin is unmet.
    const auto currentBounds = [&] {
        QMap<QString, QRectF> result;
        for (const QString &name : controlNames)
            if (buttons.value(name)->isVisible()) result.insert(name, bounds(buttons.value(name)));
        return result;
    };
    const auto beginNarrowSplit = [&](const QString &surface) {
        window->resize(1440, 800);
        settle(100);
        action("sourceModeButton", 0, false);
        action("previewSplitButton", 1, false);
        action("visualEditToggle", 1, true);
        auto *target = surface == "source" ? editor : pane->findChild<QQuickItem *>("visualEditor");
        check(target && QMetaObject::invokeMethod(target, "forceActiveFocus"), "Cannot focus narrow fallback fixture surface");
        check(waitUntil([&] { return layout->property("activeSurface").toString() == surface; }),
              "Writing focus did not update the real activeSurface binding");
        window->resize(720, 800);
        settle(150);
    };
    beginNarrowSplit("source");
    check(layout->property("effectiveLayoutMode").toInt() == 0,
          "Narrow source-focused Split did not fall back to Source");
    check(layout->property("visualEditEnabled").toBool()
          && !buttons.value("visualEditToggle")->property("checked").toBool(),
          "Hidden visual preference incorrectly appears enabled in Source");
    auto narrowBounds = currentBounds();
    record("narrow-source-fallback", narrowBounds);
    action("visualEditToggle", 2, true);
    record("narrow-source-to-visual-one-click", narrowBounds);
    capture("narrow-source-to-visual");
    beginNarrowSplit("visual");
    check(layout->property("effectiveLayoutMode").toInt() == 2,
          "Narrow visual-focused Split did not fall back to Full Visual");
    narrowBounds = currentBounds();
    action("visualEditToggle", 2, false);
    record("narrow-visual-to-full-preview-one-click", narrowBounds);
    capture("narrow-visual-to-preview");
    window->resize(1440, 800);
    settle(100);
    action("sourceModeButton", 0, false);
    action("previewSplitButton", 1, false);
    window->resize(720, 800);
    settle(100);
    window->resize(820, 800);
    settle(100);
    check(layout->property("effectiveLayoutMode").toInt() == 0
          && buttons.value("previewSplitButton")->isEnabled(),
          "Fixture did not reach enabled Split inside the restoration margin");
    narrowBounds = currentBounds();
    action("previewSplitButton", 1, false);
    record("explicit-split-inside-restoration-margin", narrowBounds);
    capture("explicit-split-at-820");

    // Verify the same controls with a readable mid-document selection, returning
    // to identical split geometry. Source/preview scroll fractions may differ.
    window->resize(1440, 800);
    settle(100);
    action("sourceModeButton", 0, false);
    action("previewSplitButton", 1, false);
    settle(80);
    const QRectF caret = editor->property("cursorRectangle").toRectF();
    sourceScroll->setProperty("contentY", editor->y() + caret.y() - sourceScroll->height() / 2);
    settle(100);
    const qreal sourceY = sourceScroll->property("contentY").toReal();
    for (const QString &destination : {QStringLiteral("previewFullButton"), QStringLiteral("sourceModeButton")}) {
        action(destination, destination == "sourceModeButton" ? 0 : 2, false);
        action("previewSplitButton", 1, false);
        check(qAbs(sourceScroll->property("contentY").toReal() - sourceY) <= 2,
              "Source reading position moved after returning to Split");
    }
    backend.setThemePreset("dark");
    settle(120);
    capture("dark-split");
    QMetaObject::invokeMethod(editor, "undo");
    check(editor->property("text").toString() == saved, "Draft Undo no longer restores saved source");
    QMetaObject::invokeMethod(editor, "redo");
    check(editor->property("text").toString() == draft, "Draft Redo no longer restores unsaved source");
    check(sample.open(QIODevice::ReadOnly), "Cannot re-read saved fixture");
    check(sample.readAll() == saved.toUtf8(), "Disposable saved document was changed by view clicks");
    if (auto *about = window->findChild<QObject *>("aboutDialog")) {
        check(QMetaObject::invokeMethod(about, "open"), "Cannot open bundled About dialog");
        check(waitUntil([&] { return about->property("opened").toBool(); }), "About dialog did not open");
        auto *pathLabel = about->findChild<QObject *>("runningApplicationPath");
        check(pathLabel && QFileInfo(pathLabel->property("text").toString()).canonicalFilePath()
              == QFileInfo(app.applicationFilePath()).canonicalFilePath(), "About identifies the wrong running executable");
        capture("running-app-about");
        QMetaObject::invokeMethod(about, "close");
    } else {
        check(false, "Bundled About dialog is missing");
    }
    backend.discardRecovery();
    return finish();
}
