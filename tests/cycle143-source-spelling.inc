// Cycle143 — misspelled words are underlined as the writer types in the Source
// editor, and a right-click on one offers suggestions, Learn Spelling and
// Ignore Spelling. Grammar joined in Cycle 144 (blue marks, tests/cycle144-source-grammar.inc).
//
// Real-checker tests use words no other test touches ("recieve", "definately",
// "occured", "accomodate"): the system checker's ignore list lasts for the whole
// test run, so a word one test ignores must not be asserted underlined by another.

namespace {
bool cycle143Underlined(QTextDocument &document, int position) {
    const auto block = document.findBlock(position);
    const int offset = position - block.position();
    for (const auto &range : block.layout()->formats())
        if (offset >= range.start && offset < range.start + range.length)
            return range.format.fontUnderline() && range.format.underlineColor() == QColor(QStringLiteral("#d93025"))
                && range.format.background().color().red() > range.format.background().color().green() + 25;
    return false;
}
QTextCharFormat cycle143Format(QTextDocument &document, int position) {
    const auto block = document.findBlock(position);
    const int offset = position - block.position();
    for (const auto &range : block.layout()->formats())
        if (offset >= range.start && offset < range.start + range.length) return range.format;
    return QTextCharFormat();
}
// Position of the misspelling inside the first occurrence of `context`.
int cycle143Word(const QString &text, const QString &context, const QString &word = QStringLiteral("recieve")) {
    const int at = text.indexOf(context);
    return at < 0 ? -1 : at + context.indexOf(word);
}
}

void FomawriteTest::sourceSpellingUnderlinesProseOnly() {
    Cycle103SettingsSnapshot settings;
    SpellCheck spell;
    if (!spell.available()) QSKIP("the system spell checker exists only on macOS");
    spell.setEnabled(true); spell.setLanguage(QStringLiteral("en_US"));
    const QString source = QString::fromUtf8(
        "# Please recieve the plan\n\n"
        "We recieve mail and receive more; `recieve` in code, ``recieve`` too.\n\n"
        "See https://example.com/recieve and [a link](https://example.com/recieve) and <span class=\"recieve\">ok</span>.\n\n"
        "```\nfenced recieve here\n```\n\n"
        "After the fence we recieve again.\n\n"
        "    indented recieve code\n\n"
        "- list item recieve\n    - nested recieve\n\n"
        "snake_recieve_case and foo/recieve/bar.\n\n"
        "Read [the recieve](https://example.com/x/y) note and <a href=\"https://x.com/p\">recieve</a> it.\n\n"
        "[ref]: https://example.com/recieve\n");
    QTextDocument document; document.setPlainText(source);
    MarkdownHighlighter highlighter(&document);
    highlighter.setSpellCheck(&spell);
    highlighter.rehighlight();
    const struct { const char *context; bool underlined; } cases[] = {
        {"Please recieve the plan", true}, {"We recieve mail", true},
        {"we recieve again", true}, {"list item recieve", true}, {"nested recieve", true},
        {"`recieve` in code", false}, {"``recieve`` too", false},
        {"https://example.com/recieve and", false}, {"[a link](https://example.com/recieve)", false},
        {"class=\"recieve\"", false}, {"fenced recieve here", false}, {"indented recieve code", false},
        {"snake_recieve_case", false}, {"foo/recieve/bar", false},
        {"[the recieve](https://example.com/x/y)", true}, {"\">recieve</a>", true}, {"[ref]: https://example.com/recieve", false},
        {"and receive more", false},
    };
    for (const auto &item : cases) {
        const QString context = QString::fromLatin1(item.context);
        const bool correctSpelling = context.contains(QStringLiteral("receive"));
        const int position = cycle143Word(source, context, correctSpelling ? QStringLiteral("receive") : QStringLiteral("recieve"));
        QVERIFY2(position >= 0, item.context);
        QVERIFY2(cycle143Underlined(document, position) == item.underlined, item.context);
        QVERIFY2(cycle143Underlined(document, position + 3) == item.underlined, item.context);
    }
    // The red underline is composed onto the syntax format, not instead of it.
    const auto heading = cycle143Format(document, cycle143Word(source, "Please recieve the plan"));
    QVERIFY(heading.fontUnderline());
    QCOMPARE(heading.underlineColor(), QColor(QStringLiteral("#d93025")));
    QCOMPARE(heading.fontWeight(), int(QFont::Bold));
    // Front matter is data, not prose.
    QTextDocument front; front.setPlainText(QStringLiteral("---\ntitle: recieve\n---\n\nBody recieve text\n"));
    MarkdownHighlighter frontHighlighter(&front);
    frontHighlighter.setSpellCheck(&spell); frontHighlighter.rehighlight();
    const QString frontText = front.toPlainText();
    QVERIFY(!cycle143Underlined(front, cycle143Word(frontText, "title: recieve")));
    QVERIFY(cycle143Underlined(front, cycle143Word(frontText, "Body recieve text")));
}

void FomawriteTest::sourceSpellingFollowsTheSettingAndLearnedWords() {
    Cycle103SettingsSnapshot settings;
    SpellCheck spell;
    if (!spell.available()) QSKIP("the system spell checker exists only on macOS");
    spell.setEnabled(true); spell.setLanguage(QStringLiteral("en_US"));
    const QString source = QStringLiteral("We definately the pieces.\n\nA second definately line.");
    QTextDocument document; document.setPlainText(source);
    MarkdownHighlighter highlighter(&document);
    highlighter.setSpellCheck(&spell); QTest::qWait(20); highlighter.rehighlight();
    const int first = cycle143Word(source, "We definately the", "definately"), second = cycle143Word(source, "second definately", "definately");
    QVERIFY(cycle143Underlined(document, first) && cycle143Underlined(document, second));
    // (b) The setting clears every underline and brings them back.
    spell.setEnabled(false);
    QVERIFY(!cycle143Underlined(document, first) && !cycle143Underlined(document, second));
    for (QTextBlock block = document.firstBlock(); block.isValid(); block = block.next())
        for (const auto &range : block.layout()->formats())
            QCOMPARE(range.format.underlineStyle(), QTextCharFormat::NoUnderline);
    spell.setEnabled(true);
    QVERIFY(cycle143Underlined(document, first) && cycle143Underlined(document, second));
    // (c) Ignoring the word clears it everywhere without touching other text.
    spell.ignoreWord(QStringLiteral("definately"));
    QVERIFY(!cycle143Underlined(document, first) && !cycle143Underlined(document, second));
    // A long document with the real checker restyles in a bounded time.
    spell.setEnabled(true);
    QStringList lines;
    for (int line = 0; line < 2000; ++line)
        lines << QStringLiteral("Paragraph %1 has the usual ordinary words, one accomodate slip and a **bold** [link](https://example.com/%1) too.").arg(line);
    QTextDocument large; large.setPlainText(lines.join('\n'));
    MarkdownHighlighter largeHighlighter(&large);
    QElapsedTimer timer; timer.start();
    largeHighlighter.setSpellCheck(&spell); // restyles every block once, uncached
    qInfo("real checker, 2000 lines: %lld ms", timer.elapsed());
    QVERIFY(timer.elapsed() < 15000);
    // Qt 6.11 restyles lazily as layout reaches each block (adjustSize was not
    // enough on the CI runner): ask for the far block's restyle explicitly.
    largeHighlighter.rehighlightBlock(large.findBlockByNumber(1500));
    QVERIFY(cycle143Underlined(large, large.findBlockByNumber(1500).text().indexOf("accomodate") + large.findBlockByNumber(1500).position()));
}

void FomawriteTest::sourceSpellingChecksOnlyChangedBlocks() {
    int calls = 0;
    const auto checker = [&calls](const QString &prose) {
        ++calls;
        QList<SpellCheck::Range> ranges;
        for (int at = prose.indexOf(QStringLiteral("tomorow")); at >= 0; at = prose.indexOf(QStringLiteral("tomorow"), at + 7))
            ranges.append({at, at + 7});
        return ranges;
    };
    QStringList lines;
    for (int line = 0; line < 2000; ++line)
        lines << QStringLiteral("Line number %1 carries a few ordinary words and tomorow.").arg(line);
    QTextDocument document; document.setPlainText(lines.join('\n'));
    MarkdownHighlighter highlighter(&document);
    highlighter.setSpellChecker(checker);
    highlighter.setSpellingEnabled(true);
    QTest::qWait(20); // the highlighter's own deferred first pass
    highlighter.rehighlight();
    QVERIFY2(calls >= 2000, qPrintable(QString::number(calls)));
    QVERIFY(cycle143Underlined(document, document.findBlockByNumber(1234).position() + lines.at(1234).indexOf("tomorow")));
    // One typed character re-checks the one block it is in, not the document.
    calls = 0;
    QTextCursor cursor(document.findBlockByNumber(1000)); cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertText(QStringLiteral("x"));
    document.adjustSize(); // Qt 6.11 restyles edited blocks as layout reaches them
    QVERIFY2(calls >= 1 && calls <= 3, qPrintable(QString::number(calls)));
    // Undoing it returns to text already checked: no new work.
    calls = 0;
    cursor.deletePreviousChar();
    document.adjustSize();
    QCOMPARE(calls, 0);
    // Restyling for a theme change re-uses every cached answer.
    highlighter.setDarkMode(false); highlighter.setDarkMode(true); document.adjustSize();
    QCOMPARE(calls, 0);
    // A word or language change asks again.
    highlighter.refreshSpelling(); document.adjustSize();
    QVERIFY2(calls >= 2000, qPrintable(QString::number(calls)));
    // Disabled: nothing is checked and nothing is drawn.
    calls = 0;
    highlighter.setSpellingEnabled(false); document.adjustSize();
    QCOMPARE(calls, 0);
    QVERIFY(!cycle143Underlined(document, document.findBlockByNumber(1234).position() + lines.at(1234).indexOf("tomorow")));
}

namespace {
QString cycle143CapturesDirectory() {
    const QString tests = QFileInfo(QFINDTESTDATA("tst_fomawrite.cpp")).absolutePath();
    const QString directory = QDir(tests).filePath("../research/cycle-143/captures");
    QDir().mkpath(directory);
    return QDir(directory).canonicalPath();
}
QRectF cycle143WordBox(const Cycle117Window &f, int start, int end) {
    QRectF first, last;
    QMetaObject::invokeMethod(f.editor, "positionToRectangle", Q_RETURN_ARG(QRectF, first), Q_ARG(int, start));
    QMetaObject::invokeMethod(f.editor, "positionToRectangle", Q_RETURN_ARG(QRectF, last), Q_ARG(int, end));
    return f.editor->mapRectToScene(QRectF(first.left(), first.top(), last.left() - first.left(), first.height()));
}
int cycle143RedPixels(const QImage &frame, const QRectF &box, qreal scale, bool strong = false) {
    int count = 0;
    const QRect pixels(QPointF(box.left() * scale, box.top() * scale).toPoint(),
                       QSizeF(box.width() * scale, (box.height() + 4) * scale).toSize());
    for (int y = pixels.top(); y <= pixels.bottom(); ++y)
        for (int x = pixels.left(); x <= pixels.right(); ++x) {
            if (!frame.rect().contains(x, y)) continue;
            const QColor c = frame.pixelColor(x, y);
            // The pale red wash (#fbd5d1) or the red underline (#d93025).
            if (strong) { if (c.red() > 170 && c.green() < 110 && c.blue() < 110) ++count; continue; }
            if (c.red() > c.green() + 25 && c.red() > c.blue() + 25 && c.red() > 150) ++count;
        }
    return count;
}
}

void FomawriteTest::sourceSpellingUnderlineIsRedInTheRenderedWindow() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setLanguage(QStringLiteral("en_US"));
    QVERIFY(f.mode(0));
    const QString text = QStringLiteral("We recieve the mail and receive it too.");
    f.editor->setProperty("text", text);
    const int bad = text.indexOf(QStringLiteral("recieve")), good = text.indexOf(QStringLiteral("receive"));
    QTRY_VERIFY(!f.backend.misspelledWordAt(bad + 2).isEmpty());
    QVERIFY(f.backend.misspelledWordAt(good + 2).isEmpty());
    const QRectF badBox = cycle143WordBox(f, bad, bad + 7), goodBox = cycle143WordBox(f, good, good + 7);
    QString why;
    QVERIFY2(cycle134SaveFrame(f.quick, QDir(cycle143CapturesDirectory()).filePath("source-spelling-underline.png"), &why), qPrintable(why));
    const QImage frame = f.quick->grabWindow();
    const qreal scale = frame.width() / qreal(f.quick->width());
    const int underBad = cycle143RedPixels(frame, badBox, scale), underGood = cycle143RedPixels(frame, goodBox, scale);
    const int strongBad = cycle143RedPixels(frame, badBox, scale, true);
    qInfo("reddish pixels under recieve: %d (saturated red, i.e. the underline itself: %d), under receive: %d", underBad, strongBad, underGood);
    QVERIFY2(underBad >= 100, qPrintable(QString("no red under the misspelled word (%1)").arg(underBad)));
    QCOMPARE(underGood, 0);
    // The caret at the end of the word (the word being typed) hides the mark in
    // the rendered frame without a rehighlight; moving on brings it back.
    const int revision = f.backend.documentRevision();
    f.editor->setProperty("cursorPosition", bad + 7);
    QVERIFY(cycle134SaveFrame(f.quick, QDir(cycle143CapturesDirectory()).filePath("source-spelling-caret.png"), &why));
    QCOMPARE(cycle143RedPixels(f.quick->grabWindow(), badBox, scale), 0);
    QCOMPARE(f.backend.documentRevision(), revision);
    f.editor->setProperty("cursorPosition", good);
    QVERIFY(cycle134SaveFrame(f.quick, QDir(cycle143CapturesDirectory()).filePath("source-spelling-caret-left.png"), &why));
    QVERIFY(cycle143RedPixels(f.quick->grabWindow(), badBox, scale) >= 100);
    QCOMPARE(f.backend.documentRevision(), revision);
    // Switching the setting off removes it from the rendered frame.
    spell->setEnabled(false);
    QVERIFY(cycle134SaveFrame(f.quick, QDir(cycle143CapturesDirectory()).filePath("source-spelling-off.png"), &why));
    QCOMPARE(cycle143RedPixels(f.quick->grabWindow(), badBox, scale), 0);
}

void FomawriteTest::sourceSpellingContextMenuOffersSuggestionsAndCorrects() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setLanguage(QStringLiteral("en_US"));
    QVERIFY(f.mode(0));
    const QString text = QStringLiteral("We recieve mail, receive more.\n\nThis `recieve` is code and occured once.");
    f.editor->setProperty("text", text);
    auto *menu = f.window->findChild<QObject *>("spellingContextMenu"); QVERIFY(menu);
    const auto pointAt = [&](const QString &context, const QString &word) {
        const int start = text.indexOf(context) + context.indexOf(word);
        return cycle143WordBox(f, start, start + word.size()).center().toPoint();
    };
    const auto rightClick = [&](const QPoint &point) {
        QTest::mouseClick(f.quick, Qt::RightButton, Qt::NoModifier, point); QTest::qWait(150);
    };
    QTRY_VERIFY(!f.backend.misspelledWordAt(text.indexOf("recieve") + 2).isEmpty());
    // Not misspelled, or inside code: the menu does not open.
    rightClick(pointAt("receive more", "receive")); QVERIFY(!menu->property("visible").toBool());
    rightClick(pointAt("`recieve` is", "recieve")); QVERIFY(!menu->property("visible").toBool());
    // On a misspelled word: suggestions, a separator, Learn and Ignore.
    rightClick(pointAt("We recieve mail", "recieve"));
    QTRY_VERIFY(menu->property("visible").toBool());
    QCOMPARE(menu->property("word").toString(), QStringLiteral("recieve"));
    QStringList offered;
    for (int i = 0; i < 5; ++i) {
        auto *item = menu->findChild<QQuickItem *>(QStringLiteral("spellingSuggestion%1").arg(i));
        QVERIFY(item);
        if (item->isVisible()) offered << item->property("text").toString();
    }
    QVERIFY2(offered.contains(QStringLiteral("receive")) && offered.size() <= 5, qPrintable(offered.join(", ")));
    auto *learn = menu->findChild<QQuickItem *>("spellingLearn"), *ignore = menu->findChild<QQuickItem *>("spellingIgnore");
    QVERIFY(learn && ignore && learn->isVisible() && ignore->isVisible());
    QCOMPARE(learn->property("text").toString(), QStringLiteral("Learn Spelling"));
    QCOMPARE(ignore->property("text").toString(), QStringLiteral("Ignore Spelling"));
    // Choosing a suggestion replaces the word, once, and Undo restores it.
    QQuickItem *choice = nullptr;
    for (int i = 0; i < 5 && !choice; ++i) {
        auto *item = menu->findChild<QQuickItem *>(QStringLiteral("spellingSuggestion%1").arg(i));
        if (item && item->isVisible() && item->property("text").toString() == QStringLiteral("receive")) choice = item;
    }
    QVERIFY(choice && cycle102Click(f.quick, choice));
    QTRY_COMPARE(f.editor->property("text").toString(), QStringLiteral("We receive mail, receive more.\n\nThis `recieve` is code and occured once."));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.editor, "undo"));
    QCOMPARE(f.editor->property("text").toString(), text);
    // Ignore Spelling removes the underline and the menu's reach.
    QTRY_VERIFY(!f.backend.misspelledWordAt(text.indexOf("occured") + 2).isEmpty());
    rightClick(pointAt("and occured once", "occured"));
    QTRY_VERIFY(menu->property("visible").toBool());
    QCOMPARE(menu->property("word").toString(), QStringLiteral("occured"));
    QVERIFY(cycle102Click(f.quick, ignore));
    QTRY_VERIFY(f.backend.misspelledWordAt(text.indexOf("occured") + 2).isEmpty());
    // Turned off, a right-click on a misspelling opens nothing.
    spell->setEnabled(false);
    rightClick(pointAt("We recieve mail", "recieve")); QVERIFY(!menu->property("visible").toBool());
}

// The word being typed is not marked until the caret leaves it, and focus mode
// leaves the dimmed text outside the focus alone (reviewer findings F1, F10).
void FomawriteTest::sourceSpellingWithholdsTheCaretWordAndDimmedText() {
    Cycle103SettingsSnapshot settings;
    SpellCheck spell;
    if (!spell.available()) QSKIP("the system spell checker exists only on macOS");
    spell.setEnabled(true); spell.setLanguage(QStringLiteral("en_US"));
    const QString source = QStringLiteral("We recieve mail here.\n\nA second recieve line.");
    QTextDocument document; document.setPlainText(source);
    MarkdownHighlighter highlighter(&document);
    highlighter.setSpellCheck(&spell); QTest::qWait(20); highlighter.rehighlight();
    const int first = cycle143Word(source, "We recieve mail"), second = cycle143Word(source, "second recieve line");
    QVERIFY(cycle143Underlined(document, first) && cycle143Underlined(document, second));
    // Caret at the end of the first word: that word is withheld, the other stays,
    // and the document revision does not move (a caret move is not an edit).
    const int revision = document.revision();
    highlighter.setCaret(first + 7);
    QVERIFY(!cycle143Underlined(document, first));
    QVERIFY(cycle143Underlined(document, second));
    QCOMPARE(document.revision(), revision);
    // Caret one short of the end, or past it: the word is drawn again.
    highlighter.setCaret(first + 6);
    QVERIFY(cycle143Underlined(document, first));
    highlighter.setCaret(first + 7);
    QVERIFY(!cycle143Underlined(document, first));
    highlighter.setCaret(first + 8);
    QVERIFY(cycle143Underlined(document, first));
    // Leaving the block restyles the withheld block too.
    highlighter.setCaret(first + 7);
    QVERIFY(!cycle143Underlined(document, first));
    highlighter.setCaret(second);
    QVERIFY(cycle143Underlined(document, first));
    QCOMPARE(document.revision(), revision);
    // Typing a word: each step keeps the caret at its end, so nothing flashes.
    QTextCursor cursor(&document); cursor.setPosition(document.characterCount() - 1);
    const QString typed = QStringLiteral(" tomor");
    for (int i = 0; i < typed.size(); ++i) {
        cursor.insertText(typed.mid(i, 1));
        highlighter.setCaret(cursor.position());
        document.adjustSize();
        QVERIFY2(!cycle143Underlined(document, cursor.position() - 1), qPrintable(QStringLiteral("after typing %1").arg(typed.left(i + 1))));
    }
    cursor.insertText(QStringLiteral(" "));
    highlighter.setCaret(cursor.position()); document.adjustSize();
    QVERIFY(cycle143Underlined(document, cursor.position() - 3)); // "tomor" is a word now
    // Focus mode: a misspelling outside the focused range carries no wash.
    highlighter.setCaret(-1);
    highlighter.setFocusRange(document.findBlock(second).position(), document.findBlock(second).position() + document.findBlock(second).length());
    QVERIFY(!cycle143Underlined(document, first));
    QVERIFY(cycle143Underlined(document, second));
    highlighter.setFocusRange(-1, -1);
    QVERIFY(cycle143Underlined(document, first));
}
