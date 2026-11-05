// Cycle155 (fix E) — wrapping toggles and composes, Live keeps the wrapped text selected,
// Code Block fences whole paragraphs and unfences. Uses Cycle155Window from cycle155-format-bar.inc.

namespace {
// Applies wrapSelection at [from,to] of the Source document and returns the new selection.
struct Cycle155Wrap {
    Cycle155Window *w;
    int from, to;
    QString apply(const QString &mark) {
        const auto r = w->backend.wrapSelection(from, to, mark, mark);
        from = r.value("start").toInt(); to = r.value("end").toInt();
        return w->canonical();
    }
};
}

// Red before the fix: the inner `*` of `**` counted as italic, so I on bold text un-wrapped
// one star instead of composing, and B on `***word***` did not give `*word*`.
void FomawriteTest::wrappingTogglesAndComposesInlineMarkers() {
    Cycle155Window w;
    QVERIFY2(w.create(QStringLiteral("a word b\n"), false, false), qPrintable(w.error));
    const QString bare = QStringLiteral("a word b\n");
    const auto fresh = [&] { w.editor->setProperty("text", bare); return Cycle155Wrap{&w, 2, 6}; };
    {   // B then I; I then B; both end in ***word***
        auto s = fresh();
        QCOMPARE(s.apply("**"), QStringLiteral("a **word** b\n"));
        QCOMPARE(s.apply("*"), QStringLiteral("a ***word*** b\n"));
        QCOMPARE(w.canonical().mid(s.from, s.to - s.from), QStringLiteral("word"));
        QCOMPARE(s.apply("**"), QStringLiteral("a *word* b\n"));      // B on ***word*** keeps the italic
        QCOMPARE(s.apply("*"), bare);
        s = fresh();
        QCOMPARE(s.apply("*"), QStringLiteral("a *word* b\n"));
        QCOMPARE(s.apply("**"), QStringLiteral("a ***word*** b\n"));
        QCOMPARE(s.apply("*"), QStringLiteral("a **word** b\n"));      // I on ***word*** keeps the bold
        QCOMPARE(s.apply("**"), bare);
    }
    {   // B twice is a no-op overall, for every toggling marker
        for (const QString &mark : {QStringLiteral("**"), QStringLiteral("*"), QStringLiteral("~~"), QStringLiteral("`"), QStringLiteral("==")}) {
            auto s = fresh();
            const QString once = s.apply(mark);
            QCOMPARE(once, QStringLiteral("a ") + mark + QStringLiteral("word") + mark + QStringLiteral(" b\n"));
            QCOMPARE(w.canonical().mid(s.from, s.to - s.from), QStringLiteral("word"));
            QCOMPARE(s.apply(mark), bare);
            QCOMPARE(w.canonical().mid(s.from, s.to - s.from), QStringLiteral("word"));
        }
    }
    {   // Strike, code and highlight compose with bold and toggle their own markers only
        auto s = fresh();
        s.apply("**");
        QCOMPARE(s.apply("~~"), QStringLiteral("a **~~word~~** b\n"));
        QCOMPARE(s.apply("~~"), QStringLiteral("a **word** b\n"));
        QCOMPARE(s.apply("=="), QStringLiteral("a **==word==** b\n"));
        QCOMPARE(s.apply("=="), QStringLiteral("a **word** b\n"));
        QCOMPARE(s.apply("`"), QStringLiteral("a **`word`** b\n"));
        QCOMPARE(s.apply("`"), QStringLiteral("a **word** b\n"));
    }
    {   // The selection may include its own markers; the whole `**word**` toggles off, `*` of `**` is not italic
        w.editor->setProperty("text", QStringLiteral("a **word** b\n"));
        auto r = w.backend.wrapSelection(2, 10, "*", "*");
        QCOMPARE(w.canonical(), QStringLiteral("a ***word*** b\n"));
        QCOMPARE(r.value("start").toInt(), 3);
        w.editor->setProperty("text", QStringLiteral("a ***word*** b\n"));
        w.backend.wrapSelection(2, 12, "**", "**");
        QCOMPARE(w.canonical(), QStringLiteral("a *word* b\n"));
    }
    {   // Caret only (unchanged behaviour): a pair of markers with the caret between them; a caret
        // between a pair of the same markers removes them, and `*` next to `**` adds, not removes.
        w.editor->setProperty("text", bare);
        auto r = w.backend.wrapSelection(4, 4, "**", "**");
        QCOMPARE(w.canonical(), QStringLiteral("a wo****rd b\n"));
        QCOMPARE(r.value("start").toInt(), 6);
        QCOMPARE(r.value("end").toInt(), 6);
        w.backend.wrapSelection(6, 6, "**", "**");
        QCOMPARE(w.canonical(), bare);
        w.editor->setProperty("text", QStringLiteral("a ****b\n"));
        w.backend.wrapSelection(4, 4, "*", "*");
        QCOMPARE(w.canonical(), QStringLiteral("a ******b\n"));
    }
    w.backend.discardRecovery();
}

// Red before the fix in Live: the host collapsed the selection to the end of the wrapped
// text, so the second click wrapped an empty selection (`****`).
void FomawriteTest::consecutiveFormatBarClicksToggleInSourceAndLive() {
    for (const bool live : {false, true}) {
        Cycle155Window w;
        QVERIFY2(w.create(QStringLiteral("a word b\n"), live, true), qPrintable(w.error));
        QVERIFY(live ? w.selectInPage(2, 6) : w.selectInSource(2, 6));
        const auto press = [&](const char *button, const QString &expected) {
            if (!w.click(button)) return false;
            if (!QTest::qWaitFor([&] { return w.canonical() == expected; }, 8000)) return false;
            if (live && !QTest::qWaitFor([&] { return w.pageText() == expected; }, 8000)) return false;
            QTest::qWait(250);   // the page applies the new selection after the edit
            return true;
        };
        const char *where = live ? "live" : "source";
        // Without ever reselecting: B, I, B, I and then S twice.
        QVERIFY2(press("formatBold", "a **word** b\n"), where);
        QVERIFY2(press("formatItalic", "a ***word*** b\n"), where);
        QVERIFY2(press("formatBold", "a *word* b\n"), where);
        QVERIFY2(press("formatItalic", "a word b\n"), where);
        QVERIFY2(press("formatItalic", "a *word* b\n"), where);
        QVERIFY2(press("formatBold", "a ***word*** b\n"), where);
        QVERIFY2(press("formatItalic", "a **word** b\n"), where);
        QVERIFY2(press("formatBold", "a word b\n"), where);
        QVERIFY2(press("formatStrike", "a ~~word~~ b\n"), where);
        QVERIFY2(press("formatStrike", "a word b\n"), where);
        QVERIFY2(press("formatCode", "a `word` b\n"), where);
        QVERIFY2(press("formatCode", "a word b\n"), where);
        QVERIFY2(press("formatHighlight", "a ==word== b\n"), where);
        QVERIFY2(press("formatHighlight", "a word b\n"), where);
        w.backend.discardRecovery();
    }
}

// Red before the fix: Code Block split the line at the caret and Body/Code Block refused or
// kept the fences inside a fenced block.
void FomawriteTest::codeBlockFencesWholeParagraphsAndUnfences() {
    Cycle155Window w;
    QVERIFY2(w.create(QStringLiteral("x\n"), false, false), qPrintable(w.error));
    const auto run = [&](const QString &text, int from, int to, const char *action) {
        w.editor->setProperty("text", text);
        w.backend.editMarkdown(QString::fromLatin1(action), from, to);
        return w.canonical();
    };
    // Caret in the middle of a word: the whole line is fenced, nothing is split.
    QCOMPARE(run("alpha\n", 2, 2, "codeBlock"), QStringLiteral("```\nalpha\n```\n"));
    QCOMPARE(run("alpha", 2, 2, "codeBlock"), QStringLiteral("```\nalpha\n```\n"));
    // Partial selection inside a line; paragraph of several lines; neighbours stay.
    QCOMPARE(run("one\ntwo\nthree\n\nfour\n", 5, 6, "codeBlock"), QStringLiteral("```\none\ntwo\nthree\n```\n\nfour\n"));
    QCOMPARE(run("intro\n\none\ntwo\n\nlast\n", 8, 12, "codeBlock"), QStringLiteral("intro\n\n```\none\ntwo\n```\n\nlast\n"));
    // A selection across two paragraphs fences both lines' paragraphs as one block.
    QCOMPARE(run("one\n\ntwo\n", 1, 7, "codeBlock"), QStringLiteral("```\none\n\ntwo\n```\n"));
    // Backticks in the content lengthen the fence.
    QCOMPARE(run("a ``` b\n", 1, 1, "codeBlock"), QStringLiteral("````\na ``` b\n````\n"));
    // Unfence from the Style menu commands, caret anywhere in the block (fences included).
    const QString fenced = QStringLiteral("before\n\n```js\nlet a;\nlet b;\n```\n\nafter\n");
    for (const int pos : {fenced.indexOf("let a") + 2, fenced.indexOf("```js") + 1, fenced.lastIndexOf("```") + 1})
        for (const char *action : {"codeBlock", "body"})
            QCOMPARE(run(fenced, pos, pos, action), QStringLiteral("before\n\nlet a;\nlet b;\n\nafter\n"));
    // Unclosed fence and an empty block.
    QCOMPARE(run("```\nlet a;\n", 5, 5, "body"), QStringLiteral("let a;\n"));
    QCOMPARE(run("```\n```\n", 1, 1, "codeBlock"), QStringLiteral("\n"));
    // Other block commands still refuse inside fences.
    QCOMPARE(run(fenced, fenced.indexOf("let a"), fenced.indexOf("let a"), "quote"), fenced);
    // The label follows the text, and one undo restores each step.
    w.editor->setProperty("text", QStringLiteral("alpha\n\nbeta\n"));
    w.backend.discardRecovery();
    w.backend.editMarkdown("codeBlock", 2, 2);
    QCOMPARE(w.backend.blockStyleAt(5), QStringLiteral("Code Block"));
    QCOMPARE(w.backend.blockStyleAt(w.canonical().indexOf("beta")), QStringLiteral("Body"));
    w.backend.editMarkdown("body", 5, 5);
    QCOMPARE(w.canonical(), QStringLiteral("alpha\n\nbeta\n"));
    QCOMPARE(w.backend.blockStyleAt(2), QStringLiteral("Body"));
    w.backend.undoSource(0);
    QCOMPARE(w.canonical(), QStringLiteral("```\nalpha\n```\n\nbeta\n"));
    w.backend.undoSource(0);
    QCOMPARE(w.canonical(), QStringLiteral("alpha\n\nbeta\n"));
    w.backend.discardRecovery();
}

// The Style menu with real clicks, in Source and Live.
void FomawriteTest::styleMenuCodeBlockAndBodyWorkWithRealClicksInSourceAndLive() {
    for (const bool live : {false, true}) {
        Cycle155Window w;
        QVERIFY2(w.create(QStringLiteral("alpha\n\nbeta\n"), live, true), qPrintable(w.error));
        const char *where = live ? "live" : "source";
        const auto choose = [&](const char *label) {
            if (!w.click("formatStyleButton")) return false;
            auto *menu = w.window->findChild<QObject *>("formatStyleMenu");
            if (!menu || !QTest::qWaitFor([&] { return menu->property("opened").toBool(); }, 3000)) return false;
            auto *item = cycle108MenuItem(menu, label);
            return item && cycle102Click(w.quick, item);
        };
        QVERIFY(live ? w.selectInPage(2, 2) : w.selectInSource(2, 2));
        QVERIFY2(choose("Code Block"), where);
        QTRY_COMPARE_WITH_TIMEOUT(w.canonical(), QStringLiteral("```\nalpha\n```\n\nbeta\n"), 8000);
        auto *styleButton = w.item("formatStyleButton");
        QTRY_COMPARE_WITH_TIMEOUT(styleButton->property("text").toString(), QStringLiteral("Code Block"), 4000);
        if (live) QTRY_COMPARE_WITH_TIMEOUT(w.pageText(), w.canonical(), 8000);
        QVERIFY2(choose("Body"), where);
        QTRY_COMPARE_WITH_TIMEOUT(w.canonical(), QStringLiteral("alpha\n\nbeta\n"), 8000);
        QTRY_COMPARE_WITH_TIMEOUT(styleButton->property("text").toString(), QStringLiteral("Body"), 4000);
        w.backend.discardRecovery();
    }
}
