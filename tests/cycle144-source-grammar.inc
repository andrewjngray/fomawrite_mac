// Cycle144 — grammar findings (the macOS grammar pass, via SpellCheck::issues)
// get a blue underline and a pale blue wash in the Source editor as the writer
// types, and a right-click on one shows the checker's message, its corrections
// and Ignore Grammar Issue. Spelling keeps its red mark and wins where the two
// overlap.
//
// Real-checker tests use words and sentences no other test touches (the doubled
// word "the the", the misspelling "recieve"); the checker's ignore list and
// QSettings are process-wide. Ignore Grammar Issue is kept by the highlighter,
// per document, so it never leaks between tests.

namespace {
const QColor cycle144BlueColor(QStringLiteral("#1a73e8"));

bool cycle144IsBlueMark(const QTextCharFormat &format) {
    return format.fontUnderline() && format.underlineColor() == cycle144BlueColor
        && format.background().color().blue() > format.background().color().red() + 15
        && format.intProperty(MarkdownHighlighter::SpellCategoryProperty) == 2;
}
bool cycle144IsBlue(QTextDocument &document, int position) {
    return cycle144IsBlueMark(cycle143Format(document, position));
}
bool cycle144Red(QTextDocument &document, int position) { return cycle143Underlined(document, position); }

// The grammar finding in the block holding `position` that contains "the the".
SpellCheck::Issue cycle144DoubledWord(MarkdownHighlighter &highlighter, QTextDocument &document, int position) {
    for (const auto &issue : highlighter.grammarIssuesInBlock(document.findBlock(position)))
        if (issue.category == QStringLiteral("Grammar")) return issue;
    return {-1, -1, QString(), QString(), QStringList()};
}
double cycle144Luminance(const QColor &color) {
    const auto channel = [](double value) { return value <= 0.03928 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4); };
    return 0.2126 * channel(color.redF()) + 0.7152 * channel(color.greenF()) + 0.0722 * channel(color.blueF());
}
double cycle144Contrast(const QColor &a, const QColor &b) {
    const double la = cycle144Luminance(a), lb = cycle144Luminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}
}

// (a) A doubled word is blue, a misspelling in the same block stays red, and
// nothing inside a fence is marked. Where the two overlap, spelling wins.
void FomawriteTest::sourceGrammarUnderlinesDoubledWordsAndSpellingStaysRed() {
    Cycle103SettingsSnapshot settings;
    SpellCheck spell;
    if (!spell.available()) QSKIP("the system spell checker exists only on macOS");
    spell.setEnabled(true); spell.setGrammarEnabled(true); spell.setLanguage(QStringLiteral("en_US"));
    const QString source = QStringLiteral(
        "We recieve the the mail today.\n\n"
        "```\nfenced the the recieve here\n```\n\n"
        "After the fence we send the the parcel.\n");
    QTextDocument document; document.setPlainText(source);
    MarkdownHighlighter highlighter(&document);
    highlighter.setSpellCheck(&spell); QTest::qWait(20); highlighter.rehighlight();
    const int first = source.indexOf(QStringLiteral("the the")), bad = source.indexOf(QStringLiteral("recieve"));
    const auto issue = cycle144DoubledWord(highlighter, document, first);
    QVERIFY2(issue.start >= 0, "the macOS grammar pass flags a doubled word");
    QVERIFY(!issue.message.isEmpty());
    const int issueStart = document.findBlock(first).position() + issue.start, issueEnd = document.findBlock(first).position() + issue.end;
    qInfo("grammar range [%d, %d) %s message: %s", issue.start, issue.end,
          qPrintable(source.mid(issueStart, issueEnd - issueStart)), qPrintable(issue.message));
    QVERIFY(issueStart <= first && issueEnd >= first + 7); // covers both "the"s
    // Blue on every character of the range except those spelling claims.
    bool anyBlue = false;
    for (int position = issueStart; position < issueEnd; ++position) {
        const bool misspelled = position >= bad && position < bad + 7;
        if (misspelled) { QVERIFY2(cycle144Red(document, position) && !cycle144IsBlue(document, position), "spelling wins the overlap"); continue; }
        QVERIFY2(cycle144IsBlue(document, position), qPrintable(QStringLiteral("blue at %1").arg(position - issueStart)));
        QVERIFY(!cycle144Red(document, position));
        anyBlue = true;
    }
    QVERIFY(anyBlue);
    for (int offset = 0; offset < 7; ++offset) QVERIFY(cycle144Red(document, bad + offset));
    // Nothing inside the fence, and the second paragraph is marked on its own.
    const int fenced = source.indexOf(QStringLiteral("fenced the the recieve"));
    for (int offset = 0; offset < 22; ++offset) {
        QVERIFY(!cycle144IsBlue(document, fenced + offset));
        QVERIFY(!cycle144Red(document, fenced + offset));
    }
    const int second = source.lastIndexOf(QStringLiteral("the the"));
    QVERIFY(cycle144IsBlue(document, second + 2) || cycle144IsBlue(document, second + 5));
    // The mark is composed onto the syntax format, and the range carries its end.
    QTextDocument heading; heading.setPlainText(QStringLiteral("# A heading with the the doubled word"));
    MarkdownHighlighter headingHighlighter(&heading);
    headingHighlighter.setSpellCheck(&spell); QTest::qWait(20); headingHighlighter.rehighlight();
    const int headingWord = heading.toPlainText().indexOf(QStringLiteral("the the"));
    const auto headingFormat = cycle143Format(heading, headingWord + 1);
    QVERIFY(cycle144IsBlueMark(headingFormat) || cycle144IsBlueMark(cycle143Format(heading, headingWord + 5)));
    QCOMPARE(headingFormat.fontWeight(), int(QFont::Bold));
    // Overlap: a sentence-level grammar range that contains a misspelled word.
    // The macOS pass only flags doubled words here, so the findings are stubbed
    // (the bare-function seam, like setSpellChecker): spelling wins for the
    // characters of "recieve"; the rest of the range is blue.
    QTextDocument overlap; overlap.setPlainText(QStringLiteral("We recieve mail today.\n\nPlain second line."));
    MarkdownHighlighter overlapHighlighter(&overlap);
    overlapHighlighter.setSpellChecker([](const QString &prose) {
        QList<SpellCheck::Range> ranges; const int at = prose.indexOf(QStringLiteral("recieve"));
        if (at >= 0) ranges.append({at, at + 7});
        return ranges; });
    overlapHighlighter.setGrammarChecker([](const QString &prose) {
        QList<SpellCheck::Issue> issues;
        if (prose.startsWith(QStringLiteral("We recieve")))
            issues.append({0, int(prose.size()), QStringLiteral("Grammar"), QStringLiteral("stub"), {QStringLiteral("We receive mail today.")}});
        return issues; });
    overlapHighlighter.setSpellingEnabled(true); overlapHighlighter.rehighlight();
    const QString overlapText = overlap.toPlainText();
    for (int position = 0; position < overlapText.indexOf(QLatin1Char('\n')); ++position) {
        if (overlapText.at(position).isSpace()) continue;
        const bool misspelled = position >= 3 && position < 10;
        QVERIFY2(misspelled ? (cycle144Red(overlap, position) && !cycle144IsBlue(overlap, position))
                            : (cycle144IsBlue(overlap, position) && !cycle144Red(overlap, position)),
                 qPrintable(QStringLiteral("overlap at %1").arg(position)));
    }
    QVERIFY(!cycle143Format(overlap, overlapText.indexOf(QStringLiteral("Plain"))).fontUnderline());
    // The two washes keep the editor's text above 4.5:1 in both themes.
    qInfo("contrast: light text #34363A / #222324 on #d9e7fb = %.2f / %.2f; dark #ECEEF2 / #eeeeee on #243a5c = %.2f / %.2f",
          cycle144Contrast(QColor("#34363A"), QColor("#d9e7fb")), cycle144Contrast(QColor("#222324"), QColor("#d9e7fb")),
          cycle144Contrast(QColor("#ECEEF2"), QColor("#243a5c")), cycle144Contrast(QColor("#eeeeee"), QColor("#243a5c")));
    for (const QColor &text : {QColor("#34363A"), QColor("#222324"), QColor("#342f27")})
        QVERIFY(cycle144Contrast(text, QColor("#d9e7fb")) > 4.5);
    for (const QColor &text : {QColor("#ECEEF2"), QColor("#eeeeee")})
        QVERIFY(cycle144Contrast(text, QColor("#243a5c")) > 4.5);
    // Dark mode swaps the wash.
    highlighter.setDarkMode(true);
    QCOMPARE(cycle143Format(document, first + 5).background().color(), QColor("#243a5c"));
    highlighter.setDarkMode(false);
    QCOMPARE(cycle143Format(document, first + 5).background().color(), QColor("#d9e7fb"));
}

// (b) The grammar setting clears blue and keeps red; turning it on brings it back.
void FomawriteTest::sourceGrammarFollowsItsSettingAndKeepsSpelling() {
    Cycle103SettingsSnapshot settings;
    SpellCheck spell;
    if (!spell.available()) QSKIP("the system spell checker exists only on macOS");
    spell.setEnabled(true); spell.setGrammarEnabled(true); spell.setLanguage(QStringLiteral("en_US"));
    const QString source = QStringLiteral("We recieve the the mail today.");
    QTextDocument document; document.setPlainText(source);
    MarkdownHighlighter highlighter(&document);
    highlighter.setSpellCheck(&spell); QTest::qWait(20); highlighter.rehighlight();
    const int word = source.indexOf(QStringLiteral("the the")), bad = source.indexOf(QStringLiteral("recieve"));
    const auto blueCount = [&] { int count = 0; for (int p = 0; p < source.size(); ++p) count += cycle144IsBlue(document, p); return count; };
    QVERIFY(blueCount() > 0);
    spell.setGrammarEnabled(false);
    QCOMPARE(blueCount(), 0);
    for (int offset = 0; offset < 7; ++offset) QVERIFY(cycle144Red(document, bad + offset));
    for (int position = word; position < word + 7; ++position) QVERIFY(!cycle143Format(document, position).fontUnderline());
    QVERIFY(highlighter.grammarIssuesInBlock(document.firstBlock()).isEmpty());
    spell.setGrammarEnabled(true);
    QVERIFY(blueCount() > 0);
    for (int offset = 0; offset < 7; ++offset) QVERIFY(cycle144Red(document, bad + offset));
    // Spelling off takes grammar with it (the service's master switch).
    spell.setEnabled(false);
    QCOMPARE(blueCount(), 0);
}

// (c) The caret rule applies to grammar: a range ending at the caret is
// withheld, and moving the caret never touches the document revision.
void FomawriteTest::sourceGrammarWithholdsTheRangeAtTheCaretWithoutARevision() {
    Cycle103SettingsSnapshot settings;
    SpellCheck spell;
    if (!spell.available()) QSKIP("the system spell checker exists only on macOS");
    spell.setEnabled(true); spell.setGrammarEnabled(true); spell.setLanguage(QStringLiteral("en_US"));
    const QString source = QStringLiteral("We send the the parcel today.\n\nA second line we send the the letter.");
    QTextDocument document; document.setPlainText(source);
    MarkdownHighlighter highlighter(&document);
    highlighter.setSpellCheck(&spell); QTest::qWait(20); highlighter.rehighlight();
    const auto firstIssue = cycle144DoubledWord(highlighter, document, 0);
    QVERIFY(firstIssue.start >= 0);
    const int end = firstIssue.end, inside = firstIssue.end - 2;
    const int secondBlock = document.findBlockByNumber(2).position();
    const auto secondIssue = cycle144DoubledWord(highlighter, document, secondBlock);
    QVERIFY(secondIssue.start >= 0);
    QVERIFY(cycle144IsBlue(document, inside) && cycle144IsBlue(document, secondBlock + secondIssue.end - 2));
    const int revision = document.revision();
    highlighter.setCaret(end);
    QVERIFY(!cycle144IsBlue(document, inside));
    QVERIFY(!cycle143Format(document, inside).fontUnderline());
    QVERIFY(cycle144IsBlue(document, secondBlock + secondIssue.end - 2));
    QCOMPARE(document.revision(), revision);
    // One short of the end: drawn again (the range does not end at the caret).
    highlighter.setCaret(end - 1);
    QVERIFY(cycle144IsBlue(document, inside));
    highlighter.setCaret(end);
    QVERIFY(!cycle144IsBlue(document, inside));
    highlighter.setCaret(end + 1);
    QVERIFY(cycle144IsBlue(document, inside));
    // Leaving the block restores the withheld block too, with the same colours.
    highlighter.setCaret(end);
    QVERIFY(!cycle144IsBlue(document, inside));
    highlighter.setCaret(secondBlock);
    QVERIFY(cycle144IsBlue(document, inside));
    QCOMPARE(cycle143Format(document, inside).underlineColor(), cycle144BlueColor);
    QCOMPARE(cycle143Format(document, inside).background().color(), QColor("#243a5c")); // the highlighter starts dark
    QCOMPARE(document.revision(), revision);
    // Focus mode: outside the focus range there is no grammar wash either.
    highlighter.setCaret(-1);
    highlighter.setFocusRange(secondBlock, secondBlock + document.findBlockByNumber(2).length());
    QVERIFY(!cycle144IsBlue(document, inside));
    QVERIFY(cycle144IsBlue(document, secondBlock + secondIssue.end - 2));
    highlighter.setFocusRange(-1, -1);
    QVERIFY(cycle144IsBlue(document, inside));
}

namespace {
int cycle144BluePixels(const QImage &frame, const QRectF &box, qreal scale) {
    int count = 0;
    const QRect pixels(QPointF(box.left() * scale, box.top() * scale).toPoint(),
                       QSizeF(box.width() * scale, (box.height() + 4) * scale).toSize());
    for (int y = pixels.top(); y <= pixels.bottom(); ++y)
        for (int x = pixels.left(); x <= pixels.right(); ++x) {
            if (!frame.rect().contains(x, y)) continue;
            const QColor c = frame.pixelColor(x, y);
            // The pale blue wash (#d9e7fb), the underline (#1a73e8) or their dark-mode kin.
            if (c.blue() > c.red() + 20 && c.blue() > c.green() + 10 && c.blue() > 150) ++count;
        }
    return count;
}
}

// (d) The rendered window draws it: pixels under "the the", none under clean
// words, none while the caret sits at the end of the range, none when off.
void FomawriteTest::sourceGrammarUnderlineIsBlueInTheRenderedWindow() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setGrammarEnabled(true); spell->setLanguage(QStringLiteral("en_US"));
    QVERIFY(f.mode(0));
    const QString text = QStringLiteral("We send the the parcel today and then ship it onward.");
    f.editor->setProperty("text", text);
    const int doubled = text.indexOf(QStringLiteral("the the")), clean = text.indexOf(QStringLiteral("onward"));
    QTRY_VERIFY(!f.backend.grammarIssueAt(doubled + 2).isEmpty());
    const auto hit = f.backend.grammarIssueAt(doubled + 2);
    const int start = hit.value("start").toInt(), end = hit.value("end").toInt();
    QVERIFY(end > start && start <= doubled && end >= doubled + 7);
    QVERIFY(f.backend.misspelledWordAt(doubled + 2).isEmpty());
    // The box covers the doubled word itself even when the range is the sentence.
    const QRectF markBox = cycle143WordBox(f, doubled, doubled + 7), cleanBox = cycle143WordBox(f, clean, clean + 6);
    // The cursor starts at 0; keep it away from the range's end.
    QString why;
    QVERIFY2(cycle134SaveFrame(f.quick, QDir(cycle143CapturesDirectory()).filePath("source-grammar-underline.png"), &why), qPrintable(why));
    QImage frame = f.quick->grabWindow();
    const qreal scale = frame.width() / qreal(f.quick->width());
    const int under = cycle144BluePixels(frame, markBox, scale);
    const int underClean = (end >= clean + 6) ? -1 : cycle144BluePixels(frame, cleanBox, scale);
    qInfo("bluish pixels under \"the the\": %d (grammar range [%d, %d) of %d chars); under \"onward\": %d",
          under, start, end, int(text.size()), underClean);
    QVERIFY2(under >= 100, qPrintable(QStringLiteral("no blue under the doubled word (%1)").arg(under)));
    if (underClean >= 0) QCOMPARE(underClean, 0);
    // No red anywhere near it: this is the grammar colour, not the spelling one.
    QCOMPARE(cycle143RedPixels(frame, markBox, scale), 0);
    // The caret at the end of the range hides it without touching the document.
    const int revision = f.backend.documentRevision();
    f.editor->setProperty("cursorPosition", end);
    QVERIFY(cycle134SaveFrame(f.quick, QDir(cycle143CapturesDirectory()).filePath("source-grammar-caret.png"), &why));
    QCOMPARE(cycle144BluePixels(f.quick->grabWindow(), markBox, scale), 0);
    QCOMPARE(f.backend.documentRevision(), revision);
    f.editor->setProperty("cursorPosition", 0);
    QVERIFY(cycle134SaveFrame(f.quick, QDir(cycle143CapturesDirectory()).filePath("source-grammar-caret-left.png"), &why));
    QVERIFY(cycle144BluePixels(f.quick->grabWindow(), markBox, scale) >= 100);
    QCOMPARE(f.backend.documentRevision(), revision);
    // Setting off: gone from the frame.
    spell->setGrammarEnabled(false);
    QVERIFY(cycle134SaveFrame(f.quick, QDir(cycle143CapturesDirectory()).filePath("source-grammar-off.png"), &why));
    QCOMPARE(cycle144BluePixels(f.quick->grabWindow(), markBox, scale), 0);
}

// (e) Right-click on the grammar range: the message (disabled), corrections,
// Ignore Grammar Issue. A correction fixes the text and Undo restores it;
// Ignore clears the mark without a revision bump.
void FomawriteTest::sourceGrammarContextMenuShowsTheMessageCorrectsAndIgnores() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setGrammarEnabled(true); spell->setLanguage(QStringLiteral("en_US"));
    QVERIFY(f.mode(0));
    const QString text = QStringLiteral("We recieve mail today.\n\nThen we send the the parcel onward.");
    f.editor->setProperty("text", text);
    auto *menu = f.window->findChild<QObject *>("grammarContextMenu"); QVERIFY(menu);
    auto *spellingMenu = f.window->findChild<QObject *>("spellingContextMenu"); QVERIFY(spellingMenu);
    const int doubled = text.indexOf(QStringLiteral("the the"));
    const auto pointAt = [&](int start, int length) { return cycle143WordBox(f, start, start + length).center().toPoint(); };
    const auto rightClick = [&](const QPoint &point) { QTest::mouseClick(f.quick, Qt::RightButton, Qt::NoModifier, point); QTest::qWait(150); };
    QTRY_VERIFY(!f.backend.grammarIssueAt(doubled + 2).isEmpty());
    QTRY_VERIFY(!f.backend.misspelledWordAt(text.indexOf(QStringLiteral("recieve")) + 2).isEmpty());
    const auto hit = f.backend.grammarIssueAt(doubled + 2);
    const int start = hit.value("start").toInt(), end = hit.value("end").toInt();
    const QString original = hit.value("word").toString();
    QCOMPARE(text.mid(start, end - start), original);
    QVERIFY(!hit.value("message").toString().isEmpty());
    qInfo("grammar hit: \"%s\" message: %s suggestions: %s", qPrintable(original), qPrintable(hit.value("message").toString()),
          qPrintable(hit.value("suggestions").toStringList().join(" | ")));
    // A spelling hit still opens the spelling menu, not the grammar one.
    rightClick(pointAt(text.indexOf(QStringLiteral("recieve")), 7));
    QTRY_VERIFY(spellingMenu->property("visible").toBool());
    QVERIFY(!menu->property("visible").toBool());
    QMetaObject::invokeMethod(spellingMenu, "close"); QTRY_VERIFY(!spellingMenu->property("visible").toBool());
    // A clean word opens neither.
    rightClick(pointAt(text.indexOf(QStringLiteral("mail")), 4));
    QVERIFY(!menu->property("visible").toBool() && !spellingMenu->property("visible").toBool());
    // The grammar range: message first (disabled), corrections, a separator, Ignore; no Learn.
    rightClick(pointAt(doubled, 7));
    QTRY_VERIFY(menu->property("visible").toBool());
    QVERIFY(!spellingMenu->property("visible").toBool());
    auto *message = menu->findChild<QQuickItem *>("grammarMessage"); QVERIFY(message);
    QVERIFY(message->isVisible() && !message->isEnabled());
    QCOMPARE(message->property("text").toString(), hit.value("message").toString());
    QStringList offered; QQuickItem *choice = nullptr;
    for (int i = 0; i < 5; ++i) {
        auto *item = menu->findChild<QQuickItem *>(QStringLiteral("grammarSuggestion%1").arg(i)); QVERIFY(item);
        if (!item->isVisible()) continue;
        offered << item->property("text").toString();
        if (!choice && item->property("text").toString() != original) choice = item;
    }
    QVERIFY2(!offered.isEmpty() && offered.size() <= 5, "the checker corrects a doubled word");
    QCOMPARE(offered, hit.value("suggestions").toStringList().mid(0, 5));
    auto *ignore = menu->findChild<QQuickItem *>("grammarIgnore"); QVERIFY(ignore && ignore->isVisible());
    QCOMPARE(ignore->property("text").toString(), QStringLiteral("Ignore Grammar Issue"));
    QVERIFY(!menu->findChild<QQuickItem *>("spellingLearn"));
    QVERIFY(choice);
    const QString replacement = choice->property("text").toString();
    QVERIFY(cycle102Click(f.quick, choice));
    QString fixed = text; fixed.replace(start, end - start, replacement);
    QTRY_COMPARE(f.editor->property("text").toString(), fixed);
    QTRY_VERIFY(!menu->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.editor, "undo"));
    QCOMPARE(f.editor->property("text").toString(), text);
    // Ignore Grammar Issue clears the mark and the menu's reach, with no revision bump.
    QTRY_VERIFY(!f.backend.grammarIssueAt(doubled + 2).isEmpty());
    rightClick(pointAt(doubled, 7));
    QTRY_VERIFY(menu->property("visible").toBool());
    const int revision = f.backend.documentRevision();
    QVERIFY(cycle102Click(f.quick, menu->findChild<QQuickItem *>("grammarIgnore")));
    QTRY_VERIFY(f.backend.grammarIssueAt(doubled + 2).isEmpty());
    QCOMPARE(f.backend.documentRevision(), revision);
    QCOMPARE(f.editor->property("text").toString(), text);
    rightClick(pointAt(doubled, 7));
    QVERIFY(!menu->property("visible").toBool());
    // The spelling mark in the other paragraph is untouched by the ignore.
    QVERIFY(!f.backend.misspelledWordAt(text.indexOf(QStringLiteral("recieve")) + 2).isEmpty());
    // Grammar off: the right-click on the range opens nothing.
    QTRY_VERIFY(f.backend.grammarIssueAt(doubled + 2).isEmpty());
    spell->setGrammarEnabled(false);
    rightClick(pointAt(doubled, 7));
    QVERIFY(!menu->property("visible").toBool());
}

// The ignore set is the highlighter's own, per document: it hides the finding
// from the underline and from issuesInBlock, strips the mark in place without a
// revision bump, and a fresh highlighter (a document attach) starts empty.
void FomawriteTest::sourceGrammarIgnoreIsPerDocumentAndRestylesInPlace() {
    Cycle103SettingsSnapshot settings;
    SpellCheck spell;
    if (!spell.available()) QSKIP("the system spell checker exists only on macOS");
    spell.setEnabled(true); spell.setGrammarEnabled(true); spell.setLanguage(QStringLiteral("en_US"));
    const QString source = QStringLiteral("We hand the the cargo over.\n\nThen we hand the the crate over.");
    QTextDocument document; document.setPlainText(source);
    MarkdownHighlighter highlighter(&document);
    highlighter.setSpellCheck(&spell); QTest::qWait(20); highlighter.rehighlight();
    const auto first = cycle144DoubledWord(highlighter, document, 0);
    const int secondBlock = document.findBlockByNumber(2).position();
    const auto second = cycle144DoubledWord(highlighter, document, secondBlock);
    QVERIFY(first.start >= 0 && second.start >= 0);
    QVERIFY(cycle144IsBlue(document, first.end - 2) && cycle144IsBlue(document, secondBlock + second.end - 2));
    const int revision = document.revision();
    highlighter.ignoreIssue(0, first.start, first.end);
    QVERIFY(!cycle144IsBlue(document, first.end - 2));
    QVERIFY(!cycle143Format(document, first.end - 2).fontUnderline());
    QVERIFY(cycle144IsBlue(document, secondBlock + second.end - 2));
    QCOMPARE(document.revision(), revision);
    QVERIFY(highlighter.grammarIssuesInBlock(document.firstBlock()).isEmpty());
    QVERIFY(!highlighter.grammarIssuesInBlock(document.findBlockByNumber(2)).isEmpty());
    // A later restyle of the block (a theme change) keeps it ignored.
    highlighter.setDarkMode(true); highlighter.setDarkMode(false);
    QVERIFY(!cycle144IsBlue(document, first.end - 2));
    QVERIFY(cycle144IsBlue(document, secondBlock + second.end - 2));
    // A new highlighter on the document (attach) starts with an empty set.
    QTextDocument copy; copy.setPlainText(source);
    MarkdownHighlighter fresh(&copy);
    fresh.setSpellCheck(&spell); QTest::qWait(20); fresh.rehighlight();
    QVERIFY(!fresh.grammarIssuesInBlock(copy.firstBlock()).isEmpty());
    highlighter.clearIgnoredIssues();
    QVERIFY(!highlighter.grammarIssuesInBlock(document.firstBlock()).isEmpty());
}
