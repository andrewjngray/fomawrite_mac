// Included after FomawriteTest's class definition; slot declarations live there.
void FomawriteTest::visualImagesExposeBoundedMetadata() {
    const QString source = QString::fromUtf8("![A 📝 café](<photos/my image.png> \"Original title\")\r\n![](empty.png)\r\nText ![inline](inline.png)\r\n![complex [alt]](x.png)\r\n");
    const auto mapping = SourceVisualMapping::create(source);
    QCOMPARE(mapping.roundTripSource().toUtf8(), source.toUtf8());
    QCOMPARE(mapping.imageObjects().size(), 2);
    const auto image = mapping.imageObjects().first();
    QCOMPARE(image.altText, QString::fromUtf8("A 📝 café"));
    QCOMPARE(image.destination, QStringLiteral("photos/my image.png"));
    QCOMPARE(source.mid(image.destinationSource.start, image.destinationSource.length), image.destination);
    const auto altVisual = mapping.visualSpanForSource(image.altSource);
    QVERIFY(altVisual.isValid());
    const auto edit = mapping.sourceEditForVisualReplacement(altVisual, QStringLiteral("A new caption"));
    QVERIFY(edit.has_value());
    QString expected = source;
    expected.replace(image.altSource.start, image.altSource.length, QStringLiteral("A new caption"));
    QString result = source;
    result.replace(edit->source.start, edit->source.length, edit->replacement);
    QCOMPARE(result, expected);
    QVERIFY(result.contains(QStringLiteral("(<photos/my image.png> \"Original title\")\r\n")));
    QVERIFY(!mapping.sourceEditForVisualReplacement({image.visual.start, 1}, QStringLiteral("x")));
    QVERIFY(!mapping.visualSpanForSource(image.destinationSource).isValid());
    QVERIFY(!mapping.sourceEditForVisualReplacement(mapping.imageObjects().at(1).visual, QStringLiteral("caption")));
    QVERIFY(mapping.visualText().contains(QStringLiteral("Text ![inline](inline.png)")));
    QVERIFY(mapping.visualText().contains(QStringLiteral("![complex [alt]](x.png)")));
}

void FomawriteTest::visualTablesEditCellsWithoutRewritingSyntax() {
    const QString source = QString::fromUtf8("| Name | Value | Note |\r\n| :--- | ---: | :---: |\r\n| Café | **bold** | 📝 |\r\n| Empty |   | fine |\r\n");
    const auto mapping = SourceVisualMapping::create(source);
    QCOMPARE(mapping.roundTripSource().toUtf8(), source.toUtf8());
    QCOMPARE(mapping.blocks().at(0).kind, SourceVisualMapping::BlockKind::TableRow);
    QCOMPARE(mapping.blocks().at(1).kind, SourceVisualMapping::BlockKind::TableDelimiter);
    QVERIFY(mapping.visualText().contains(QString::fromUtf8("Café  │  bold  │  📝")));
    const int boldStart = source.indexOf(QStringLiteral("bold"));
    const auto bold = mapping.visualSpanForSource({boldStart, 4});
    QVERIFY(bold.isValid());
    const auto edit = mapping.sourceEditForVisualReplacement(bold, QStringLiteral("strong"));
    QVERIFY(edit.has_value());
    QString candidate = source;
    candidate.replace(edit->source.start, edit->source.length, edit->replacement);
    QVERIFY(candidate.contains(QStringLiteral("| Café | **strong** | 📝 |\r\n")));
    QCOMPARE(candidate.left(boldStart), source.left(boldStart));
    QCOMPARE(candidate.mid(boldStart + 6), source.mid(boldStart + 4));
    QVERIFY(!mapping.sourceEditForVisualReplacement({mapping.blocks().at(1).visual.start, 1}, QStringLiteral("x")));
    const int separator = mapping.visualText().indexOf(QString::fromUtf8("│"));
    QVERIFY(!mapping.sourceEditForVisualReplacement({separator, 1}, QStringLiteral("x")));
    QVERIFY(!mapping.sourceEditForVisualReplacement(bold, QStringLiteral("left | right")));
    const auto emptyBlock = mapping.blocks().at(3);
    const int emptyPosition = mapping.visualText().indexOf(QString::fromUtf8("  │    │  "), emptyBlock.visual.start) + 5;
    QVERIFY(emptyPosition >= 5);
    const auto fillEmpty = mapping.sourceEditForVisualReplacement({emptyPosition, 0}, QStringLiteral("filled"));
    QVERIFY(fillEmpty.has_value());
    candidate = source;
    candidate.replace(fillEmpty->source.start, fillEmpty->source.length, fillEmpty->replacement);
    QVERIFY(candidate.contains(QStringLiteral("| Empty |   filled| fine |")));
}

void FomawriteTest::visualTablesKeepAmbiguousRowsInSource() {
    const QString source = QStringLiteral("| A | B |\n| --- | --- |\n| escaped\\|pipe | value |\n| `code|pipe` | value |\n| short |\n| [reference][id] | value |\n\nordinary | prose\n");
    const auto mapping = SourceVisualMapping::create(source);
    QCOMPARE(mapping.roundTripSource(), source);
    for (int row = 2; row <= 5; ++row) {
        QCOMPARE(mapping.blocks().at(row).kind, SourceVisualMapping::BlockKind::SourceOnly);
        QVERIFY(!mapping.blocks().at(row).editable);
    }
    QVERIFY(mapping.visualText().contains(QStringLiteral("escaped\\|pipe")));
    QVERIFY(mapping.visualText().contains(QStringLiteral("`code|pipe`")));
    QCOMPARE(mapping.blocks().at(7).kind, SourceVisualMapping::BlockKind::SourceOnly);
    const auto laterDashes = SourceVisualMapping::create(QStringLiteral("| A | B |\n| --- | --- |\n| item | value |\n| --- | --- |"));
    QCOMPARE(laterDashes.blocks().at(3).kind, SourceVisualMapping::BlockKind::TableRow);
    QVERIFY(laterDashes.blocks().at(3).editable);
    const auto ambiguousHeader = SourceVisualMapping::create(QStringLiteral("| `a|b` | c |\n| --- | --- |\n| value | other |"));
    for (const auto &block : ambiguousHeader.blocks()) QVERIFY(!block.editable);
}

void FomawriteTest::visualFencesHonorMarkerAndLength() {
    const QString source = QStringLiteral("````md\n**inside**\n```\n*still literal*\n~~~\n``` trailing text\n````\n**outside**\n~~~lang\n```\n~~~ trailing text\n**still fenced**\n~~~~\n*after*\n");
    const auto mapping = SourceVisualMapping::create(source);
    QCOMPARE(mapping.roundTripSource(), source);
    for (int row = 0; row < 7; ++row) QVERIFY(!mapping.blocks().at(row).editable);
    QVERIFY(mapping.blocks().at(7).editable);
    for (int row = 8; row <= 12; ++row) QVERIFY(!mapping.blocks().at(row).editable);
    QVERIFY(mapping.blocks().at(13).editable);
    QVERIFY(mapping.visualText().contains(QStringLiteral("*still literal*")));
    QVERIFY(mapping.visualText().contains(QStringLiteral("outside\n")));
    const auto unclosed = SourceVisualMapping::create(QStringLiteral("```\nbody\n"));
    QCOMPARE(unclosed.blocks().size(), 2);
    const auto comment = SourceVisualMapping::create(QStringLiteral("<!--\n**comment**\n```\n-->\n*outside*"));
    for (int row = 0; row <= 3; ++row) QVERIFY(!comment.blocks().at(row).editable);
    QVERIFY(comment.blocks().at(4).editable);
    const auto unclosedComment = SourceVisualMapping::create(QStringLiteral("<!--\nprivate\n"));
    QCOMPARE(unclosedComment.blocks().size(), 2);
    const auto rules = SourceVisualMapping::create(QStringLiteral("- - -\n***\n___\n---"));
    for (const auto &block : rules.blocks()) QVERIFY(!block.editable);
    QVERIFY(!unclosed.sourceEditForVisualReplacement({int(unclosed.visualText().size()), 0}, QStringLiteral("leak")));
}

void FomawriteTest::visualInlineCodePreservesDelimitersAndUnicode() {
    const QString source = QString::fromUtf8("Text `literal *stars*` and ``tick ` content`` with 📝 and café.\r\n");
    const auto mapping = SourceVisualMapping::create(source);
    QCOMPARE(mapping.roundTripSource(), source);
    QVERIFY(mapping.visualText().contains(QStringLiteral("literal *stars* and tick ` content")));
    int codeFormats = 0;
    for (const auto &span : mapping.visualFormatSpans())
        if (span.kind == SourceVisualMapping::VisualFormatKind::InlineCode) ++codeFormats;
    QCOMPARE(codeFormats, 2);
    const auto codeVisual = mapping.visualSpanForSource({int(source.indexOf(QStringLiteral("literal"))), 7});
    QVERIFY(codeVisual.isValid());
    const auto edit = mapping.sourceEditForVisualReplacement(codeVisual, QStringLiteral("plain"));
    QVERIFY(edit.has_value());
    QString candidate = source;
    candidate.replace(edit->source.start, edit->source.length, edit->replacement);
    QVERIFY(candidate.contains(QStringLiteral("`plain *stars*`")));
    const int emoji = mapping.visualText().indexOf(QString::fromUtf8("📝"));
    QVERIFY(!mapping.sourceEditForVisualReplacement({emoji + 1, 0}, QStringLiteral("x")));
    QVERIFY(!mapping.sourceEditForVisualReplacement({emoji, 1}, QStringLiteral("x")));
    QVERIFY(mapping.sourceEditForVisualReplacement({emoji, 2}, QStringLiteral("note")));
    const int combining = mapping.visualText().indexOf(QString::fromUtf8("é"));
    QVERIFY(!mapping.sourceEditForVisualReplacement({combining, 1}, QStringLiteral("x")));
    QVERIFY(mapping.sourceEditForVisualReplacement({combining, 2}, QStringLiteral("é")));
    QVERIFY(!mapping.sourceEditForVisualReplacement({emoji, 2}, QString(QChar(0xd800))));
    for (const QString &line : {QStringLiteral("` spaced `"), QStringLiteral("`unclosed"), QStringLiteral("`` mixed `")}) {
        const auto protectedMapping = SourceVisualMapping::create(line);
        QCOMPARE(protectedMapping.visualText(), line);
        QVERIFY(!protectedMapping.blocks().first().editable);
    }
}

void FomawriteTest::visualListsContinueAndExitWithoutLosingMarkers() {
    for (const auto &pair : {qMakePair(QStringLiteral("- one"), QStringLiteral("- one\n- ")),
                             qMakePair(QStringLiteral("  + one"), QStringLiteral("  + one\n  + ")),
                             qMakePair(QStringLiteral("09) nine"), QStringLiteral("09) nine\n10) ")),
                             qMakePair(QStringLiteral("  - [X] done"), QStringLiteral("  - [X] done\n  - [ ] "))}) {
        const auto mapping = SourceVisualMapping::create(pair.first);
        const auto edit = mapping.sourceEditForVisualBreak(mapping.visualText().size());
        QVERIFY2(edit.has_value(), qPrintable(pair.first));
        QString candidate = pair.first;
        candidate.replace(edit->edit.source.start, edit->edit.source.length, edit->edit.replacement);
        QCOMPARE(candidate, pair.second);
        const auto projected = SourceVisualMapping::create(candidate);
        QCOMPARE(edit->visualCursor, int(projected.visualText().size()));
        QVERIFY(projected.sourceEditForVisualReplacement({edit->visualCursor, 0}, QStringLiteral("next")));
        const auto exit = projected.sourceEditForVisualBreak(edit->visualCursor);
        QVERIFY(exit.has_value());
        candidate.replace(exit->edit.source.start, exit->edit.source.length, exit->edit.replacement);
        QCOMPARE(candidate, pair.first + QLatin1Char('\n'));
    }
    const QString crlf = QStringLiteral("1. first\r\n2. last");
    const auto mapping = SourceVisualMapping::create(crlf);
    const auto edit = mapping.sourceEditForVisualBreak(mapping.visualText().size());
    QVERIFY(edit.has_value());
    QCOMPARE(edit->edit.replacement, QStringLiteral("\r\n3. "));
    const auto indented = SourceVisualMapping::create(QStringLiteral("  - nested\n    - ambiguous code"));
    QVERIFY(indented.visualText().startsWith(QString::fromUtf8("  • nested")));
    QVERIFY(!indented.blocks().at(1).editable);
    QVERIFY(!indented.sourceEditForVisualBreak(5));
}

void FomawriteTest::visualEmptyParagraphsAndStructuralEditsStaySafe() {
    for (const QString &source : {QString(), QStringLiteral("\n"), QStringLiteral("Before\n\n"), QStringLiteral("Before\r\n\r\n")}) {
        const auto mapping = SourceVisualMapping::create(source);
        const auto edit = mapping.sourceEditForVisualReplacement({int(mapping.visualText().size()), 0}, QStringLiteral("New text"));
        QVERIFY(edit.has_value());
        QString candidate = source;
        candidate.replace(edit->source.start, edit->source.length, edit->replacement);
        QCOMPARE(candidate, source + QStringLiteral("New text"));
        QVERIFY(!mapping.sourceEditForVisualReplacement({int(mapping.visualText().size()), 0}, QStringLiteral("# Heading")));
    }
    QVERIFY(SourceVisualMapping::create(QString()).sourceEditForVisualBreak(0));
    const auto plain = SourceVisualMapping::create(QStringLiteral("ordinary"));
    QVERIFY(!plain.sourceEditForVisualReplacement({0, 0}, QStringLiteral("- ")));
    QVERIFY(!plain.sourceEditForVisualReplacement({0, 0}, QStringLiteral("`")));
    QVERIFY(!plain.sourceEditForVisualReplacement({0, 0}, QStringLiteral("<")));
    QVERIFY(!plain.sourceEditForVisualReplacement({0, 8}, QStringLiteral("**bold**")));
    const auto strong = SourceVisualMapping::create(QStringLiteral("**bold**"));
    QVERIFY(!strong.sourceEditForVisualReplacement({0, 4}, QString()));
}

void FomawriteTest::visualFormatsComposeWithoutChangingSource() {
    const QString source = QStringLiteral(
        "# *Quiet* and **strong** [link](https://example.com) `code`\n\n"
        "| *Name* | [Site](https://example.com) |\n| --- | --- |\n| one | two |\n\n"
        "**[nested](https://example.com)**");
    const auto mapping = SourceVisualMapping::create(source);
    QTextDocument document;
    document.setPlainText(mapping.visualText());
    VisualTextHighlighter highlighter(&document);
    VisualTextHighlighter::Style style;
    style.font = QFont(QStringLiteral("Helvetica"), 14);
    style.textColor = QColor(QStringLiteral("#34363a"));
    style.linkColor = QColor(QStringLiteral("#176fa1"));
    style.sourceOnlyBackground = QColor(QStringLiteral("#efefef"));
    highlighter.setStyle(style);
    highlighter.setMapping(mapping);
    const auto formatAt = [&](const QString &needle) {
        const int position = mapping.visualText().indexOf(needle);
        const QTextBlock block = document.findBlock(position);
        const int relative = position - block.position();
        for (const auto &range : block.layout()->formats())
            if (relative >= range.start && relative < range.start + range.length) return range.format;
        return QTextCharFormat();
    };
    const auto quiet = formatAt(QStringLiteral("Quiet"));
    QVERIFY(quiet.fontItalic());
    QCOMPARE(quiet.fontWeight(), int(QFont::DemiBold));
    QVERIFY(quiet.fontPointSize() > style.font.pointSizeF());
    QCOMPARE(formatAt(QStringLiteral("strong")).fontWeight(), int(QFont::Bold));
    const auto headingLink = formatAt(QStringLiteral("link"));
    QVERIFY(headingLink.fontUnderline());
    QCOMPARE(headingLink.foreground().color(), style.linkColor);
    QCOMPARE(headingLink.fontWeight(), int(QFont::DemiBold));
    const auto code = formatAt(QStringLiteral("code"));
    QCOMPARE(code.fontFamilies().toStringList(), QStringList{QStringLiteral("Menlo")});
    QCOMPARE(code.background().color(), style.sourceOnlyBackground);
    QVERIFY(code.fontPointSize() > style.font.pointSizeF());
    const auto tableEmphasis = formatAt(QStringLiteral("Name"));
    QVERIFY(tableEmphasis.fontItalic());
    QCOMPARE(tableEmphasis.fontWeight(), int(QFont::DemiBold));
    const auto tableLink = formatAt(QStringLiteral("Site"));
    QVERIFY(tableLink.fontUnderline());
    QCOMPARE(tableLink.foreground().color(), style.linkColor);
    QCOMPARE(tableLink.fontWeight(), int(QFont::DemiBold));
    const auto nested = formatAt(QStringLiteral("nested"));
    QVERIFY(nested.fontUnderline());
    QCOMPARE(nested.foreground().color(), style.linkColor);
    QCOMPARE(nested.fontWeight(), int(QFont::Bold));
    QCOMPARE(document.toPlainText(), mapping.visualText());
    QCOMPARE(mapping.roundTripSource(), source);
    // Rehighlighting affects layout formats only and does not clear text Undo.
    QTextCursor cursor(&document);
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(QStringLiteral("!"));
    QVERIFY(document.isUndoAvailable());
    highlighter.setStyle(style);
    QVERIFY(document.isUndoAvailable());
    document.undo();
    QCOMPARE(document.toPlainText(), mapping.visualText());
}
