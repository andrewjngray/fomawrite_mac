// Cycle143 — the shared spell checker: one macOS checker behind both writing
// surfaces, with a setting, a language, suggestions, and learned/ignored words.

void FomawriteTest::spellCheckServiceFindsWordsAndHonoursIgnoreAndSetting() {
    Cycle103SettingsSnapshot settings;
    SpellCheck check;
#ifndef Q_OS_MACOS
    QVERIFY(!check.available());
    QVERIFY(check.misspellings(QStringLiteral("tomorow")).isEmpty());
    return;
#endif
    QVERIFY(check.available());
    QVERIFY(check.enabled()); // default on
    check.setLanguage(QStringLiteral("en_US"));
    const QString text = QStringLiteral("Their going too the shop tomorow. Fine words here.");
    const auto ranges = check.misspellings(text);
    QVERIFY2(!ranges.isEmpty(), "the system checker flags tomorow");
    bool found = false;
    for (const auto &range : ranges) {
        QVERIFY(range.start >= 0 && range.end <= text.size() && range.end > range.start);
        if (text.mid(range.start, range.end - range.start) == QStringLiteral("tomorow")) found = true;
    }
    QVERIFY(found);
    // The QML form carries the same ranges.
    const QVariantList list = check.misspelledRanges(text);
    QCOMPARE(list.size(), ranges.size());
    QCOMPARE(list.first().toMap().value("start").toInt(), ranges.first().start);
    // Suggestions come best first and are bounded.
    const QStringList guesses = check.suggestions(QStringLiteral("tomorow"));
    QVERIFY(!guesses.isEmpty() && guesses.size() <= 8);
    QVERIFY(guesses.contains(QStringLiteral("tomorrow")));
    QVERIFY(check.suggestions(QString()).isEmpty());
    // Ignoring a word hides it for the run and tells the surfaces.
    QSignalSpy words(&check, &SpellCheck::wordsChanged);
    check.ignoreWord(QStringLiteral("tomorow"));
    QCOMPARE(words.count(), 1);
    bool stillFlagged = false;
    for (const auto &range : check.misspellings(text))
        if (text.mid(range.start, range.end - range.start) == QStringLiteral("tomorow")) stillFlagged = true;
    QVERIFY(!stillFlagged);
    // The setting switches checking off entirely and persists.
    QSignalSpy enabled(&check, &SpellCheck::enabledChanged);
    check.setEnabled(false);
    QCOMPARE(enabled.count(), 1);
    QVERIFY(check.misspellings(QStringLiteral("tomorow mispellled")).isEmpty());
    QCOMPARE(QSettings().value("writing/checkSpellingWhileTyping").toBool(), false);
    SpellCheck second; QVERIFY(!second.enabled());
    QCOMPARE(second.language(), QStringLiteral("en_US"));
    check.setEnabled(true);
    const QStringList languages = check.languages();
    QVERIFY2(!languages.isEmpty(), "the system offers at least one language");
    bool english = false; for (const QString &tag : languages) if (tag.startsWith(QStringLiteral("en"))) english = true;
    QVERIFY2(english, qPrintable(languages.join(", ")));
}

// The command and the native Edit item drive the same setting.
void FomawriteTest::checkSpellingWhileTypingCommandTogglesTheSetting() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *commands = f.window->findChild<QObject *>("workspaceCommands"); QVERIFY(commands);
    auto *spell = f.backend.spellCheck();
    QVERIFY(testDesktopMenusMatchPlatform(f.window.data()));
    if (!spell->nativeEngineAvailable()) {
        // This command/setting test needs a ready checker, independently of
        // the page's asynchronous bundled-WASM load (covered by bridge tests).
        f.backend.editorBridge()->ready();
        f.backend.editorBridge()->harperReady(QStringLiteral("2.10.0-test"));
    }
    QVERIFY(spell->available());
    const bool before = spell->enabled();
    QVariant checked;
    QVERIFY(QMetaObject::invokeMethod(commands, "isChecked", Q_RETURN_ARG(QVariant, checked), Q_ARG(QVariant, QStringLiteral("checkSpellingWhileTyping"))));
    QCOMPARE(checked.toBool(), before);
    QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("checkSpellingWhileTyping"))));
    QCOMPARE(spell->enabled(), !before);
    QVERIFY(QMetaObject::invokeMethod(commands, "isChecked", Q_RETURN_ARG(QVariant, checked), Q_ARG(QVariant, QStringLiteral("checkSpellingWhileTyping"))));
    QCOMPARE(checked.toBool(), !before);
    auto *item = f.window->property("isMac").toBool()
        ? f.window->findChild<QObject *>("editCheckSpellingWhileTyping")
        : testDesktopCommand(f.window.data(), QStringLiteral("checkSpellingWhileTyping"));
    QVERIFY(item && item->property("enabled").toBool());
    QCOMPARE(item->property("text").toString(), QStringLiteral("Check Spelling While Typing"));
    QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("checkSpellingWhileTyping"))));
    QCOMPARE(spell->enabled(), before);
}

// The macOS grammar pass is not present on every machine (the GitHub runner's
// image has none), so grammar tests skip where a doubled word is not flagged.
static bool cycle144GrammarAvailable(SpellCheck &check) {
    if (!check.available()) return false;
    const bool enabled = check.enabled(), grammar = check.grammarEnabled();
    check.setEnabled(true); check.setGrammarEnabled(true);
    const bool found = !check.grammarIssues(QStringLiteral("This is is a test.")).isEmpty();
    check.setEnabled(enabled); check.setGrammarEnabled(grammar);
    return found;
}
#define CYCLE144_NEEDS_GRAMMAR(check) if (!cycle144GrammarAvailable(check)) QSKIP("this macOS has no grammar pass for the test language")

namespace {
// Implemented by the Review fixture later in this translation unit. These rows
// load the shipped JS/WASM page; no synthetic ready or lint answers are used.
void cycle144ReviewCheckerRows();
bool cycle144StartBundledReviewChecker(Cycle117Window &fixture, QString *error);
struct Cycle144GrammarService {
    SpellCheck native;
    std::unique_ptr<Cycle117Window> page;
    SpellCheck *checker = nullptr;
    QString error;
    bool start(bool bundledHarper) {
        if (!bundledHarper) { native.setEngine(QStringLiteral("macos")); checker = &native; return true; }
        page = std::make_unique<Cycle117Window>();
        if (!page->start(1100)) { error = page->error; return false; }
        if (!cycle144StartBundledReviewChecker(*page, &error)) return false;
        checker = page->backend.spellCheck();
        checker->setStyleEnabled(false);
        return true;
    }
};
QList<SpellCheck::Issue> cycle144CheckerGrammar(SpellCheck &checker, const QString &text) {
    // grammarIssues() is the native grammar pass; Harper's asynchronous shared
    // API is issuesIfKnown()/issues(), with the same public category contract.
    if (!checker.usesHarper()) return checker.grammarIssues(text);
    QList<SpellCheck::Issue> grammar;
    for (const auto &issue : checker.issues(text))
        if (issue.category == QStringLiteral("Grammar")) grammar.append(issue);
    return grammar;
}
bool cycle144AwaitGrammar(SpellCheck &checker, MarkdownHighlighter &highlighter, QTextDocument &document) {
    if (!checker.usesHarper()) return true;
    // Ask through the actual highlighter so its markup-masked prose, rather
    // than the raw document, is what the real engine answers and caches.
    return QTest::qWaitFor([&] {
        for (QTextBlock block = document.firstBlock(); block.isValid(); block = block.next())
            highlighter.issuesInBlock(block);
        return checker.harper()->inFlightCount() == 0 && checker.harper()->queuedCount() == 0;
    }, 20000);
}
}

// Grammar findings come from the same service with a message and a category,
// follow their own setting, and issuesInBlock lists spelling and grammar in order.
void FomawriteTest::spellCheckServiceReportsGrammarIssuesWithCategory_data() { cycle144ReviewCheckerRows(); }

void FomawriteTest::spellCheckServiceReportsGrammarIssuesWithCategory() {
    QFETCH(bool, bundledHarper);
    Cycle103SettingsSnapshot settings;
    Cycle144GrammarService service; QVERIFY2(service.start(bundledHarper), qPrintable(service.error));
    SpellCheck &check = *service.checker;
    if (!bundledHarper && !check.available()) QSKIP("the system checker exists only on macOS");
    check.setEnabled(true); check.setGrammarEnabled(true); check.setLanguage(QStringLiteral("en_US"));
    if (!bundledHarper) { CYCLE144_NEEDS_GRAMMAR(check); }
    const QString text = QStringLiteral("This is is a test. We recieve the the mail here.");
    if (bundledHarper) { QTRY_VERIFY_WITH_TIMEOUT(check.issuesIfKnown(text).has_value(), 20000); }
    const auto grammar = cycle144CheckerGrammar(check, text);
    QVERIFY2(!grammar.isEmpty(), "the real checker flags a doubled word");
    for (const auto &issue : grammar) {
        QCOMPARE(issue.category, QStringLiteral("Grammar"));
        QVERIFY(issue.start >= 0 && issue.end <= text.size() && issue.end > issue.start);
    }
    const auto all = check.issues(text);
    QVERIFY(all.size() >= grammar.size() + 1);
    for (int i = 1; i < all.size(); ++i) QVERIFY(all.at(i).start >= all.at(i - 1).start);
    bool spelling = false; for (const auto &issue : all) if (issue.category == QStringLiteral("Spelling") && text.mid(issue.start, issue.end - issue.start) == QStringLiteral("recieve")) spelling = true;
    QVERIFY(spelling);
    QCOMPARE(check.issueList(text).size(), all.size());
    QSignalSpy changed(&check, &SpellCheck::grammarEnabledChanged);
    check.setGrammarEnabled(false);
    QCOMPARE(changed.count(), 1);
    QVERIFY(cycle144CheckerGrammar(check, text).isEmpty());
    QVERIFY(check.issues(text).size() < all.size());
    QCOMPARE(QSettings().value("writing/checkGrammarWhileTyping").toBool(), false);
    check.setGrammarEnabled(true);
    // The highlighter lists both per block, and nothing inside a fence.
    QTextDocument document; document.setPlainText(text + QStringLiteral("\n\n```\nthe the recieve\n```\n"));
    MarkdownHighlighter highlighter(&document);
    highlighter.setSpellCheck(&check); QTest::qWait(20); highlighter.rehighlight();
    QVERIFY(cycle144AwaitGrammar(check, highlighter, document));
    const auto first = highlighter.issuesInBlock(document.firstBlock());
    QCOMPARE(first.size(), all.size());
    QVERIFY(highlighter.issuesInBlock(document.findBlockByNumber(3)).isEmpty());
    check.setGrammarEnabled(false);
    QVERIFY(highlighter.issuesInBlock(document.firstBlock()).size() < first.size());
}

// Grammar runs in sentence-aligned chunks with a per-chunk cache: a long
// paragraph costs about one chunk per keystroke instead of a super-linear
// whole-paragraph pass (0.9 s at 8,700 characters before, reviewer finding).
void FomawriteTest::grammarChecksLongParagraphsInCachedChunks_data() { cycle144ReviewCheckerRows(); }

void FomawriteTest::grammarChecksLongParagraphsInCachedChunks() {
    QFETCH(bool, bundledHarper);
    Cycle103SettingsSnapshot settings;
    // Chunking is pure: sentence ends win, then line ends, then spaces, never longer than the limit.
    QString text;
    for (int i = 0; i < 120; ++i) text += QStringLiteral("Sentence number %1 says something ordinary about the weather today. ").arg(i);
    text += QStringLiteral("Finally the the end.");
    const auto chunks = SpellCheck::grammarChunks(text, 1000);
    QVERIFY(chunks.size() >= 8);
    int cursor = 0;
    for (const auto &chunk : chunks) {
        QCOMPARE(chunk.start, cursor);
        QVERIFY(chunk.end > chunk.start && chunk.end - chunk.start <= 1000);
        if (chunk.end < text.size()) QVERIFY2(text.at(chunk.end - 1).isSpace() && text.at(chunk.end - 2) == QLatin1Char('.'), "cut at a sentence end");
        cursor = chunk.end;
    }
    QCOMPARE(cursor, text.size());
    const auto noSentences = SpellCheck::grammarChunks(QString(2500, QLatin1Char('x')), 1000);
    QCOMPARE(noSentences.size(), 3);
    QCOMPARE(SpellCheck::grammarChunks(QString(), 1000).size(), 0);
    Cycle144GrammarService service; QVERIFY2(service.start(bundledHarper), qPrintable(service.error));
    SpellCheck &check = *service.checker;
    if (!bundledHarper) { CYCLE144_NEEDS_GRAMMAR(check); }
    check.setEnabled(true); check.setGrammarEnabled(true); check.setLanguage(QStringLiteral("en_US"));
    if (bundledHarper) {
        // Harper caches whole prose requests, rather than the native grammar
        // pass's chunks. Preserve long-text offsets and prove real cache reuse
        // by counting bridge requests, without imposing a native timing model.
        auto *bridge = service.page->backend.editorBridge();
        QSignalSpy requests(bridge, &EditorBridge::harperLint);
        const auto countFor = [&](const QString &prose) {
            int count = 0; for (const auto &request : requests) if (request.at(1).toString() == prose) ++count;
            return count;
        };
        QTRY_VERIFY_WITH_TIMEOUT(check.issuesIfKnown(text).has_value(), 20000);
        const auto cold = cycle144CheckerGrammar(check, text);
        bool found = false;
        for (const auto &issue : cold)
            if (text.mid(issue.start, issue.end - issue.start) == QStringLiteral("the the")) found = true;
        QVERIFY2(found, "the last grammar finding keeps its full long-paragraph offset");
        QCOMPARE(countFor(text), 1);
        const auto cached = cycle144CheckerGrammar(check, text);
        QCOMPARE(cached, cold);
        QTest::qWait(50);
        QCOMPARE(countFor(text), 1);
        const QString edited = text + QStringLiteral(" More.");
        QTRY_VERIFY_WITH_TIMEOUT(check.issuesIfKnown(edited).has_value(), 20000);
        const auto warm = cycle144CheckerGrammar(check, edited);
        QCOMPARE(warm, cold);
        QCOMPARE(countFor(edited), 1);
        QCOMPARE(countFor(text), 1);
        return;
    }
    QElapsedTimer timer; timer.start();
    const auto cold = check.grammarIssues(text);
    const qint64 coldMs = timer.elapsed();
    bool found = false;
    for (const auto &issue : cold) if (text.mid(issue.start, issue.end - issue.start) == QStringLiteral("the the")) found = true;
    QVERIFY2(found, "the doubled word in the last chunk is reported at its document offset");
    QVERIFY2(coldMs < 2000, qPrintable(QStringLiteral("cold pass took %1 ms").arg(coldMs)));
    // One edit near the end re-checks one chunk: the earlier chunks come from the cache.
    timer.restart();
    const auto warm = check.grammarIssues(text + QStringLiteral(" More."));
    const qint64 warmMs = timer.elapsed();
    qInfo("grammar chunks: cold %lld ms, after a one-chunk edit %lld ms", coldMs, warmMs);
    QCOMPARE(warm.size(), cold.size());
    QVERIFY2(warmMs * 2 <= coldMs + 20, "the cache makes the second pass markedly cheaper");
}
