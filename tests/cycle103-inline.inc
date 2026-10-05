// Combined-emphasis presentation must never rewrite the Markdown or style code.
void FomawriteTest::sourceTripleEmphasisKeepsFormattingAndLiteralCode() {
    const QString source = QString::fromUtf8(
        "***stárs*** and ___under___ and **bold** and *italic* and __heavy__ and _slant_\n"
        "`***literal stars***` and ``___literal underscores___``\n"
        "```md\n***fenced stars***\n___fenced underscores___\n```\n"
        "\\***escaped***");
    QTextDocument document;
    document.setPlainText(source);
    document.setModified(false);
    MarkdownHighlighter highlighter(&document);
    highlighter.setDarkMode(false);
    auto formatAt = [&document](int position) {
        const auto block = document.findBlock(position);
        const int offset = position - block.position();
        for (const auto &range : block.layout()->formats())
            if (offset >= range.start && offset < range.start + range.length)
                return range.format;
        return QTextCharFormat();
    };
    const auto parsed = MarkdownHighlighter::inlineMarkup(document.firstBlock().text());
    int combinedCount = 0;
    for (const auto &item : parsed) {
        if (item.kind != MarkdownHighlighter::InlineKind::BoldItalic) continue;
        ++combinedCount;
        QCOMPARE(item.markers[0].length, 3);
        QCOMPARE(item.markers[1].length, 3);
    }
    QCOMPARE(combinedCount, 2);
    QVERIFY(MarkdownHighlighter::inlineMarkup(document.findBlockByNumber(1).text()).isEmpty());
    QVERIFY(MarkdownHighlighter::inlineMarkup(document.lastBlock().text()).isEmpty());

    for (const bool showMarkers : {false, true}) {
        highlighter.setShowMarkup(showMarkers);
        highlighter.rehighlight();
        for (const QString &word : {QString::fromUtf8("stárs"), QStringLiteral("under")}) {
            const int start = source.indexOf(word);
            for (int offset = 0; offset < word.size(); ++offset) {
                const auto content = formatAt(start + offset);
                QCOMPARE(content.fontWeight(), int(QFont::Bold));
                QVERIFY(content.fontItalic());
            }
            for (const int markerStart : {start - 3, start + int(word.size())}) {
                for (int offset = 0; offset < 3; ++offset) {
                    const auto marker = formatAt(markerStart + offset);
                    if (showMarkers) {
                        QCOMPARE(marker.foreground().color(), QColor("#68707B"));
                        QVERIFY(marker.fontPointSize() != 1.0);
                    } else {
                        QCOMPARE(marker.fontPointSize(), qreal(1.0));
                    }
                }
            }
        }
        for (const QString &word : {QStringLiteral("bold"), QStringLiteral("heavy")}) {
            const auto format = formatAt(source.indexOf(word));
            QCOMPARE(format.fontWeight(), int(QFont::Bold));
            QVERIFY(!format.fontItalic());
        }
        for (const QString &word : {QStringLiteral("italic"), QStringLiteral("slant")}) {
            const auto format = formatAt(source.indexOf(word));
            QVERIFY(format.fontItalic());
            QVERIFY(format.fontWeight() != QFont::Bold);
        }
        for (const QString &literal : {QStringLiteral("***literal stars***"), QStringLiteral("___literal underscores___"),
                                      QStringLiteral("***fenced stars***"), QStringLiteral("___fenced underscores___")}) {
            const int start = source.indexOf(literal);
            for (int offset = 0; offset < literal.size(); ++offset) {
                const auto format = formatAt(start + offset);
                QVERIFY(format.fontWeight() != QFont::Bold);
                QVERIFY(!format.fontItalic());
                QVERIFY(format.fontPointSize() != 1.0);
                if (literal.startsWith(QStringLiteral("***fenced"))
                    || literal.startsWith(QStringLiteral("___fenced")))
                    QCOMPARE(format.background().style(), Qt::NoBrush);
                else
                    QCOMPARE(format.background().color(), QColor("#EAECF0"));
            }
        }
        QCOMPARE(document.toPlainText(), source);
        QVERIFY(!document.isModified());
        QVERIFY(!document.isUndoAvailable());
    }
    QTextCursor cursor(&document);
    cursor.setPosition(source.indexOf(QStringLiteral("under")));
    cursor.insertText(QString::fromUtf8("É"));
    const QString edited = document.toPlainText();
    highlighter.setShowMarkup(false);
    highlighter.setDarkMode(true);
    highlighter.rehighlight();
    QCOMPARE(document.toPlainText(), edited);
    document.undo();
    QCOMPARE(document.toPlainText(), source);
    document.redo();
    QCOMPARE(document.toPlainText(), edited);
}
