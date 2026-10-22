// One registration per Windows chord; native Mac menu shortcuts remain intact.
#include <QKeySequence>
void FomawriteTest::windowsShortcutsUseExistingWindowCommands_data() {
    QTest::addColumn<QString>("shortcutName"); QTest::addColumn<QString>("sequence");
    QTest::addColumn<QString>("command"); QTest::addColumn<QString>("nativeName");
    QTest::addColumn<int>("key"); QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    const auto add = [](const char *name, const char *sequence, const char *command, const char *native, Qt::Key key, Qt::KeyboardModifiers modifiers) {
        QTest::newRow(name) << QString::fromLatin1(name) << QString::fromLatin1(sequence)
            << QString::fromLatin1(command) << QString::fromLatin1(native) << int(key) << modifiers;
    };
    add("windowsShortcutOpenPath", "Ctrl+Shift+O", "openPath", "fileOpenPath", Qt::Key_O, Qt::ControlModifier | Qt::ShiftModifier);
    add("windowsShortcutCommandPalette", "Ctrl+Shift+P", "palette", "goCommandPalette", Qt::Key_P, Qt::ControlModifier | Qt::ShiftModifier);
    add("windowsShortcutSpelling", "Ctrl+:", "spelling", "editSpelling", Qt::Key_Colon, Qt::ControlModifier);
    add("windowsShortcutNextIssue", "Ctrl+;", "nextIssue", "editNextIssue", Qt::Key_Semicolon, Qt::ControlModifier);
    add("windowsShortcutPreviousIssue", "Ctrl+Alt+;", "previousIssue", "editPreviousIssue", Qt::Key_Semicolon, Qt::ControlModifier | Qt::AltModifier);
}

void FomawriteTest::windowsShortcutsUseExistingWindowCommands() {
    QFETCH(QString, shortcutName); QFETCH(QString, sequence); QFETCH(QString, command);
    QFETCH(QString, nativeName); QFETCH(int, key); QFETCH(Qt::KeyboardModifiers, modifiers);
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString path = directory.filePath("shortcut.md");
    const QString text = QStringLiteral("A flomptoid of ideas.\n\nWe recieve the mail.\n\nAnd quorbleflat there.\n");
    QVERIFY(cycle103Write(path, text.toUtf8()));
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    QVERIFY(testDesktopMenusMatchPlatform(f.window.data()));
    auto *loader = f.window->findChild<QObject *>("windowsMenuLoader"); QVERIFY(loader);
    if (!f.window->property("isWindows").toBool()) loader->setProperty("active", true);
    QTRY_VERIFY(f.window->findChild<QObject *>("windowsMenuBar"));
    auto *bar = f.window->findChild<QObject *>("windowsMenuBar");
    auto *shortcut = bar->findChild<QObject *>(shortcutName);
    QVERIFY2(shortcut, qPrintable(QStringLiteral("Windows shortcut registration missing: %1").arg(shortcutName)));
    QCOMPARE(QKeySequence(shortcut->property("sequence").toString()), QKeySequence(sequence));
    QCOMPARE(shortcut->property("context").toInt(), int(Qt::WindowShortcut));
    int matching = 0;
    for (auto *candidate : f.window->findChildren<QObject *>())
        if (candidate->metaObject()->indexOfSignal("activated()") >= 0
            && QKeySequence(candidate->property("sequence").toString()) == QKeySequence(sequence)) ++matching;
    QCOMPARE(matching, 1);
    const bool windows = f.window->property("isWindows").toBool();
    if (!windows) {
        QVERIFY(!shortcut->property("enabled").toBool());
        QObject *native = nullptr;
        for (auto *candidate : f.window->findChildren<QObject *>(nativeName)) {
            QObject *owner = candidate;
            while (owner && owner != bar) owner = owner->parent();
            if (owner != bar) { QVERIFY(!native); native = candidate; }
        }
        QVERIFY(native); // Scope excludes the explicitly loaded Windows Go item.
        QCOMPARE(QKeySequence(native->property("shortcut").toString()), QKeySequence(sequence));
        return; // Real Windows key delivery below is required in native Windows CI.
    }
    const bool review = command == "spelling" || command == "nextIssue" || command == "previousIssue";
    if (review) {
        QString error; QVERIFY2(cycle144StartBundledReviewChecker(f, &error), qPrintable(error));
        f.backend.spellCheck()->setEnabled(true);
        f.backend.spellCheck()->setGrammarEnabled(false);
        f.backend.spellCheck()->setLanguage(QStringLiteral("en_US"));
    }
    QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    QTRY_COMPARE(f.editor->property("text").toString(), text);
    QCoreApplication::processEvents();
    QVERIFY(QMetaObject::invokeMethod(f.editor, "insert", Q_ARG(int, text.size()), Q_ARG(QString, QStringLiteral("Unsaved ending."))));
    const QString draft = f.editor->property("text").toString();
    if (review) {
        QTRY_VERIFY_WITH_TIMEOUT(cycle144ReviewTextKnown(f.backend.spellCheck(), draft), 15000);
        QTRY_VERIFY(f.backend.reviewIssues().size() >= 3);
    }
    QVERIFY(shortcut->property("enabled").toBool());
    // A second real Main owns the same registrations. WindowShortcut must only
    // activate the intended foreground window, never this other document.
    Cycle117Window other; QVERIFY2(other.start(900), qPrintable(other.error));
    other.editor->setProperty("text", QStringLiteral("Other window unchanged."));
    auto *otherShortcut = other.window->findChild<QObject *>(shortcutName); QVERIFY(otherShortcut);
    QSignalSpy otherActivated(otherShortcut, SIGNAL(activated()));
    f.isolation.reset();
    f.quick->requestActivate(); QTRY_VERIFY(f.quick->isActive());
    f.editor->forceActiveFocus();
    f.editor->setProperty("cursorPosition", 0);
    QTRY_VERIFY(f.editor->hasActiveFocus());
    QSignalSpy activated(shortcut, SIGNAL(activated()));
    const auto press = [&] { QTest::keyClick(f.quick, Qt::Key(key), modifiers, 0); };
    auto *dialog = f.window->findChild<QObject *>(command == "openPath" ? "openPathDialog" : "commandPalette");
    if (!review) {
        QVERIFY(dialog);
        press(); QCOMPARE(activated.count(), 1);
        QTRY_VERIFY(dialog->property("opened").toBool());
        // A modal popup blocks the owning WindowShortcut, as existing Qt scope
        // requires; it must not reopen/dispatch through that boundary.
        press(); QCOMPARE(activated.count(), 1);
        QTest::keyClick(f.quick, Qt::Key_Escape);
        QTRY_VERIFY(!dialog->property("visible").toBool());
    } else if (command == "spelling") {
        f.layout->setProperty("reviewVisible", false);
        press(); QCOMPARE(activated.count(), 1);
        QTRY_VERIFY(f.layout->property("effectiveReviewVisible").toBool());
        press(); QCOMPARE(activated.count(), 2);
        QTRY_VERIFY(!f.layout->property("reviewVisible").toBool());
    } else {
        const QString first = command == "nextIssue" ? QStringLiteral("flomptoid") : QStringLiteral("quorbleflat");
        const QString second = QStringLiteral("recieve");
        press(); QCOMPARE(activated.count(), 1);
        QTRY_COMPARE(f.editor->property("selectedText").toString(), first);
        press(); QCOMPARE(activated.count(), 2);
        QTRY_COMPARE(f.editor->property("selectedText").toString(), second);
    }
    if (review) {
        // Remove the actual service attachment only for the unavailable-guard
        // boundary, then restore that same real engine. No readiness is forged.
        auto *spell = f.backend.spellCheck(); auto *harper = spell->harper(); QVERIFY(harper);
        const auto restore = qScopeGuard([&] { spell->setHarper(harper); });
        spell->setHarper(nullptr);
        QVERIFY(!spell->available());
        QTRY_VERIFY(!shortcut->property("enabled").toBool());
        const int count = activated.count();
        const int cursor = f.editor->property("cursorPosition").toInt();
        const bool pane = f.layout->property("reviewVisible").toBool();
        press(); QCOMPARE(activated.count(), count);
        QCOMPARE(f.editor->property("cursorPosition").toInt(), cursor);
        QCOMPARE(f.layout->property("reviewVisible").toBool(), pane);
    }
    QCOMPARE(otherActivated.count(), 0);
    QCOMPARE(other.editor->property("text").toString(), QStringLiteral("Other window unchanged."));
    QCOMPARE(f.editor->property("text").toString(), draft);
    QVERIFY(f.backend.modified()); QVERIFY(f.editor->property("canUndo").toBool());
    QCOMPARE(cycle103Read(path), text.toUtf8());
    QVERIFY2(f.error.isEmpty(), qPrintable(f.error));
}
