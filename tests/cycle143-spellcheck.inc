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
    const bool before = spell->enabled();
    QVariant checked;
    QVERIFY(QMetaObject::invokeMethod(commands, "isChecked", Q_RETURN_ARG(QVariant, checked), Q_ARG(QVariant, QStringLiteral("checkSpellingWhileTyping"))));
    QCOMPARE(checked.toBool(), before);
    QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("checkSpellingWhileTyping"))));
    QCOMPARE(spell->enabled(), !before);
    QVERIFY(QMetaObject::invokeMethod(commands, "isChecked", Q_RETURN_ARG(QVariant, checked), Q_ARG(QVariant, QStringLiteral("checkSpellingWhileTyping"))));
    QCOMPARE(checked.toBool(), !before);
    auto *item = f.window->findChild<QObject *>("editCheckSpellingWhileTyping"); QVERIFY(item);
    QCOMPARE(item->property("text").toString(), QStringLiteral("Check Spelling While Typing"));
    QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("checkSpellingWhileTyping"))));
    QCOMPARE(spell->enabled(), before);
}
