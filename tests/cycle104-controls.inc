namespace {
qreal cycle104BodyLineHeight(QObject *surface) {
    auto *quick = qvariant_cast<QQuickTextDocument *>(surface->property("textDocument"));
    if (!quick || !quick->textDocument()) return 0;
    for (auto block = quick->textDocument()->begin(); block.isValid(); block = block.next()) {
        if (block.text().contains(QString::fromUtf8("café")) && block.layout() && block.layout()->lineCount() > 0)
            return block.layout()->lineAt(0).height();
    }
    return 0;
}
bool cycle104HeaderControlsFit(QObject *window) {
    QList<QPair<QString, QRectF>> visible;
    for (const char *name : {"workspaceFolderButton", "newDocumentButton", "collapseFilesButton",
                            "collapseOrganizerButton", "restoreOrganizerButton", "restoreFilesButton",
                            "documentBackButton", "documentForwardButton", "documentSearchButton",
                            "compactBoldButton", "compactItalicButton", "compactLinkButton", "compactParagraphButton",
                            "compactFormatButton", "sourceZoomOutButton", "sourceZoomMenuButton", "sourceZoomInButton",
                            "previewZoomOutButton", "previewZoomMenuButton", "previewZoomInButton",
                            "workspaceAppearanceButton", "exportHubButton", "topChromeLibraryButton"}) {
        auto *item = window->findChild<QQuickItem *>(name);
        if (!item || !item->isVisible()) continue;
        auto *header = item->parentItem();
        while (header && header->objectName() != "documentHeader"
                      && header->objectName() != "previewHeader"
                      && header->objectName() != "filesHeader"
                      && header->objectName() != "organizerHeader") header = header->parentItem();
        if (!header || item->width() < 1 || item->height() < 1) return false;
        const auto bounds = cycle103SceneBounds(item);
        if (!cycle103SceneBounds(header).adjusted(-0.1, -0.1, 0.1, 0.1).contains(bounds)) return false;
        for (const auto &[other, rectangle] : visible) {
            if (bounds.intersected(rectangle).width() > 0.1 && bounds.intersected(rectangle).height() > 0.1) {
                qWarning() << "Overlapping header controls:" << name << bounds << other << rectangle;
                return false;
            }
        }
        visible.append({QString::fromLatin1(name), bounds});
    }
    return true;
}
}

void FomawriteTest::headerQuickZoomPreservesMarkdownAndUndo() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    const QByteArray source = QString::fromUtf8("# Quick text size\n\nA **bold** café draft — 你好.\n").toUtf8();
    const QString path = directory.filePath("Quick text size.md");
    QVERIFY(cycle103Write(path, source));
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *settings = window->findChild<QObject *>("workspaceSettings");
    auto *zoom = window->findChild<QObject *>("paneZoomState");
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    auto *preview = pane ? pane->findChild<QQuickItem *>("renderedPreview") : nullptr;
    auto *visual = pane ? pane->findChild<QQuickItem *>("visualEditor") : nullptr;
    QVERIFY(quick && settings && zoom && layout && editor && pane && preview && visual);
    if (QGuiApplication::platformName() == "cocoa") quick->setFlag(Qt::WindowDoesNotAcceptFocus);
    Cycle106MouseInputIsolation input(quick);
    quick->resize(1440, 900); quick->show();
    QVERIFY(QTest::qWaitForWindowExposed(quick));
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    backend.setOutputStyle(6);
    const auto pointSize = backend.outputPointSize(); const auto font = backend.outputFont();
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    auto click = [&](const char *name) { return cycle102Click(quick, window->findChild<QQuickItem *>(name)); };
    QVERIFY(click("sourceZoomInButton"));
    QCOMPARE(zoom->property("sourceZoom").toInt(), 110);
    QCOMPARE(zoom->property("previewZoom").toInt(), 100);
    QVERIFY(!backend.modified()); QVERIFY(!editor->property("canUndo").toBool());
    QVERIFY(click("sourceZoomOutButton"));
    QCOMPARE(zoom->property("sourceZoom").toInt(), 100);
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, editor->property("text").toString().size()), Q_ARG(QString, QStringLiteral("Unsaved addition."))));
    const QString draft = editor->property("text").toString();
    QVERIFY(draft.endsWith("Unsaved addition.")); QVERIFY(backend.modified());
    QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 5), Q_ARG(int, 12)));
    layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "setDocumentView", Q_ARG(QVariant, 1)));
    QTRY_VERIFY(editor->isVisible() && pane->isVisible());
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_VERIFY(cycle104BodyLineHeight(editor) > 0 && cycle104BodyLineHeight(preview) > 0);
    const auto sourceFont = editor->property("font").value<QFont>();
    const auto previewFont = preview->property("font").value<QFont>();
    const qreal sourceLine = cycle104BodyLineHeight(editor), previewLine = cycle104BodyLineHeight(preview);
    QVERIFY(click("sourceZoomInButton"));
    QTRY_VERIFY(cycle104BodyLineHeight(editor) > sourceLine);
    QCOMPARE(preview->property("font").value<QFont>(), previewFont);
    QCOMPARE(cycle104BodyLineHeight(preview), previewLine);
    QCOMPARE(editor->property("selectionStart").toInt(), 5); QCOMPARE(editor->property("selectionEnd").toInt(), 12);
    const auto enlargedSource = editor->property("font").value<QFont>();
    QVERIFY(click("previewZoomInButton"));
    QTRY_VERIFY(preview->property("font").value<QFont>().pixelSize() > previewFont.pixelSize());
    QTRY_VERIFY(cycle104BodyLineHeight(preview) > previewLine);
    QCOMPARE(editor->property("font").value<QFont>(), enlargedSource);
    QCOMPARE(zoom->property("sourceZoom").toInt(), 110); QCOMPARE(zoom->property("previewZoom").toInt(), 110);
    QVERIFY(click("sourceZoomOutButton")); QVERIFY(click("previewZoomOutButton"));
    QTRY_COMPARE(editor->property("font").value<QFont>(), sourceFont);
    QTRY_COMPARE(preview->property("font").value<QFont>(), previewFont);
    // Visual Edit shares the right pane's independent zoom, including in Split.
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, "visual")));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "setDocumentView", Q_ARG(QVariant, 1)));
    QTRY_VERIFY(editor->isVisible() && visual->isVisible());
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    const int visualSize = visual->property("font").value<QFont>().pixelSize();
    QTRY_VERIFY(cycle104BodyLineHeight(visual) > 0);
    const qreal visualLine = cycle104BodyLineHeight(visual);
    QVERIFY(click("previewZoomInButton"));
    QTRY_VERIFY(visual->property("font").value<QFont>().pixelSize() > visualSize);
    QTRY_VERIFY(cycle104BodyLineHeight(visual) > visualLine);
    QCOMPARE(editor->property("font").value<QFont>(), sourceFont);
    QCOMPARE(zoom->property("sourceZoom").toInt(), 100);
    QCOMPARE(zoom->property("previewZoom").toInt(), 110);
    for (const QString &mode : {QStringLiteral("source"), QStringLiteral("preview"), QStringLiteral("visual")}) {
        QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, mode)));
        QTRY_VERIFY(!window->property("changingDocumentView").toBool());
        QCOMPARE(zoom->property("sourceZoom").toInt(), 100); QCOMPARE(zoom->property("previewZoom").toInt(), 110);
        QCOMPARE(editor->property("text").toString(), draft);
    }
    QCOMPARE(backend.outputStyle(), 6); QCOMPARE(backend.outputPointSize(), pointSize); QCOMPARE(backend.outputFont(), font);
    QCOMPARE(editor->property("selectionStart").toInt(), 5); QCOMPARE(editor->property("selectionEnd").toInt(), 12);
    QVERIFY(backend.modified());
    QVERIFY(QMetaObject::invokeMethod(editor, "undo")); QCOMPARE(editor->property("text").toString(), QString::fromUtf8(source));
    QVERIFY(QMetaObject::invokeMethod(editor, "redo")); QCOMPARE(editor->property("text").toString(), draft);
    QCOMPARE(cycle103Read(path), source);
}

void FomawriteTest::headerQuickControlsStayReachableWhenResized() {
    Cycle103SettingsSnapshot isolatedSettings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *zoom = window->findChild<QObject *>("paneZoomState");
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    QVERIFY(quick && zoom && layout && editor && pane);
    if (QGuiApplication::platformName() == "cocoa") quick->setFlag(Qt::WindowDoesNotAcceptFocus);
    Cycle106MouseInputIsolation input(quick);
    quick->resize(1440, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    editor->setProperty("text", "# Quick controls\n\nA café draft stays within reach.\n");
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    for (bool dark : {false, true}) {
        backend.setThemePreset(dark ? "dark" : "studio");
        for (int mode : {0, 1, 2}) {
            for (int width : {1440, 1100, 980, 900, 820, 803, 720}) {
                quick->resize(width, 800);
                QVERIFY(QMetaObject::invokeMethod(window.data(), "setDocumentView", Q_ARG(QVariant, mode)));
                QTRY_VERIFY(!window->property("changingDocumentView").toBool());
                QTRY_VERIFY(cycle104HeaderControlsFit(window.data()));
                for (const QString &target : {QStringLiteral("source"), QStringLiteral("preview")}) {
                    const bool visible = target == "source" ? editor->isVisible() : pane->isVisible();
                    if (!visible) continue;
                    auto *minus = window->findChild<QQuickItem *>(target + "ZoomOutButton");
                    auto *plus = window->findChild<QQuickItem *>(target + "ZoomInButton");
                    auto *menu = window->findChild<QQuickItem *>(target + "ZoomMenuButton");
                    QVERIFY(minus && plus && menu && minus->isVisible() && plus->isVisible() && menu->isVisible());
                    QVERIFY(cycle102Click(quick, plus)); QCOMPARE(zoom->property(qPrintable(target + "Zoom")).toInt(), 110);
                    QVERIFY(cycle102Click(quick, minus)); QCOMPARE(zoom->property(qPrintable(target + "Zoom")).toInt(), 100);
                    QVERIFY(cycle102Click(quick, menu));
                    auto *popup = window->findChild<QObject *>(target + "ZoomMenu"); QVERIFY(popup);
                    QTRY_VERIFY(popup->property("opened").toBool());
                    auto *parent = qvariant_cast<QQuickItem *>(popup->property("parent")); QVERIFY(parent);
                    const QRectF bounds = parent->mapRectToScene(QRectF(popup->property("x").toReal(), popup->property("y").toReal(), popup->property("width").toReal(), popup->property("height").toReal()));
                    QVERIFY(QRectF(QPointF(), quick->size()).adjusted(-1, -1, 1, 1).contains(bounds));
                    QVERIFY(QMetaObject::invokeMethod(popup, "close")); QTRY_VERIFY(!popup->property("visible").toBool());
                }
            }
        }
    }
}

void FomawriteTest::folderHeaderButtonRequestsFolderPicker() {
    // Exercise the real header without Main's native dialog handler so the test
    // verifies the click contract without opening a blocking system folder sheet.
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    QVERIFY(QDir(directory.path()).mkpath("Writing"));
    Backend backend;
    auto *library = qobject_cast<FileLibrary *>(backend.library()); QVERIFY(library);
    library->setRootFolder(QUrl::fromLocalFile(directory.filePath("Writing")));
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine);
    component.setData(R"QML(
import QtQuick
import QtQuick.Window
Window {
    id: fixture; width: 1100; height: 180; visible: true
    property bool isMac: false
    property string lastWritingSurface: "source"
    property bool canFormatSource: false
    QtObject { id: preferences; property int titleBarMode: 1; property int toolbarVisibilityMode: 1; property int writingSize: 16 }
    QtObject { id: state }
    QtObject {
        id: zoomPreferences
        property int sourceZoom: 100; property int previewZoom: 100
        property bool linkedZoom: false; property int zoomRevision: 1; property int writingSize: 16
    }
    PaneZoomState { id: zoom; settings: zoomPreferences }
    Item { id: organizer; x: 0; width: 184 }
    Item { id: files; x: 185; width: 232 }
    Item { id: editor; x: 418; width: 682 }
    Item { id: preview; visible: false; property bool visualEditEnabled: false }
    WorkspaceHeader {
        anchors.left: parent.left; anchors.right: parent.right
        window: fixture; settings: preferences; layoutState: state; zoomController: zoom
        organizerSlot: organizer; filesSlot: files; editorPane: editor; previewPane: preview
    }
}
)QML", QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *header = window->findChild<QObject *>("topChrome");
    auto *folder = window->findChild<QQuickItem *>("workspaceFolderButton");
    QVERIFY(quick && header && folder);
    quick->requestActivate();
    QSignalSpy actions(header, SIGNAL(actionRequested(QString,QVariant)));
    QVERIFY(actions.isValid());
    QCOMPARE(folder->property("text").toString(), QStringLiteral("Writing"));
    QVERIFY(cycle102Click(quick, folder));
    QCOMPARE(actions.size(), 1);
    const auto action = actions.takeFirst();
    QCOMPARE(action[0].toString(), QStringLiteral("chooseFolder"));
    QCOMPARE(qvariant_cast<QObject *>(action[1]), folder);
    QCOMPARE(library->rootFolder(), QUrl::fromLocalFile(QFileInfo(directory.filePath("Writing")).canonicalFilePath()));
}
