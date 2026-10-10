// Cycle145 — Harper behind both writing surfaces and the review pane.
//
// The engine (HarperEngine) is the host's half of a conversation with Harper
// running in the editor page. The unit tests below stand in for the page: a bare
// EditorBridge, with harperReady/harperReply/harperFailed called by the test
// (no page, no WebEngine). The window test at the end uses the real engine in the
// real page.
//
// Harper's answers are given by the tests, so nothing here depends on the macOS
// grammar pass; the real-engine test depends only on Harper.

#include "harperengine.h"

namespace {
// One Harper lint as the page sends it.
QJsonObject cycle145Lint(int start, int end, const QString &kind, const QString &message = QString(), const QStringList &suggestions = {}) {
    return QJsonObject{{"start", start}, {"end", end}, {"kind", kind}, {"message", message}, {"suggestions", QJsonArray::fromStringList(suggestions)}};
}
QString cycle145Json(const QJsonArray &lints) { return QString::fromUtf8(QJsonDocument(lints).toJson(QJsonDocument::Compact)); }

// A host with a fake page: every harperLint the engine sends is recorded, and
// answer() replies to it the way the page would.
struct Cycle145Fixture {
    EditorBridge bridge;
    HarperEngine engine{&bridge};
    SpellCheck spell;
    QList<QPair<int, QString>> sent;
    QStringList loads, imports;
    Cycle145Fixture(bool readyAtOnce = true) {
        QObject::connect(&bridge, &EditorBridge::harperLint, &bridge, [this](int token, const QString &text) { sent.append({token, text}); });
        QObject::connect(&bridge, &EditorBridge::harperLoad, &bridge, [this](const QString &dialect) { loads << dialect; });
        QObject::connect(&bridge, &EditorBridge::harperImportWords, &bridge, [this](const QString &json) { imports << json; });
        spell.setHarper(&engine);
        if (readyAtOnce) { bridge.ready(); bridge.harperReady(QStringLiteral("2.10.0-test")); }
    }
    int tokenFor(const QString &text) const {
        for (int i = int(sent.size()) - 1; i >= 0; --i) if (sent.at(i).second == text) return sent.at(i).first;
        return -1;
    }
    bool answer(const QString &text, const QJsonArray &lints) {
        const int token = tokenFor(text);
        if (token < 0) return false;
        bridge.harperReply(token, cycle145Json(lints));
        return true;
    }
};
// The underline colour of the mark at a document position ("" when none).
QColor cycle145Underline(QTextDocument &document, int position) {
    const QTextCharFormat format = cycle143Format(document, position);
    return format.hasProperty(MarkdownHighlighter::SpellMarkProperty) && format.fontUnderline() ? format.underlineColor() : QColor();
}
}

// (a) Caching, dedupe, the kind mapping, failure, dialect and words.
void FomawriteTest::harperEngineCachesDedupesAndMapsKinds() {
    EditorBridge bridge;
    HarperEngine engine(&bridge);
    QSignalSpy loads(&bridge, &EditorBridge::harperLoad), lints(&bridge, &EditorBridge::harperLint), words(&bridge, &EditorBridge::harperImportWords);
    QSignalSpy results(&engine, &HarperEngine::resultsReady), failures(&engine, &HarperEngine::failed), readiness(&engine, &HarperEngine::readyChanged);
    QVERIFY(!engine.ready());
    QVERIFY2(engine.issues(QStringLiteral("   \n")).has_value(), "blank text has nothing to find");
    QCOMPARE(lints.count(), 0);
    // Asked before the page has Harper: queued, nothing sent.
    const QString text = QStringLiteral("We recieve the the mail and it is very unique.");
    QVERIFY(!engine.issues(text).has_value());
    QCOMPARE(lints.count(), 0);
    QCOMPARE(engine.queuedCount(), 1);
    // The page connects: the engine asks it to load Harper in the (default) dialect.
    bridge.ready();
    QCOMPARE(loads.count(), 1);
    QCOMPARE(loads.first().first().toString(), QStringLiteral("Australian"));
    QVERIFY(!engine.ready());
    bridge.harperReady(QStringLiteral("2.10.0-test"));
    QVERIFY(engine.ready());
    QCOMPARE(engine.version(), QStringLiteral("2.10.0-test"));
    QCOMPARE(readiness.count(), 1);
    // The queued text goes out now, once, however often it is asked for.
    QCOMPARE(lints.count(), 1);
    QCOMPARE(lints.first().at(1).toString(), text);
    QVERIFY(!engine.issues(text).has_value());
    QVERIFY(!engine.issues(text).has_value());
    QCOMPARE(lints.count(), 1);
    QCOMPARE(engine.inFlightCount(), 1);
    // The answer: spans past the text or empty are dropped, kinds become categories, kind is kept.
    const int recieve = text.indexOf(QStringLiteral("recieve")), doubled = text.indexOf(QStringLiteral("the the")), unique = text.indexOf(QStringLiteral("very unique"));
    bridge.harperReply(lints.first().first().toInt(), cycle145Json({
        cycle145Lint(unique, unique + 11, QStringLiteral("Redundancy"), QStringLiteral("Wordy"), {QStringLiteral("unique")}),
        cycle145Lint(recieve, recieve + 7, QStringLiteral("Spelling"), QStringLiteral("Did you mean"), {QStringLiteral("receive")}),
        cycle145Lint(doubled, doubled + 7, QStringLiteral("Repetition"), QStringLiteral("Repeated word"), {QStringLiteral("the")}),
        cycle145Lint(0, 9999, QStringLiteral("Grammar")), cycle145Lint(5, 5, QStringLiteral("Grammar"))}));
    QCOMPARE(results.count(), 1);
    QCOMPARE(results.first().first().toString(), text);
    QCOMPARE(engine.inFlightCount(), 0);
    const auto cached = engine.issues(text);
    QVERIFY(cached.has_value());
    QCOMPARE(lints.count(), 1); // a cache hit asks nothing
    QCOMPARE(cached->size(), 3);
    QCOMPARE(cached->at(0).start, recieve); // document order
    QCOMPARE(cached->at(0).category, QStringLiteral("Spelling"));
    QCOMPARE(cached->at(0).kind, QStringLiteral("Spelling"));
    QCOMPARE(cached->at(0).suggestions, QStringList{QStringLiteral("receive")});
    QCOMPARE(cached->at(1).category, QStringLiteral("Grammar"));
    QCOMPARE(cached->at(1).kind, QStringLiteral("Repetition"));
    QCOMPARE(cached->at(1).message, QStringLiteral("Repeated word"));
    QCOMPARE(cached->at(2).category, QStringLiteral("Style"));
    QCOMPARE(cached->at(2).kind, QStringLiteral("Redundancy"));
    QCOMPARE(engine.suggestionsFor(QStringLiteral("recieve")), QStringList{QStringLiteral("receive")});
    // Every kind Harper has, and one it might add.
    const QStringList grammar{"Agreement", "BoundaryError", "Capitalization", "Eggcorn", "Grammar", "Malapropism", "Nonstandard", "Punctuation",
                              "Repetition", "Typo", "Usage", "WordOrder"};
    const QStringList style{"Enhancement", "Formatting", "Miscellaneous", "Readability", "Redundancy", "Regionalism", "Style", "WordChoice", "SomeFutureKind"};
    QCOMPARE(HarperEngine::categoryForKind(QStringLiteral("Spelling")), QStringLiteral("Spelling"));
    for (const QString &kind : grammar) QCOMPARE(HarperEngine::categoryForKind(kind), QStringLiteral("Grammar"));
    for (const QString &kind : style) QCOMPARE(HarperEngine::categoryForKind(kind), QStringLiteral("Style"));
    // A failure drops the pending entry (and reports); the text is asked for again next time.
    QVERIFY(!engine.issues(QStringLiteral("Other text here.")).has_value());
    QCOMPARE(lints.count(), 2);
    const int failedToken = lints.last().first().toInt();
    bridge.harperFailed(failedToken, QStringLiteral("page said no"));
    QCOMPARE(failures.count(), 1);
    QCOMPARE(failures.first().first().toString(), QStringLiteral("page said no"));
    QCOMPARE(engine.inFlightCount(), 0);
    QCOMPARE(results.count(), 1);
    QVERIFY(!engine.issues(QStringLiteral("Other text here.")).has_value());
    QCOMPARE(lints.count(), 3);
    bridge.harperReply(failedToken, QStringLiteral("[]")); // a late answer to the dropped question
    QCOMPARE(results.count(), 1);
    // A dialect change drops every answer, and a late reply from the old dialect is ignored.
    const int oldToken = lints.last().first().toInt();
    engine.setDialect(QStringLiteral("American"));
    QCOMPARE(loads.count(), 2);
    QCOMPARE(loads.last().first().toString(), QStringLiteral("American"));
    QCOMPARE(engine.cachedCount(), 0);
    QCOMPARE(engine.inFlightCount(), 0);
    bridge.harperReply(oldToken, QStringLiteral("[]"));
    QCOMPARE(results.count(), 1);
    QVERIFY(!engine.issues(text).has_value());
    // Learned words go to the page whole, and again whenever the engine (re)loads.
    QCOMPARE(words.count(), 0);
    engine.importWords({QStringLiteral("zxqv"), QStringLiteral("plimble")});
    QCOMPARE(words.count(), 1);
    QCOMPARE(words.last().first().toString(), QStringLiteral("[\"zxqv\",\"plimble\"]"));
    QCOMPARE(engine.cachedCount(), 0);
    bridge.harperReady(QStringLiteral("2.10.0-test"));
    QCOMPARE(words.count(), 2);
    QCOMPARE(readiness.count(), 1); // same engine: not a change of readiness
    bridge.ready(); // a new page: the engine is not ready until it says so
    QVERIFY(!engine.ready());
    QCOMPARE(readiness.count(), 2);
    QCOMPARE(loads.last().first().toString(), QStringLiteral("American"));
    bridge.harperReady(QStringLiteral("2.11.0-test"));
    QVERIFY(engine.ready());
    QCOMPARE(engine.version(), QStringLiteral("2.11.0-test"));
    QCOMPARE(words.count(), 3);
    // A dialect chosen before any page exists is what the page is asked to load.
    EditorBridge later; HarperEngine laterEngine(&later);
    QSignalSpy laterLoads(&later, &EditorBridge::harperLoad);
    laterEngine.setDialect(QStringLiteral("British"));
    QCOMPARE(laterLoads.count(), 0);
    later.ready();
    QCOMPARE(laterLoads.count(), 1);
    QCOMPARE(laterLoads.first().first().toString(), QStringLiteral("British"));
}

// (b) A burst of texts: at most 64 wait, the newest go first, nothing is lost,
// and the cache stays bounded.
void FomawriteTest::harperEngineQueuesNewestFirstAndBoundsItsCache() {
    EditorBridge bridge;
    HarperEngine engine(&bridge);
    QList<QPair<int, QString>> sent;
    QObject::connect(&bridge, &EditorBridge::harperLint, &bridge, [&](int token, const QString &text) { sent.append({token, text}); });
    for (int i = 0; i < 100; ++i) QVERIFY(!engine.issues(QStringLiteral("Paragraph number %1.").arg(i)).has_value());
    QCOMPARE(engine.queuedCount(), HarperEngine::QueueLimit);
    QCOMPARE(engine.overflowCount(), 100 - HarperEngine::QueueLimit);
    bridge.ready(); bridge.harperReady(QStringLiteral("test"));
    QCOMPARE(engine.inFlightCount(), HarperEngine::InFlightLimit);
    QCOMPARE(sent.first().second, QStringLiteral("Paragraph number 99.")); // newest first
    // The page answers everything it is given; the engine keeps feeding it until all 100 are known.
    QSet<QString> answered;
    for (int i = 0; i < sent.size(); ++i) {
        bridge.harperReply(sent.at(i).first, QStringLiteral("[]"));
        answered.insert(sent.at(i).second);
    }
    QCOMPARE(answered.size(), 100);
    QCOMPARE(engine.cachedCount(), 100);
    QCOMPARE(engine.inFlightCount(), 0);
    QCOMPARE(engine.queuedCount() + engine.overflowCount(), 0);
    for (int i = 0; i < 100; ++i) QVERIFY(engine.issues(QStringLiteral("Paragraph number %1.").arg(i)).has_value());
    // The cache never grows past its limit.
    for (int i = 0; i < HarperEngine::CacheLimit + 50; ++i) {
        const QString text = QStringLiteral("Distinct text %1.").arg(i);
        engine.issues(text);
        bridge.harperReply(sent.last().first, QStringLiteral("[]"));
    }
    QVERIFY(engine.cachedCount() <= HarperEngine::CacheLimit);
    QVERIFY(engine.cachedCount() > 0);
}

// (c) The service's settings persist, and Harper only answers once it is ready.
void FomawriteTest::spellCheckEngineDialectAndStyleSettingsPersistAndFallBackUntilHarperIsReady() {
    Cycle103SettingsSnapshot settings;
    QSettings().setValue(QStringLiteral("writing/learnedWords"), QStringList{QStringLiteral("zxqvlearned")});
    {
        Cycle145Fixture f(false);
        // Defaults.
        QCOMPARE(f.spell.engine(), QStringLiteral("harper"));
        QCOMPARE(f.spell.dialect(), QStringLiteral("Australian"));
        QVERIFY(!f.spell.styleEnabled());
        QCOMPARE(f.spell.dialects(), (QStringList{"Australian", "British", "American", "Canadian"}));
        // Until Harper says it is ready, the macOS checker answers.
        QVERIFY(!f.spell.harperReady());
        QCOMPARE(f.spell.effectiveEngine(), QStringLiteral("macos"));
        QCOMPARE(f.spell.harperVersion(), QString());
        QSignalSpy engineChanged(&f.spell, &SpellCheck::engineChanged), readyChanged(&f.spell, &SpellCheck::harperReadyChanged);
        QSignalSpy resultsForwarded(&f.spell, &SpellCheck::resultsReady);
        f.bridge.ready();
        QCOMPARE(f.loads, QStringList{QStringLiteral("Australian")});
        f.bridge.harperReady(QStringLiteral("2.10.0-test"));
        QVERIFY(f.spell.harperReady());
        QCOMPARE(f.spell.harperVersion(), QStringLiteral("2.10.0-test"));
        QCOMPARE(f.spell.effectiveEngine(), QStringLiteral("harper"));
        QCOMPARE(readyChanged.count(), 1);
        QVERIFY(engineChanged.count() >= 1);
        // The stored learned words were handed to Harper when it came up.
        QCOMPARE(f.imports, QStringList{QStringLiteral("[\"zxqvlearned\"]")});
        // The setting can say macOS: Harper is ready but not used.
        f.spell.setEngine(QStringLiteral("macos"));
        QCOMPARE(f.spell.effectiveEngine(), QStringLiteral("macos"));
        QCOMPARE(QSettings().value("writing/engine").toString(), QStringLiteral("macos"));
        f.spell.setEngine(QStringLiteral("nonsense")); // ignored
        QCOMPARE(f.spell.engine(), QStringLiteral("macos"));
        f.spell.setEngine(QStringLiteral("harper"));
        QCOMPARE(f.spell.effectiveEngine(), QStringLiteral("harper"));
        // Dialect: validated, stored, passed to the engine.
        QSignalSpy dialectChanged(&f.spell, &SpellCheck::dialectChanged);
        f.spell.setDialect(QStringLiteral("Klingon")); // ignored
        QCOMPARE(dialectChanged.count(), 0);
        f.spell.setDialect(QStringLiteral("American"));
        QCOMPARE(dialectChanged.count(), 1);
        QCOMPARE(QSettings().value("writing/dialect").toString(), QStringLiteral("American"));
        QCOMPARE(f.engine.dialect(), QStringLiteral("American"));
        QCOMPARE(f.loads.last(), QStringLiteral("American"));
        // Style.
        QSignalSpy styleChanged(&f.spell, &SpellCheck::styleEnabledChanged);
        f.spell.setStyleEnabled(true);
        QCOMPARE(styleChanged.count(), 1);
        QCOMPARE(QSettings().value("writing/checkStyleWhileTyping").toBool(), true);
        f.spell.setStyleEnabled(false);
        // issues(): empty while pending, then Harper's, with the three categories and the kind;
        // Style is always reported (the surfaces decide whether to draw it).
        const QString text = QStringLiteral("The colour is very unique.");
        f.spell.setEnabled(true); f.spell.setGrammarEnabled(true);
        QVERIFY(!f.spell.issuesIfKnown(text).has_value());
        QVERIFY(f.spell.issues(text).isEmpty());
        QVERIFY(f.answer(text, {cycle145Lint(4, 10, "Spelling", "", {"color"}), cycle145Lint(14, 25, "Redundancy", "Wordy"), cycle145Lint(11, 13, "Agreement", "Bad")}));
        QCOMPARE(resultsForwarded.count(), 1);
        QCOMPARE(resultsForwarded.first().first().toString(), text);
        auto issues = f.spell.issues(text);
        QCOMPARE(issues.size(), 3);
        QStringList categories; for (const auto &issue : issues) categories << issue.category;
        QCOMPARE(categories, (QStringList{"Spelling", "Grammar", "Style"}));
        QCOMPARE(issues.at(1).kind, QStringLiteral("Agreement"));
        // Grammar off: Grammar findings go, the others stay (no new lint needed).
        f.spell.setGrammarEnabled(false);
        categories.clear(); for (const auto &issue : f.spell.issues(text)) categories << issue.category;
        QCOMPARE(categories, (QStringList{"Spelling", "Style"}));
        f.spell.setGrammarEnabled(true);
        // Ignore: one list. A grammar or style finding by its text and message, a word by the word.
        f.spell.ignoreGrammar(QStringLiteral("very unique"), QStringLiteral("Wordy"));
        categories.clear(); for (const auto &issue : f.spell.issues(text)) categories << issue.category;
        QCOMPARE(categories, (QStringList{"Spelling", "Grammar"}));
        f.spell.setEnabled(false);
        QVERIFY(f.spell.issues(text).isEmpty()); // the master switch
        f.spell.setEnabled(true);
        // Dialect change drops what Harper said (the old dialect's answers are wrong now).
        f.spell.setDialect(QStringLiteral("Australian"));
        QVERIFY(!f.spell.issuesIfKnown(text).has_value());
    }
    // The settings are read by the next service.
    {
        SpellCheck next;
        QCOMPARE(next.engine(), QStringLiteral("harper"));
        QCOMPARE(next.dialect(), QStringLiteral("Australian"));
        QSettings().setValue(QStringLiteral("writing/engine"), QStringLiteral("macos"));
        QSettings().setValue(QStringLiteral("writing/dialect"), QStringLiteral("Canadian"));
        QSettings().setValue(QStringLiteral("writing/checkStyleWhileTyping"), true);
        SpellCheck reread;
        QCOMPARE(reread.engine(), QStringLiteral("macos"));
        QCOMPARE(reread.dialect(), QStringLiteral("Canadian"));
        QVERIFY(reread.styleEnabled());
        QSettings().setValue(QStringLiteral("writing/dialect"), QStringLiteral("Martian"));
        QCOMPARE(SpellCheck().dialect(), QStringLiteral("Australian")); // a stored value that is not a dialect
    }
}

// (d) The Source layer: a pending answer keeps the old marks; the reply restyles
// the right block and never touches QTextDocument::revision.
void FomawriteTest::sourceHarperPendingAnswerKeepsOldMarksAndReplyRestylesWithoutARevision() {
    Cycle103SettingsSnapshot settings;
    Cycle145Fixture f;
    f.spell.setEnabled(true); f.spell.setGrammarEnabled(true); f.spell.setStyleEnabled(false);
    QCOMPARE(f.spell.effectiveEngine(), QStringLiteral("harper"));
    const QString first = QStringLiteral("We recieve the the mail and it is very unique.");
    const QString second = QStringLiteral("A second line is fine.");
    const QString source = first + QStringLiteral("\n\n") + second;
    QTextDocument document; document.setPlainText(source);
    MarkdownHighlighter highlighter(&document);
    highlighter.setDarkMode(false);
    highlighter.setSpellCheck(&f.spell); QTest::qWait(20); highlighter.rehighlight();
    const int recieve = first.indexOf(QStringLiteral("recieve")), doubled = first.indexOf(QStringLiteral("the the")), unique = first.indexOf(QStringLiteral("very unique"));
    const QJsonArray firstLints{cycle145Lint(recieve, recieve + 7, "Spelling", "", {"receive"}), cycle145Lint(doubled, doubled + 7, "Repetition", "Repeated word", {"the"}),
                                cycle145Lint(unique, unique + 11, "Redundancy", "Wordy", {"unique"})};
    // Asked for, not answered: nothing is drawn yet, and both blocks are waiting.
    QVERIFY(f.tokenFor(first) > 0 && f.tokenFor(second) > 0);
    QVERIFY(!cycle143Underlined(document, recieve));
    // The reply restyles the block that was waiting (and only that one), through the layout.
    int revision = document.revision();
    QVERIFY(f.answer(first, firstLints));
    QVERIFY(cycle143Underlined(document, recieve));
    QCOMPARE(document.revision(), revision);
    const QTextCharFormat blue = cycle143Format(document, doubled);
    QCOMPARE(blue.underlineColor(), QColor("#1a73e8"));
    QCOMPARE(blue.intProperty(MarkdownHighlighter::SpellCategoryProperty), 2);
    QVERIFY(!cycle145Underline(document, unique).isValid()); // style is off: pane only
    QVERIFY(!cycle145Underline(document, source.indexOf(QStringLiteral("second"))).isValid());
    QVERIFY(f.answer(second, {}));
    QCOMPARE(document.revision(), revision);
    // The pane's list has all three categories (style regardless of the underline), with Harper's kind.
    const auto issues = highlighter.issuesInBlock(document.firstBlock());
    QStringList seen; for (const auto &issue : issues) seen << issue.category + ":" + issue.kind;
    QCOMPARE(seen, (QStringList{"Spelling:Spelling", "Grammar:Repetition", "Style:Redundancy"}));
    QCOMPARE(issues.at(1).message, QStringLiteral("Repeated word"));
    QCOMPARE(issues.at(0).suggestions, QStringList{QStringLiteral("receive")});
    // Style goes on: grey underline and wash (light), category 3; the dark wash is swapped in.
    f.spell.setStyleEnabled(true);
    const QTextCharFormat grey = cycle143Format(document, unique);
    QVERIFY(grey.fontUnderline());
    QCOMPARE(grey.underlineColor(), QColor("#8a8f98"));
    QCOMPARE(grey.background().color(), QColor("#ebedf0"));
    QCOMPARE(grey.intProperty(MarkdownHighlighter::SpellCategoryProperty), 3);
    highlighter.setDarkMode(true);
    QCOMPARE(cycle143Format(document, unique).background().color(), QColor("#2e3238"));
    highlighter.setDarkMode(false);
    f.spell.setStyleEnabled(false);
    QVERIFY(!cycle145Underline(document, unique).isValid());
    f.spell.setStyleEnabled(true);
    // Typing in the block: the new text is not answered yet, and the old marks stay where
    // they belong (mapped through the edit) instead of flashing off.
    QTextCursor cursor(&document);
    cursor.setPosition(0); cursor.insertText(QStringLiteral("So "));
    QVERIFY(f.tokenFor(QStringLiteral("So ") + first) > 0);
    QVERIFY2(cycle143Underlined(document, recieve + 3), "the red mark moved with the text and stayed drawn");
    QVERIFY(!cycle143Underlined(document, recieve));
    QCOMPARE(cycle143Format(document, doubled + 3).underlineColor(), QColor("#1a73e8"));
    QCOMPARE(cycle143Format(document, unique + 3).underlineColor(), QColor("#8a8f98"));
    // An edit inside a marked word drops that mark (the rest stay).
    cursor.setPosition(recieve + 3 + 3); cursor.insertText(QStringLiteral("x"));
    QVERIFY(!cycle143Underlined(document, recieve + 3));
    QCOMPARE(cycle143Format(document, doubled + 4).underlineColor(), QColor("#1a73e8"));
    cursor.deletePreviousChar(); // back to "So " + first
    // The answer for the edited text arrives: restyled in place, revision untouched.
    const QString edited = QStringLiteral("So ") + first;
    revision = document.revision();
    QVERIFY(f.answer(edited, {cycle145Lint(recieve + 3, recieve + 10, "Spelling", "", {"receive"})}));
    QVERIFY(cycle143Underlined(document, recieve + 3));
    QVERIFY2(!cycle145Underline(document, doubled + 3).isValid() && !cycle145Underline(document, unique + 3).isValid(), "the new answer replaced the carried marks");
    QCOMPARE(document.revision(), revision);
    // An answer that finds nothing clears the mark, again without a revision.
    cursor.setPosition(document.firstBlock().length() - 1); cursor.insertText(QStringLiteral("!"));
    QVERIFY(cycle143Underlined(document, recieve + 3)); // carried while pending
    revision = document.revision();
    QVERIFY(f.answer(edited + QStringLiteral("!"), {}));
    QVERIFY(!cycle143Underlined(document, recieve + 3));
    QCOMPARE(document.revision(), revision);
    // The caret rule still applies to what a reply draws: a word the caret ends is withheld.
    cursor.setPosition(document.firstBlock().length() - 1); cursor.insertText(QStringLiteral("?"));
    revision = document.revision();
    QVERIFY(f.answer(edited + QStringLiteral("!?"), {cycle145Lint(recieve + 3, recieve + 10, "Spelling", "", {"receive"})}));
    QVERIFY(cycle143Underlined(document, recieve + 3));
    highlighter.setCaret(recieve + 3 + 7);
    QVERIFY(!cycle143Underlined(document, recieve + 3));
    highlighter.setCaret(-1);
    QVERIFY(cycle143Underlined(document, recieve + 3));
    QCOMPARE(document.revision(), revision);
}

// (e) Block numbers move when lines are added above; a reply still finds its block.
void FomawriteTest::sourceHarperReplyFindsItsBlockAfterLinesAreInsertedAbove() {
    Cycle103SettingsSnapshot settings;
    Cycle145Fixture f;
    f.spell.setEnabled(true); f.spell.setGrammarEnabled(true);
    const QString line = QStringLiteral("We like the colour red.");
    QTextDocument document; document.setPlainText(QStringLiteral("Heading line.\n\n") + line + QStringLiteral("\n\nTail line."));
    MarkdownHighlighter highlighter(&document);
    highlighter.setSpellCheck(&f.spell); QTest::qWait(20); highlighter.rehighlight();
    QVERIFY(f.answer(line, {}));
    // American: the same text is asked again and is not answered yet.
    f.spell.setDialect(QStringLiteral("American"));
    QVERIFY(f.tokenFor(line) > 0);
    QTextCursor cursor(&document); cursor.setPosition(0); cursor.insertText(QStringLiteral("One\nTwo\nThree\n"));
    const int colour = document.toPlainText().indexOf(QStringLiteral("colour"));
    QVERIFY(!cycle143Underlined(document, colour));
    const int revision = document.revision();
    QVERIFY(f.answer(line, {cycle145Lint(12, 18, "Spelling", "", {"color"})}));
    QVERIFY2(cycle143Underlined(document, colour), "the block moved down three lines; the reply found it");
    QCOMPARE(document.revision(), revision);
}

// (f) The Live page's own underlines: with Harper the host waits for the answers,
// replies once with every segment, and gives up after two seconds.
void FomawriteTest::liveSpellingRequestWaitsForHarperAndRepliesOnce() {
    Cycle103SettingsSnapshot settings;
    Backend backend;
    auto *bridge = backend.editorBridge();
    auto *spell = backend.spellCheck();
    spell->setEnabled(true); spell->setGrammarEnabled(true); spell->setStyleEnabled(false);
    QList<QPair<int, QString>> sent;
    QObject::connect(bridge, &EditorBridge::harperLint, bridge, [&](int token, const QString &text) { sent.append({token, text}); });
    QSignalSpy replies(bridge, &EditorBridge::spellingReply);
    bridge->ready(); bridge->harperReady(QStringLiteral("2.10.0-test"));
    QCOMPARE(spell->effectiveEngine(), QStringLiteral("harper"));
    const QString a = QStringLiteral("We recieve the the mail."), b = QStringLiteral("It is very unique.");
    const QString segments = cycle145Json({QJsonObject{{"from", 10}, {"to", 10 + a.size()}, {"text", a}}, QJsonObject{{"from", 100}, {"to", 100 + b.size()}, {"text", b}}});
    bridge->checkSpelling(7, segments);
    QCOMPARE(sent.size(), 2);
    QCOMPARE(replies.count(), 0);
    // One answer is not enough: the page is told when both are in.
    bridge->harperReply(sent.at(0).first, cycle145Json({cycle145Lint(3, 10, "Spelling", "", {"receive"}), cycle145Lint(11, 18, "Repetition", "Repeated word", {"the"})}));
    QCOMPARE(replies.count(), 0);
    bridge->harperReply(sent.at(1).first, cycle145Json({cycle145Lint(6, 17, "Redundancy", "Wordy", {"unique"})}));
    QCOMPARE(replies.count(), 1);
    QCOMPARE(replies.first().at(0).toInt(), 7);
    auto entries = QJsonDocument::fromJson(replies.first().at(1).toString().toUtf8()).array();
    // Document offsets; spelling stays small, grammar carries message and kind; style is withheld while style is off.
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.at(0).toObject().value("from").toInt(), 13);
    QCOMPARE(entries.at(0).toObject().value("category").toString(), QStringLiteral("Spelling"));
    QCOMPARE(entries.at(1).toObject().value("category").toString(), QStringLiteral("Grammar"));
    QCOMPARE(entries.at(1).toObject().value("kind").toString(), QStringLiteral("Repetition"));
    QCOMPARE(entries.at(1).toObject().value("message").toString(), QStringLiteral("Repeated word"));
    // Style on: the same texts (now known) answer at once, and include the style finding. The page is told to check again.
    QSignalSpy checks(bridge, &EditorBridge::setSpellCheck);
    spell->setStyleEnabled(true);
    QVERIFY(checks.count() >= 1);
    bridge->checkSpelling(8, segments);
    QCOMPARE(replies.count(), 2);
    entries = QJsonDocument::fromJson(replies.last().at(1).toString().toUtf8()).array();
    QCOMPARE(entries.size(), 3);
    QCOMPARE(entries.at(2).toObject().value("category").toString(), QStringLiteral("Style"));
    QCOMPARE(entries.at(2).toObject().value("from").toInt(), 106);
    QCOMPARE(entries.at(2).toObject().value("kind").toString(), QStringLiteral("Redundancy"));
    // A text that never gets an answer: the reply comes after two seconds with what is known.
    const QString never = QStringLiteral("Nobody answers this one.");
    bridge->checkSpelling(9, cycle145Json({QJsonObject{{"from", 0}, {"to", never.size()}, {"text", never}}, QJsonObject{{"from", 50}, {"to", 50 + a.size()}, {"text", a}}}));
    QCOMPARE(replies.count(), 2);
    QTRY_COMPARE_WITH_TIMEOUT(replies.count(), 3, 4000);
    QCOMPARE(replies.last().at(0).toInt(), 9);
    entries = QJsonDocument::fromJson(replies.last().at(1).toString().toUtf8()).array();
    QCOMPARE(entries.size(), 2); // the known text's findings (a spelling and a grammar), none for the unanswered one
    QCOMPARE(entries.at(0).toObject().value("from").toInt(), 53);
    // When the late answer lands, the page is asked to check again.
    checks.clear();
    QVERIFY(sent.last().second == never);
    bridge->harperReply(sent.last().first, QStringLiteral("[]"));
    QVERIFY(checks.count() >= 1);
    backend.discardRecovery();
}

namespace {
// The Edit menu's Writing Checker commands through the shared command surface.
bool cycle145Checked(QObject *window, const char *command) {
    QVariant value;
    QMetaObject::invokeMethod(cycle144Commands(window), "isChecked", Q_RETURN_ARG(QVariant, value), Q_ARG(QVariant, QString::fromLatin1(command)));
    return value.toBool();
}
bool cycle145Enabled(QObject *window, const char *command) {
    QVariant value;
    QMetaObject::invokeMethod(cycle144Commands(window), "isEnabled", Q_RETURN_ARG(QVariant, value), Q_ARG(QVariant, QString::fromLatin1(command)));
    return value.toBool();
}
QString cycle145Label(QObject *window, const char *name) {
    auto *item = window->findChild<QQuickItem *>(QString::fromLatin1(name));
    return item ? item->property("text").toString() : QStringLiteral("<missing>");
}
QStringList cycle145RowTags(QObject *window) {
    QStringList tags;
    for (int i = 0; i < 12; ++i) {
        auto *row = cycle144Row(window, i);
        if (!row) break;
        const auto labels = cycle103VisualItems(row, QStringLiteral("reviewTag"));
        if (labels.isEmpty()) continue;
        auto *label = labels.first()->findChild<QQuickItem *>("reviewTagLabel");
        tags << (label ? label->property("text").toString() : QString());
    }
    return tags;
}
// The environment variable that defers the hidden Live page to first use is set for
// the whole suite (initTestCase); the tests that want the warm-up clear it.
struct Cycle145Prewarm {
    QByteArray previous; bool had;
    explicit Cycle145Prewarm(bool on) : previous(qgetenv("FOMAWRITE_NO_LIVE_PREWARM")), had(qEnvironmentVariableIsSet("FOMAWRITE_NO_LIVE_PREWARM")) {
        if (on) qunsetenv("FOMAWRITE_NO_LIVE_PREWARM"); else qputenv("FOMAWRITE_NO_LIVE_PREWARM", "1");
    }
    ~Cycle145Prewarm() { if (had) qputenv("FOMAWRITE_NO_LIVE_PREWARM", previous); else qunsetenv("FOMAWRITE_NO_LIVE_PREWARM"); }
};
}

// (g) The Edit menu's Writing Checker commands, and the pane: Style filter, kind
// tags, the engine status line and the dialect list (Harper answers are given
// by the test, as above; no page).
void FomawriteTest::writingCheckerCommandsMenuAndPaneFollowTheEngine() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1200), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setGrammarEnabled(true); spell->setStyleEnabled(false);
    QVERIFY(f.mode(0));
    QObject *window = f.window.data();
    // Menu: the submenu and its items exist (the two existing items kept their ids).
    for (const char *name : {"editWritingChecker", "editWritingEngine", "editWritingDialect", "editEngineHarper", "editEngineMacos", "editDialectAustralian",
                             "editDialectBritish", "editDialectAmerican", "editDialectCanadian", "editCheckSpellingWhileTyping",
                             "editCheckGrammarWhileTyping", "editCheckStyleWhileTyping"})
        QVERIFY2(window->findChild<QObject *>(QString::fromLatin1(name)), name);
    // Ticks follow the settings; the dialects are Harper's, so they dim with the macOS engine.
    QVERIFY(cycle145Checked(window, "engineHarper")); QVERIFY(!cycle145Checked(window, "engineMacos"));
    QVERIFY(cycle145Checked(window, "dialectAustralian"));
    QVERIFY(cycle144Run(window, "dialectBritish"));
    QCOMPARE(spell->dialect(), QStringLiteral("British"));
    QVERIFY(cycle145Checked(window, "dialectBritish") && !cycle145Checked(window, "dialectAustralian"));
    QVERIFY(window->findChild<QObject *>("editDialectBritish")->property("checked").toBool());
    QVERIFY(cycle145Enabled(window, "dialectAmerican") && cycle145Enabled(window, "checkStyleWhileTyping"));
    QVERIFY(cycle144Run(window, "engineMacos"));
    QCOMPARE(spell->engine(), QStringLiteral("macos"));
    QVERIFY(cycle145Checked(window, "engineMacos") && !cycle145Checked(window, "engineHarper"));
    QVERIFY(!cycle145Enabled(window, "dialectAmerican") && !cycle145Enabled(window, "checkStyleWhileTyping"));
    QVERIFY(cycle144Run(window, "engineHarper"));
    QCOMPARE(spell->engine(), QStringLiteral("harper"));
    QVERIFY(!cycle145Checked(window, "checkStyleWhileTyping"));
    QVERIFY(cycle144Run(window, "checkStyleWhileTyping"));
    QVERIFY(spell->styleEnabled() && cycle145Checked(window, "checkStyleWhileTyping"));
    QVERIFY(cycle144Run(window, "checkStyleWhileTyping"));
    QVERIFY(cycle144Run(window, "dialectAustralian"));
    // The pane with the macOS checker answering (Harper is not ready): status line and language list.
    QVERIFY(cycle144Run(window, "spelling"));
    QTRY_VERIFY(cycle144Control(window, "reviewFilterStyle"));
    QCOMPARE(cycle145Label(window, "reviewEngine"), QStringLiteral("macOS checker (Harper is loading)"));
    QCOMPARE(cycle144Control(window, "reviewLanguage")->property("text").toString(), QStringLiteral("System language"));
    // Harper comes up and answers: three categories, kinds as tags, Style filter, version in the header.
    auto *bridge = f.backend.editorBridge();
    QList<QPair<int, QString>> sent;
    QObject::connect(bridge, &EditorBridge::harperLint, bridge, [&](int token, const QString &text) { sent.append({token, text}); });
    bridge->ready(); bridge->harperReady(QStringLiteral("2.10.0-test"));
    QCOMPARE(spell->effectiveEngine(), QStringLiteral("harper"));
    const QString text = QStringLiteral("We recieve the the mail and it is very unique.\n");
    f.editor->setProperty("text", text);
    QTRY_VERIFY(!sent.isEmpty());
    const QString prose = text.chopped(1);
    int token = -1; for (const auto &entry : sent) if (entry.second == prose) token = entry.first;
    QVERIFY(token > 0);
    const int recieve = prose.indexOf(QStringLiteral("recieve")), doubled = prose.indexOf(QStringLiteral("the the")), unique = prose.indexOf(QStringLiteral("very unique"));
    bridge->harperReply(token, cycle145Json({cycle145Lint(recieve, recieve + 7, "Spelling", "Did you mean to spell `recieve` this way?", {"receive", "relieve"}),
        cycle145Lint(doubled, doubled + 7, "Repetition", "Did you mean to repeat this word?", {"the"}),
        cycle145Lint(unique, unique + 11, "WordChoice", "`Unique` is absolute.", {"unique"})}));
    QTRY_COMPARE(f.backend.reviewIssues().size(), 3);
    const QVariantList issues = f.backend.reviewIssues();
    QCOMPARE(issues.at(0).toMap()["category"].toString(), QStringLiteral("Spelling"));
    QCOMPARE(issues.at(1).toMap()["category"].toString(), QStringLiteral("Grammar"));
    QCOMPARE(issues.at(1).toMap()["kind"].toString(), QStringLiteral("Repetition"));
    QCOMPARE(issues.at(2).toMap()["category"].toString(), QStringLiteral("Style"));
    QCOMPARE(issues.at(2).toMap()["kind"].toString(), QStringLiteral("WordChoice"));
    QCOMPARE(issues.at(2).toMap()["word"].toString(), QStringLiteral("very unique"));
    QCOMPARE(cycle145Label(window, "reviewCounts"), QStringLiteral("1 spelling · 1 grammar · 1 style"));
    QCOMPARE(cycle145Label(window, "reviewEngine"), QStringLiteral("Harper 2.10.0-test (built-in)"));
    QCOMPARE(cycle144Control(window, "reviewLanguage")->property("text").toString(), QStringLiteral("Australian")); // the dialect, not a macOS language
    QTRY_COMPARE(cycle145RowTags(window), (QStringList{"Spelling", "Repetition", "Word Choice"}));
    // The Style filter shows the style row only; the underline setting does not matter to the pane.
    QVERIFY(!spell->styleEnabled());
    QVERIFY(cycle102Click(f.quick, cycle144Control(window, "reviewFilterStyle")));
    QTRY_COMPARE(cycle145RowTags(window), (QStringList{"Word Choice"}));
    QVERIFY(cycle102Click(f.quick, cycle144Control(window, "reviewFilterAll")));
    QTRY_COMPARE(cycle145RowTags(window).size(), 3);
    // Ignore on a style row goes to the one ignore list and the row goes.
    auto *styleRow = cycle144Row(window, 2); QVERIFY(styleRow);
    QVERIFY(cycle102Click(f.quick, cycle144RowButton(styleRow, QStringLiteral("reviewIgnore"))));
    QTRY_COMPARE(f.backend.reviewIssues().size(), 2);
    // Back to the macOS engine: the header says so and the language list returns.
    spell->setEngine(QStringLiteral("macos"));
    QTRY_COMPARE(cycle145Label(window, "reviewEngine"), QStringLiteral("macOS checker"));
    QCOMPARE(cycle144Control(window, "reviewLanguage")->property("text").toString(), QStringLiteral("System language"));
}

// (h) The real engine in the real page. The window starts in Source and never
// opens Live: the page exists because of the warm-up a second after the window
// shows, and Harper loads in it. Source gets its marks from Harper, the pane lists
// all three categories with Harper's kinds, a dialect change re-judges "colour",
// and the Live page draws the same findings.
void FomawriteTest::harperDrivesSourceTheLivePageAndThePaneInTheRealPage() {
    // The page half of Cycle 145 (the bridge's harperLoad / harperLint / harperImportWords) is another
    // builder's; until the bundle has it there is no engine to ask.
    QFile pageBundle(QStringLiteral(":/editor/dist/editor.js"));
    if (!pageBundle.open(QIODevice::ReadOnly) || !pageBundle.readAll().contains("harperLoad"))
        QSKIP("the page bundle has no Harper bridge yet (the page half of Cycle 145)");
    Cycle145Prewarm prewarm(true);
    Cycle103SettingsSnapshot settings;
    QElapsedTimer clock; clock.start();
    Cycle117Window f; QVERIFY2(f.start(1200), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setGrammarEnabled(true); spell->setStyleEnabled(false);
    QVERIFY(f.mode(0));
    QObject *window = f.window.data();
    QCOMPARE(spell->engine(), QStringLiteral("harper"));
    QTRY_VERIFY_WITH_TIMEOUT(spell->harperReady(), 60000);
    qInfo("harper: ready %lld ms after the window was created (Source only, Live never opened); version %s", clock.elapsed(), qPrintable(spell->harperVersion()));
    QVERIFY(!f.layout->property("liveEditEnabled").toBool());
    QCOMPARE(spell->effectiveEngine(), QStringLiteral("harper"));
    auto *quickDocument = f.editor->property("textDocument").value<QQuickTextDocument *>(); QVERIFY(quickDocument);
    QTextDocument &document = *quickDocument->textDocument();
    const QString sentence = QStringLiteral("We recieve the the mail and it is very unique.");
    f.editor->setProperty("text", sentence + QStringLiteral("\n"));
    const int recieve = sentence.indexOf(QStringLiteral("recieve")), doubled = sentence.indexOf(QStringLiteral("the the")), unique = sentence.indexOf(QStringLiteral("very unique"));
    QTRY_VERIFY_WITH_TIMEOUT(cycle143Underlined(document, recieve), 20000);
    QTRY_VERIFY_WITH_TIMEOUT(cycle144IsBlue(document, doubled), 20000);
    QVERIFY(cycle144IsBlue(document, doubled + 6));
    QVERIFY(!cycle145Underline(document, unique).isValid()); // style findings are in the pane, not underlined, until asked for
    // The pane lists all three, with Harper's kinds.
    QTRY_COMPARE_WITH_TIMEOUT(f.backend.reviewIssues().size(), 3, 10000);
    QStringList seen;
    for (const QVariant &entry : f.backend.reviewIssues()) { const auto issue = entry.toMap(); seen << issue["category"].toString() + ":" + issue["kind"].toString() + ":" + issue["word"].toString(); }
    QCOMPARE(seen, (QStringList{"Spelling:Spelling:recieve", "Grammar:Repetition:the the", "Style:WordChoice:very unique"}));
    QVERIFY(!f.backend.reviewIssues().at(1).toMap()["message"].toString().isEmpty());
    QVERIFY(f.backend.reviewIssues().at(0).toMap()["suggestions"].toStringList().contains(QStringLiteral("receive")));
    QVERIFY(cycle144Run(window, "spelling"));
    QTRY_COMPARE(cycle145RowTags(window), (QStringList{"Spelling", "Repetition", "Word Choice"}));
    QVERIFY(cycle145Label(window, "reviewEngine").startsWith(QStringLiteral("Harper ")));
    // Style goes on: grey under "very unique"; off: gone.
    spell->setStyleEnabled(true);
    QTRY_VERIFY(cycle145Underline(document, unique) == QColor("#8a8f98"));
    spell->setStyleEnabled(false);
    QTRY_VERIFY(!cycle145Underline(document, unique).isValid());
    spell->setStyleEnabled(true);
    // A dialect: "colour" is fine in Australian, a spelling finding in American, and fine again.
    const QString colour = QStringLiteral("We like the colour red.");
    f.editor->setProperty("text", sentence + QStringLiteral("\n\n") + colour + QStringLiteral("\n"));
    const int where = sentence.size() + 2 + colour.indexOf(QStringLiteral("colour"));
    QTRY_VERIFY_WITH_TIMEOUT(spell->issuesIfKnown(colour).has_value(), 20000);
    QVERIFY(spell->issues(colour).isEmpty());
    QVERIFY(!cycle143Underlined(document, where));
    spell->setDialect(QStringLiteral("American"));
    QTRY_VERIFY_WITH_TIMEOUT(cycle143Underlined(document, where), 20000);
    QCOMPARE(f.backend.issueAt(where)["category"].toString(), QStringLiteral("Spelling"));
    QVERIFY(f.backend.issueAt(where)["suggestions"].toStringList().contains(QStringLiteral("color")));
    spell->setDialect(QStringLiteral("Australian"));
    QTRY_VERIFY_WITH_TIMEOUT(!cycle143Underlined(document, where), 20000);
    QVERIFY(f.backend.issueAt(where).isEmpty());
    // Live: the same page now draws the same findings itself. The page asked the host (spellingRequested),
    // and the host answered once, with Style because style is on.
    QSignalSpy replies(f.backend.editorBridge(), &EditorBridge::spellingReply);
    f.editor->setProperty("text", sentence + QStringLiteral("\n"));
    QVERIFY(QMetaObject::invokeMethod(window, "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    auto *view = window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QTRY_VERIFY_WITH_TIMEOUT(f.layout->property("liveEditEnabled").toBool(), 5000);
    QTRY_COMPARE_WITH_TIMEOUT(cycle144PageText(f.backend.editorBridge()), sentence + QStringLiteral("\n"), 15000);
    QTRY_VERIFY_WITH_TIMEOUT(cycle143Underlined(view).contains(QStringLiteral("recieve")), 20000);
    QTRY_VERIFY_WITH_TIMEOUT(cycle144Texts(view, QStringLiteral(".fw-grammar")).join(QString()).contains(QStringLiteral("the the")), 20000);
    // What the host sent the page, whatever the page does with it: findings with Harper's kind, Style among them.
    const auto repliedWith = [&](const QString &needle) {
        for (const auto &args : replies) if (args.at(1).toString().contains(needle)) return true;
        return false;
    };
    QTRY_VERIFY_WITH_TIMEOUT(repliedWith(QStringLiteral("\"category\":\"Style\"")) && repliedWith(QStringLiteral("\"kind\":\"WordChoice\"")), 20000);
    // The page draws Style as .fw-style when its bundle knows the category (the page half of Cycle 145).
    QFile pageScript(QStringLiteral(":/editor/dist/editor.js"));
    const bool pageDrawsStyle = pageScript.open(QIODevice::ReadOnly) && pageScript.readAll().contains("fw-style");
    if (pageDrawsStyle) {
        QTRY_VERIFY_WITH_TIMEOUT(cycle144Texts(view, QStringLiteral(".fw-style")).join(QString()).contains(QStringLiteral("very unique")), 20000);
        // Style off: the page's grey underline goes, the red and blue stay.
        spell->setStyleEnabled(false);
        QTRY_VERIFY_WITH_TIMEOUT(cycle144Texts(view, QStringLiteral(".fw-style")).isEmpty(), 20000);
        QVERIFY(cycle143Underlined(view).contains(QStringLiteral("recieve")));
    } else {
        qWarning("the page bundle has no .fw-style yet: the Live style underline was not asserted");
    }
}

// (i) The warm-up: a second after the window shows the hidden Live page is created
// (and Harper loads in it); with FOMAWRITE_TEST_PREWARM=0 it is not, as before.
// Prints the numbers the cycle record quotes: time to first paint, the longest
// stall of the event loop around the page's creation, and the memory of the
// process tree. Run twice (with and without) to compare.
void FomawriteTest::livePageWarmUpDoesNotDelayFirstPaintAndIsMeasured() {
    const bool on = qgetenv("FOMAWRITE_TEST_PREWARM") != "0";
    Cycle145Prewarm prewarm(on);
    Cycle103SettingsSnapshot settings;
    QElapsedTimer clock; clock.start();
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    const qint64 created = clock.elapsed();
    QSignalSpy painted(quick, &QQuickWindow::frameSwapped);
    quick->resize(1200, 800); quick->show();
    QVERIFY(QTest::qWaitForWindowExposed(quick));
    QTRY_VERIFY(!painted.isEmpty());
    const qint64 firstPaint = clock.elapsed();
    // Event loop stalls: a 10 ms timer that notes how late it fires.
    qint64 worstStall = 0; QElapsedTimer tick; tick.start();
    QTimer probe; probe.setInterval(10);
    QObject::connect(&probe, &QTimer::timeout, [&] { worstStall = qMax(worstStall, tick.elapsed() - 10); tick.restart(); });
    probe.start();
    QTest::qWait(on ? 4000 : 2500);
    probe.stop();
    auto *view = window->findChild<QObject *>("liveEditorView");
    QCOMPARE(view != nullptr, on); // the page exists only because of the warm-up
    if (on) { QTRY_VERIFY_WITH_TIMEOUT(backend.editorBridge()->isReady(), 20000); }
    // Memory: resident set of this process and its descendants (the WebEngine renderer and GPU processes).
    QProcess ps; ps.start(QStringLiteral("ps"), {QStringLiteral("-axo"), QStringLiteral("pid=,ppid=,rss=,args=")});
    QVERIFY(ps.waitForFinished(5000));
    QHash<qint64, QList<qint64>> children; QHash<qint64, qint64> rss; QHash<qint64, QString> kind;
    for (const QByteArray &line : ps.readAllStandardOutput().split('\n')) {
        const auto parts = line.simplified().split(' ');
        if (parts.size() < 3) continue;
        children[parts.at(1).toLongLong()].append(parts.at(0).toLongLong()); rss[parts.at(0).toLongLong()] = parts.at(2).toLongLong();
        const int type = line.indexOf("--type=");
        kind[parts.at(0).toLongLong()] = type >= 0 ? QString::fromLatin1(line.mid(type + 7).split(' ').first()) : QStringLiteral("app");
    }
    qint64 total = 0; QList<qint64> stack{QCoreApplication::applicationPid()};
    QStringList breakdown;
    while (!stack.isEmpty()) { const qint64 pid = stack.takeLast(); total += rss.value(pid); breakdown << QStringLiteral("%1 %2 MB").arg(kind.value(pid)).arg(rss.value(pid) / 1024); stack += children.value(pid); }
    qInfo() << "processes:" << breakdown;
    qInfo("warm-up %s: window created in %lld ms, first paint %lld ms after that began (%lld ms after the window showed), worst event-loop stall %lld ms, resident %lld MB (process tree)",
          on ? "ON" : "OFF", created, firstPaint, firstPaint - created, worstStall, total / 1024);
    QVERIFY2(firstPaint < 15000, "the first paint is not held up by the page");
    backend.discardRecovery();
}
