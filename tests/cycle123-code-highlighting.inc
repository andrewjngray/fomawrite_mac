void FomawriteTest::syntaxColorsRespectLanguagesAndLiteralMarkdown() {
    const auto formatAt = [](QTextDocument &document, int position) {
        const auto block = document.findBlock(position);
        const int offset = position - block.position();
        for (const auto &range : block.layout()->formats())
            if (offset >= range.start && offset < range.start + range.length)
                return range.format;
        return QTextCharFormat();
    };
    const auto checkColor = [&](const QString &source, const QString &probe, const QColor &expected) {
        QTextDocument document;
        document.setPlainText(source);
        document.setModified(false);
        MarkdownHighlighter highlighter(&document);
        highlighter.setDarkMode(true);
        highlighter.setCodeStyle(true);
        highlighter.rehighlight();
        QCOMPARE(formatAt(document, source.indexOf(probe)).foreground().color(), expected);
        QCOMPARE(document.toPlainText(), source);
        QVERIFY(!document.isModified());
        QVERIFY(!document.isUndoAvailable());
    };

    checkColor(QStringLiteral("```typescript\nconst value = \"return\";\n```"),
               QStringLiteral("const"), QColor("#C792EA"));
    checkColor(QStringLiteral("```js\n/* return\nif (true) */\nconst text = `return\nwhile`;\n```"),
               QStringLiteral("return"), QColor("#7F9F7F"));
    checkColor(QStringLiteral("```js\nconst text = `hello\nreturn`;\n```"),
               QStringLiteral("return"), QColor("#C3E88D"));
    checkColor(QStringLiteral("```python\nvalue = 'return'\n# if false\n```"),
               QStringLiteral("if"), QColor("#7F9F7F"));
    checkColor(QStringLiteral("```json\n{\"enabled\": true}\n```"),
               QStringLiteral("true"), QColor("#C792EA"));
    checkColor(QStringLiteral("```css\n/* if */\ncolor: red;\n```"),
               QStringLiteral("if"), QColor("#7F9F7F"));
    checkColor(QStringLiteral("```html\n<div class=\"return\">hello</div>\n```"),
               QStringLiteral("div"), QColor("#FFCB6B"));
    checkColor(QStringLiteral("```cpp\nint main() { return 2; }\n```"),
               QStringLiteral("main"), QColor("#82AAFF"));

    const QString unknown = QStringLiteral("```made-up\nreturn *not markdown*\n```");
    QTextDocument unknownDocument;
    unknownDocument.setPlainText(unknown);
    MarkdownHighlighter unknownHighlighter(&unknownDocument);
    unknownHighlighter.setCodeStyle(true);
    unknownHighlighter.rehighlight();
    QCOMPARE(formatAt(unknownDocument, unknown.indexOf(QStringLiteral("return"))).foreground().style(), Qt::NoBrush);
    QCOMPARE(unknownDocument.toPlainText(), unknown);

    const QString source = QStringLiteral("```js\nconst answer = 42;\n```");
    QTextDocument document;
    document.setPlainText(source);
    document.setModified(false);
    MarkdownHighlighter highlighter(&document);
    highlighter.setCodeStyle(true);
    QTextCursor cursor(&document);
    cursor.setPosition(source.indexOf(QStringLiteral("answer")));
    cursor.insertText(QStringLiteral("edited"));
    const QString edited = document.toPlainText();
    QVERIFY(document.isModified());
    QVERIFY(document.isUndoAvailable());
    highlighter.setCodeStyle(false);
    highlighter.setCodeLanguage(QStringLiteral("js"));
    highlighter.setCodeStyle(true);
    QCOMPARE(document.toPlainText(), edited);
    QVERIFY(document.isModified());
    QVERIFY(document.isUndoAvailable());
    document.undo();
    QCOMPARE(document.toPlainText(), source);
}
