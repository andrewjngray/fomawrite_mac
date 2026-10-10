// Cycle145 — Harper, the offline grammar engine, runs inside the editor page's
// Chromium from the app's own resources (bin/fetch-harper puts the release
// pinned in harper.lock.json under qrc:/editor/dist/harper/).

namespace {
QVariant cycle145PageResult(QObject *view, const QString &script, int timeoutMs = 2000) {
    auto *engine = qmlEngine(view);
    if (!engine) return {};
    engine->globalObject().setProperty("cycle145PageResult", QJSValue(QJSValue::UndefinedValue));
    const QJSValue callback = engine->evaluate("(function(value) { cycle145PageResult = value; })");
    // World 0 (the page's own): the engine and its results are page globals, not DOM.
    if (!QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, script), Q_ARG(uint, 0), Q_ARG(QJSValue, callback))) return {};
    QElapsedTimer timer; timer.start();
    while (engine->globalObject().property("cycle145PageResult").isUndefined() && timer.elapsed() < timeoutMs) QTest::qWait(10);
    return engine->globalObject().property("cycle145PageResult").toVariant();
}
}

void FomawriteTest::harperLintsInsideTheEditorPage() {
    Cycle103SettingsSnapshot settings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    quick->resize(1200, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    auto *bridge = backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 20000);
    auto *view = window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QTRY_VERIFY_WITH_TIMEOUT(!view->property("loading").toBool(), 15000);
    // Kick the engine off and lint a sentence with planted errors; the page stores the answer.
    const QString text = QStringLiteral("This is is a test. We recieve the mail tomorow, their going too the shop. I could of gone and we was late. Colour and organise are fine.");
    QElapsedTimer timer; timer.start();
    QVERIFY(QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, QStringLiteral(
        "window.__harperResult = undefined; window.__harperError = undefined; try {"
        "window.fomawriteHarper.lint(%1, 'Australian').then(function(r) { window.__harperResult = JSON.stringify(r); window.__harperMs = performance.now(); })"
        ".catch(function(e) { window.__harperError = String(e && e.stack || e); }); } catch (e) { window.__harperError = 'sync: ' + String(e && e.stack || e); }").arg(QString::fromUtf8(QJsonDocument(QJsonArray{text}).toJson(QJsonDocument::Compact)).mid(1).chopped(1)))));
    QVariant result;
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        // One round trip per poll: the error (if any), the loader's stage, and the answer.
        const QVariant state = cycle145PageResult(view, QStringLiteral(
            "JSON.stringify({ error: window.__harperError || null, stage: window.__harperStage || '', result: window.__harperResult || null })"));
        const QJsonObject o = QJsonDocument::fromJson(state.toString().toUtf8()).object();
        if (!o.value("error").isNull()) { qWarning() << "harper error:" << o.value("error").toString() << "at stage" << o.value("stage").toString(); return true; }
        if (o.value("result").isNull()) return false;
        result = o.value("result").toString();
        return true;
    })(), 45000);
    qInfo("harper: first load + lint took %lld ms", timer.elapsed());
    QVERIFY2(!result.toString().isEmpty(), "the engine answered");
    const QJsonArray findings = QJsonDocument::fromJson(result.toString().toUtf8()).array();
    QStringList seen;
    for (const auto &entry : findings) {
        const auto f = entry.toObject();
        seen << QStringLiteral("%1:%2").arg(f.value("kind").toString(), text.mid(f.value("start").toInt(), f.value("end").toInt() - f.value("start").toInt()));
    }
    qInfo() << "harper findings:" << seen;
    QVERIFY(seen.contains(QStringLiteral("Spelling:recieve")));
    QVERIFY(seen.contains(QStringLiteral("Spelling:tomorow")));
    QVERIFY(seen.contains(QStringLiteral("Repetition:is is")));
    QVERIFY2(seen.filter(QStringLiteral(":too")).size() == 1, "their going too -> to");
    QVERIFY2(seen.filter(QStringLiteral(":could of")).size() == 1, "could of -> could have");
    QVERIFY2(seen.filter(QStringLiteral(":was")).size() == 1, "we was -> were");
    for (const QString &s : seen) QVERIFY2(!s.contains(QStringLiteral("Colour")) && !s.contains(QStringLiteral("organise")), "Australian spelling is not flagged");
    // A second lint is fast: the engine is loaded once.
    timer.restart();
    QVERIFY(QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, QStringLiteral(
        "window.__harperResult = undefined; window.fomawriteHarper.lint('We recieve it.').then(function(r) { window.__harperResult = JSON.stringify(r); });"))));
    QTRY_VERIFY_WITH_TIMEOUT(!cycle145PageResult(view, QStringLiteral("window.__harperResult || null")).toString().isEmpty(), 5000);
    qInfo("harper: warm lint round trip %lld ms", timer.elapsed());
    QVERIFY(timer.elapsed() < 1500);
    backend.discardRecovery();
}
