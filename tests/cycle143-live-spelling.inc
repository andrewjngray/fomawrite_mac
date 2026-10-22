// Cycle143 — spelling in the Live page. The page decides which text is prose,
// the host's shared SpellCheck service decides which words are misspelled, and
// the page underlines them (.fw-misspelled). The setting switches it off.

namespace {
// Evaluates `script` in the Live page and returns its (string) answer, or a null
// QVariant when none arrives in time. The callback is scoped to this engine.
QVariant cycle143PageResult(QObject *view, const QString &script, uint world = 1) {
    auto *engine = qmlEngine(view);
    if (!engine) return {};
    engine->globalObject().setProperty("cycle143PageResult", QJSValue(QJSValue::UndefinedValue));
    const QJSValue callback = engine->evaluate("(function(value) { cycle143PageResult = value; })");
    if (!QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, script), Q_ARG(uint, world), Q_ARG(QJSValue, callback))) return {};
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
QStringList cycle143MarkedLines(QObject *view) {
    const QVariant answer = cycle143PageResult(view, QStringLiteral(
        "JSON.stringify(Array.from(document.querySelectorAll('.fw-misspelled')).map(function(e) { return e.closest('.cm-line').textContent; }))"));
    QStringList lines;
    for (const auto &value : QJsonDocument::fromJson(answer.toString().toUtf8()).array()) lines << value.toString();
    return lines;
}
// JavaScript only reads the real rendered element's rectangle. The actual
// context-menu/correction handlers receive Qt mouse input through WebEngine.
bool cycle143ClickLiveElement(QQuickWindow *window, QObject *view, const QString &selector,
                              const QString &text = QString(), Qt::MouseButton button = Qt::LeftButton) {
    const auto quote = [](const QString &value) {
        return QString::fromUtf8(QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
    };
    const QString script = QStringLiteral("(function() { var e=Array.from(document.querySelectorAll(%1)).find(function(x) { return %2 === '' || x.textContent === %2; }); if (!e) return '[]'; var r=e.getBoundingClientRect(); return JSON.stringify([r.left+r.width/2,r.top+r.height/2,r.width,r.height]); })()").arg(quote(selector), quote(text));
    const QJsonArray box = QJsonDocument::fromJson(cycle143PageResult(view, script).toString().toUtf8()).array();
    auto *item = qobject_cast<QQuickItem *>(view);
    if (!item || box.size() != 4 || box.at(2).toDouble() <= 0 || box.at(3).toDouble() <= 0) return false;
    const QPoint point = item->mapToScene(QPointF(box.at(0).toDouble(), box.at(1).toDouble())).toPoint();
    if (!QRect(QPoint(), window->size()).contains(point)) return false;
    QTest::mouseClick(window, button, Qt::NoModifier, point);
    return true;
}
}

void FomawriteTest::liveSpellingUnderlinesMisspelledWordsAndFollowsTheSetting_data() { cycle143SpellingCheckerRows(); }

void FomawriteTest::liveSpellingUnderlinesMisspelledWordsAndFollowsTheSetting() {
    QFETCH(bool, bundledHarper);
#ifndef Q_OS_MACOS
    if (!bundledHarper) QSKIP("the system spell checker is only available on macOS");
#endif
    Cycle103SettingsSnapshot settings;
    Backend backend;
    QSignalSpy asked(backend.editorBridge(), &EditorBridge::spellingRequested); // before the page exists: the first check is the one we want to see
    backend.spellCheck()->setLanguage(QStringLiteral("en_US"));
    backend.spellCheck()->setEngine(bundledHarper ? QStringLiteral("harper") : QStringLiteral("macos"));
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
    if (bundledHarper) {
        QTRY_VERIFY_WITH_TIMEOUT(backend.spellCheck()->harperReady(), 60000);
        QVERIFY(!HarperSchemeHandler::bundledVersion().isEmpty());
        QCOMPARE(HarperSchemeHandler::servedVersion(), HarperSchemeHandler::bundledVersion());
        QCOMPARE(backend.spellCheck()->harperVersion(), HarperSchemeHandler::bundledVersion());
        QCOMPARE(backend.spellCheck()->effectiveEngine(), QStringLiteral("harper"));
    }

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
    if (bundledHarper) {
        // Source and Live apply their Markdown exclusions before asking Harper.
        // Each positive context must survive; matching just a count would let
        // an erroneous code underline substitute for a missed prose word.
        const QString prose = QStringLiteral(
            "# We recieve parcels\n\nWe recieve mail and receive more.\n\n"
            "`recieve` and https://example.com/recieve\n\n"
            "```\nrecieve\n```\n\n    recieve\n\n- List item recieve\n");
        QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("source"))));
        editor->setProperty("text", prose);
        QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
        bridge->placeCursor(0);
        QTRY_COMPARE_WITH_TIMEOUT(cycle143Underlined(view).count(QStringLiteral("recieve")), 3, 15000);
        // Verify the actual prose request excludes code/URLs, so a delayed
        // response cannot masquerade as a correctly unmarked literal region.
        QVERIFY(!asked.isEmpty());
        const QJsonArray actualSegments = QJsonDocument::fromJson(asked.last().at(1).toString().toUtf8()).array();
        int requestedTypos = 0;
        for (const auto &value : actualSegments) {
            const QString text = value.toObject().value("text").toString();
            requestedTypos += text.count(QStringLiteral("recieve"));
            QVERIFY(!text.contains(QStringLiteral("https://")));
        }
        QCOMPARE(requestedTypos, 3);
        const QStringList marked = cycle143MarkedLines(view);
        QCOMPARE(marked.size(), 3);
        QVERIFY(marked.at(0).contains(QStringLiteral("We recieve parcels")));
        QVERIFY(marked.at(1).contains(QStringLiteral("We recieve mail and receive more.")));
        QVERIFY(marked.at(2).contains(QStringLiteral("List item recieve")));
        QVERIFY(!cycle143Underlined(view).contains(QStringLiteral("receive")));
        QCOMPARE(editor->property("text").toString(), prose);

        // A known finding is withheld only while the caret ends that word.
        // Cursor movement must not edit the canonical document or revision.
        const int first = prose.indexOf(QStringLiteral("recieve"));
        const int revision = backend.documentRevision();
        // The Live caret rule applies when a lint reply arrives. Recheck the
        // known findings at this caret, rather than assume moving onto an
        // already-drawn mark starts a new check.
        backend.spellCheck()->setEnabled(false);
        QTRY_VERIFY(cycle143Underlined(view).isEmpty());
        bridge->placeCursor(first + 7);
        QTRY_COMPARE(cycle143PageResult(view, QStringLiteral("window.fomawriteEditor.view.state.selection.main.head"), 0).toInt(), first + 7);
        backend.spellCheck()->setEnabled(true);
        QTRY_COMPARE_WITH_TIMEOUT(cycle143Underlined(view).count(QStringLiteral("recieve")), 2, 5000);
        QVERIFY(!cycle143MarkedLines(view).join(QLatin1Char('\n')).contains(QStringLiteral("We recieve parcels")));
        QCOMPARE(backend.documentRevision(), revision);
        bridge->placeCursor(0);
        QTRY_COMPARE_WITH_TIMEOUT(cycle143Underlined(view).count(QStringLiteral("recieve")), 3, 5000);
        QCOMPARE(backend.documentRevision(), revision);

        QVERIFY(cycle143ClickLiveElement(quick, view, QStringLiteral(".fw-misspelled"), QStringLiteral("recieve"), Qt::RightButton));
        QTRY_VERIFY_WITH_TIMEOUT(cycle143PageResult(view, QStringLiteral("document.querySelector('.fw-spell-menu') !== null")).toBool(), 5000);
        const auto menuWords = [&] {
            QStringList words;
            const auto answer = cycle143PageResult(view, QStringLiteral("JSON.stringify(Array.from(document.querySelectorAll('.fw-spell-suggestion')).map(function(e) { return e.textContent; }))"));
            for (const auto &value : QJsonDocument::fromJson(answer.toString().toUtf8()).array()) words << value.toString();
            return words;
        };
        QTRY_VERIFY_WITH_TIMEOUT(menuWords().contains(QStringLiteral("receive")), 5000);
        QVERIFY(menuWords().size() <= 5);
        QCOMPARE(cycle143PageResult(view, QStringLiteral("document.querySelector('.fw-spell-learn').textContent")).toString(), QStringLiteral("Learn Spelling"));
        QCOMPARE(cycle143PageResult(view, QStringLiteral("document.querySelector('.fw-spell-ignore').textContent")).toString(), QStringLiteral("Ignore Spelling"));
        QVERIFY(cycle143ClickLiveElement(quick, view, QStringLiteral(".fw-spell-suggestion"), QStringLiteral("receive")));
        QString corrected = prose; corrected.replace(first, 7, QStringLiteral("receive"));
        QTRY_COMPARE_WITH_TIMEOUT(editor->property("text").toString(), corrected, 5000);
        bridge->requestUndo();
        QTRY_COMPARE_WITH_TIMEOUT(editor->property("text").toString(), prose, 5000);
    }
    backend.discardRecovery();
}
