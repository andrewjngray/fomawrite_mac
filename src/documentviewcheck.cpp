#include "documentviewcheck.h"

#include "backend.h"
#include "publishingthemes.h"
#include "systemtheme.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJSValue>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQmlEngine>
#include <QWheelEvent>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QTextBlock>
#include <QTextLayout>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QPdfDocument>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QUuid>
#include <functional>
#include <tuple>

#ifdef Q_OS_MACOS
void configureMacWindowChrome(QWindow *window);
void applyMacWindowTheme(QWindow *window, bool followSystem, bool dark);
#endif

namespace {
const QStringList viewNames{QStringLiteral("sourceModeButton"), QStringLiteral("liveEditToggle"), QStringLiteral("singleModeButton"), QStringLiteral("previewSplitButton")};
const QStringList stationaryNames{QStringLiteral("singleModeButton"), QStringLiteral("previewSplitButton")};
const QStringList controlNames = viewNames + QStringList{QStringLiteral("sourceAppearanceButton"), QStringLiteral("previewTemplateButton"),
    QStringLiteral("webPublishingButton"), QStringLiteral("pdfPublishingButton")};

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

// A nonactivating native window can be occluded by another app, which stops
// Qt Quick's automatic render/polish frames. Flush a fixed number of real
// frames before reading hit targets or asserting geometry; never wait for the
// geometry itself to match an expected result.
bool renderBarrier(QQuickWindow *window) {
    if (!window) return false;
    for (int frame = 0; frame < 2; ++frame) {
        window->requestUpdate();
        if (window->grabWindow().isNull()) return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
    return true;
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
    if (!renderBarrier(window)) return false;
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

// Deliver the second press as well as its double-click notification, matching
// QPA delivery while keeping every event inside this isolated window.
bool doubleClick(QQuickWindow *window, QQuickItem *item) {
    if (!renderBarrier(window) || !item || !item->isVisible()) return false;
    const QPointF point = bounds(item).center();
    if (!QRectF(QPointF(), window->size()).contains(point)) return false;
    const QPointF global = window->mapToGlobal(point.toPoint());
    const quint64 timestamp = quint64(QDateTime::currentMSecsSinceEpoch());
    const QEvent::Type types[]{QEvent::MouseButtonPress, QEvent::MouseButtonRelease,
                              QEvent::MouseButtonPress, QEvent::MouseButtonDblClick,
                              QEvent::MouseButtonRelease};
    for (int index = 0; index < 5; ++index) {
        const bool released = types[index] == QEvent::MouseButtonRelease;
        QMouseEvent event(types[index], point, point, global, Qt::LeftButton,
                          released ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
        event.setTimestamp(timestamp + quint64(index * 35));
        QCoreApplication::sendEvent(window, &event);
        QCoreApplication::processEvents();
    }
    return true;
}

// Deliver keyboard input through the same QQuickWindow dispatch used by an
// actual editor. No OS-level events or activation of the user's other apps.
void key(QQuickWindow *window, int value, const QString &text = {},
         Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
    QKeyEvent press(QEvent::KeyPress, value, modifiers, text);
    QCoreApplication::sendEvent(window, &press);
    QKeyEvent release(QEvent::KeyRelease, value, modifiers, text);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
}

bool enterText(QQuickWindow *window, QQuickItem *field, const QString &text) {
    if (!click(window, field)) return false;
    field->forceActiveFocus();
    if (!QMetaObject::invokeMethod(field, "selectAll")) return false;
    if (text.isEmpty()) key(window, Qt::Key_Backspace);
    else key(window, Qt::Key_unknown, text);
    return waitUntil([&] { return field->property("text").toString() == text; });
}

// Read actual publishing DOM geometry in the isolated application world.
// The callback is scoped to this engine and does not enable document scripts.
QVariant publishingDomResult(QQuickItem *field, const QString &script) {
    auto *engine = qmlEngine(field);
    if (!engine) return {};
    engine->globalObject().setProperty("nativePublishingDomResult", QJSValue(QJSValue::UndefinedValue));
    const QJSValue callback = engine->evaluate("(function(value) { nativePublishingDomResult = value; })");
    if (!QMetaObject::invokeMethod(field, "runJavaScript", Q_ARG(QString, script), Q_ARG(uint, 1), Q_ARG(QJSValue, callback))) return {};
    if (!waitUntil([&] { return !engine->globalObject().property("nativePublishingDomResult").isUndefined(); }, 3000)) return {};
    return engine->globalObject().property("nativePublishingDomResult").toVariant();
}
QVariantMap publishingDomAnchor(QQuickItem *field) {
    return publishingDomResult(field, "(function(){var a=document.body.querySelectorAll('p,h1,h2,h3,h4,h5,h6,pre,table,li,img');for(var i=0;i<a.length;i++){var r=a[i].getBoundingClientRect();if(r.bottom>0)return {index:i,offset:r.top};}return null;})()").toMap();
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
    QJsonArray dailyWritingChecks;
    QJsonArray paneZoomChecks;
    QJsonArray editorAcceptanceChecks;
    QJsonArray paneChromeChecks;
    QJsonArray editingLayoutChecks;
    QJsonArray publishingThemeChecks;
    QJsonArray paneZoomReadings;
    QJsonArray paneZoomDividerEvents;
    QJsonArray screenshots;
    QJsonArray qmlWarnings, expectedImageWarnings;
    QString expectedMissingPreviewImage;
    QJsonObject report{{"schema", 1}, {"startedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
                       {"applicationVersion", app.applicationVersion()},
                       {"executable", QFileInfo(app.applicationFilePath()).canonicalFilePath()},
                       {"executableSha256", fileHash(app.applicationFilePath())},
                       {"platform", QGuiApplication::platformName()},
                       {"qmlEntry", "qrc:/Main.qml"},
                       {"checkScope", "full-document-views"},
                       {"input", "Synthetic Qt window mouse and key events; native input rejected"}};
    const auto check = [&](bool condition, const QString &message) {
        if (!condition) failures.append(message);
        return condition;
    };
    const auto finish = [&] {
        if (!qmlWarnings.isEmpty()) failures.append("The bundled QML emitted engine warnings; see qmlWarnings");
        report.insert("passed", failures.isEmpty());
        report.insert("failures", QJsonArray::fromStringList(failures));
        report.insert("scenarios", scenarios);
        report.insert("dailyWritingChecks", dailyWritingChecks);
        report.insert("paneZoomChecks", paneZoomChecks);
        report.insert("editorAcceptanceChecks", editorAcceptanceChecks);
        report.insert("paneChromeChecks", paneChromeChecks);
        report.insert("editingLayoutChecks", editingLayoutChecks);
        report.insert("publishingThemeChecks", publishingThemeChecks);
        report.insert("paneZoomReadings", paneZoomReadings);
        report.insert("paneZoomDividerEvents", paneZoomDividerEvents);
        report.insert("screenshots", screenshots);
        report.insert("qmlWarnings", qmlWarnings);
        report.insert("expectedImageWarnings", expectedImageWarnings);
        report.insert("finishedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        QSaveFile file(output.filePath(QStringLiteral("report.json")));
        const QByteArray json = QJsonDocument(report).toJson(QJsonDocument::Indented);
        if (!file.open(QIODevice::WriteOnly) || file.write(json) != json.size() || !file.commit()) {
            qCritical("Cannot write report.json.");
            return 2;
        }
        qInfo().noquote() << "Document view check:" << (failures.isEmpty() ? "PASS" : "FAIL")
                          << scenarios.size() << "footer states," << dailyWritingChecks.size() << "daily-writing checks," << paneZoomChecks.size() << "pane-zoom checks," << editorAcceptanceChecks.size() << "editor acceptance checks," << publishingThemeChecks.size() << "publishing theme checks; report:" << file.fileName();
        for (const QString &failure : failures) qWarning().noquote() << failure;
        return failures.isEmpty() ? 0 : 1;
    };
    QJsonObject resources;
    for (const QString &name : {QStringLiteral("Main.qml"), QStringLiteral("DocumentFooter.qml"),
            QStringLiteral("FooterButton.qml"), QStringLiteral("PublishingPreview.qml"), QStringLiteral("PreviewPane.qml"), QStringLiteral("LibraryPane.qml"),
            QStringLiteral("WorkspaceLayout.qml"), QStringLiteral("WorkspaceCommands.qml"), QStringLiteral("AboutDialog.qml"),
            QStringLiteral("DocumentFindBar.qml"), QStringLiteral("DocumentOutline.qml"),
            QStringLiteral("PaneZoomState.qml"), QStringLiteral("PaneZoomControls.qml"), QStringLiteral("WorkspaceHeader.qml"), QStringLiteral("LinkEditor.qml"), QStringLiteral("LinkSyntax.js"), QStringLiteral("ExportHub.qml"), QStringLiteral("SquareDialogButton.qml")}) {
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
        for (const auto &warning : warnings) {
            const QString message = warning.toString();
            // The incomplete-image fixture deliberately exercises a missing
            // resource; keep that expected diagnostic separate, all other warnings fail.
            if (!expectedMissingPreviewImage.isEmpty() && message.contains("Cannot open: " + expectedMissingPreviewImage))
                expectedImageWarnings.append(message);
            else qmlWarnings.append(message);
        }
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
    settings->setProperty("autoHideChrome", false);
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
    if (qEnvironmentVariable("FOMAWRITE_PUBLISHING_CHECK_ONLY") == QStringLiteral("1")) {
        // Isolate the normal preview workflow from unrelated geometry checks,
        // which synchronously grab/force frames on a Chromium-backed window.
        // No renderBarrier() or capture has occurred before this early path.
        report.insert("checkScope", "publishing-preview-only");
        window->resize(1440, 900);
        layout->setProperty("organizerVisible", false);
        layout->setProperty("filesVisible", false);
        check(backend.setExportPaperSize("a4") && backend.setExportOrientation("portrait"),
              "Cannot select A4 portrait for focused preview checks");
        const QString sourcePath = samplePath;
        const auto themeCheck = [&](const QString &name, const std::function<void()> &exercise) {
            const qsizetype before = failures.size();
            exercise();
            publishingThemeChecks.append(QJsonObject{{"name", name}, {"passed", failures.size() == before},
                {"newFailures", int(failures.size() - before)}});
        };
        #include "publishingpreviewacceptancecheck.inc"
        return finish();
    }
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
        check(renderBarrier(window), step + ": cannot render geometry frame");
        check(waitUntil(ready), step + ": viewport did not settle after rendering");
        check(renderBarrier(window), step + ": cannot render settled geometry frame");
        QJsonObject controls;
        const int mode = layout->property("effectiveLayoutMode").toInt();
        const bool live = layout->property("liveEditEnabled").toBool();
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
            const bool shouldBeVisible = name == "sourceAppearanceButton" || name == "sourceModeButton" || name == "liveEditToggle" ? mode != 2
                : name == "previewTemplateButton" || name == "webPublishingButton" || name == "pdfPublishingButton" ? mode != 0 : true;
            check(button->isVisible() == shouldBeVisible, step + ": wrong pane control visibility: " + name);
            if (!button->isVisible()) continue;
            check(QRectF(QPointF(), window->size()).contains(rect), step + ": control is clipped: " + name);
            if (viewNames.contains(name))
                check(label && !label->property("truncated").toBool(), step + ": label is truncated: " + name);
            if (stationaryNames.contains(name)) {
                const QRectF baseline = expectedBounds.value(name);
                check(qAbs(rect.x() - baseline.x()) < 0.5 && qAbs(rect.y() - baseline.y()) < 0.5
                      && qAbs(rect.width() - baseline.width()) < 0.5 && qAbs(rect.height() - baseline.height()) < 0.5,
                      step + ": view control moved: " + name);
            } else if (name == "sourceAppearanceButton" || name == "previewTemplateButton") {
                auto *owner = name == "sourceAppearanceButton" ? window->findChild<QQuickItem *>("editorPane") : pane;
                check(owner && qAbs(rect.left() - bounds(owner).left() - 12) < 0.5
                      && rect.right() <= bounds(owner).right() - 11.5, step + ": control lost its pane anchor: " + name);
            }
            if (!viewNames.contains(name)) continue;
            const bool checked = name == "liveEditToggle" ? mode != 2 && live
                : name == "sourceModeButton" ? mode != 2 && !live : name == "previewSplitButton" ? mode == 1 : mode == 0;
            check(button->property("checked").toBool() == checked, step + ": incorrect checked state: " + name);
        }
        for (int first = 0; first < controlNames.size(); ++first) {
            auto *a = buttons.value(controlNames[first]); if (!a->isVisible()) continue;
            for (int second = first + 1; second < controlNames.size(); ++second) {
                auto *b = buttons.value(controlNames[second]); if (!b->isVisible()) continue;
                const auto overlap = bounds(a).intersected(bounds(b));
                check(overlap.width() <= 0.5 || overlap.height() <= 0.5, step + ": pane controls overlap: " + a->objectName() + "/" + b->objectName());
            }
        }
        check(editor->property("text").toString() == draft, step + ": canonical draft changed");
        check(editor->property("cursorPosition").toInt() == cursor
              && editor->property("selectionStart").toInt() == selectionStart
              && editor->property("selectionEnd").toInt() == selectionEnd, step + ": source selection changed");
        check(backend.documentRevision() == revision && backend.modified(), step + ": document revision/dirty state changed");
        auto *previewScroll = pane->findChild<QQuickItem *>("previewScroll");
        scenarios.append(QJsonObject{{"step", step}, {"windowWidth", window->width()},
            {"windowHeight", window->height()}, {"layoutMode", mode}, {"liveEditing", live},
            {"documentFooter", rectJson(bounds(footer))}, {"controls", controls},
            {"draftSha256", sha256(editor->property("text").toString().toUtf8())},
            {"documentRevision", backend.documentRevision()}, {"cursor", editor->property("cursorPosition").toInt()},
            {"selectionStart", editor->property("selectionStart").toInt()}, {"selectionEnd", editor->property("selectionEnd").toInt()},
            {"sourceScrollY", sourceScroll->property("contentY").toDouble()},
            {"sourceContentHeight", sourceScroll->property("contentHeight").toDouble()},
            {"previewScrollY", previewScroll ? previewScroll->property("contentY").toDouble() : -1},
            {"previewContentHeight", previewScroll ? previewScroll->property("contentHeight").toDouble() : -1}});
    };
    const auto action = [&](const QString &name, int mode, bool live) {
        if (name == "previewOnly")
            check(QMetaObject::invokeMethod(window, "selectWritingMode", Q_ARG(QVariant, "preview")), "Cannot open Preview Only reading action");
        else check(click(window, buttons.value(name), true), "Cannot click visible control: " + name);
        check(waitUntil([&] { return layout->property("effectiveLayoutMode").toInt() == mode
            && layout->property("liveEditEnabled").toBool() == live && ready(); }),
            QString("Click %1 did not settle in mode %2, live %3").arg(name).arg(mode).arg(live));
        settle(30);
    };
    const auto sourceSingle = [&] {
        if (layout->property("effectiveLayoutMode").toInt() == 2)
            check(QMetaObject::invokeMethod(window, "selectWritingMode", Q_ARG(QVariant, "source")), "Cannot return from reading to Source");
        else check(click(window, buttons.value("sourceModeButton")), "Cannot choose Source editing");
        action("singleModeButton", 0, false);
    };
    for (int width : {1440, 1280, 960, 739, 720}) {
        window->resize(width, 800);
        settle(100);
        sourceSingle();
        check(renderBarrier(window), "Cannot render footer baseline at width " + QString::number(width));
        QMap<QString, QRectF> baseline;
        for (const QString &name : controlNames)
            if (buttons.value(name)->isVisible()) baseline.insert(name, bounds(buttons.value(name)));
        const bool canSplit = width >= 803;
        check(buttons.value("previewSplitButton")->isEnabled() == canSplit, "Split availability does not match window width");
        for (int pass = 0; pass < 3; ++pass) {
            struct Step { const char *button; int mode; bool live; };
            const QList<Step> steps{{"singleModeButton", 0, false}, {"liveEditToggle", 0, true},
                {"liveEditToggle", 0, true}, {"previewSplitButton", canSplit ? 1 : 0, true},
                {"sourceModeButton", canSplit ? 1 : 0, false}, {"singleModeButton", 0, false},
                {"liveEditToggle", 0, true}, {"sourceModeButton", 0, false}};
            for (int index = 0; index < steps.size(); ++index) {
                const auto &step = steps.at(index);
                action(step.button, step.mode, step.live);
                const QString name = QString("width-%1-pass-%2-step-%3").arg(width).arg(pass).arg(index);
                record(name, baseline);
                if (pass == 0 && (index == 0 || index == 1 || index == 3 || index == 7)) capture(name);
            }
        }
    }
    // Explicit view clicks resolve the currently visible narrow arrangement,
    // including a remembered Split request whose restoration margin is unmet.
    const auto currentBounds = [&] {
        check(renderBarrier(window), "Cannot render responsive footer baseline");
        QMap<QString, QRectF> result;
        for (const QString &name : controlNames)
            if (buttons.value(name)->isVisible()) result.insert(name, bounds(buttons.value(name)));
        return result;
    };
    const auto beginNarrowSplit = [&](bool live) {
        window->resize(1440, 800); settle(100); sourceSingle();
        if (live) action("liveEditToggle", 0, true);
        action("previewSplitButton", 1, live);
        window->resize(720, 800); settle(150);
        check(layout->property("effectiveLayoutMode").toInt() == 0
              && layout->property("liveEditEnabled").toBool() == live,
              "Narrow Split changed the chosen editor instead of contracting layout");
    };
    beginNarrowSplit(false);
    auto narrowBounds = currentBounds();
    record("narrow-source-single-fallback", narrowBounds);
    action("liveEditToggle", 0, true);
    record("narrow-source-to-live-one-click", narrowBounds);
    capture("narrow-source-to-live");
    beginNarrowSplit(true);
    narrowBounds = currentBounds();
    action("sourceModeButton", 0, false);
    record("narrow-live-to-source-one-click", narrowBounds);
    capture("narrow-live-to-source");
    window->resize(1440, 800);
    settle(100);
    sourceSingle();
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
    sourceSingle();
    action("previewSplitButton", 1, false);
    settle(80);
    const QRectF caret = editor->property("cursorRectangle").toRectF();
    sourceScroll->setProperty("contentY", editor->y() + caret.y() - sourceScroll->height() / 2);
    settle(100);
    const qreal sourceY = sourceScroll->property("contentY").toReal();
    for (const QString &destination : {QStringLiteral("previewOnly"), QStringLiteral("singleModeButton")}) {
        action(destination, destination == "singleModeButton" ? 0 : 2, false);
        action("previewSplitButton", 1, false);
        check(qAbs(sourceScroll->property("contentY").toReal() - sourceY) <= 2,
              "Source reading position moved after returning to Split");
    }
    backend.setThemePreset("dark");
    settle(120);
    capture("dark-split");
    QMetaObject::invokeMethod(window, "undoEditing");
    check(editor->property("text").toString() == saved, "Draft Undo no longer restores saved source");
    QMetaObject::invokeMethod(window, "redoEditing");
    check(editor->property("text").toString() == draft, "Draft Redo no longer restores unsaved source");
    check(sample.open(QIODevice::ReadOnly), "Cannot re-read saved fixture");
    check(sample.readAll() == saved.toUtf8(), "Disposable saved document was changed by view clicks");
    // The same shipped executable also exercises the daily writing interfaces.
    // Keep these separate from the original 124 footer-state records so changes
    // in navigation/search do not weaken that stable-placement regression gate.
    const auto daily = [&](const QString &name, const std::function<void()> &exercise) {
        const qsizetype before = failures.size();
        exercise();
        dailyWritingChecks.append(QJsonObject{{"name", name}, {"passed", failures.size() == before},
            {"newFailures", int(failures.size() - before)}, {"windowWidth", window->width()},
            {"effectiveLayoutMode", layout->property("effectiveLayoutMode").toInt()},
            {"sourceSha256", sha256(editor->property("text").toString().toUtf8())},
            {"documentRevision", backend.documentRevision()}});
    };
    const auto child = [&](const char *name) { return window->findChild<QQuickItem *>(name); };
    const auto loadSample = [&](const QString &name, const QString &text) {
        sourceSingle();
        const QString path = temporary.filePath(name + ".md");
        QFile file(path);
        if (!check(file.open(QIODevice::WriteOnly), "Cannot create daily writing sample: " + name)) return false;
        file.write(text.toUtf8()); file.close();
        if (!check(backend.open(QUrl::fromLocalFile(path)), "Cannot load daily writing sample: " + name)) return false;
        check(waitUntil([&] { return editor->property("text").toString() == text; }), "Sample did not reach editor: " + name);
        settle(180);
        return true;
    };
    auto *findBar = child("documentFindBar");
    auto *findField = child("searchField");
    auto *replaceField = child("replaceField");
    auto *searchButton = child("documentSearchButton");
    auto *previousMatch = child("findPreviousButton");
    auto *nextMatch = child("findNextButton");
    auto *closeFind = child("findCloseButton");
    auto *replaceToggle = child("findReplaceToggle");
    auto *replaceCurrent = child("replaceCurrentButton");
    auto *replaceAll = child("replaceAllButton");
    auto *matchStatus = child("searchMatchStatus");
    const bool findAvailable = check(findBar && findField && replaceField && searchButton
        && previousMatch && nextMatch && closeFind && replaceToggle && replaceCurrent && replaceAll && matchStatus,
        "Shipped Find interface is missing named interactive controls");
    if (findAvailable) {
        const QString original = QString::fromUtf8("# Finding words\n\ncafé one.\n\nA quiet passage.\n\ncafé two.\n\ncafé three.\n");
        const QString addition = "\nAn unsaved ending.\n";
        const QString findDraft = original + addition;
        const int second = original.indexOf(QString::fromUtf8("café two"));
        window->resize(1440, 800);
        backend.setThemePreset("studio");
        if (loadSample("Find and replace", original)) {
            QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, original.size()), Q_ARG(QString, addition));
            editor->setProperty("cursorPosition", second - 2);
            settle(100);
            const bool untouchedDirtyState = backend.modified();
            daily("find-nearest-match-and-keyboard-navigation", [&] {
                check(click(window, searchButton), "Cannot click document Find button");
                check(waitUntil([&] { return findBar->isVisible() && findField->hasActiveFocus(); }), "Find did not take query focus");
                check(renderBarrier(window), "Cannot render Find navigation baseline");
                QMap<QString, QRectF> findBounds;
                for (auto *control : {previousMatch, nextMatch, closeFind}) findBounds.insert(control->objectName(), bounds(control));
                check(enterText(window, findField, QString::fromUtf8("café")), "Cannot type Unicode Find query");
                check(waitUntil([&] { return window->property("searchMatchIndex").toInt() == 1; }), "Find did not choose nearest following match");
                check(editor->property("selectionStart").toInt() == second, "Nearest query selected wrong source range");
                check(click(window, nextMatch), "Cannot click Next match");
                check(window->property("searchMatchIndex").toInt() == 2, "Next match did not advance");
                check(click(window, nextMatch), "Cannot click wrapping Next match");
                check(window->property("searchMatchIndex").toInt() == 0, "Next match did not wrap");
                check(click(window, previousMatch), "Cannot click Previous match");
                check(window->property("searchMatchIndex").toInt() == 2, "Previous match did not wrap");
                findField->forceActiveFocus();
                key(window, Qt::Key_Return, {}, Qt::ShiftModifier);
                check(window->property("searchMatchIndex").toInt() == 1, "Shift Return did not find previous match");
                for (auto *control : {previousMatch, nextMatch, closeFind})
                    check(bounds(control) == findBounds.value(control->objectName()), "Find navigation moved when the query gained results");
                check(editor->property("text").toString() == findDraft && backend.modified() == untouchedDirtyState,
                      "Find navigation changed the draft");
                capture("daily-find-wide");
            });
            daily("replace-current-advances-and-replace-all-is-undoable", [&] {
                check(click(window, replaceToggle), "Cannot show replacement row");
                check(waitUntil([&] { return replaceField->isVisible(); }), "Replacement row did not appear");
                check(enterText(window, replaceField, "bistro"), "Cannot type replacement text");
                check(click(window, replaceCurrent), "Cannot click Replace current");
                QString afterCurrent = findDraft;
                afterCurrent.replace(second, QString::fromUtf8("café").size(), "bistro");
                check(waitUntil([&] { return editor->property("text").toString() == afterCurrent; }), "Replace current changed the wrong text");
                check(editor->property("selectionStart").toInt() == afterCurrent.lastIndexOf(QString::fromUtf8("café")),
                      "Replace current jumped back instead of advancing");
                capture("daily-replace-wide");
                check(click(window, replaceAll), "Cannot click Replace all");
                QString afterAll = findDraft;
                afterAll.replace(QString::fromUtf8("café"), "bistro");
                check(waitUntil([&] { return editor->property("text").toString() == afterAll; }), "Replace all did not update every remaining match");
                check(!previousMatch->isEnabled() && !nextMatch->isEnabled()
                      && !replaceCurrent->isEnabled() && !replaceAll->isEnabled()
                      && matchStatus->property("text").toString() == "No matches", "No-match status/navigation is inconsistent");
                QMetaObject::invokeMethod(window, "undoEditing");
                check(editor->property("text").toString() == afterCurrent, "Replace all was not one undo unit");
                QMetaObject::invokeMethod(window, "undoEditing");
                check(editor->property("text").toString() == findDraft, "Replace current Undo did not preserve the unsaved ending");
                QMetaObject::invokeMethod(window, "redoEditing");
                QMetaObject::invokeMethod(window, "redoEditing");
                check(editor->property("text").toString() == afterAll, "Replacement Redo did not restore exact draft");
                replaceField->forceActiveFocus();
                key(window, Qt::Key_Escape);
                check(waitUntil([&] { return !findBar->isVisible() && editor->hasActiveFocus(); }), "Escape in replacement field did not close Find and restore Source focus");
            });
            daily("find-narrow-layout-and-escape", [&] {
                window->resize(720, 800);
                settle(100);
                action("sourceModeButton", 0, false);
                check(click(window, searchButton), "Cannot open Find from the narrow Source editor");
                check(waitUntil([&] { return findBar->isVisible() && layout->property("effectiveLayoutMode").toInt() == 0; }),
                      "Find in the narrow Source editor did not open");
                check(enterText(window, findField, "missing-phrase"), "Cannot enter no-match query");
                check(click(window, replaceToggle), "Cannot reveal narrow replacement row");
                settle(80);
                check(renderBarrier(window), "Cannot render narrow Find containment frame");
                for (auto *control : {findField, replaceField, previousMatch, nextMatch, closeFind, replaceToggle, replaceCurrent, replaceAll, matchStatus}) {
                    check(control->isVisible() && bounds(findBar).adjusted(-0.5, -0.5, 0.5, 0.5).contains(bounds(control)),
                          "Narrow Find control is outside its bar: " + control->objectName());
                }
                check(findField->width() >= 80 && replaceField->width() >= 80, "Narrow Find text fields are unusably small");
                check(bounds(findField).right() <= bounds(previousMatch).left()
                      && bounds(replaceField).right() <= bounds(replaceCurrent).left(), "Narrow Find fields overlap buttons");
                check(matchStatus->property("text").toString() == "No matches", "Narrow Find does not explain no results");
                capture("daily-find-narrow");
                backend.setThemePreset("dark"); settle(80);
                capture("daily-find-narrow-dark");
                findField->forceActiveFocus();
                key(window, Qt::Key_Escape);
                check(waitUntil([&] { return !findBar->isVisible() && editor->hasActiveFocus(); }), "Escape in query field did not restore Source focus");
                QFile file(temporary.filePath("Find and replace.md"));
                check(file.open(QIODevice::ReadOnly) && file.readAll() == original.toUtf8(), "Find/Replace saved the diagnostic draft without request");
            });
        }
    }

    auto *outline = window->findChild<QObject *>("documentOutline");
    auto *outlineSearch = child("documentOutlineSearch");
    auto *outlineList = child("documentOutlineList");
    auto *workspaceButton = child("topChromeLibraryButton");
    auto *outlineEntry = child("workspaceOutlineEntry");
    if (check(outline && outlineSearch && outlineList && workspaceButton && outlineEntry, "Shipped searchable Outline controls are missing")) {
        const QString outlineSource = QString::fromUtf8("# First chapter\n\nAn opening.\n\n## Second café chapter\n\nA middle passage.\n\n## Last scene\n\nA closing.\n");
        const int middleHeading = outlineSource.indexOf("## Second");
        const int lastHeading = outlineSource.indexOf("## Last");
        backend.setThemePreset("studio");
        window->resize(1440, 800); settle(100);
        if (loadSample("Chapter outline", outlineSource)) {
            const auto openOutline = [&] {
                check(click(window, workspaceButton), "Cannot open workspace menu for Outline");
                check(waitUntil([&] { return outlineEntry->isVisible(); }), "Outline menu entry did not appear");
                check(click(window, outlineEntry), "Cannot click Document outline menu entry");
                check(waitUntil([&] { return outline->property("opened").toBool() && outlineSearch->hasActiveFocus(); }), "Outline did not open with search focus");
            };
            daily("outline-current-section-filter-and-dismiss", [&] {
                const int start = outlineSource.indexOf("middle passage");
                QMetaObject::invokeMethod(editor, "select", Q_ARG(int, start), Q_ARG(int, start + 6));
                editor->forceActiveFocus();
                settle(50);
                openOutline();
                check(outlineList->property("currentIndex").toInt() == 1, "Outline did not preselect the current chapter");
                check(enterText(window, outlineSearch, QString::fromUtf8("CAFÉ second")), "Cannot type multi-term heading search");
                check(waitUntil([&] { return outlineList->property("count").toInt() == 1; }), "Outline did not filter terms case-insensitively");
                capture("daily-outline-filtered");
                check(enterText(window, outlineSearch, "not present in any heading"), "Cannot type no-result Outline search");
                check(waitUntil([&] { return outlineList->property("count").toInt() == 0; }), "Outline no-result filter kept headings visible");
                key(window, Qt::Key_Return);
                check(outline->property("visible").toBool()
                      && editor->property("selectionStart").toInt() == start, "Empty Outline Enter navigated or dismissed unexpectedly");
                key(window, Qt::Key_Escape);
                check(waitUntil([&] { return !outline->property("visible").toBool(); }), "Escape did not dismiss Outline");
                check(editor->property("selectionStart").toInt() == start && editor->property("selectionEnd").toInt() == start + 6,
                      "Dismissing Outline changed the document selection");
                check(editor->property("text").toString() == outlineSource, "Outline search changed source text");
            });
            daily("outline-keyboard-jump-and-narrow-live-routing", [&] {
                openOutline();
                key(window, Qt::Key_Down);
                check(outlineList->property("currentIndex").toInt() == 2, "Outline Down did not move the selected result");
                key(window, Qt::Key_Return);
                check(waitUntil([&] { return !outline->property("visible").toBool() && editor->property("cursorPosition").toInt() == lastHeading; }),
                      "Outline Enter did not jump to the exact heading source offset");
                window->resize(720, 800); settle(100);
                action("liveEditToggle", 0, true);
                openOutline();
                check(enterText(window, outlineSearch, "second"), "Cannot filter Outline in narrow Live");
                capture("daily-outline-narrow-live");
                key(window, Qt::Key_Return);
                check(waitUntil([&] { return !outline->property("visible").toBool()
                    && layout->property("effectiveLayoutMode").toInt() == 0 && layout->property("liveEditEnabled").toBool()
                    && backend.liveCursor() == middleHeading; }),
                    "Narrow Outline jump did not place the Live caret at the exact heading");
                check(editor->property("text").toString() == outlineSource && !backend.modified(), "Heading navigation modified the document");
                capture("daily-outline-jump-live");
                action("sourceModeButton", 0, false);
            });
        }
    }

    // View zoom is a presentation preference. Exercise the shipped pane controls
    // independently of the existing footer/daily-writing regression counts.
    auto *zoom = window->findChild<QObject *>("paneZoomState");
    auto *commands = window->findChild<QObject *>("workspaceCommands");
    if (check(zoom && commands, "Shipped pane zoom controller is missing")) {
        const auto percent = [&](const QString &target) { return zoom->property((target + "Zoom").toUtf8()).toInt(); };
        const auto zoomItem = [&](const QString &target, const QString &suffix) {
            return window->findChild<QQuickItem *>(target + "Zoom" + suffix);
        };
        const auto settledZoom = [&] {
            check(renderBarrier(window), "Cannot render pane zoom frame");
            check(waitUntil(ready), "Pane zoom did not settle its viewport");
            check(renderBarrier(window), "Cannot render settled pane zoom frame");
        };
        const auto chooseZoom = [&](const QString &target, const QString &entry) {
            if (!check(click(window, zoomItem(target, "MenuButton")), "Cannot open " + target + " zoom menu")) return;
            auto *menu = window->findChild<QObject *>(target + "ZoomMenu");
            if (!check(menu && waitUntil([&] { return menu->property("opened").toBool(); }), "Pane zoom menu did not open")) return;
            auto *item = zoomItem(target, entry);
            if (check(item, "Missing zoom menu item: " + target + entry)) {
                check(click(window, item), "Cannot click zoom menu item: " + target + entry);
                check(waitUntil([&] { return !menu->property("visible").toBool(); }), "Pane zoom menu did not close after selection");
            }
            settledZoom();
        };
        const auto zoomCheck = [&](const QString &name, const std::function<void()> &exercise) {
            const qsizetype before = failures.size();
            exercise();
            QJsonObject controls;
            for (const QString &target : {QStringLiteral("source"), QStringLiteral("preview")}) {
                for (const QString &suffix : {QStringLiteral("OutButton"), QStringLiteral("MenuButton"), QStringLiteral("InButton")}) {
                    if (auto *item = zoomItem(target, suffix)) {
                        auto entry = rectJson(bounds(item));
                        entry.insert("visible", item->isVisible()); entry.insert("enabled", item->isEnabled());
                        controls.insert(target + "Zoom" + suffix, entry);
                    }
                }
            }
            if (layout->property("effectiveLayoutMode").toInt() == 1) {
                for (const QString &suffix : {QStringLiteral("OutButton"), QStringLiteral("MenuButton"), QStringLiteral("InButton")}) {
                    auto *sourceControl = zoomItem("source", suffix);
                    auto *previewControl = zoomItem("preview", suffix);
                    check(sourceControl && previewControl && qAbs(bounds(sourceControl).y() - bounds(previewControl).y()) <= 0.5
                        && qAbs(sourceControl->height() - previewControl->height()) <= 0.5,
                        "Source and Preview zoom controls do not share a baseline: " + suffix);
                }
            }
            paneZoomChecks.append(QJsonObject{{"name", name}, {"passed", failures.size() == before},
                {"newFailures", int(failures.size() - before)}, {"sourceZoom", percent("source")},
                {"previewZoom", percent("preview")}, {"linked", zoom->property("linked").toBool()},
                {"effectivePane", zoom->property("effectivePane").toString()},
                {"windowWidth", window->width()}, {"sourcePixelSize", window->property("editorFontPixelSize").toInt()},
                {"previewPixelSize", pane->property("textSize").toInt()}, {"controls", controls},
                {"sourceSha256", sha256(editor->property("text").toString().toUtf8())}});
        };
        window->resize(1440, 800);
        backend.setThemePreset("studio");
        backend.setOutputStyle(0);
        const QString zoomOriginal = QString::fromUtf8("# Two comfortable reading sizes\n\nSource and Preview can have different zoom without changing Markdown.\n\n## A longer passage\n\ncafé 東京 — keep the source, cursor and Undo history intact.\n");
        const QString zoomAddition = "\nAn unsaved ending stays an ordinary edit.\n";
        if (loadSample("Independent pane zoom", zoomOriginal)) {
            QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, zoomOriginal.size()), Q_ARG(QString, zoomAddition));
            const QString zoomDraft = zoomOriginal + zoomAddition;
            const int zoomSelection = zoomDraft.indexOf("different zoom");
            QMetaObject::invokeMethod(editor, "select", Q_ARG(int, zoomSelection), Q_ARG(int, zoomSelection + 14));
            action("previewSplitButton", 1, false);
            chooseZoom("source", "Reset"); chooseZoom("preview", "Reset");
            const QString initialOutput = temporary.filePath("zoom-before.html");
            check(backend.exportDocument(QUrl::fromLocalFile(initialOutput), "html"), "Cannot export initial zoom sample");
            const QString initialOutputHash = fileHash(initialOutput);
            const int outputStyle = backend.outputStyle(), outputPointSize = backend.outputPointSize();
            const QString outputFont = backend.outputFont();
            zoomCheck("independent-pane-buttons-and-presets", [&] {
                check(!zoom->property("linked").toBool() && percent("source") == 100 && percent("preview") == 100,
                      "Fresh pane zoom does not start independent at 100 percent");
                const int sourceSize = window->property("editorFontPixelSize").toInt();
                const int previewSize = pane->property("textSize").toInt();
                check(previewSize >= 17, "Standard Preview 100 percent is below the readable 17px baseline");
                check(click(window, zoomItem("source", "InButton")), "Cannot click Source zoom plus"); settledZoom();
                check(percent("source") == 110 && percent("preview") == 100
                    && window->property("editorFontPixelSize").toInt() > sourceSize
                    && pane->property("textSize").toInt() == previewSize, "Source zoom changed Preview or failed to enlarge Source");
                const int enlargedSource = window->property("editorFontPixelSize").toInt();
                check(click(window, zoomItem("preview", "InButton")), "Cannot click Preview zoom plus"); settledZoom();
                check(percent("source") == 110 && percent("preview") == 110
                    && window->property("editorFontPixelSize").toInt() == enlargedSource
                    && pane->property("textSize").toInt() > previewSize, "Preview zoom changed Source or failed to enlarge Preview");
                chooseZoom("source", "Preset75");
                check(percent("source") == 75 && zoomItem("source", "OutButton") && !zoomItem("source", "OutButton")->isEnabled(), "Minimum Source zoom still allows minus");
                chooseZoom("preview", "Preset200");
                check(percent("preview") == 200 && zoomItem("preview", "InButton") && !zoomItem("preview", "InButton")->isEnabled(), "Maximum Preview zoom still allows plus");
                chooseZoom("source", "Preset125"); chooseZoom("preview", "Preset150");
                check(editor->property("selectionStart").toInt() == zoomSelection
                    && editor->property("selectionEnd").toInt() == zoomSelection + 14, "Zoom controls changed the Source selection");
                capture("zoom-independent-split");
            });
            zoomCheck("pane-zoom-link-reset-and-active-routing", [&] {
                chooseZoom("preview", "Link");
                check(zoom->property("linked").toBool() && percent("source") == 150 && percent("preview") == 150,
                      "Link zoom did not adopt the clicked Preview pane size");
                check(click(window, zoomItem("source", "OutButton")), "Cannot decrease linked zoom"); settledZoom();
                check(percent("source") == 140 && percent("preview") == 140, "Linked minus did not adjust both panes");
                chooseZoom("source", "Link");
                check(!zoom->property("linked").toBool() && percent("source") == 140 && percent("preview") == 140,
                      "Unlink zoom changed the current pane sizes");
                chooseZoom("source", "Preset125"); chooseZoom("preview", "Preset150");
                editor->forceActiveFocus();
                check(waitUntil([&] { return zoom->property("effectivePane").toString() == "source"; }), "Source focus did not route global zoom");
                check(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QVariant("larger"))), "Cannot dispatch global Larger Text"); settledZoom();
                check(percent("source") == 135 && percent("preview") == 150, "Global Larger Text changed the wrong pane");
                check(QMetaObject::invokeMethod(pane, "focusRenderedSurface"), "Cannot focus rendered pane for global zoom");
                check(waitUntil([&] { return zoom->property("effectivePane").toString() == "preview"; }), "Preview focus did not route global zoom");
                check(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QVariant("smaller"))), "Cannot dispatch global Smaller Text"); settledZoom();
                check(percent("source") == 135 && percent("preview") == 140, "Global Smaller Text changed the wrong pane");
                chooseZoom("source", "Reset");
                check(percent("source") == 100 && percent("preview") == 140, "Source reset altered independent Preview zoom");
                chooseZoom("preview", "Reset");
                check(percent("source") == 100 && percent("preview") == 100, "Preview reset did not restore 100 percent");
            });
            zoomCheck("pane-zoom-persists-through-document-modes", [&] {
                chooseZoom("source", "Preset125"); chooseZoom("preview", "Preset150");
                for (int round = 0; round < 2; ++round) {
                    action("previewOnly", 2, false);
                    check(zoomItem("preview", "MenuButton") && zoomItem("preview", "MenuButton")->isVisible(), "Full Preview has no percentage control");
                    check(QMetaObject::invokeMethod(window, "selectWritingMode", Q_ARG(QVariant, "live")),
                          "Cannot return from Preview Only to Live");
                    check(waitUntil([&] { return layout->property("effectiveLayoutMode").toInt() == 1
                        && layout->property("liveEditEnabled").toBool() && ready(); }),
                          "Preview Only return did not restore Live Split");
                    sourceSingle();
                    check(zoomItem("source", "MenuButton") && zoomItem("source", "MenuButton")->isVisible(), "Source view has no percentage control");
                    action("previewSplitButton", 1, false);
                    check(percent("source") == 125 && percent("preview") == 150, "Changing document view lost independent zoom values");
                }
                capture("zoom-restored-split");
            });
            zoomCheck("minimum-pane-zoom-headers-and-source-safety", [&] {
                if (auto *hide = child("collapseOrganizerButton"); hide && hide->isVisible()) check(click(window, hide), "Cannot collapse organizer for narrow zoom check");
                if (auto *hide = child("collapseFilesButton"); hide && hide->isVisible()) check(click(window, hide), "Cannot collapse library for narrow zoom check");
                check(QMetaObject::invokeMethod(layout, "updateWidth", Q_ARG(QVariant, QVariant("preview")), Q_ARG(QVariant, QVariant(320))),
                      "Cannot prepare minimum Preview width");
                window->resize(803, 800); settledZoom();
                action("previewSplitButton", 1, false);
                for (const QString &target : {QStringLiteral("source"), QStringLiteral("preview")}) chooseZoom(target, "Preset200");
                settledZoom();
                auto *sourceHeader = child("documentHeader"); auto *previewHeader = child("previewHeader");
                check(sourceHeader && previewHeader && sourceHeader->width() >= 480 && sourceHeader->width() <= 484
                    && previewHeader->width() >= 320 && previewHeader->width() <= 324, "Narrow zoom fixture did not reach 480px Source and 320px Preview");
                for (const QString &target : {QStringLiteral("source"), QStringLiteral("preview")}) {
                    auto *header = target == "source" ? sourceHeader : previewHeader;
                    QRectF previous;
                    for (const QString &suffix : {QStringLiteral("OutButton"), QStringLiteral("MenuButton"), QStringLiteral("InButton")}) {
                        auto *control = zoomItem(target, suffix);
                        if (!check(header && control && control->isVisible(), "Narrow pane zoom control is hidden: " + target + suffix)) continue;
                        const QRectF rect = bounds(control);
                        check(bounds(header).adjusted(-0.5, -0.5, 0.5, 0.5).contains(rect)
                            && QRectF(QPointF(), window->size()).contains(rect), "Narrow zoom control crosses its header: " + target + suffix);
                        check(previous.isNull() || previous.right() <= rect.left() + 0.5, "Narrow zoom buttons overlap"); previous = rect;
                    }
                }
                capture("zoom-minimum-pane-headers");
                backend.setThemePreset("dark"); settledZoom(); capture("zoom-minimum-pane-headers-dark");
                chooseZoom("source", "Reset"); chooseZoom("preview", "Reset");
                check(editor->property("text").toString() == zoomDraft && backend.modified(), "Pane zoom changed the draft or dirty state");
                check(backend.outputStyle() == outputStyle && backend.outputPointSize() == outputPointSize && backend.outputFont() == outputFont,
                      "Pane zoom changed output typography");
                const QString afterOutput = temporary.filePath("zoom-after.html");
                check(backend.exportDocument(QUrl::fromLocalFile(afterOutput), "html")
                    && fileHash(afterOutput) == initialOutputHash, "Screen zoom changed exported HTML bytes");
                QMetaObject::invokeMethod(window, "undoEditing");
                check(editor->property("text").toString() == zoomOriginal, "Zoom polluted Undo of the unsaved ending");
                QMetaObject::invokeMethod(window, "redoEditing");
                check(editor->property("text").toString() == zoomDraft, "Zoom destroyed Redo of the unsaved ending");
                QFile file(temporary.filePath("Independent pane zoom.md"));
                check(file.open(QIODevice::ReadOnly) && file.readAll() == zoomOriginal.toUtf8(), "Zoom saved a diagnostic draft without request");
            });
            zoomCheck("zoom-reading-anchors-and-divider-double-click", [&] {
                window->resize(1280, 800);
                backend.setThemePreset("studio");
                settings->setProperty("synchronizedScroll", false);
                QString readingSource = "# Reading position across zoom\n\n";
                for (int index = 0; index < 70; ++index)
                    readingSource += QString("Paragraph %1 keeps a distinct place in a longer document. Changing the size of the letters should keep this passage in view, even when the lines wrap differently.\n\n").arg(index);
                if (!loadSample("Zoom reading position", readingSource)) return;
                action("previewSplitButton", 1, false);
                chooseZoom("source", "Reset"); chooseZoom("preview", "Reset");
                check(QMetaObject::invokeMethod(layout, "updateWidth", Q_ARG(QVariant, QVariant("preview")), Q_ARG(QVariant, QVariant(400))),
                      "Cannot prepare unequal writing panes");
                settledZoom();
                auto *previewScroll = pane->findChild<QQuickItem *>("previewScroll");
                auto *rendered = pane->findChild<QQuickItem *>("renderedPreview");
                if (!check(previewScroll && rendered, "Rendered reading surface is missing")) return;
                sourceScroll->setProperty("contentY", 700.0);
                previewScroll->setProperty("contentY", 1100.0);
                settledZoom();
                const auto readingAnchor = [&](QObject *owner, const char *method) {
                    QVariant result;
                    check(QMetaObject::invokeMethod(owner, method, Q_RETURN_ARG(QVariant, result)), "Cannot capture reading anchor");
                    if (result.canConvert<QJSValue>()) return result.value<QJSValue>().toVariant().toMap();
                    return result.toMap();
                };
                const QVariantMap sourceAnchor = readingAnchor(window, "captureSourceReadingAnchor");
                const QVariantMap previewAnchor = publishingDomAnchor(rendered);
                check(sourceAnchor.value("position").toInt() > 0 && previewAnchor.contains("index") && previewAnchor.value("index").toInt() > 0,
                      "Reading-position fixture did not scroll both panes into the document");
                QString anchorStage = "initial";
                const auto assertAnchor = [&](QQuickItem *field, QQuickItem *scroll, const QVariantMap &anchor, const QString &name) {
                    QRectF rectangle;
                    check(QMetaObject::invokeMethod(field, "positionToRectangle", Q_RETURN_ARG(QRectF, rectangle),
                        Q_ARG(int, anchor.value("position").toInt())), "Cannot locate preserved " + name + " reading anchor");
                    const qreal offset = field->y() + rectangle.y() - scroll->property("contentY").toReal();
                    const qreal expectedOffset = anchor.value("offset").toReal();
                    const qreal delta = offset - expectedOffset;
                    paneZoomReadings.append(QJsonObject{{"stage", anchorStage}, {"pane", name},
                        {"position", anchor.value("position").toInt()}, {"expectedOffset", expectedOffset},
                        {"actualOffset", offset}, {"delta", delta}, {"lineHeight", rectangle.height()},
                        {"positionRect", rectJson(rectangle)}, {"fieldWidth", field->width()}, {"fieldY", field->y()},
                        {"scrollY", scroll->property("contentY").toReal()}, {"contentHeight", scroll->property("contentHeight").toReal()},
                        {"sourceZoom", percent("source")}, {"previewZoom", percent("preview")},
                        {"transitionPending", window->property("changingDocumentView").toBool()},
                        {"previewRefreshPending", pane->property("viewportRefreshPending").toBool()}});
                    check(qAbs(delta) <= 2.0,
                          QString("%1 reading anchor moved at %2: position %3, expected offset %4, actual %5, delta %6, line height %7, zoom %8/%9")
                          .arg(name, anchorStage).arg(anchor.value("position").toInt()).arg(expectedOffset).arg(offset)
                          .arg(delta).arg(rectangle.height()).arg(percent("source")).arg(percent("preview")));
                };
                const auto assertBothAnchors = [&] {
                    assertAnchor(editor, sourceScroll, sourceAnchor, "Source");
                    const QVariant currentTop = publishingDomResult(rendered,
                        QString("(function(){var e=document.body.querySelectorAll('p,h1,h2,h3,h4,h5,h6,pre,table,li,img')[%1];return e?e.getBoundingClientRect().top:null;})()")
                            .arg(previewAnchor.value("index").toInt()));
                    if (!check(currentTop.isValid() && !currentTop.isNull(), "Cannot locate preserved publishing DOM reading anchor")) return;
                    const qreal expectedOffset = previewAnchor.value("offset").toReal();
                    const qreal actualOffset = currentTop.toReal();
                    const qreal delta = actualOffset - expectedOffset;
                    paneZoomReadings.append(QJsonObject{{"stage", anchorStage}, {"pane", "Preview"},
                        {"domElementIndex", previewAnchor.value("index").toInt()}, {"expectedOffset", expectedOffset},
                        {"actualOffset", actualOffset}, {"delta", delta},
                        {"scrollY", previewScroll->property("contentY").toReal()},
                        {"sourceZoom", percent("source")}, {"previewZoom", percent("preview")},
                        {"transitionPending", window->property("changingDocumentView").toBool()},
                        {"previewRefreshPending", pane->property("viewportRefreshPending").toBool()}});
                    check(qAbs(delta) <= 2.0,
                        QString("Preview DOM reading anchor moved at %1: element %2, expected offset %3, actual %4, delta %5, zoom %6/%7")
                            .arg(anchorStage).arg(previewAnchor.value("index").toInt()).arg(expectedOffset).arg(actualOffset)
                            .arg(delta).arg(percent("source")).arg(percent("preview")));
                };
                capture("zoom-scrolled-before");
                for (const QString &target : {QStringLiteral("source"), QStringLiteral("preview")}) {
                    anchorStage = target + "-125";
                    chooseZoom(target, "Preset125"); assertBothAnchors();
                    if (target == "preview") capture("zoom-scrolled-preview125");
                    anchorStage = target + "-reset100";
                    chooseZoom(target, "Reset"); assertBothAnchors();
                }
                auto *sourceHeader = child("documentHeader");
                auto *previewHeader = child("previewHeader");
                auto *divider = child("documentSplitDivider");
                if (!check(sourceHeader && previewHeader && divider, "Document divider is missing")) return;
                check(qAbs(sourceHeader->width() - previewHeader->width()) > 80, "Divider fixture is already balanced");
                const auto dividerState = [&](const QString &stage) {
                    auto *gestures = child("documentDividerGestures");
                    QJsonObject gestureState;
                    if (gestures) {
                        gestureState = rectJson(bounds(gestures));
                        gestureState.insert("class", QString::fromLatin1(gestures->metaObject()->className()));
                        gestureState.insert("enabled", gestures->isEnabled());
                        gestureState.insert("visible", gestures->isVisible());
                        gestureState.insert("pressed", gestures->property("pressed").toBool());
                        gestureState.insert("balanceOnRelease", gestures->property("balanceOnRelease").toBool());
                    }
                    paneZoomDividerEvents.append(QJsonObject{{"stage", stage}, {"divider", rectJson(bounds(divider))},
                        {"sourceWidth", sourceHeader->width()}, {"previewWidth", previewHeader->width()},
                        {"requestedPreviewWidth", layout->property("previewWidth").toReal()}, {"gestures", gestureState}});
                };
                dividerState("before");
                check(doubleClick(window, divider), "Cannot double-click the document divider");
                settledZoom();
                dividerState("after");
                check(qAbs(sourceHeader->width() - previewHeader->width()) <= 2,
                      QString("Actual divider double-click did not balance panes: %1 / %2, requested preview %3")
                          .arg(sourceHeader->width()).arg(previewHeader->width()).arg(layout->property("previewWidth").toReal()));
                anchorStage = "divider-double-click";
                assertBothAnchors();
                check(editor->property("text").toString() == readingSource && !backend.modified(), "Reading zoom or divider reset changed the document");
                capture("zoom-balanced-panes");
            });
        }
    }

#include "editoracceptancecheck.inc"
#include "panechromeacceptancecheck.inc"
#include "editinglayoutacceptancecheck.inc"

    editingLayoutCheck("literal-source-markers-typography-and-view-cycles", [&] {
        backend.setThemePreset("studio"); window->resize(1280, 820);
        layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
        settings->setProperty("autoHideChrome", false);
        settings->setProperty("writingAppearance", "manuscript");
        settings->setProperty("sourceZoom", 100);
        settings->setProperty("showMarkup", false); // Simulate the legacy persisted preference.
        const QString sample = QString::fromUtf8(
            "# A clearer place to write\n\n"
            "**Purpose:** Keep every Markdown marker visible in Source, including *emphasis*, `inline code`, and [links](https://example.com).\n\n"
            "## A little structure\n\n"
            "- **Readable lists** keep their literal markers. A longer sentence wraps beneath its own words, making the shape of a paragraph clear while you write.\n"
            "  - Nested items retain their original spaces and align their continuation lines with their own text. Café and 你好 remain untouched.\n"
            "12. Numbered items follow the same rule, even when the marker has more than one digit. Nothing is inserted into the saved Markdown.\n"
            "- [x] Task boxes remain literal and the text after the box sets the wrapping position for this longer task.\n\n"
            "> A quotation keeps its visible arrow and wraps beneath its words. The extra breathing room makes the editor easier to read.\n\n"
            "```text\n**This fenced code stays literal.**\n```\n");
        if (!loadSample("Source readability", sample)) return;
        action("sourceModeButton", 0, false); action("singleModeButton", 0, false);
        auto *quickDocument = qvariant_cast<QQuickTextDocument *>(editor->property("textDocument"));
        if (!check(quickDocument, "Source has no text document")) return;
        auto *document = quickDocument->textDocument();
        check(waitUntil([&] { return document->defaultFont().family() == "iA Writer Mono S"; }), "Source is not using bundled iA Writer Mono");
        backend.refreshSourceTypography(); renderBarrier(window);
        check(backend.hiddenRangesAt(sample.indexOf("Purpose")).isEmpty(), "Legacy setting still hides inline Markdown");
        const int marker = sample.indexOf("**Purpose");
        const auto block = document->findBlock(marker);
        for (const auto &range : block.layout()->formats())
            check(range.format.fontPointSize() != 1.0, "Source contains collapsed syntax glyphs");
        const auto bullet = document->findBlock(sample.indexOf("- **Readable"));
        check(bullet.blockFormat().textIndent() < 0 && qAbs(bullet.blockFormat().leftMargin() + bullet.blockFormat().textIndent() - document->firstBlock().blockFormat().leftMargin()) < 1, "Source list lacks a hanging indent");
        check(bullet.blockFormat().lineHeight() == 155, "Source line spacing is not applied");
        capture("source-readable-single");
        for (int mode : {1, 0, 1}) {
            action(mode ? "previewSplitButton" : "singleModeButton", mode, false);
            action("liveEditToggle", mode, true);
            action("sourceModeButton", mode, false);
            check(document->toPlainText() == sample && !backend.modified(), "Source/Live cycles altered literal Markdown or dirty state");
            check(backend.hiddenRangesAt(marker).isEmpty(), "Returning to Source hid Markdown markers");
        }
        capture("source-readable-split");
        backend.save(); QFile saved(backend.fileUrl().toLocalFile());
        check(saved.open(QIODevice::ReadOnly) && saved.readAll() == sample.toUtf8(), "Source typography changed saved bytes");
        editor->forceActiveFocus(); editor->setProperty("cursorPosition", sample.indexOf("- **Readable"));
        key(window, Qt::Key_unknown, "> ");
        QString edited = sample; edited.insert(sample.indexOf("- **Readable"), "> ");
        check(waitUntil([&] { return document->toPlainText() == edited; }), "Source prefix typing failed");
        settle();
        QMetaObject::invokeMethod(window, "undoEditing");
        check(document->toPlainText() == sample, "App Undo stopped at presentation instead of reversing typing");
        QMetaObject::invokeMethod(window, "redoEditing");
        check(document->toPlainText() == edited, "App Redo did not restore exact prefix typing");
    });

    editingLayoutCheck("cycle123-manuscript-headings-and-code-appearance", [&] {
        backend.setThemePreset("studio"); window->resize(1440, 900);
        layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
        settings->setProperty("autoHideChrome", false);
        if (zoom) QMetaObject::invokeMethod(zoom, "setZoom", Q_ARG(QVariant, "source"), Q_ARG(QVariant, 100));
        const QString heading = "A readable heading keeps its words on the body column while its Markdown markers hang beside them. ";
        const QString sample = QString::fromUtf8("# Source appearance\n\n")
            + "### " + heading.repeated(3) + "\n\n"
              "The source stays ordinary Markdown. Café and 你好 stay exact through every appearance change.\n\n"
              "- A longer list item keeps its continuation lines under its own words while the prefix remains visible.\n"
              "    - A nested item retains its original spaces.\n\n"
              "```javascript\nfunction greet(name) {\n    const message = \"Hello, \" + name;\n\n"
              "    // The empty line shares the block background.\n    return message;\n}\n```\n\n"
              "Inline `greet(name)` has a compact background within this ordinary paragraph.\n";
        if (!loadSample("Cycle123 source appearance", sample)) return;
        auto *quickDocument = qvariant_cast<QQuickTextDocument *>(editor->property("textDocument"));
        if (!check(quickDocument && commands, "Cycle123 source document or appearance commands are missing")) return;
        auto *document = quickDocument->textDocument();
        const QString path = backend.fileUrl().toLocalFile();
        const QString originalHash = fileHash(path);
        const auto appearance = [&](const QString &command, const QString &expected) {
            const bool wasModified = backend.modified();
            check(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, command)),
                  "Cannot dispatch Source appearance command: " + command);
            check(waitUntil([&] { return window->property("activeWritingAppearance").toString() == expected; }),
                  "Source appearance command did not select " + expected);
            check(renderBarrier(window), "Cannot render Source appearance: " + expected);
            check(document->toPlainText() == sample && backend.modified() == wasModified, "Source appearance changed Markdown or dirty state: " + expected);
        };
        appearance("writingManuscript", "manuscript");
        action("singleModeButton", 0, false);
        backend.refreshSourceTypography(); renderBarrier(window);
        const auto headingBlock = document->findBlock(sample.indexOf("### "));
        const auto bodyBlock = document->findBlock(sample.indexOf("The source stays"));
        document->documentLayout()->documentSize();
        if (check(headingBlock.layout()->lineCount() > 1 && bodyBlock.layout()->lineCount() > 0,
                  "Cycle123 heading geometry did not produce wrapped text")) {
            const qreal bodyX = bodyBlock.layout()->lineAt(0).cursorToX(0);
            const auto first = headingBlock.layout()->lineAt(0);
            const qreal headingX = first.cursorToX(4);
            const qreal markerX = first.cursorToX(0);
            const qreal wrapX = headingBlock.layout()->lineAt(1).x();
            report.insert("cycle123HeadingGeometry", QJsonObject{{"bodyX", bodyX}, {"headingTextX", headingX},
                {"markerX", markerX}, {"continuationX", wrapX}});
            check(headingBlock.blockFormat().textIndent() < 0 && markerX < bodyX
                && qAbs(headingX - bodyX) < 1 && qAbs(wrapX - bodyX) < 1,
                  "Native Manuscript layout did not hang heading markers beside the aligned body column; see cycle123HeadingGeometry");
        }
        const auto fence = document->findBlock(sample.indexOf("```javascript"));
        const auto close = document->findBlock(sample.indexOf("```\n\nInline"));
        const QBrush background = fence.blockFormat().background();
        check(background.style() != Qt::NoBrush, "Fenced code has no full-width block background");
        for (auto block = fence; block.isValid() && block.blockNumber() <= close.blockNumber(); block = block.next())
            check(block.blockFormat().background() == background && block.blockFormat().topMargin() == 0
                && block.blockFormat().bottomMargin() == 0, "Fenced code background is interrupted, including its empty line");
        check(close.next().blockFormat().background().style() == Qt::NoBrush, "Fenced background leaked into following prose");
        capture("manuscript-hanging-headings-code-blocks");
        sourceScroll->setProperty("contentY", document->documentLayout()->blockBoundingRect(fence).top() - 160);
        renderBarrier(window);
        capture("manuscript-continuous-code-background");
        sourceScroll->setProperty("contentY", 0);
        appearance("writingCode", "code");
        const QFontMetricsF metrics(qvariant_cast<QFont>(editor->property("font")));
        check(qAbs(metrics.horizontalAdvance('i') - metrics.horizontalAdvance('W')) < 0.1, "Code appearance is not monospace");
        for (auto block = document->begin(); block.isValid(); block = block.next())
            check(block.blockFormat().lineHeight() == 135 && block.blockFormat().leftMargin() == 0
                && block.blockFormat().textIndent() == 0, "Code appearance retained prose indentation or line spacing");
        action("previewSplitButton", 1, false);
        capture("code-style-light");
        backend.setThemePreset("dark"); action("singleModeButton", 0, false);
        capture("code-style-dark");
        backend.setThemePreset("studio");
        for (const auto &choice : {qMakePair(QString("writingBook"), QString("book")),
                qMakePair(QString("writingEditorial"), QString("editorial")),
                qMakePair(QString("writingManuscript"), QString("manuscript"))})
            appearance(choice.first, choice.second);
        backend.save();
        check(fileHash(path) == originalHash, "Appearance changes altered the sample's saved UTF-8 bytes");
        appearance("writingCode", "code");
        const int position = sample.indexOf("    const message");
        editor->setProperty("cursorPosition", position); editor->forceActiveFocus();
        key(window, Qt::Key_Tab, "\t");
        QString indented = sample; indented.insert(position, "    ");
        check(waitUntil([&] { return document->toPlainText() == indented; }), "Native Code Tab did not indent the line");
        key(window, Qt::Key_Backtab, {}, Qt::ShiftModifier);
        check(waitUntil([&] { return document->toPlainText() == sample; }), "Native Code Shift-Tab did not remove one indent");
        QMetaObject::invokeMethod(window, "undoEditing");
        check(document->toPlainText() == indented, "Native Code outdent was not a single Undo operation");
        QMetaObject::invokeMethod(window, "undoEditing");
        check(document->toPlainText() == sample, "Native Code Tab was not a single Undo operation");
        const int newline = sample.indexOf('\n', sample.indexOf("function greet"));
        editor->setProperty("cursorPosition", newline); editor->forceActiveFocus();
        key(window, Qt::Key_Return, "\n");
        QString entered = sample; entered.insert(newline, "\n    ");
        check(waitUntil([&] { return document->toPlainText() == entered; }), "Native Code Return did not keep brace indentation");
        QMetaObject::invokeMethod(window, "undoEditing");
        check(document->toPlainText() == sample, "Native Code Return was not a single Undo operation");
        appearance("writingManuscript", "manuscript");
    });

    editingLayoutCheck("publishing-web-pdf-renderers-preserve-source", [&] {
        window->resize(1280, 720);
        layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
        settings->setProperty("autoHideChrome", false);
        settings->setProperty("publishingFormat", "web");
        const QString sample = QString::fromUtf8("# Actual publishing output\n\nA café paragraph keeps its Markdown.\n\n| Name | Value |\n| --- | --- |\n| Sample | 42 |\n");
        if (!loadSample("Publishing output", sample)) return;
        action("previewSplitButton", 1, false);
        auto *web = pane->findChild<QQuickItem *>("renderedPreview");
        auto *pdfScroll = pane->findChild<QQuickItem *>("previewScroll");
        auto *webChoice = buttons.value("webPublishingButton"), *pdfChoice = buttons.value("pdfPublishingButton");
        if (!check(web && pdfScroll && webChoice && pdfChoice, "Publishing renderer controls are missing")) return;
        const int revision = backend.documentRevision();
        const QVariantMap dom = publishingDomResult(web,
            "({title:document.querySelector('h1')?document.querySelector('h1').innerText.trim():'',rows:Array.from(document.querySelectorAll('table tr')).map(function(r){return Array.from(r.querySelectorAll('th,td')).map(function(c){return c.innerText.trim();});})})").toMap();
        report.insert("publishingDom", QJsonObject::fromVariantMap(dom));
        const QVariantList rows = dom.value("rows").toList();
        const bool expectedTable = rows.size() == 2
            && rows.at(0).toList() == QVariantList{QString("Name"), QString("Value")}
            && rows.at(1).toList() == QVariantList{QString("Sample"), QString("42")};
        check(web->isVisible() && !pdfScroll->isVisible() && dom.value("title").toString() == "Actual publishing output"
            && expectedTable, "Web publishing did not render the actual exported HTML DOM");
        capture("publishing-web-actual-html");
        check(click(window, pdfChoice), "Cannot choose PDF publishing");
        check(waitUntil([&] { return ready() && pane->property("publishingMode").toString() == "pdf"; }), "PDF publishing did not finish loading");
        const QUrl pdfUrl = pane->property("outputUrl").toUrl();
        QFile pdfFile(pdfUrl.toLocalFile());
        check(pdfScroll->isVisible() && !web->isVisible() && pane->property("errorText").toString().isEmpty()
            && pdfFile.open(QIODevice::ReadOnly) && pdfFile.read(5) == "%PDF-", "PDF publishing did not load a real PDF document");
        check(renderBarrier(window), "Cannot render the publishing PDF page");
        capture("publishing-pdf-actual-pages");
        auto *nativeWeb = window->findChild<QObject *>("native_webPreview");
        check(nativeWeb && !nativeWeb->property("checked").toBool(), "Native Web command disagrees with PDF publishing");
        check(nativeWeb && QMetaObject::invokeMethod(nativeWeb, "triggered"), "Cannot select Web publishing through the native View command");
        check(waitUntil([&] { return ready() && pane->property("publishingMode").toString() == "web"; }), "Native Web command did not select Web output");
        check(click(window, webChoice), "Cannot reselect Web publishing");
        check(waitUntil([&] { return ready() && pane->property("publishingMode").toString() == "web"; }), "Web publishing return did not finish loading");
        check(editor->property("text").toString() == sample && backend.documentRevision() == revision && !backend.modified(),
            "Publishing format selection changed canonical source or dirty state");
    });
#include "publishingthemeacceptancecheck.inc"
    editingLayoutCheck("document-chrome-input-hover-and-always-visible-setting", [&] {
        settings->setProperty("autoHideChrome", true);
        auto *header = child("topChrome");
        auto *toggle = window->findChild<QObject *>("nativeAutoHideDocumentChrome");
        auto *fade = window->findChild<QObject *>("native_titleBarFade");
        auto *always = window->findChild<QObject *>("native_titleBarAlways");
        if (!check(header && toggle && fade && always, "Document chrome menu controls are missing")) return;
        check(fade->property("text").toString() == "Fade In/Out"
            && always->property("text").toString() == "Always Show", "Native Title Bar labels differ from the requested choices");
        check(fade->property("checked").toBool() && !always->property("checked").toBool(),
            "Native Title Bar choices disagree with enabled auto-hide");
        const auto geometry = [&] {
            QMap<QString, QRectF> result;
            for (auto *item : {child("editorPane"), sourceScroll, pane, header, footer})
                if (item) result.insert(item->objectName(), bounds(item));
            return result;
        };
        const auto baseline = geometry();
        const auto movePointer = [&](const QPointF &point) {
            QMouseEvent event(QEvent::MouseMove, point, point, window->mapToGlobal(point.toPoint()), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &event); QCoreApplication::processEvents();
        };
        const QPointF writingPoint = bounds(sourceScroll).center();
        movePointer(writingPoint);
        editor->forceActiveFocus(Qt::MouseFocusReason);
        key(window, Qt::Key_unknown, "x");
        check(waitUntil([&] { return window->property("documentChromeHidden").toBool()
            && !header->property("documentChromeVisible").toBool() && !footer->property("chromeVisible").toBool(); }), "Source typing did not hide document bars");
        check(geometry() == baseline, "Hiding document bars changed writing geometry");
        capture("document-chrome-hidden-writing");
        movePointer(QPointF(window->width() - 4, 4));
        check(waitUntil([&] { return header->property("documentChromeVisible").toBool(); })
            && !footer->property("chromeVisible").toBool(), "Top edge did not independently reveal its document bar");
        movePointer(writingPoint);
        settle(200);
        check(header->property("documentChromeVisible").toBool() && !footer->property("chromeVisible").toBool(),
            "Top bar did not stay independently revealed after pointer departure");
        check(geometry() == baseline, "Leaving the top edge changed writing geometry");
        editor->forceActiveFocus(Qt::MouseFocusReason);
        key(window, Qt::Key_unknown, "y");
        check(waitUntil([&] { return !header->property("documentChromeVisible").toBool()
            && !footer->property("chromeVisible").toBool(); }), "Source typing did not clear the top-edge reveal");
        movePointer(QPointF(window->width() - 4, window->height() - 4));
        check(waitUntil([&] { return footer->property("chromeVisible").toBool(); })
            && !header->property("documentChromeVisible").toBool(), "Bottom edge did not independently reveal its document bar");
        movePointer(bounds(pane).center());
        settle(200);
        check(footer->property("chromeVisible").toBool() && !header->property("documentChromeVisible").toBool(),
            "Bottom bar did not stay independently revealed after pointer departure");
        check(geometry() == baseline, "Revealing document bars changed writing geometry");
        capture("document-chrome-bottom-edge-reveal");
        check(click(window, pane), "Cannot click Publishing after revealing the bottom bar");
        settle(200);
        check(footer->property("chromeVisible").toBool() && !header->property("documentChromeVisible").toBool(),
            "A Publishing click cleared the bottom-edge reveal without scrolling or editing");
        check(geometry() == baseline, "A Publishing click changed writing geometry");
        QWheelEvent wheel(bounds(pane).center(), window->mapToGlobal(bounds(pane).center().toPoint()), QPoint(), QPoint(0, -120),
            Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(window, &wheel); QCoreApplication::processEvents();
        check(waitUntil([&] { return window->property("documentChromeHidden").toBool()
            && !header->property("documentChromeVisible").toBool() && !footer->property("chromeVisible").toBool(); }),
            "Publishing wheel interaction did not clear the bottom-edge reveal");
        for (int cycle = 0; cycle < 2; ++cycle) {
            movePointer(QPointF(window->width() - 4, 4));
            check(waitUntil([&] { return header->property("documentChromeVisible").toBool(); })
                && !footer->property("chromeVisible").toBool(), "Repeated top-edge reveal was not independent");
            movePointer(QPointF(window->width() - 4, window->height() - 4));
            check(waitUntil([&] { return footer->property("chromeVisible").toBool(); }), "Repeated bottom-edge reveal failed");
            movePointer(writingPoint);
            settle(200);
            check(header->property("documentChromeVisible").toBool() && footer->property("chromeVisible").toBool(),
                "Revealing another edge or leaving it discarded a latched bar");
            editor->forceActiveFocus(Qt::MouseFocusReason);
            key(window, Qt::Key_unknown, "z");
            check(waitUntil([&] { return !header->property("documentChromeVisible").toBool()
                && !footer->property("chromeVisible").toBool(); }), "Repeated source typing did not reset both edge reveals");
            check(geometry() == baseline, "Repeated edge reveals changed writing geometry");
        }
        check(QMetaObject::invokeMethod(toggle, "triggered"), "Cannot choose native always-visible document bars");
        check(waitUntil([&] { return !settings->property("autoHideChrome").toBool() && footer->property("chromeVisible").toBool(); }),
            "Native View toggle did not restore always-visible document bars");
        check(always->property("checked").toBool() && !fade->property("checked").toBool(),
            "Native checkbox did not synchronize Title Bar choices");
        settings->setProperty("titleBarMode", 0);
        const int toolbarPreference = settings->property("toolbarVisibilityMode").toInt();
        check(QMetaObject::invokeMethod(always, "triggered"), "Cannot select Title Bar Always Show");
        movePointer(writingPoint);
        editor->forceActiveFocus(Qt::MouseFocusReason);
        const QString alwaysText = editor->property("text").toString();
        key(window, Qt::Key_unknown, "a");
        check(waitUntil([&] { return editor->property("text").toString() != alwaysText; }), "Always Show typing did not edit source");
        check(!settings->property("autoHideChrome").toBool() && !toggle->property("checked").toBool()
            && always->property("checked").toBool() && !fade->property("checked").toBool()
            && header->property("documentChromeVisible").toBool() && footer->property("chromeVisible").toBool()
            && header->property("titleContentOpacity").toReal() == 1,
            "Always Show did not retain document bars while typing with legacy fade preference");
        settings->setProperty("titleBarMode", 1);
        check(QMetaObject::invokeMethod(fade, "triggered"), "Cannot select Title Bar Fade In/Out");
        const QString fadeText = editor->property("text").toString();
        key(window, Qt::Key_unknown, "f");
        check(waitUntil([&] { return editor->property("text").toString() != fadeText
            && !header->property("documentChromeVisible").toBool() && !footer->property("chromeVisible").toBool(); }),
            "Fade In/Out did not restore hiding during actual typing");
        check(settings->property("autoHideChrome").toBool() && toggle->property("checked").toBool()
            && fade->property("checked").toBool() && !always->property("checked").toBool()
            && header->property("titleContentOpacity").toReal() == 0
            && settings->property("toolbarVisibilityMode").toInt() == toolbarPreference,
            "Title Bar Fade selection changed toolbar preference or disagreed with checkbox");
        movePointer(QPointF(window->width() - 4, 4));
        check(waitUntil([&] { return header->property("documentChromeVisible").toBool(); }), "Fade selection lost top-edge reveal");
        movePointer(writingPoint);
        settle(200);
        check(header->property("documentChromeVisible").toBool() && header->property("titleContentOpacity").toReal() == 1,
            "Fade selection lost sticky title reveal after pointer departure");
        check(geometry() == baseline, "Native chrome preference changed writing geometry");
    });

    if (auto *about = window->findChild<QObject *>("aboutDialog")) {
        check(QMetaObject::invokeMethod(about, "open"), "Cannot open bundled About dialog");
        check(waitUntil([&] {
            // Offscreen platforms need an explicit frame for render-thread
            // popup animations while the diagnostic drives its event loop.
            if (QGuiApplication::platformName() == "offscreen") window->grabWindow();
            return about->property("opened").toBool();
        }), "About dialog did not open");
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
