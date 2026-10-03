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
                            "compactFormatButton", "workspaceZoomOutButton", "workspaceZoomInButton",
                            "workspaceAppearanceButton", "exportHubButton", "topChromeLibraryButton"}) {
        auto *item = window->findChild<QQuickItem *>(name);
        if (!item || !item->isVisible()) continue;
        auto *header = item->parentItem();
        while (header && header->objectName() != "documentHeader"
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
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *settings = window->findChild<QObject *>("workspaceSettings");
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *editor = window->findChild<QObject *>("sourceEditor");
    auto *previewPane = window->findChild<QQuickItem *>("previewPane");
    auto *preview = previewPane ? previewPane->findChild<QQuickItem *>("renderedPreview") : nullptr;
    auto *visual = previewPane ? previewPane->findChild<QQuickItem *>("visualEditor") : nullptr;
    auto *minus = window->findChild<QQuickItem *>("workspaceZoomOutButton");
    auto *plus = window->findChild<QQuickItem *>("workspaceZoomInButton");
    QVERIFY(quick && settings && layout && editor && preview && visual && minus && plus);
    quick->resize(1280, 900); quick->show(); quick->requestActivate();
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    backend.setOutputStyle(6);
    const auto pointSize = backend.outputPointSize();
    const auto font = backend.outputFont();
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });

    // The user-facing controls must change text size without creating an edit.
    QVERIFY(cycle102Click(quick, plus));
    QCOMPARE(settings->property("writingSize").toInt(), 18);
    QVERIFY(!backend.modified());
    QVERIFY(!editor->property("canUndo").toBool());
    QVERIFY(cycle102Click(quick, minus));
    QCOMPARE(settings->property("writingSize").toInt(), 16);
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, editor->property("text").toString().size()),
                                     Q_ARG(QString, QStringLiteral("Unsaved addition."))));
    const QString draft = editor->property("text").toString();
    QVERIFY(backend.modified());

    for (int mode = 0; mode < 3; ++mode) {
        layout->setProperty("layoutMode", mode == 0 ? 0 : 2);
        layout->setProperty("visualEditEnabled", mode == 2);
        QObject *surface = mode == 0 ? editor : mode == 1 ? preview : visual;
        QTRY_VERIFY(qobject_cast<QQuickItem *>(surface)->isVisible());
        QTest::qWait(180); // Wait for the preview's Markdown reparse/style pass too.
        QTRY_VERIFY(cycle104BodyLineHeight(surface) > 0);
        const qreal beforeLine = cycle104BodyLineHeight(surface);
        const int before = surface->property("font").value<QFont>().pixelSize();
        QVERIFY(cycle102Click(quick, minus));
        QCOMPARE(settings->property("writingSize").toInt(), 14);
        QTest::qWait(180);
        QTRY_VERIFY(cycle104BodyLineHeight(surface) < beforeLine);
        QVERIFY(cycle102Click(quick, plus));
        QCOMPARE(settings->property("writingSize").toInt(), 16);
        QTest::qWait(180);
        QTRY_COMPARE(cycle104BodyLineHeight(surface), beforeLine);
        QVERIFY(cycle102Click(quick, plus));
        QCOMPARE(settings->property("writingSize").toInt(), 18);
        QTRY_VERIFY(surface->property("font").value<QFont>().pixelSize() > before);
        QTest::qWait(180);
        QTRY_VERIFY(cycle104BodyLineHeight(surface) > beforeLine);
        const qreal largerLine = cycle104BodyLineHeight(surface);
        const int larger = surface->property("font").value<QFont>().pixelSize();
        QVERIFY(cycle102Click(quick, plus));
        QCOMPARE(settings->property("writingSize").toInt(), 20);
        QTRY_VERIFY(surface->property("font").value<QFont>().pixelSize() > larger);
        QTest::qWait(180);
        QTRY_VERIFY(cycle104BodyLineHeight(surface) > largerLine);
        QVERIFY(cycle102Click(quick, minus));
        QVERIFY(cycle102Click(quick, minus));
        QCOMPARE(settings->property("writingSize").toInt(), 16);
        QTRY_COMPARE(surface->property("font").value<QFont>().pixelSize(), before);
        QTest::qWait(180);
        QTRY_COMPARE(cycle104BodyLineHeight(surface), beforeLine);
        QCOMPARE(editor->property("text").toString(), draft);
        QCOMPARE(backend.outputStyle(), 6);
        QCOMPARE(backend.outputPointSize(), pointSize);
        QCOMPARE(backend.outputFont(), font);
        QVERIFY(backend.modified());
    }

    layout->setProperty("visualEditEnabled", false);
    layout->setProperty("layoutMode", 0);
    while (settings->property("writingSize").toInt() < 32) QVERIFY(cycle102Click(quick, plus));
    QTRY_VERIFY(!plus->isEnabled());
    QVERIFY(minus->isEnabled());
    while (settings->property("writingSize").toInt() > 12) QVERIFY(cycle102Click(quick, minus));
    QTRY_VERIFY(!minus->isEnabled());
    QVERIFY(plus->isEnabled());
    QCOMPARE(editor->property("text").toString(), draft);
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QCOMPARE(editor->property("text").toString(), QString::fromUtf8(source));
    QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
    QCOMPARE(editor->property("text").toString(), draft);
    QCOMPARE(cycle103Read(path), source);
}

void FomawriteTest::headerQuickControlsStayReachableWhenResized() {
    Cycle103SettingsSnapshot isolatedSettings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *settings = window->findChild<QObject *>("workspaceSettings");
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *editor = window->findChild<QObject *>("sourceEditor");
    auto *minus = window->findChild<QQuickItem *>("workspaceZoomOutButton");
    auto *plus = window->findChild<QQuickItem *>("workspaceZoomInButton");
    auto *appearance = window->findChild<QQuickItem *>("workspaceAppearanceButton");
    QVERIFY(quick && settings && layout && editor && minus && plus && appearance);
    quick->resize(1440, 900); quick->show(); quick->requestActivate();
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    editor->setProperty("text", "# A quieter place to write\n\nQuick text-size controls keep the writing close.\n");
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    for (int mode : {0, 1, 2}) {
        layout->setProperty("layoutMode", mode);
        for (int width : {1440, 1100, 980, 900, 720}) {
            quick->resize(width, 800);
            QTRY_VERIFY(cycle104HeaderControlsFit(window.data()));
            QVERIFY(minus->isVisible() && plus->isVisible() && appearance->isVisible());
            QVERIFY(cycle102Click(quick, plus));
            QCOMPARE(settings->property("writingSize").toInt(), 18);
            QVERIFY(cycle102Click(quick, minus));
            QCOMPARE(settings->property("writingSize").toInt(), 16);
            if (mode == 0 && (width == 1440 || width == 720))
                QVERIFY(cycle102Capture(quick, width == 1440 ? "cycle104-controls-wide" : "cycle104-controls-narrow"));
        }
    }
    layout->setProperty("organizerVisible", false);
    layout->setProperty("filesVisible", false);
    QTRY_VERIFY(cycle104HeaderControlsFit(window.data()));
    QVERIFY(cycle102Click(quick, plus));
    QCOMPARE(settings->property("writingSize").toInt(), 18);
    QVERIFY(cycle102Capture(quick, "cycle104-controls-collapsed"));
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
    QtObject { id: preferences; property int titleBarMode: 1; property int toolbarVisibilityMode: 1; property int writingSize: 16 }
    QtObject { id: state }
    Item { id: organizer; x: 0; width: 184 }
    Item { id: files; x: 185; width: 232 }
    Item { id: editor; x: 418; width: 682 }
    Item { id: preview; visible: false; property bool visualEditEnabled: false }
    WorkspaceHeader {
        anchors.left: parent.left; anchors.right: parent.right
        window: fixture; settings: preferences; layoutState: state
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
