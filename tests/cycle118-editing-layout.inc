void FomawriteTest::editingAndLayoutChoicesAreIndependent() {
    Cycle103SettingsSnapshot isolatedSettings;
    Cycle117Window f; QVERIFY2(f.start(1280), qPrintable(f.error));
    auto *visualPane = f.item("visualEditorPane"), *visualEditor = f.item("visualEditor");
    auto *sourceChoice = f.item("sourceModeButton"), *visualChoice = f.item("visualEditToggle");
    auto *single = f.item("singleModeButton"), *split = f.item("previewSplitButton");
    auto *editingGroup = f.item("documentEditingControls"), *layoutGroup = f.item("documentViewControls");
    QVERIFY(visualPane && visualEditor && sourceChoice && visualChoice && single && split && editingGroup && layoutGroup);
    const QString saved = QString::fromUtf8("# Independent choices\n\nA café draft — 你好.\n");
    f.editor->setProperty("text", saved);
    QVERIFY(QMetaObject::invokeMethod(f.editor, "insert", Q_ARG(int, saved.size()), Q_ARG(QString, "Unsaved ending.")));
    const QString draft = f.editor->property("text").toString();
    QVERIFY(f.mode(0)); QTRY_VERIFY(!f.window->property("changingDocumentView").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, 2), Q_ARG(int, 8)));
    const auto fixed = cycle107ViewBounds(f.footer);
    for (int pass = 0; pass < 3; ++pass) {
        for (auto *arrangement : {single, split, split, single}) {
            const bool chosenBefore = f.layout->property("visualEditEnabled").toBool();
            QVERIFY(cycle102Click(f.quick, arrangement));
            const int mode = arrangement == single ? 0 : 1;
            QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), mode);
            QCOMPARE(f.layout->property("visualEditEnabled").toBool(), chosenBefore);
            for (auto *choice : {sourceChoice, visualChoice, visualChoice, sourceChoice}) {
                QVERIFY(cycle102Click(f.quick, choice));
                QTRY_COMPARE(f.layout->property("visualEditEnabled").toBool(), choice == visualChoice);
                QCOMPARE(f.layout->property("effectiveLayoutMode").toInt(), mode);
                QTRY_COMPARE(f.editor->isVisible(), choice == sourceChoice);
                QTRY_COMPARE(visualPane->isVisible(), choice == visualChoice);
                QCOMPARE(f.preview->isVisible(), mode == 1);
                QVERIFY(!f.preview->property("visualEditEnabled").toBool());
                QVERIFY(cycle107ViewChecked(f.footer, f.layout));
                QVERIFY(cycle107SameBounds(cycle107ViewBounds(f.footer), fixed));
                QVERIFY(cycle103SceneBounds(editingGroup).right() + 10 <= cycle103SceneBounds(layoutGroup).left());
                QVERIFY(cycle117PaneFooterAnchors(f.window.data()));
                QCOMPARE(f.editor->property("text").toString(), draft);
                QCOMPARE(f.editor->property("selectionStart").toInt(), 2);
                QCOMPARE(f.editor->property("selectionEnd").toInt(), 8);
            }
        }
    }
    // Find owns canonical Markdown; choosing Visual must close it without
    // retaining an invisible Source selection as a native clipboard target.
    auto *find = f.item("documentSearchButton"), *findBar = f.item("documentFindBar");
    auto *appearance = f.item("workspaceAppearanceButton");
    auto *nativeDelete = f.window->findChild<QObject *>("editDelete");
    auto *nativePaste = f.window->findChild<QObject *>("editPastePlain");
    QVERIFY(find && findBar && appearance && nativeDelete && nativePaste);
    QVERIFY(cycle102Click(f.quick, find)); QTRY_VERIFY(findBar->isVisible());
    QVERIFY(cycle102Click(f.quick, visualChoice)); QTRY_VERIFY(!findBar->isVisible());
    QVERIFY(cycle102Click(f.quick, appearance));
    QTRY_VERIFY(!f.window->property("sourceClipboardAllowed").toBool());
    QVERIFY(!nativeDelete->property("enabled").toBool()); QVERIFY(!nativePaste->property("enabled").toBool());
    auto *appearanceMenu = f.window->findChild<QObject *>("writingOptions");
    QVERIFY(appearanceMenu); QVERIFY(QMetaObject::invokeMethod(appearanceMenu, "close"));
    QTRY_VERIFY(!appearanceMenu->property("visible").toBool());
    QCOMPARE(f.editor->property("text").toString(), draft);
    QVERIFY(cycle102Click(f.quick, visualChoice)); QVERIFY(cycle102Click(f.quick, split));
    f.quick->resize(720, 620);
    QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 0);
    QVERIFY(f.layout->property("visualEditEnabled").toBool()); QVERIFY(visualEditor->isVisible());
    QVERIFY(!f.editor->isVisible() && !f.preview->isVisible());
    f.quick->resize(1280, 720);
    QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 1);
    QVERIFY(visualEditor->isVisible() && f.preview->isVisible());
    QVERIFY(cycle118ReadPreview(f.window.data())); QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 2);
    QVERIFY(!sourceChoice->property("checked").toBool() && !visualChoice->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "selectWritingMode", Q_ARG(QVariant, "visual"))); QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 1);
    QVERIFY(f.layout->property("visualEditEnabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "undoEditing")); QCOMPARE(f.editor->property("text").toString(), saved);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "redoEditing")); QCOMPARE(f.editor->property("text").toString(), draft);
}

void FomawriteTest::visualSplitEditsCanonicalSourceAndKeepsPreviewReadOnly() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    const QString saved = QString::fromUtf8("# A visual draft\n\nPlain café text.\n");
    const QString path = directory.filePath("Visual draft.md"); QVERIFY(cycle103Write(path, saved.toUtf8()));
    Cycle117Window f; QVERIFY2(f.start(1280), qPrintable(f.error)); QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    auto *settings = f.window->findChild<QObject *>("workspaceSettings");
    auto *visualPane = f.item("visualEditorPane"), *visual = f.item("visualEditor");
    auto *sourceChoice = f.item("sourceModeButton"), *visualChoice = f.item("visualEditToggle");
    QVERIFY(settings && visualPane && visual && sourceChoice && visualChoice);
    settings->setProperty("sourceZoom", 125); settings->setProperty("previewZoom", 150);
    QVERIFY(cycle102Click(f.quick, visualChoice)); QVERIFY(f.mode(1));
    QTRY_VERIFY(visual->isVisible() && f.preview->isVisible());
    QTRY_VERIFY(!f.window->property("changingDocumentView").toBool());
    QVERIFY(!f.preview->property("visualEditEnabled").toBool());
    auto *rendered = f.preview->findChild<QQuickItem *>("renderedPreview"); QVERIFY(rendered && rendered->isVisible());
    QVERIFY(!f.preview->property("allowVisualEdit").toBool());
    QTRY_COMPARE(visualPane->property("visualTextSize").toInt(), f.window->property("editorFontPixelSize").toInt());
    const int point = visual->property("text").toString().indexOf("café"); QVERIFY(point >= 0);
    visual->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(visual, "select", Q_ARG(int, point), Q_ARG(int, point + 4)));
    cycle111KeyClick(f.quick, 'b');
    const QString edited = QString(saved).replace("café", "b");
    QTRY_COMPARE(f.editor->property("text").toString(), edited);
    QTRY_VERIFY(f.backend.modified());
    const int revision = f.backend.documentRevision();
    QVERIFY(QMetaObject::invokeMethod(f.preview, "focusRenderedSurface"));
    cycle111KeyClick(f.quick, 'x');
    QCOMPARE(f.editor->property("text").toString(), edited); QCOMPARE(f.backend.documentRevision(), revision);
    for (int mode : {0, 1, 0, 1}) {
        QVERIFY(f.mode(mode)); QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), mode);
        QVERIFY(f.layout->property("visualEditEnabled").toBool());
        QCOMPARE(settings->property("sourceZoom").toInt(), 125); QCOMPARE(settings->property("previewZoom").toInt(), 150);
        QCOMPARE(f.editor->property("text").toString(), edited);
    }
    QVERIFY(cycle102Click(f.quick, sourceChoice)); QTRY_VERIFY(f.editor->isVisible());
    QCOMPARE(f.layout->property("effectiveLayoutMode").toInt(), 1);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "undoEditing")); QTRY_COMPARE(f.editor->property("text").toString(), saved);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "redoEditing")); QTRY_COMPARE(f.editor->property("text").toString(), edited);
    QCOMPARE(cycle103Read(path), saved.toUtf8());
}
