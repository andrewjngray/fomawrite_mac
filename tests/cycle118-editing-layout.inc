void FomawriteTest::editingAndLayoutChoicesAreIndependent() {
    Cycle103SettingsSnapshot isolatedSettings;
    Cycle117Window f; QVERIFY2(f.start(1280), qPrintable(f.error));
    auto *sourceChoice = f.item("sourceModeButton"), *liveChoice = f.item("liveEditToggle");
    auto *single = f.item("singleModeButton"), *split = f.item("previewSplitButton");
    auto *editingGroup = f.item("documentEditingControls"), *layoutGroup = f.item("documentViewControls");
    QVERIFY(sourceChoice && liveChoice && single && split && editingGroup && layoutGroup);
    const auto liveVisible = [&] { auto *p = f.item("liveEditorPane"); return p && p->isVisible(); };
    const QString saved = QString::fromUtf8("# Independent choices\n\nA café draft — 你好.\n");
    f.editor->setProperty("text", saved);
    QVERIFY(QMetaObject::invokeMethod(f.editor, "insert", Q_ARG(int, saved.size()), Q_ARG(QString, "Unsaved ending.")));
    const QString draft = f.editor->property("text").toString();
    QVERIFY(f.mode(0)); QTRY_VERIFY(!f.window->property("changingDocumentView").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, 2), Q_ARG(int, 8)));
    const auto fixed = cycle107ViewBounds(f.footer);
    for (int pass = 0; pass < 3; ++pass) {
        for (auto *arrangement : {single, split, split, single}) {
            const bool chosenBefore = f.layout->property("liveEditEnabled").toBool();
            QVERIFY(cycle102Click(f.quick, arrangement));
            const int mode = arrangement == single ? 0 : 1;
            QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), mode);
            QCOMPARE(f.layout->property("liveEditEnabled").toBool(), chosenBefore);
            for (auto *choice : {sourceChoice, liveChoice, liveChoice, sourceChoice}) {
                QVERIFY(cycle102Click(f.quick, choice));
                QTRY_COMPARE(f.layout->property("liveEditEnabled").toBool(), choice == liveChoice);
                QCOMPARE(f.layout->property("effectiveLayoutMode").toInt(), mode);
                QTRY_COMPARE(f.editor->isVisible(), choice == sourceChoice);
                QTRY_COMPARE(liveVisible(), choice == liveChoice);
                QCOMPARE(f.preview->isVisible(), mode == 1);
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
    // Find owns canonical Markdown; choosing Live must close it without
    // retaining an invisible Source selection as a native clipboard target.
    auto *find = f.item("documentSearchButton"), *findBar = f.item("documentFindBar");
    auto *appearance = f.item("workspaceAppearanceButton");
    auto *nativeDelete = f.window->findChild<QObject *>("editDelete");
    auto *nativePaste = f.window->findChild<QObject *>("editPastePlain");
    QVERIFY(find && findBar && appearance && nativeDelete && nativePaste);
    QVERIFY(cycle102Click(f.quick, find)); QTRY_VERIFY(findBar->isVisible());
    QVERIFY(cycle102Click(f.quick, liveChoice)); QTRY_VERIFY(!findBar->isVisible());
    QVERIFY(cycle102Click(f.quick, appearance));
    QTRY_VERIFY(!f.window->property("sourceClipboardAllowed").toBool());
    QVERIFY(!nativeDelete->property("enabled").toBool()); QVERIFY(!nativePaste->property("enabled").toBool());
    auto *appearanceMenu = f.window->findChild<QObject *>("writingOptions");
    QVERIFY(appearanceMenu); QVERIFY(QMetaObject::invokeMethod(appearanceMenu, "close"));
    QTRY_VERIFY(!appearanceMenu->property("visible").toBool());
    QCOMPARE(f.editor->property("text").toString(), draft);
    QVERIFY(cycle102Click(f.quick, liveChoice)); QVERIFY(cycle102Click(f.quick, split));
    f.quick->resize(720, 620);
    QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 0);
    QVERIFY(f.layout->property("liveEditEnabled").toBool()); QVERIFY(liveVisible());
    QVERIFY(!f.editor->isVisible() && !f.preview->isVisible());
    f.quick->resize(1280, 720);
    QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 1);
    QVERIFY(liveVisible() && f.preview->isVisible());
    QVERIFY(cycle118ReadPreview(f.window.data())); QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 2);
    QVERIFY(!sourceChoice->property("checked").toBool() && !liveChoice->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "selectWritingMode", Q_ARG(QVariant, "live"))); QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 1);
    QVERIFY(f.layout->property("liveEditEnabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "selectWritingMode", Q_ARG(QVariant, "source")));
    QTRY_VERIFY(!f.layout->property("liveEditEnabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "undoEditing")); QCOMPARE(f.editor->property("text").toString(), saved);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "redoEditing")); QCOMPARE(f.editor->property("text").toString(), draft);
}

