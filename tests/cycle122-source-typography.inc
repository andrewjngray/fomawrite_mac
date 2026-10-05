void FomawriteTest::sourceHangingIndentPreservesLiteralMarkdown() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString longText = QString::fromUtf8("Readable café 你好 text that wraps under its own words. ").repeated(9);
    const QString source = "- " + longText + "\n  - " + longText + "\n    - " + longText
        + "\n    - " + longText + "\n12. " + longText + "\n- [x] " + longText
        + "\n> " + longText + "\n> > " + longText
        + "\n> - [ ] " + longText
        + "\n\n```md\n- literal code\n> literal code\n```\n"
          "> ~~~md\n> - quoted literal code\n> ~~~\n"
          "$$\n- literal math\n> literal math\n$$\n"
          "\n    - indented literal code\n\n| Name | Value |\n| --- | --- |\n"
          "- - -\n\nOrdinary text.\n"
          "- Parent with code\n\n      - literal list code inside item\n      > literal quote code inside item\n"
          "\n  - Ordinary nested sibling\n\n        - literal code inside nested item\n"
          "    - Ordinary grandchild\n";
    const QString path = directory.filePath("Source.md");
    QVERIFY(cycle103Write(path, source.toUtf8()));
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *editor = window->findChild<QObject *>("sourceEditor"); QVERIFY(editor);
    auto *quick = qvariant_cast<QQuickTextDocument *>(editor->property("textDocument"));
    QVERIFY(quick);
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    auto *document = quick->textDocument();
    QCOMPARE(document->toPlainText(), source);
    QVERIFY(!backend.modified()); QVERIFY(!document->isUndoAvailable());
    const qreal gutter = document->findBlock(source.indexOf("Ordinary text.")).blockFormat().leftMargin();
    qreal previousOffset = gutter;
    for (int number = 0; number < 9; ++number) {
        const QTextBlock block = document->findBlockByNumber(number);
        const auto format = block.blockFormat();
        QVERIFY(format.leftMargin() > 0);
        QCOMPARE(format.textIndent(), gutter - format.leftMargin());
        QCOMPARE(format.lineHeight(), qreal(155));
        if (number < 3) {
            QVERIFY(format.leftMargin() > previousOffset);
            previousOffset = format.leftMargin();
        }
        // Verify the actual Qt wrap geometry, including the literal prefix.
        document->documentLayout()->documentSize();
        auto *layout = block.layout(); QVERIFY(layout && layout->lineCount() > 1);
        const int bodyStart = block.text().size() - longText.size();
        const qreal textX = layout->lineAt(0).cursorToX(bodyStart);
        QVERIFY2(qAbs(layout->lineAt(1).x() - textX) < 1.0, qPrintable(block.text().left(bodyStart)));
    }
    for (QTextBlock block = document->findBlockByNumber(9); block.isValid(); block = block.next()) {
        if (block.text() == "- Parent with code" || block.text() == "  - Ordinary nested sibling"
                || block.text() == "    - Ordinary grandchild") {
            QVERIFY(block.blockFormat().leftMargin() > 0);
            QCOMPARE(block.blockFormat().textIndent(), gutter - block.blockFormat().leftMargin());
        } else {
            QCOMPARE(block.blockFormat().leftMargin(), gutter);
            QCOMPARE(block.blockFormat().textIndent(), qreal(0));
        }
    }
    backend.save();
    QFile saved(path); QVERIFY(saved.open(QIODevice::ReadOnly));
    QCOMPARE(saved.readAll(), source.toUtf8());
    QCOMPARE(document->toPlainText(), source);
    QVERIFY(!backend.modified()); QVERIFY(!document->isUndoAvailable());
}

void FomawriteTest::sourceHangingIndentKeepsUndoRedoAndSelection() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString original = "- Original café words.\n";
    const QString path = directory.filePath("History.md");
    QVERIFY(cycle103Write(path, original.toUtf8()));
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *editor = window->findChild<QObject *>("sourceEditor");
    auto *settings = window->findChild<QObject *>("workspaceSettings");
    auto *zoom = window->findChild<QObject *>("paneZoomState");
    QVERIFY(editor && settings && zoom);
    auto *quick = qvariant_cast<QQuickTextDocument *>(editor->property("textDocument")); QVERIFY(quick);
    auto *document = quick->textDocument();
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    // Finish the deferred document-load caret reset before selecting text.
    QCoreApplication::processEvents();
    QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 2), Q_ARG(int, 10)));
    settings->setProperty("writingAppearance", "book");
    QVERIFY(QMetaObject::invokeMethod(zoom, "setZoom", Q_ARG(QVariant, QStringLiteral("source")), Q_ARG(QVariant, 125)));
    QCoreApplication::processEvents();
    QVERIFY(backend.refreshSourceTypography());
    QCOMPARE(document->toPlainText(), original);
    QCOMPARE(editor->property("selectionStart").toInt(), 2);
    QCOMPARE(editor->property("selectionEnd").toInt(), 10);
    QVERIFY(!backend.modified()); QVERIFY(!document->isUndoAvailable());

    const QString added = "  - [ ] Added text that stays exact.\n";
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, original.size()), Q_ARG(QString, added)));
    const QString firstEdit = original + added;
    QCOMPARE(document->toPlainText(), firstEdit);
    QCoreApplication::processEvents();
    QVERIFY(document->findBlockByNumber(1).blockFormat().leftMargin() > 0);
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, 0), Q_ARG(QString, QStringLiteral("> "))));
    const QString secondEdit = "> " + firstEdit;
    QCOMPARE(document->toPlainText(), secondEdit);
    QCoreApplication::processEvents();
    QVERIFY(QMetaObject::invokeMethod(window.data(), "undoEditing"));
    QCOMPARE(document->toPlainText(), firstEdit);
    QVERIFY(document->isRedoAvailable());
    QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 2), Q_ARG(int, 8)));
    const qreal offsetBeforeZoom = document->firstBlock().blockFormat().leftMargin();
    const bool dirtyBeforeZoom = backend.modified();
    settings->setProperty("writingAppearance", "manuscript");
    QVERIFY(QMetaObject::invokeMethod(zoom, "setZoom", Q_ARG(QVariant, QStringLiteral("source")), Q_ARG(QVariant, 160)));
    QCoreApplication::processEvents();
    QVERIFY(!backend.refreshSourceTypography());
    QCOMPARE(document->firstBlock().blockFormat().leftMargin(), offsetBeforeZoom);
    QCOMPARE(document->toPlainText(), firstEdit);
    QCOMPARE(editor->property("selectionStart").toInt(), 2);
    QCOMPARE(editor->property("selectionEnd").toInt(), 8);
    QCOMPARE(backend.modified(), dirtyBeforeZoom);
    QVERIFY(document->isRedoAvailable());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "undoEditing"));
    QCOMPARE(document->toPlainText(), original);
    QVERIFY(document->isRedoAvailable());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "redoEditing"));
    QCOMPARE(document->toPlainText(), firstEdit);
    QVERIFY(document->isRedoAvailable());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "redoEditing"));
    QCOMPARE(document->toPlainText(), secondEdit);
    QVERIFY(backend.refreshSourceTypography());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "undoEditing"));
    QCOMPARE(document->toPlainText(), firstEdit);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "undoEditing"));
    QCOMPARE(document->toPlainText(), original);

    // A new branch must discard stale presentation records, while intentional
    // authorship formatting must remain an independent Undo operation.
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, 0), Q_ARG(QString, QStringLiteral("> "))));
    QCoreApplication::processEvents();
    const QString branch = "> " + original;
    QCOMPARE(document->toPlainText(), branch);
    backend.markAuthorship(4, 12, "AI", "Test author");
    QVERIFY(!backend.authorshipRanges().isEmpty());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "undoEditing"));
    QCOMPARE(document->toPlainText(), branch);
    QVERIFY(backend.authorshipRanges().isEmpty());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "undoEditing"));
    QCOMPARE(document->toPlainText(), original);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "redoEditing"));
    QCOMPARE(document->toPlainText(), branch);
    QVERIFY(backend.authorshipRanges().isEmpty());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "redoEditing"));
    QCOMPARE(document->toPlainText(), branch);
    QVERIFY(!backend.authorshipRanges().isEmpty());
}
