// Cycle144 — the Review pane. Every spelling and grammar finding in the
// document is a row in a pane right of the writing surface, with the fixes at
// hand; the old modal dialog is gone. The model is Backend::reviewIssues (built
// from the highlighter's per-block findings), the pane is ReviewPane.qml.
//
// Real-checker tests use words no other test touches ("recieve", "colage",
// "quorbleflat", "This is is a test"): the system checker's ignore list lasts for
// the whole test process, and Learn Spelling would write to the user's own
// dictionary, so no test calls learnWord.

namespace {
QObject *cycle144Commands(QObject *window) { return window->findChild<QObject *>("workspaceCommands"); }
bool cycle144Run(QObject *window, const char *command) {
    return QMetaObject::invokeMethod(cycle144Commands(window), "run", Q_ARG(QVariant, QString::fromLatin1(command)));
}
QVariantMap cycle144Find(const QVariantList &issues, const QString &category, const QString &word) {
    for (const QVariant &entry : issues) {
        const QVariantMap issue = entry.toMap();
        if (issue.value("category").toString() == category && issue.value("word").toString() == word) return issue;
    }
    return {};
}
// Controls made by a Repeater are not QObject children of the window: walk the visual tree.
QQuickItem *cycle144Control(QObject *window, const char *name) {
    auto *pane = window->findChild<QQuickItem *>("reviewPane");
    if (!pane) return nullptr;
    const auto matches = cycle103VisualItems(pane, QString::fromLatin1(name));
    return matches.isEmpty() ? nullptr : matches.first();
}
QQuickItem *cycle144Row(QObject *window, int index) {
    auto *list = window->findChild<QQuickItem *>("reviewList");
    if (!list) return nullptr;
    const auto rows = cycle103VisualItems(list, QStringLiteral("reviewRow_%1").arg(index));
    return rows.isEmpty() ? nullptr : rows.first();
}
// A click on the row's empty top-right corner: the row itself, not one of its buttons.
QPoint cycle144RowPoint(QQuickItem *row) {
    const QRectF bounds = cycle103SceneBounds(row);
    return QPoint(int(bounds.right()) - 12, int(bounds.top()) + 8);
}
QQuickItem *cycle144RowButton(QQuickItem *row, const QString &name, const QString &text = QString()) {
    for (auto *button : cycle103VisualItems(row, name))
        if (text.isEmpty() || button->property("text").toString() == text) return button;
    return nullptr;
}
// The page's own text, asked for over the bridge.
QString cycle144PageText(EditorBridge *bridge, int timeoutMs = 3000) {
    QSignalSpy received(bridge, &EditorBridge::textReceived);
    const int token = bridge->fetchText();
    QElapsedTimer timer; timer.start();
    while (timer.elapsed() < timeoutMs) {
        QTest::qWait(10);
        for (const auto &args : received) if (args.at(0).toInt() == token) return args.at(1).toString();
    }
    return QStringLiteral("<timeout>");
}
// Text with every kind of finding, laid out over several blocks.
const QString cycle144Text() {
    return QStringLiteral("# Notes\n\nA colage of ideas.\n\nThis is is a test. We recieve the the mail here.\n\n```\nrecieve the the\n```\n\nLast quorbleflat line.\n");
}
}

void FomawriteTest::reviewIssuesListSpellingAndGrammarInOrderWithDocumentOffsets() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setGrammarEnabled(true); spell->setLanguage(QStringLiteral("en_US"));
    CYCLE144_NEEDS_GRAMMAR((*spell));
    QVERIFY(f.mode(0));
    const QString text = cycle144Text();
    f.editor->setProperty("text", text);
    QTRY_VERIFY(!f.backend.reviewIssues().isEmpty());
    QTRY_VERIFY(!cycle144Find(f.backend.reviewIssues(), "Spelling", "quorbleflat").isEmpty()); // the last block is checked too
    const QVariantList issues = f.backend.reviewIssues();
    QVERIFY(!f.backend.reviewIssuesTruncated());
    int previous = -1; bool sawGrammar = false;
    for (const QVariant &entry : issues) {
        const QVariantMap issue = entry.toMap();
        const int start = issue["start"].toInt(), end = issue["end"].toInt();
        // Document offsets: the slice of the whole text is the word, in order, and never inside the fence.
        QVERIFY2(start >= previous, qPrintable(QString("out of order at %1").arg(start)));
        QCOMPARE(text.mid(start, end - start), issue["word"].toString());
        QVERIFY(start < text.indexOf("```") || start > text.lastIndexOf("```"));
        QVERIFY(issue["category"].toString() == "Spelling" || issue["category"].toString() == "Grammar");
        QVERIFY2(issue["context"].toString().contains(issue["word"].toString()), "the context holds the word");
        QCOMPARE(issue["context"].toString().mid(issue["wordOffset"].toInt(), issue["word"].toString().size()), issue["word"].toString());
        QCOMPARE(issue["blockNumber"].toInt(), text.left(start).count('\n'));
        if (issue["category"].toString() == "Grammar") { sawGrammar = true; QVERIFY(!issue["message"].toString().isEmpty()); }
        previous = start;
    }
    QVERIFY2(sawGrammar, "the doubled words are a grammar finding");
    const auto colage = cycle144Find(issues, "Spelling", "colage");
    QCOMPARE(colage["start"].toInt(), text.indexOf("colage"));
    QVERIFY(colage["message"].toString().isEmpty());
    QVERIFY(!colage["suggestions"].toStringList().isEmpty());
    QCOMPARE(colage["context"].toString(), QStringLiteral("A colage of ideas."));
    const auto recieve = cycle144Find(issues, "Spelling", "recieve");
    QCOMPARE(recieve["start"].toInt(), text.indexOf("We recieve") + 3);
    // The sentence is the context, not the whole block.
    QCOMPARE(recieve["context"].toString(), QStringLiteral("We recieve the the mail here."));
    // issueAt: containing or touching a position, from the block now; Spelling only for the menu.
    QCOMPARE(f.backend.issueAt(recieve["start"].toInt() + 2)["word"].toString(), QStringLiteral("recieve"));
    QCOMPARE(f.backend.issueAt(recieve["end"].toInt())["start"].toInt(), recieve["start"].toInt()); // touching the end
    QVERIFY(f.backend.issueAt(text.indexOf("Notes")).isEmpty());
    QCOMPARE(f.backend.misspelledWordAt(recieve["start"].toInt() + 1)["word"].toString(), QStringLiteral("recieve"));
    for (const QVariant &entry : issues) {
        const QVariantMap issue = entry.toMap();
        if (issue["category"].toString() == "Grammar") QVERIFY(f.backend.misspelledWordAt(issue["start"].toInt() + 1)["category"].toString() != "Grammar");
    }
    // The setting and the grammar switch take effect at once, not after a pause.
    spell->setGrammarEnabled(false);
    for (const QVariant &entry : f.backend.reviewIssues()) QCOMPARE(entry.toMap()["category"].toString(), QStringLiteral("Spelling"));
    QVERIFY(!f.backend.reviewIssues().isEmpty());
    spell->setEnabled(false);
    QVERIFY(f.backend.reviewIssues().isEmpty());
    spell->setEnabled(true); spell->setGrammarEnabled(true);
    QVERIFY(!cycle144Find(f.backend.reviewIssues(), "Spelling", "colage").isEmpty());
    // Ignoring a word re-lists at once (process-wide; "colage" is used only here).
    spell->ignoreWord(QStringLiteral("colage"));
    QVERIFY(cycle144Find(f.backend.reviewIssues(), "Spelling", "colage").isEmpty());
}

void FomawriteTest::reviewIssuesAreCappedAndTheCapIsReported() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setGrammarEnabled(false); spell->setLanguage(QStringLiteral("en_US"));
    QVERIFY(f.mode(0));
    QString text;
    for (int i = 0; i < 620; ++i) text += QStringLiteral("Line %1 with zxqvwhatsit in it.\n\n").arg(i);
    f.editor->setProperty("text", text);
    QTRY_VERIFY_WITH_TIMEOUT(f.backend.reviewIssuesTruncated(), 15000);
    QCOMPARE(f.backend.reviewIssues().size(), Backend::ReviewIssueLimit);
    // Still in document order: the first 500, not a sample.
    QCOMPARE(f.backend.reviewIssues().last().toMap()["blockNumber"].toInt(), (Backend::ReviewIssueLimit - 1) * 2);
    f.editor->setProperty("text", QStringLiteral("Only one zxqvwhatsit here.\n"));
    QTRY_COMPARE(f.backend.reviewIssues().size(), 1);
    QVERIFY(!f.backend.reviewIssuesTruncated());
}

void FomawriteTest::reviewIssuesAreRebuiltAfterAPauseAndTypingCoalesces() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setGrammarEnabled(false); spell->setLanguage(QStringLiteral("en_US"));
    QVERIFY(f.mode(0));
    f.editor->setProperty("text", QStringLiteral("Start here.\n"));
    QTRY_VERIFY(f.backend.reviewIssues().isEmpty());
    QTest::qWait(400); // let the load's own rebuild finish
    QSignalSpy changed(&f.backend, &Backend::reviewIssuesChanged);
    // Typing "recieve " one letter at a time, faster than the pause: nothing is
    // rebuilt in the middle (half-typed words would be listed and removed again),
    // and one rebuild follows the last key.
    const QString typed = QStringLiteral("recieve ");
    for (int i = 0; i < typed.size(); ++i) {
        QVERIFY(QMetaObject::invokeMethod(f.editor, "insert", Q_ARG(int, i), Q_ARG(QString, typed.mid(i, 1))));
        QTest::qWait(60);
    }
    QCOMPARE(changed.count(), 0);
    QVERIFY(f.backend.reviewIssues().isEmpty());
    QTRY_COMPARE(changed.count(), 1);
    QCOMPARE(f.backend.reviewIssues().size(), 1);
    QCOMPARE(f.backend.reviewIssues().first().toMap()["word"].toString(), QStringLiteral("recieve"));
    QTest::qWait(500);
    QCOMPARE(changed.count(), 1);
}

void FomawriteTest::reviewPaneTogglesWithTheCommandAndTheMenuTickFollows() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    if (!f.backend.spellCheck()->available()) QSKIP("the system spell checker exists only on macOS");
    QVERIFY(f.mode(0));
    auto *slot = f.window->findChild<QQuickItem *>("reviewSlot"); QVERIFY(slot);
    auto *menuItem = f.window->findChild<QObject *>("editSpelling"); QVERIFY(menuItem);
    QCOMPARE(menuItem->property("text").toString(), QStringLiteral("Spelling and Grammar")); // no ellipsis: it is a pane, not a dialog
    QVERIFY(menuItem->property("checkable").toBool());
    QVERIFY(menuItem->property("enabled").toBool());
    QVERIFY(!menuItem->property("checked").toBool());
    QVERIFY(!f.layout->property("reviewVisible").toBool());
    QVERIFY(!slot->isVisible());
    QVERIFY(f.window->findChild<QObject *>("editNextIssue")->property("enabled").toBool());
    QVERIFY(f.window->findChild<QObject *>("editPreviousIssue")->property("enabled").toBool());
    QVERIFY(cycle144Run(f.window.data(), "spelling"));
    QTRY_VERIFY(slot->isVisible());
    QTRY_COMPARE(slot->width(), 320.0); // laid out, not just shown
    QVERIFY(f.layout->property("reviewVisible").toBool());
    QVERIFY(f.layout->property("effectiveReviewVisible").toBool());
    QVERIFY(menuItem->property("checked").toBool());
    QVERIFY(f.window->findChild<QQuickItem *>("reviewPane")->isVisible());
    QVERIFY(f.window->findChild<QQuickItem *>("reviewList"));
    for (const char *name : {"reviewFilterAll", "reviewFilterSpelling", "reviewFilterGrammar", "reviewLanguage"})
        QVERIFY2(cycle144Control(f.window.data(), name), name);
    // At the narrowest width every header control still sits inside the pane.
    for (qreal width : {240.0, 320.0}) {
        QVERIFY(QMetaObject::invokeMethod(f.layout, "updateWidth", Q_ARG(QVariant, "review"), Q_ARG(QVariant, width)));
        QTRY_COMPARE(slot->width(), width);
        auto *pane = f.window->findChild<QQuickItem *>("reviewPane");
        for (const char *name : {"reviewClose", "reviewLanguage", "reviewFilterAll", "reviewFilterSpelling", "reviewFilterGrammar"}) {
            auto *control = cycle144Control(f.window.data(), name); QVERIFY2(control, name);
            QVERIFY2(cycle103SceneBounds(pane).adjusted(-0.5, -0.5, 0.5, 0.5).contains(cycle103SceneBounds(control)), name);
        }
    }
    // The close button is the same toggle.
    auto *close = f.window->findChild<QQuickItem *>("reviewClose"); QVERIFY(close);
    QVERIFY(cycle102Click(f.quick, close));
    QTRY_VERIFY(!slot->isVisible());
    QVERIFY(!menuItem->property("checked").toBool());
    QVERIFY(!f.layout->property("reviewVisible").toBool());
    QVERIFY(cycle144Run(f.window.data(), "spelling")); QTRY_VERIFY(slot->isVisible());
    QVERIFY(cycle144Run(f.window.data(), "spelling")); QTRY_VERIFY(!slot->isVisible());
    // The checker switched off: the pane says so and offers to turn it on.
    f.backend.spellCheck()->setEnabled(false);
    QVERIFY(cycle144Run(f.window.data(), "spelling")); QTRY_VERIFY(slot->isVisible());
    QTRY_COMPARE(slot->width(), 320.0);
    auto *empty = f.window->findChild<QQuickItem *>("reviewEmpty"); QVERIFY(empty);
    QCOMPARE(empty->property("text").toString(), QStringLiteral("Spelling check is off"));
    auto *turnOn = f.window->findChild<QQuickItem *>("reviewTurnOn"); QVERIFY(turnOn && turnOn->isVisible());
    QVERIFY(cycle102Click(f.quick, turnOn));
    QTRY_VERIFY(f.backend.spellCheck()->enabled());
    QTRY_COMPARE(empty->property("text").toString(), QStringLiteral("No spelling or grammar issues"));
    QVERIFY(!turnOn->isVisible());
}

void FomawriteTest::reviewRowClickMovesTheSourceCaret() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setGrammarEnabled(false); spell->setLanguage(QStringLiteral("en_US"));
    QVERIFY(f.mode(0));
    const QString text = QStringLiteral("A flomptoid of ideas.\n\nWe recieve the mail, and quorbleflat too.\n");
    f.editor->setProperty("text", text);
    QVERIFY(cycle144Run(f.window.data(), "spelling"));
    QTRY_VERIFY(cycle144Row(f.window.data(), 2));
    auto *counts = f.window->findChild<QQuickItem *>("reviewCounts");
    QCOMPARE(counts->property("text").toString(), QStringLiteral("3 spelling"));
    // The rows are in document order; each shows its category tag and its context with the word in it.
    auto *second = cycle144Row(f.window.data(), 1); QVERIFY(second);
    QVERIFY(cycle144RowButton(second, "reviewLearn")->isVisible());
    QVERIFY(cycle144RowButton(second, "reviewIgnore")->isVisible());
    auto *context = cycle144RowButton(second, "reviewContext"); QVERIFY(context);
    QVERIFY(context->property("text").toString().contains("<b>"));
    QVERIFY(context->property("text").toString().contains("recieve"));
    QVERIFY(cycle144RowButton(second, "reviewSuggestion_0"));
    QTest::mouseClick(f.quick, Qt::LeftButton, Qt::NoModifier, cycle144RowPoint(second));
    const int start = text.indexOf("recieve");
    QTRY_COMPARE(f.editor->property("selectionStart").toInt(), start);
    QCOMPARE(f.editor->property("selectionEnd").toInt(), start + 7);
    QCOMPARE(f.editor->property("selectedText").toString(), QStringLiteral("recieve"));
    QVERIFY(f.editor->hasActiveFocus());
    // The first row goes to the first word.
    QTest::mouseClick(f.quick, Qt::LeftButton, Qt::NoModifier, cycle144RowPoint(cycle144Row(f.window.data(), 0)));
    QTRY_COMPARE(f.editor->property("selectedText").toString(), QStringLiteral("flomptoid"));
    // Filtering: Grammar shows none (grammar is off here), Spelling all three.
    QVERIFY(cycle102Click(f.quick, cycle144Control(f.window.data(), "reviewFilterGrammar")));
    QTRY_VERIFY(!cycle144Row(f.window.data(), 0));
    QTRY_COMPARE(f.window->findChild<QQuickItem *>("reviewEmpty")->property("text").toString(), QStringLiteral("Grammar check is off"));
    QVERIFY(cycle102Click(f.quick, cycle144Control(f.window.data(), "reviewFilterSpelling")));
    QTRY_VERIFY(cycle144Row(f.window.data(), 2));
    QVERIFY(cycle102Click(f.quick, cycle144Control(f.window.data(), "reviewFilterAll")));
    QTRY_VERIFY(cycle144Row(f.window.data(), 2));
}

void FomawriteTest::reviewSuggestionAppliesAsOneUndoableEdit() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setGrammarEnabled(false); spell->setLanguage(QStringLiteral("en_US"));
    QVERIFY(f.mode(0));
    const QString text = QStringLiteral("We recieve the mail.\n\nA wrimblesnort here.\n");
    f.editor->setProperty("text", text);
    QVERIFY(cycle144Run(f.window.data(), "spelling"));
    QTRY_VERIFY(cycle144Row(f.window.data(), 1));
    auto *row = cycle144Row(f.window.data(), 0);
    QCOMPARE(cycle144RowButton(row, "reviewContext")->property("text").toString().contains("recieve"), true);
    // At most five suggestions, "receive" among them.
    int shown = 0; while (cycle144RowButton(row, QStringLiteral("reviewSuggestion_%1").arg(shown))) ++shown;
    QVERIFY(shown >= 1 && shown <= 5);
    auto *choice = cycle144RowButton(row, QStringLiteral("reviewSuggestion_0"));
    for (int i = 0; i < shown; ++i) {
        auto *button = cycle144RowButton(row, QStringLiteral("reviewSuggestion_%1").arg(i));
        if (button->property("text").toString() == QStringLiteral("receive")) choice = button;
    }
    QCOMPARE(choice->property("text").toString(), QStringLiteral("receive"));
    const int undoStepsBefore = f.backend.documentRevision();
    Q_UNUSED(undoStepsBefore);
    QVERIFY(cycle102Click(f.quick, choice));
    const QString fixed = QStringLiteral("We receive the mail.\n\nA wrimblesnort here.\n");
    QTRY_COMPARE(f.editor->property("text").toString(), fixed);
    // The finding is gone from the list after the pause; the other stays.
    QTRY_VERIFY(cycle144Find(f.backend.reviewIssues(), "Spelling", "recieve").isEmpty());
    QCOMPARE(f.backend.reviewIssues().size(), 1);
    // One undo restores the whole edit, and the finding returns.
    QVERIFY(QMetaObject::invokeMethod(f.editor, "undo"));
    QCOMPARE(f.editor->property("text").toString(), text);
    QTRY_VERIFY(!cycle144Find(f.backend.reviewIssues(), "Spelling", "recieve").isEmpty());
    // Ignore drops the row (the checker's ignore list; Learn would write the user's dictionary and is not clicked).
    QTRY_VERIFY(cycle144Row(f.window.data(), 1));
    auto *second = cycle144Row(f.window.data(), 1);
    QCOMPARE(cycle144RowButton(second, "reviewContext")->property("text").toString().contains("wrimblesnort"), true);
    QVERIFY(cycle102Click(f.quick, cycle144RowButton(second, "reviewIgnore")));
    QTRY_COMPARE(f.backend.reviewIssues().size(), 1);
    QTRY_VERIFY(!cycle144Row(f.window.data(), 1));
}

void FomawriteTest::nextAndPreviousIssueWrapRoundTheDocument() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setGrammarEnabled(false); spell->setLanguage(QStringLiteral("en_US"));
    QVERIFY(f.mode(0));
    const QString text = QStringLiteral("A flomptoid of ideas.\n\nWe recieve the mail.\n\nAnd quorbleflat there.\n");
    f.editor->setProperty("text", text);
    QTRY_COMPARE(f.backend.reviewIssues().size(), 3);
    const int first = text.indexOf("flomptoid"), second = text.indexOf("recieve"), third = text.indexOf("quorbleflat");
    // The model: strictly after / before, wrapping, -1 with nothing to find.
    QCOMPARE(f.backend.nextIssue(-1, false), first);
    QCOMPARE(f.backend.nextIssue(first, false), second);
    QCOMPARE(f.backend.nextIssue(second + 3, false), third);
    QCOMPARE(f.backend.nextIssue(third, false), first);           // wraps forwards
    QCOMPARE(f.backend.nextIssue(third + 4, false), first);
    QCOMPARE(f.backend.nextIssue(third, true), second);
    QCOMPARE(f.backend.nextIssue(first, true), third);            // wraps backwards
    QCOMPARE(f.backend.nextIssue(0, true), third);
    // The commands move and select in Source.
    QVERIFY(f.window->findChild<QObject *>("editNextIssue")->property("enabled").toBool());
    f.editor->setProperty("cursorPosition", 0);
    QVERIFY(cycle144Run(f.window.data(), "nextIssue"));
    QTRY_COMPARE(f.editor->property("selectedText").toString(), QStringLiteral("flomptoid"));
    QVERIFY(cycle144Run(f.window.data(), "nextIssue")); QTRY_COMPARE(f.editor->property("selectedText").toString(), QStringLiteral("recieve"));
    QVERIFY(cycle144Run(f.window.data(), "nextIssue")); QTRY_COMPARE(f.editor->property("selectedText").toString(), QStringLiteral("quorbleflat"));
    QVERIFY(cycle144Run(f.window.data(), "nextIssue")); QTRY_COMPARE(f.editor->property("selectedText").toString(), QStringLiteral("flomptoid")); // wrapped
    QVERIFY(cycle144Run(f.window.data(), "previousIssue")); QTRY_COMPARE(f.editor->property("selectedText").toString(), QStringLiteral("quorbleflat")); // wrapped back
    QVERIFY(cycle144Run(f.window.data(), "previousIssue")); QTRY_COMPARE(f.editor->property("selectedText").toString(), QStringLiteral("recieve"));
    QVERIFY(cycle144Run(f.window.data(), "previousIssue")); QTRY_COMPARE(f.editor->property("selectedText").toString(), QStringLiteral("flomptoid"));
    // Nothing to find: no movement, no error.
    spell->setEnabled(false);
    QVERIFY(!f.window->findChild<QObject *>("editSpelling")->property("checked").toBool());
    QCOMPARE(f.backend.nextIssue(0, false), -1);
    // And the commands are unavailable only when there is no checker at all.
    QVERIFY(f.window->findChild<QObject *>("editNextIssue")->property("enabled").toBool());
}

void FomawriteTest::reviewPaneResizesWithTheSplitViewAndItsWidthPersists() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1280), qPrintable(f.error));
    if (!f.backend.spellCheck()->available()) QSKIP("the system spell checker exists only on macOS");
    QVERIFY(f.mode(0));
    auto *slot = f.window->findChild<QQuickItem *>("reviewSlot"); QVERIFY(slot);
    auto *editorPane = f.window->findChild<QQuickItem *>("editorPane"); QVERIFY(editorPane);
    QCOMPARE(f.layout->property("reviewWidth").toReal(), 320.0);
    QVERIFY(cycle144Run(f.window.data(), "spelling"));
    QTRY_VERIFY(slot->isVisible());
    QTRY_COMPARE(slot->width(), 320.0);
    // The slot is the rightmost item and ends at the window's edge.
    QVERIFY(slot->x() > editorPane->x());
    QTRY_VERIFY(qAbs(cycle103SceneBounds(slot).right() - f.quick->width()) < 1.5);
    // Drag its handle (the divider just left of it) 60 px to the left: the pane grows by about 60.
    QQuickItem *handle = nullptr;
    for (auto *candidate : cycle103VisualItems(f.window->findChild<QQuickItem *>("workspaceSplit"), "workspaceDivider")) {
        if (!handle || qAbs(candidate->x() - slot->x()) < qAbs(handle->x() - slot->x())) handle = candidate;
    }
    QVERIFY(handle);
    const QPoint grab = cycle103SceneBounds(handle).center().toPoint();
    QTest::mousePress(f.quick, Qt::LeftButton, Qt::NoModifier, grab);
    QTest::mouseMove(f.quick, grab + QPoint(-30, 0), 40);
    QTest::mouseMove(f.quick, grab + QPoint(-60, 0), 40);
    QTest::mouseRelease(f.quick, Qt::LeftButton, Qt::NoModifier, grab + QPoint(-60, 0));
    QTRY_VERIFY2(slot->width() >= 370, qPrintable(QString::number(slot->width())));
    // The new width is the desired width, and it survives the saved state and a restart of the layout.
    QTRY_VERIFY(qAbs(f.layout->property("reviewWidth").toReal() - slot->width()) < 2);
    const qreal dragged = f.layout->property("reviewWidth").toReal();
    QVariant saved; QVERIFY(QMetaObject::invokeMethod(f.layout, "saveState", Q_RETURN_ARG(QVariant, saved)));
    const QVariantMap state = saved.value<QJSValue>().toVariant().toMap();
    QCOMPARE(state["reviewVisible"].toBool(), true);
    QVERIFY(qAbs(state["reviewWidth"].toReal() - dragged) < 0.01);
    QVERIFY(cycle144Run(f.window.data(), "spelling")); QTRY_VERIFY(!slot->isVisible());
    QVERIFY(QMetaObject::invokeMethod(f.layout, "updateWidth", Q_ARG(QVariant, "review"), Q_ARG(QVariant, 300)));
    QCOMPARE(f.layout->property("reviewWidth").toReal(), 300.0);
    QVariant restored;
    QVERIFY(QMetaObject::invokeMethod(f.layout, "restoreState", Q_RETURN_ARG(QVariant, restored), Q_ARG(QVariant, saved)));
    QVERIFY(restored.toBool());
    QCOMPARE(f.layout->property("reviewVisible").toBool(), true);
    QVERIFY(qAbs(f.layout->property("reviewWidth").toReal() - dragged) < 0.01);
    QTRY_VERIFY(slot->isVisible());
    // Bounds: no narrower than 240, no wider than 640.
    QVERIFY(QMetaObject::invokeMethod(f.layout, "updateWidth", Q_ARG(QVariant, "review"), Q_ARG(QVariant, 100)));
    QCOMPARE(f.layout->property("reviewWidth").toReal(), 240.0);
    QVERIFY(QMetaObject::invokeMethod(f.layout, "updateWidth", Q_ARG(QVariant, "review"), Q_ARG(QVariant, 2000)));
    QCOMPARE(f.layout->property("reviewWidth").toReal(), 640.0);
    // A state saved before this cycle has no review fields: hidden, default width.
    QVariantMap legacy = state; legacy.remove("reviewVisible"); legacy.remove("reviewWidth");
    QVERIFY(QMetaObject::invokeMethod(f.layout, "restoreState", Q_RETURN_ARG(QVariant, restored), Q_ARG(QVariant, legacy)));
    QCOMPARE(f.layout->property("reviewVisible").toBool(), false);
    QCOMPARE(f.layout->property("reviewWidth").toReal(), 320.0);
}

// The footer spans the window; the pane sits above it on the right. Opening and
// closing the pane must not move a single footer control.
void FomawriteTest::reviewPaneLeavesTheFooterControlsWhereTheyAre() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1280), qPrintable(f.error));
    if (!f.backend.spellCheck()->available()) QSKIP("the system spell checker exists only on macOS");
    for (int width : {1280, 960}) {
        f.quick->resize(width, 700); QTRY_COMPARE(f.quick->size(), QSize(width, 700));
        for (int mode : {0, 1}) {
            // Split plus the pane needs 480 + 320 + 240; a narrower window contracts to Single (by design).
            if (mode == 1 && width < 1040) continue;
            QVERIFY(f.mode(mode)); QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), mode);
            QVERIFY(cycle118FooterRenderBarrier(f.quick));
            const auto closed = cycle107ViewBounds(f.footer);
            auto *group = f.footer->findChild<QQuickItem *>("documentViewControls"); QVERIFY(group);
            const QRectF closedFooter = cycle103SceneBounds(f.footer);
            QVERIFY(qAbs(cycle103SceneBounds(group).right() - (width - 12)) < 0.1);
            QVERIFY(cycle144Run(f.window.data(), "spelling"));
            QTRY_VERIFY(f.layout->property("effectiveReviewVisible").toBool());
            auto *slot = f.window->findChild<QQuickItem *>("reviewSlot");
            QTRY_VERIFY(slot && slot->isVisible() && slot->width() >= 240);
            QCOMPARE(f.layout->property("effectiveLayoutMode").toInt(), mode);
            QVERIFY(cycle118FooterRenderBarrier(f.quick));
            QTRY_VERIFY(cycle107SameBounds(cycle107ViewBounds(f.footer), closed));
            QCOMPARE(cycle103SceneBounds(f.footer), closedFooter);
            QVERIFY(qAbs(cycle103SceneBounds(group).right() - (width - 12)) < 0.1);
            QTRY_VERIFY(cycle106FooterControlsFit(f.window.data()));
            QTRY_VERIFY(cycle117PaneFooterAnchors(f.window.data()));
            // The pane stops above the footer; its list is not hidden behind it.
            auto *pane = f.window->findChild<QQuickItem *>("reviewPane");
            QVERIFY(cycle103SceneBounds(pane).bottom() <= cycle103SceneBounds(f.footer).top() + 0.5);
            QVERIFY(cycle144Run(f.window.data(), "spelling"));
            QTRY_VERIFY(!f.layout->property("effectiveReviewVisible").toBool());
            QVERIFY(cycle118FooterRenderBarrier(f.quick));
            QTRY_VERIFY(cycle107SameBounds(cycle107ViewBounds(f.footer), closed));
        }
    }
}

// Live: the same pane acts on the page. The fix is a host edit that the existing
// mirror delivers to the page; a row click places the page's caret.
void FomawriteTest::reviewPaneFixesAndJumpsInTheLivePage() {
#ifndef Q_OS_MACOS
    QSKIP("the system spell checker is only available on macOS");
#endif
    Cycle103SettingsSnapshot settings;
    Backend backend;
    backend.spellCheck()->setEnabled(true); backend.spellCheck()->setGrammarEnabled(false); backend.spellCheck()->setLanguage(QStringLiteral("en_US"));
    backend.spellCheck()->setEngine(QStringLiteral("macos")); // this test is about the macOS checker (Harper: tests/cycle145-harper-engine.inc)
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    auto *editor = window->findChild<QQuickItem *>("sourceEditor"); QVERIFY(editor);
    auto *layout = window->findChild<QObject *>("workspaceLayout"); QVERIFY(layout);
    quick->resize(1280, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
    const QString text = QStringLiteral("A snorfbit of ideas.\n\nWe recieve the mail.\n");
    editor->setProperty("text", text);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    auto *bridge = backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 20000);
    auto *view = window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QTRY_VERIFY_WITH_TIMEOUT(!view->property("loading").toBool(), 15000);
    QVERIFY(layout->property("liveEditEnabled").toBool());
    QTRY_COMPARE(cycle144PageText(bridge), text);
    QVERIFY(cycle144Run(window.data(), "spelling"));
    QTRY_VERIFY(cycle144Row(window.data(), 1));
    // Jump: the page's caret goes to the finding (and the Source editor stays untouched).
    QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier, cycle144RowPoint(cycle144Row(window.data(), 1)));
    QTRY_COMPARE_WITH_TIMEOUT(bridge->lastCursor(), text.indexOf("recieve"), 5000);
    QVERIFY(layout->property("liveEditEnabled").toBool());
    // Fix: the host document changes and the page shows the corrected text.
    auto *row = cycle144Row(window.data(), 1);
    QQuickItem *choice = nullptr;
    for (int i = 0; i < 5; ++i) {
        auto *button = cycle144RowButton(row, QStringLiteral("reviewSuggestion_%1").arg(i));
        if (button && button->property("text").toString() == QStringLiteral("receive")) choice = button;
    }
    QVERIFY(choice);
    QVERIFY(cycle102Click(quick, choice));
    const QString fixed = QStringLiteral("A snorfbit of ideas.\n\nWe receive the mail.\n");
    QTRY_COMPARE(editor->property("text").toString(), fixed);
    QTRY_COMPARE_WITH_TIMEOUT(cycle144PageText(bridge), fixed, 10000);
    QTRY_COMPARE(backend.reviewIssues().size(), 1);
    // The page's own undo takes the fix back (measured: the host edit is in the page's history), and the host follows.
    bridge->requestUndo();
    const QString original = QStringLiteral("A snorfbit of ideas.\n\nWe recieve the mail.\n");
    QTRY_COMPARE_WITH_TIMEOUT(editor->property("text").toString(), original, 10000);
    QCOMPARE(cycle144PageText(bridge), original);
    QTRY_COMPARE(backend.reviewIssues().size(), 2);
    // Next Issue in Live moves the page's caret.
    bridge->placeCursor(0);
    QTRY_COMPARE_WITH_TIMEOUT(bridge->lastCursor(), 0, 5000);
    QVERIFY(cycle144Run(window.data(), "nextIssue"));
    QTRY_COMPARE_WITH_TIMEOUT(bridge->lastCursor(), fixed.indexOf("snorfbit"), 5000);
    backend.discardRecovery();
}

// Frames of the real window with the pane open, light and dark, for the cycle
// record (research/cycle-144/captures, local only). The assertion is only that
// the pane paints something other than one flat colour in each theme.
void FomawriteTest::reviewPaneRendersInLightAndDarkThemes() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1280), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setGrammarEnabled(true); spell->setLanguage(QStringLiteral("en_US"));
    CYCLE144_NEEDS_GRAMMAR((*spell));
    QVERIFY(f.mode(1));
    f.editor->setProperty("text", cycle144Text());
    QVERIFY(cycle144Run(f.window.data(), "spelling"));
    QTRY_VERIFY(cycle144Row(f.window.data(), 2));
    const QString directory = QDir(QFileInfo(QFINDTESTDATA("tst_fomawrite.cpp")).absolutePath()).filePath("../research/cycle-144/captures");
    QDir().mkpath(directory);
    for (const char *theme : {"light", "dark"}) {
        f.backend.setThemePreset(QString::fromLatin1(theme));
        QTest::qWait(300);
        QString why;
        const QString path = QDir(directory).filePath(QStringLiteral("review-pane-%1.png").arg(QString::fromLatin1(theme)));
        QVERIFY2(cycle134SaveFrame(f.quick, path, &why), qPrintable(why));
        const QImage frame = f.quick->grabWindow();
        auto *pane = f.window->findChild<QQuickItem *>("reviewPane");
        const qreal scale = frame.width() / qreal(f.quick->width());
        const QRect box = QRectF(cycle103SceneBounds(pane).topLeft() * scale, cycle103SceneBounds(pane).size() * scale).toRect();
        QSet<QRgb> colours;
        for (int y = box.top(); y < box.bottom(); y += 3) for (int x = box.left(); x < box.right(); x += 3) colours.insert(frame.pixel(x, y));
        QVERIFY2(colours.size() > 8, theme);
    }
}

// In Live the host never sees the Source caret move, so the pane used to list
// every half-typed word 300 ms after the keystroke (reviewer finding). The
// hold-back now follows whichever caret moved last.
void FomawriteTest::reviewPaneWithholdsTheWordBeingTypedInLive() {
#ifndef Q_OS_MACOS
    QSKIP("the system spell checker is only available on macOS");
#endif
    Cycle103SettingsSnapshot settings;
    Backend backend;
    backend.spellCheck()->setEnabled(true); backend.spellCheck()->setGrammarEnabled(false); backend.spellCheck()->setLanguage(QStringLiteral("en_US"));
    backend.spellCheck()->setEngine(QStringLiteral("macos")); // this test is about the macOS checker (Harper: tests/cycle145-harper-engine.inc)
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    auto *editor = window->findChild<QQuickItem *>("sourceEditor"); QVERIFY(editor);
    auto *layout = window->findChild<QObject *>("workspaceLayout"); QVERIFY(layout);
    quick->resize(1280, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
    const QString text = QStringLiteral("We recieve the mail.");
    editor->setProperty("text", text);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    auto *bridge = backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 20000);
    auto *view = window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QTRY_VERIFY_WITH_TIMEOUT(!view->property("loading").toBool(), 15000);
    QTRY_COMPARE(cycle144PageText(bridge), text);
    QVERIFY(cycle144Run(window.data(), "spelling"));
    QTRY_VERIFY(!cycle144Find(backend.reviewIssues(), "Spelling", "recieve").isEmpty());
    // Type a misspelled word at the end of the page, caret left at its end.
    const QString typed = QStringLiteral(" zxqvwha");
    const QString script = QStringLiteral(R"JS(
        (function() { const v = window.fomawriteEditor.view; const end = v.state.doc.length;
          v.dispatch({ changes: { from: end, insert: "%1" }, selection: { anchor: end + %2 }, userEvent: "input.type" }); })())JS")
        .arg(typed).arg(typed.size());
    QVERIFY(QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, script)));
    QTRY_COMPARE_WITH_TIMEOUT(editor->property("text").toString(), text + typed, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(bridge->lastCursor(), text.size() + typed.size(), 5000);
    // Within the typing pause the half-typed word is not a row.
    QTest::qWait(700);
    QVERIFY2(cycle144Find(backend.reviewIssues(), "Spelling", "zxqvwha").isEmpty(), "the word at the Live caret is withheld");
    QVERIFY(!cycle144Find(backend.reviewIssues(), "Spelling", "recieve").isEmpty());
    // The caret leaves the word: it is listed.
    bridge->placeCursor(0);
    QTRY_VERIFY_WITH_TIMEOUT(!cycle144Find(backend.reviewIssues(), "Spelling", "zxqvwha").isEmpty(), 5000);
    backend.discardRecovery();
}

// Ignore Grammar Issue from the Source menu, the pane or the Live page lands in
// the one list the service keeps, so every surface drops the finding.
void FomawriteTest::ignoreGrammarFromAnySurfaceReachesEverySurface() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    spell->setEnabled(true); spell->setGrammarEnabled(true); spell->setLanguage(QStringLiteral("en_US"));
    CYCLE144_NEEDS_GRAMMAR((*spell));
    const QString text = QStringLiteral("This is is a test.\n\nAnd the the end of it.\n");
    f.editor->setProperty("text", text);
    QTRY_VERIFY(!cycle144Find(f.backend.reviewIssues(), "Grammar", "is is").isEmpty());
    QTRY_VERIFY(!cycle144Find(f.backend.reviewIssues(), "Grammar", "the the").isEmpty());
    // Source menu path: Backend::ignoreGrammarIssue(start, end).
    const QVariantMap doubled = cycle144Find(f.backend.reviewIssues(), "Grammar", "is is"); // "This is is": the range the checker gives, not indexOf
    const int isIs = doubled.value("start").toInt();
    f.backend.ignoreGrammarIssue(isIs, doubled.value("end").toInt());
    QTRY_VERIFY(cycle144Find(f.backend.reviewIssues(), "Grammar", "is is").isEmpty());
    QVERIFY(f.backend.grammarIssueAt(isIs + 2).isEmpty());
    QVERIFY(spell->grammarIssues(QStringLiteral("This is is a test.")).isEmpty()); // what the Live host reply is built from
    QVERIFY(!cycle144Find(f.backend.reviewIssues(), "Grammar", "the the").isEmpty());
    // Live page path: the bridge slot with the text and the message.
    const QString message = cycle144Find(f.backend.reviewIssues(), "Grammar", "the the").value("message").toString();
    QVERIFY(!message.isEmpty());
    f.backend.editorBridge()->ignoreGrammar(QStringLiteral("the the"), message);
    QTRY_VERIFY(cycle144Find(f.backend.reviewIssues(), "Grammar", "the the").isEmpty());
    QVERIFY(f.backend.grammarIssueAt(text.indexOf(QStringLiteral("the the")) + 2).isEmpty());
    // Spelling is untouched by grammar ignores.
    f.editor->setProperty("text", text + QStringLiteral("\nWe recieve it.\n"));
    QTRY_VERIFY(!cycle144Find(f.backend.reviewIssues(), "Spelling", "recieve").isEmpty());
}
