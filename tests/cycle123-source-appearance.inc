namespace {
QTextDocument *cycle123SourceDocument(Cycle117Window &fixture) {
    auto *quick = qvariant_cast<QQuickTextDocument *>(fixture.editor->property("textDocument"));
    return quick ? quick->textDocument() : nullptr;
}
QTextCharFormat cycle123FormatAt(QTextDocument *document, int position) {
    const auto block = document->findBlock(position);
    for (const auto &range : block.layout()->formats())
        if (position - block.position() >= range.start
                && position - block.position() < range.start + range.length)
            return range.format;
    return {};
}
}

void FomawriteTest::manuscriptHeadingMarkersHangOutsideBodyColumn() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString body = QStringLiteral("Heading words that wrap onto another line. ").repeated(12);
    const QString source = "# " + body + "\n### " + body + "\nBody column.\n"
        "```md\n# Literal heading\n```\n> # Quoted heading\n";
    const QString path = directory.filePath("Headings.md");
    QVERIFY(cycle103Write(path, source.toUtf8()));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    auto *settings = f.window->findChild<QObject *>("workspaceSettings"); QVERIFY(settings);
    settings->setProperty("writingAppearance", "manuscript");
    f.backend.setSourceAppearance("manuscript");
    auto *document = cycle123SourceDocument(f); QVERIFY(document);
    document->documentLayout()->documentSize();
    const auto bodyBlock = document->findBlockByNumber(2);
    QVERIFY(bodyBlock.layout()->lineCount() > 0);
    const qreal bodyX = bodyBlock.layout()->lineAt(0).cursorToX(0);
    for (int number : {0, 1}) {
        const auto block = document->findBlockByNumber(number);
        QCOMPARE(block.blockFormat().leftMargin(), bodyBlock.blockFormat().leftMargin());
        QVERIFY(block.blockFormat().textIndent() < 0);
        QVERIFY(block.layout()->lineCount() > 1);
        const auto first = block.layout()->lineAt(0);
        const int prefix = number == 0 ? 2 : 4;
        QVERIFY(first.cursorToX(0) < bodyX);
        QVERIFY(qAbs(first.cursorToX(prefix) - bodyX) < 1.0);
        QVERIFY(qAbs(block.layout()->lineAt(1).x() - bodyX) < 1.0);
    }
    QCOMPARE(document->findBlockByNumber(4).blockFormat().textIndent(), qreal(0));
    QCOMPARE(document->toPlainText(), source);
    QVERIFY(!f.backend.modified()); QVERIFY(!document->isUndoAvailable());
}

void FomawriteTest::sourceCodeBackgroundAndAppearancePreserveSavedBytes() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString source = QString::fromUtf8("# Café 你好\n- List item\n> Quote\n\n```js\nconst value = 42;\n\n  return value;\n```\nInline `value` stays compact.\n");
    const QString path = directory.filePath("Appearance.md");
    QVERIFY(cycle103Write(path, source.toUtf8()));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    auto *document = cycle123SourceDocument(f); QVERIFY(document);
    auto *settings = f.window->findChild<QObject *>("workspaceSettings"); QVERIFY(settings);
    for (const QString &appearance : {QString("manuscript"), QString("code"), QString("book"), QString("editorial"), QString("manuscript")}) {
        settings->setProperty("writingAppearance", appearance);
        f.backend.setSourceAppearance(appearance);
        QCoreApplication::processEvents();
        const QBrush background = document->findBlockByNumber(4).blockFormat().background();
        QVERIFY(background.style() != Qt::NoBrush);
        for (int number = 4; number <= 8; ++number) {
            const auto block = document->findBlockByNumber(number);
            QCOMPARE(block.blockFormat().background(), background);
            QCOMPARE(block.blockFormat().topMargin(), qreal(0));
            QCOMPARE(block.blockFormat().bottomMargin(), qreal(0));
        }
        QCOMPARE(document->findBlockByNumber(9).blockFormat().background().style(), Qt::NoBrush);
        const int inlineStart = source.indexOf("`value`");
        QVERIFY(cycle123FormatAt(document, inlineStart + 1).background().style() != Qt::NoBrush);
        QCOMPARE(cycle123FormatAt(document, inlineStart - 1).background().style(), Qt::NoBrush);
        if (appearance == "code") {
            for (auto block = document->begin(); block.isValid(); block = block.next()) {
                QCOMPARE(block.blockFormat().leftMargin(), qreal(0));
                QCOMPARE(block.blockFormat().textIndent(), qreal(0));
                QCOMPARE(block.blockFormat().lineHeight(), qreal(135));
            }
            const auto lines = f.backend.sourceLineDecorations(0, 100000);
            QCOMPARE(lines.size(), document->blockCount());
            QCOMPARE(lines[7].toMap().value("indent").toInt(), 2);
            QCOMPARE(lines[7].toMap().value("number").toInt(), 8);
            const QFont font = qvariant_cast<QFont>(f.editor->property("font"));
            const QFontMetricsF metrics(font);
            QVERIFY(qAbs(metrics.horizontalAdvance('i') - metrics.horizontalAdvance('W')) < 0.1);
        }
        QCOMPARE(document->toPlainText(), source);
        QVERIFY(!f.backend.modified()); QVERIFY(!document->isUndoAvailable());
    }
    f.backend.save();
    QFile saved(path); QVERIFY(saved.open(QIODevice::ReadOnly));
    QCOMPARE(saved.readAll(), source.toUtf8());
}

void FomawriteTest::codeIndentKeysAndQuotesKeepAtomicUndo() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString original = "if (ready) {\n    run();\n    finish();\n}\n";
    const QString path = directory.filePath("Indent.js");
    QVERIFY(cycle103Write(path, original.toUtf8()));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    auto *settings = f.window->findChild<QObject *>("workspaceSettings"); QVERIFY(settings);
    settings->setProperty("writingAppearance", "code");
    settings->setProperty("smartQuotes", true);
    settings->setProperty("smartDashes", true);
    QCoreApplication::processEvents();
    auto *document = cycle123SourceDocument(f); QVERIFY(document);
    QCOMPARE(f.window->property("activeWritingAppearance").toString(), QString("code"));
    const int start = original.indexOf("    run");
    const int end = original.indexOf("}\n");
    QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, start), Q_ARG(int, end)));
    f.editor->forceActiveFocus();
    QTest::keyClick(f.quick, Qt::Key_Tab);
    const QString indented = "if (ready) {\n        run();\n        finish();\n}\n";
    QTRY_COMPARE(document->toPlainText(), indented);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "undoEditing"));
    QCOMPARE(document->toPlainText(), original);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "redoEditing"));
    QCOMPARE(document->toPlainText(), indented);
    const int indentedEnd = indented.indexOf("}\n");
    QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, start), Q_ARG(int, indentedEnd)));
    QTest::keyClick(f.quick, Qt::Key_Backtab, Qt::ShiftModifier);
    QTRY_COMPARE(document->toPlainText(), original);
    f.backend.undoSource(0);
    QCOMPARE(document->toPlainText(), indented);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "undoEditing"));
    QCOMPARE(document->toPlainText(), original);
    f.editor->setProperty("cursorPosition", original.indexOf('\n'));
    f.editor->forceActiveFocus();
    QTest::keyClick(f.quick, Qt::Key_Return);
    const QString withNewline = "if (ready) {\n    \n    run();\n    finish();\n}\n";
    QTRY_COMPARE(document->toPlainText(), withNewline);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "undoEditing"));
    QCOMPARE(document->toPlainText(), original);
    f.editor->setProperty("cursorPosition", original.size());
    f.editor->forceActiveFocus();
    QTest::keyClick(f.quick, Qt::Key_QuoteDbl);
    QTest::keyClick(f.quick, Qt::Key_Minus);
    QTest::keyClick(f.quick, Qt::Key_Minus);
    QTRY_COMPARE(document->toPlainText(), original + "\"--");
    QVERIFY(f.backend.modified());
}
