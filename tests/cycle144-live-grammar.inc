// Cycle144 — grammar underlines in the Live page. The reply to checkSpelling now
// carries a category per range; Grammar ranges (the checker's message and
// corrections attached) are drawn blue (.fw-grammar) next to the red spelling
// ones (.fw-misspelled). The grammar setting switches only the blue ones off.

namespace {
// The text of every element matching `selector` currently in the page.
QStringList cycle144Texts(QObject *view, const QString &selector) {
    const QVariant answer = cycle143PageResult(view, QStringLiteral(
        "JSON.stringify(Array.from(document.querySelectorAll('%1')).map(function(e) { return e.textContent; }))").arg(selector));
    QStringList texts;
    for (const auto &value : QJsonDocument::fromJson(answer.toString().toUtf8()).array()) texts << value.toString();
    return texts;
}
}

void FomawriteTest::liveGrammarUnderlinesFollowTheGrammarSetting() {
#ifndef Q_OS_MACOS
    QSKIP("the system spell checker is only available on macOS");
#endif
    Cycle103SettingsSnapshot settings;
    Backend backend;
    backend.spellCheck()->setLanguage(QStringLiteral("en_US"));
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    auto *editor = window->findChild<QQuickItem *>("sourceEditor"); QVERIFY(editor);
    quick->resize(1200, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    editor->setProperty("text", QStringLiteral("This is is a test. We recieve the mail.\n"));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    auto *bridge = backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 20000);
    auto *view = window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QTRY_VERIFY_WITH_TIMEOUT(!view->property("loading").toBool(), 15000);
    QVERIFY(backend.spellCheck()->enabled());
    QVERIFY(backend.spellCheck()->grammarEnabled());

    // Both kinds arrive in one reply: the doubled word is blue, the misspelling red.
    QTRY_VERIFY_WITH_TIMEOUT(!cycle144Texts(view, QStringLiteral(".fw-grammar")).isEmpty(), 15000);
    QTRY_VERIFY_WITH_TIMEOUT(cycle144Texts(view, QStringLiteral(".fw-misspelled")).contains(QStringLiteral("recieve")), 15000);
    QVERIFY(cycle144Texts(view, QStringLiteral(".fw-grammar")).join(QString()).contains(QStringLiteral("is is")));

    // The page holds the checker's message and corrections on the mark itself: a computed colour proves the CSS applied.
    const QVariant colour = cycle143PageResult(view, QStringLiteral(
        "getComputedStyle(document.querySelector('.fw-grammar')).textDecorationColor"));
    QCOMPARE(colour.toString(), QStringLiteral("rgb(26, 115, 232)"));

    // Grammar off: the blue mark goes at once, the red one stays.
    backend.spellCheck()->setGrammarEnabled(false);
    QTRY_VERIFY_WITH_TIMEOUT(cycle144Texts(view, QStringLiteral(".fw-grammar")).isEmpty(), 10000);
    QTRY_VERIFY_WITH_TIMEOUT(cycle144Texts(view, QStringLiteral(".fw-misspelled")).contains(QStringLiteral("recieve")), 10000);
    QTest::qWait(800); // the re-check that setSpellCheck triggers must not bring grammar back
    QVERIFY(cycle144Texts(view, QStringLiteral(".fw-grammar")).isEmpty());
    QVERIFY(cycle144Texts(view, QStringLiteral(".fw-misspelled")).contains(QStringLiteral("recieve")));

    // And back on: the blue mark returns.
    backend.spellCheck()->setGrammarEnabled(true);
    QTRY_VERIFY_WITH_TIMEOUT(!cycle144Texts(view, QStringLiteral(".fw-grammar")).isEmpty(), 15000);
    QVERIFY(cycle144Texts(view, QStringLiteral(".fw-misspelled")).contains(QStringLiteral("recieve")));
    backend.discardRecovery();
}
