// Cycle143 — spelling in the Live page. The page decides which text is prose,
// the host's shared SpellCheck service decides which words are misspelled, and
// the page underlines them (.fw-misspelled). The setting switches it off.

namespace {
// Evaluates `script` in the Live page and returns its (string) answer, or a null
// QVariant when none arrives in time. The callback is scoped to this engine.
QVariant cycle143PageResult(QObject *view, const QString &script) {
    auto *engine = qmlEngine(view);
    if (!engine) return {};
    engine->globalObject().setProperty("cycle143PageResult", QJSValue(QJSValue::UndefinedValue));
    const QJSValue callback = engine->evaluate("(function(value) { cycle143PageResult = value; })");
    if (!QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, script), Q_ARG(uint, 1), Q_ARG(QJSValue, callback))) return {};
    QElapsedTimer timer; timer.start();
    while (engine->globalObject().property("cycle143PageResult").isUndefined() && timer.elapsed() < 2000) QTest::qWait(10);
    return engine->globalObject().property("cycle143PageResult").toVariant();
}
// The text of every underlined word currently in the page.
QStringList cycle143Underlined(QObject *view) {
    const QVariant answer = cycle143PageResult(view, QStringLiteral(
        "JSON.stringify(Array.from(document.querySelectorAll('.fw-misspelled')).map(function(e) { return e.textContent; }))"));
    QStringList words;
    for (const auto &value : QJsonDocument::fromJson(answer.toString().toUtf8()).array()) words << value.toString();
    return words;
}
}

void FomawriteTest::liveSpellingUnderlinesMisspelledWordsAndFollowsTheSetting() {
#ifndef Q_OS_MACOS
    QSKIP("the system spell checker is only available on macOS");
#endif
    Cycle103SettingsSnapshot settings;
    Backend backend;
    QSignalSpy asked(backend.editorBridge(), &EditorBridge::spellingRequested); // before the page exists: the first check is the one we want to see
    backend.spellCheck()->setLanguage(QStringLiteral("en_US"));
    backend.spellCheck()->setEngine(QStringLiteral("macos")); // this test is about the macOS checker (Harper: tests/cycle145-harper-engine.inc)
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    auto *editor = window->findChild<QQuickItem *>("sourceEditor"); QVERIFY(editor);
    quick->resize(1200, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    editor->setProperty("text", QStringLiteral("Their going too the shop tomorow. Also zxqvplumb.\n"));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    auto *bridge = backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 20000);
    auto *view = window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QTRY_VERIFY_WITH_TIMEOUT(!view->property("loading").toBool(), 15000);
    QVERIFY(backend.spellCheck()->enabled());

    // The page asks, the shared checker answers, and the word is wrapped in an underlined element. The word is a
    // nonsense one of our own: spellCheckServiceFindsWordsAndHonoursIgnoreAndSetting ignores "tomorow" for the rest of
    // the process (the system keeps ignored words per app), so that word is not reliably flagged later in a full run.
    QTRY_VERIFY_WITH_TIMEOUT(cycle143Underlined(view).contains(QStringLiteral("zxqvplumb")), 15000);
    QVERIFY(asked.count() >= 1);
    const QJsonArray segments = QJsonDocument::fromJson(asked.last().at(1).toString().toUtf8()).array();
    QVERIFY(!segments.isEmpty());
    QCOMPARE(segments.at(0).toObject().value("from").toInt(), 0);
    QVERIFY(segments.at(0).toObject().value("text").toString().startsWith(QStringLiteral("Their going too")));

    // Switching the setting off takes the underline away at once and the page stops asking.
    backend.spellCheck()->setEnabled(false);
    QTRY_VERIFY_WITH_TIMEOUT(cycle143Underlined(view).isEmpty(), 10000);
    const int askedWhileOff = asked.count();
    QTest::qWait(800);
    QCOMPARE(asked.count(), askedWhileOff);
    QVERIFY(cycle143Underlined(view).isEmpty());

    // And back on: it is checked again.
    backend.spellCheck()->setEnabled(true);
    QTRY_VERIFY_WITH_TIMEOUT(cycle143Underlined(view).contains(QStringLiteral("zxqvplumb")), 15000);

    // Ignoring the word (Ignore Spelling in the menu) re-checks the page: the underline goes.
    // Only the ignored word is asserted: "tomorow" is in the document too and
    // its state depends on whether another test ignored it earlier in the run.
    backend.spellCheck()->ignoreWord(QStringLiteral("zxqvplumb"));
    QTRY_VERIFY_WITH_TIMEOUT(!cycle143Underlined(view).contains(QStringLiteral("zxqvplumb")), 10000);
    backend.discardRecovery();
}
