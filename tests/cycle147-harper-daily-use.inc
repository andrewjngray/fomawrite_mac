// Cycle147 — Harper in daily use, from Andrew's first morning with build 146.
// A long list of misspellings (his test document) showed red in Source and
// nothing in Live: the page sends windows of up to 4,000 characters, and
// Harper reads a run of lines without sentence punctuation as one sentence,
// answering with a single readability note and no spelling. The host now
// lints the page's segments line by line.

void FomawriteTest::liveMarksALongListOfMisspellingsLineByLine() {
    Cycle103SettingsSnapshot settings;
    Cycle145Prewarm prewarm(true);
    Backend backend;
    backend.spellCheck()->setEnabled(true); backend.spellCheck()->setGrammarEnabled(true); backend.spellCheck()->setLanguage(QStringLiteral("en_US"));
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    auto *editor = window->findChild<QQuickItem *>("sourceEditor"); QVERIFY(editor);
    auto *layout = window->findChild<QObject *>("workspaceLayout"); QVERIFY(layout);
    quick->resize(1400, 900); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
    const char *words[] = {"raining","writings","yellow","woods","hanging","aggression","looking","eligible","electricity","scold","adaptable","immature","swivel","fur","stabbed","disturb","prize","wednesday","succession","straight"};
    const char *bad[] = {"woodes","haing","agression","loking","begining","eligble","electrisity","schold","adabtable","imature","swival","furr","stabed","distrebe","prizer","wensday","sucession","strate","increented","backen"};
    QStringList lines;
    for (int i = 0; i < 300; ++i) lines << QStringLiteral("%1: %2, %3, %4").arg(QLatin1String(words[i % 20]), QLatin1String(bad[i % 20]), QLatin1String(bad[(i + 7) % 20]), QLatin1String(bad[(i + 13) % 20]));
    const QString text = lines.join('\n') + '\n';
    editor->setProperty("text", text);
    auto *bridge = backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 20000);
    QTRY_VERIFY_WITH_TIMEOUT(backend.spellCheck()->harperReady(), 30000);
    // Source and the pane see the misspellings.
    QTRY_VERIFY_WITH_TIMEOUT(backend.reviewIssues().size() >= 400, 60000);
    // A fix from the pane is reflected within half a second (the page's re-check and the rebuild).
    const int at = text.indexOf("woodes");
    QSignalSpy changed(&backend, &Backend::reviewIssuesChanged);
    QElapsedTimer fix; fix.start();
    QVERIFY(backend.correctWriting(at, at + 6, "woodes", "woods"));
    QTRY_VERIFY_WITH_TIMEOUT(changed.count() >= 1, 20000);
    QVERIFY2(fix.elapsed() < 1500, qPrintable(QStringLiteral("pane rebuilt after %1 ms").arg(fix.elapsed())));
    // A suggestion button removes its row at once (before the re-check lands) and the next row is selected.
    QVERIFY(cycle144Run(window.data(), "spelling"));
    auto *pane = window->findChild<QQuickItem *>("reviewPane"); QVERIFY(pane);
    QTRY_VERIFY(cycle144Row(window.data(), 1));
    const int rowsBefore = pane->property("shown").toList().size();
    const QVariantMap first = pane->property("shown").toList().first().toMap(), second = pane->property("shown").toList().at(1).toMap();
    auto *button = cycle144RowButton(cycle144Row(window.data(), 0), QStringLiteral("reviewSuggestion_0")); QVERIFY(button);
    QVERIFY(cycle102Click(quick, button));
    QCOMPARE(pane->property("shown").toList().size(), rowsBefore - 1); // gone at once, not after the rebuild
    QCOMPARE(pane->property("selectedStart").toInt(), second.value("start").toInt());
    // The word at that exact place is replaced (the same misspelling recurs elsewhere in the list).
    QTRY_VERIFY_WITH_TIMEOUT(editor->property("text").toString().mid(first.value("start").toInt(), first.value("word").toString().size()) != first.value("word").toString(), 5000);
    // Live: the same document shows marks, line by line.
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    auto *view = window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QSignalSpy asked(bridge, &EditorBridge::spellingRequested);
    QTRY_VERIFY_WITH_TIMEOUT(cycle143Underlined(view).size() > 20, 30000);
    QVERIFY(cycle143Underlined(view).contains(QStringLiteral("haing")));
    backend.discardRecovery();
}
