void FomawriteTest::visualListMiddleBreaksPreserveMarkersAndUnicode() {
    struct Example { QString source; QString tail; QString expected; };
    const QList<Example> examples{
        {"- earlier\n- alphabeta\n- later\n", "beta", "- earlier\n- alpha\n- beta\n- later\n"},
        {"* OneTwo\n", "Two", "* One\n* Two\n"},
        {QString::fromUtf8("  + café東京"), QString::fromUtf8("東京"), QString::fromUtf8("  + café\n  + 東京")},
        {"- [X] donepending\n", "pending", "- [X] done\n- [ ] pending\n"},
        {"+ [ ] firstsecond", "second", "+ [ ] first\n+ [ ] second"},
        {"09) leftright\n10) untouched\n", "right", "09) left\n10) right\n10) untouched\n"},
        {QString::fromUtf8("009)\t[x]\t🚀AlphaBeta\r\n010)\t[ ]\tuntouched\r\n"), "Beta",
         QString::fromUtf8("009)\t[x]\t🚀Alpha\r\n010)\t[ ]\tBeta\r\n010)\t[ ]\tuntouched\r\n")},
        {QString::fromUtf8("- café😀fin\r\n- following\r\n"), "fin", QString::fromUtf8("- café😀\r\n- fin\r\n- following\r\n")},
        {"1. earlier\r\n2. leftright", "right", "1. earlier\r\n2. left\r\n3. right"},
        {"999999998. leftright", "right", "999999998. left\n999999999. right"},
        {"- **bold** plain", "plain", "- **bold** \n- plain"},
        {"- alpha [site](https://example.com)", "site", "- alpha \n- [site](https://example.com)"},
        {"- first `sample`", "sample", "- first \n- `sample`"},
        {"- word", "word", "- \n- word"}
    };
    for (const auto &example : examples) {
        const auto mapping = SourceVisualMapping::create(example.source);
        const int position = mapping.visualText().indexOf(example.tail);
        QVERIFY2(position >= 0, qPrintable(example.source));
        const auto result = mapping.sourceEditForVisualBreak(position);
        QVERIFY2(result.has_value(), qPrintable(example.source));
        QCOMPARE(result->edit.source.length, 0); // One insertion, never a list rewrite.
        QString candidate = example.source;
        candidate.replace(result->edit.source.start, result->edit.source.length, result->edit.replacement);
        QCOMPARE(candidate, example.expected);
        QCOMPARE(mapping.roundTripSource(), example.source);
        const auto projected = SourceVisualMapping::create(candidate);
        QCOMPARE(result->visualCursor, projected.visualText().indexOf(example.tail));
        const auto typing = projected.sourceEditForVisualReplacement({result->visualCursor, 0}, QStringLiteral("new "));
        QVERIFY(typing.has_value());
        candidate.replace(typing->source.start, typing->source.length, typing->replacement);
        QString typed = example.expected;
        typed.insert(typed.indexOf(example.tail), QStringLiteral("new "));
        QCOMPARE(candidate, typed);
    }
}

void FomawriteTest::visualListBreaksRejectAmbiguousBoundaries() {
    struct Example { QString source; QString tail; };
    const QList<Example> protectedExamples{
        {"- **abcd**", "cd"}, {"- [abcd](https://example.com)", "cd"}, {"- `abcd`", "cd"},
        {"- alpha# Heading", "# Heading"}, {"- alpha- nested", "- nested"},
        {"- alpha> quote", "> quote"}, {"- alpha[ ] checkbox", "[ ] checkbox"},
        {"- parent\n    continued", "ent"}, {"- parent\n  - child", "ent"},
        {"- parent\nlazy continuation", "ent"}, {"- parent\n\n    code block", "ent"},
        {"- alpha   beta", "   beta"}, {"999999999. leftright", "right"},
        {"```\n- not a list\n```", "a list"}, {"> - quoted list", "list"}
    };
    for (const auto &example : protectedExamples) {
        const auto mapping = SourceVisualMapping::create(example.source);
        const int position = mapping.visualText().indexOf(example.tail);
        QVERIFY2(position >= 0, qPrintable(example.source));
        QVERIFY2(!mapping.sourceEditForVisualBreak(position), qPrintable(example.source));
        QCOMPARE(mapping.roundTripSource(), example.source);
    }
    for (const QString &cluster : {QString::fromUtf8("😀"), QString::fromUtf8("👨‍👩‍👧‍👦"), QString::fromUtf8("á")}) {
        const QString source = QStringLiteral("- before") + cluster + QStringLiteral("after");
        const auto mapping = SourceVisualMapping::create(source);
        const int position = mapping.visualText().indexOf(cluster);
        QVERIFY(position >= 0);
        for (int offset = 1; offset < cluster.size(); ++offset)
            QVERIFY2(!mapping.sourceEditForVisualBreak(position + offset), "Return must not split a Unicode grapheme.");
    }
    const auto ordinary = SourceVisualMapping::create(QStringLiteral("- alphabeta"));
    QVERIFY(!ordinary.sourceEditForVisualBreak(0));
    QVERIFY(!ordinary.sourceEditForVisualBreak(1));
    QVERIFY(!ordinary.sourceEditForVisualBreak(7, true)); // Shift-Return remains a Source operation for lists.
}

void FomawriteTest::visualListReturnSplitsWithAtomicUndo() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString original = QString::fromUtf8("# Tasks\n\n03) [X] café🚀東京\n04) [ ] Preserve this item.\n");
    const QString expected = QString::fromUtf8("# Tasks\n\n03) [X] café🚀\n04) [ ] 東京\n04) [ ] Preserve this item.\n");
    const QString path = directory.filePath("Visual list draft.md");
    QVERIFY(cycle103Write(path, original.toUtf8()));
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    auto *visual = pane ? pane->findChild<QQuickItem *>("visualEditor") : nullptr;
    QVERIFY(quick && editor && pane && visual);
    cycle106KeepNativeFocus(quick);
    Cycle106MouseInputIsolation nativeInput(quick);
    quick->resize(1100, 800); quick->show();
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, "visual")));
    QVERIFY(QTest::qWaitForWindowExposed(quick));
    if (QGuiApplication::platformName() != "cocoa") quick->requestActivate();
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_COMPARE(visual->property("text").toString(), SourceVisualMapping::create(original).visualText());
    const int position = visual->property("text").toString().indexOf(QString::fromUtf8("東京"));
    QVERIFY(position > 0);
    QVERIFY(visual->setProperty("cursorPosition", position));
    visual->forceActiveFocus();
    cycle111KeyClick(quick, Qt::Key_Return);
    QTRY_COMPARE(editor->property("text").toString(), expected);
    QTRY_COMPARE(visual->property("text").toString(), SourceVisualMapping::create(expected).visualText());
    QCOMPARE(visual->property("cursorPosition").toInt(), visual->property("text").toString().indexOf(QString::fromUtf8("東京")));
    QVERIFY(backend.modified());
    QCOMPARE(cycle103Read(path), original.toUtf8());

    // The caret lands before the second half. Typing there is its own undo unit,
    // and one more Undo reverses the Return without rewriting either neighbour.
    for (char key : QByteArray("new ")) cycle111KeyClick(quick, key);
    QString typed = expected;
    typed.insert(typed.indexOf(QString::fromUtf8("東京")), QStringLiteral("new "));
    QTRY_COMPARE(editor->property("text").toString(), typed);
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QCOMPARE(editor->property("text").toString(), expected);
    QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
    QCOMPARE(editor->property("text").toString(), typed);
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QCOMPARE(editor->property("text").toString(), original);
    QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
    QCOMPARE(editor->property("text").toString(), expected);
    QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
    QCOMPARE(editor->property("text").toString(), typed);
    QCOMPARE(cycle103Read(path), original.toUtf8());

    // A stale projection cannot split text the user has changed elsewhere.
    QVERIFY(!backend.applyVisualBreak(position, false, original).value("applied").toBool());
    QCOMPARE(editor->property("text").toString(), typed);
    // Unsupported formatting boundaries leave both source and selection intact.
    const QString protectedSource = QStringLiteral("- **abcd**\n");
    editor->setProperty("text", protectedSource);
    QTRY_COMPARE(visual->property("text").toString(), SourceVisualMapping::create(protectedSource).visualText());
    const int protectedPosition = visual->property("text").toString().indexOf(QStringLiteral("cd"));
    visual->setProperty("cursorPosition", protectedPosition);
    visual->forceActiveFocus();
    cycle111KeyClick(quick, Qt::Key_Return);
    QCOMPARE(editor->property("text").toString(), protectedSource);
    QCOMPARE(visual->property("cursorPosition").toInt(), protectedPosition);
    QVERIFY(pane->property("visualStatus").toString().contains(QStringLiteral("Source")));
}
