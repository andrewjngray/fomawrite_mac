// Cycle134 — the Live editor spike. Part 1 (no browser): the bridge applies
// the page's change lists to the canonical QTextDocument byte-exactly and
// replays app state to a page that connects late.

namespace {
QString cycle134ApplyReference(QString text, const QJsonArray &changes) {
    // Reference implementation of the contract: pre-change offsets, ascending,
    // applied in reverse so no offset adjustment is needed.
    for (int i = changes.size() - 1; i >= 0; --i) {
        const auto change = changes[i].toObject();
        text.replace(change["from"].toInt(), change["to"].toInt() - change["from"].toInt(), change["insert"].toString());
    }
    return text;
}
}

void FomawriteTest::liveChangesApplyToDocumentByteExactly() {
    Cycle103SettingsSnapshot settings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
    QScopedPointer<QObject> editor(component.create()); QVERIFY2(editor, qPrintable(component.errorString()));
    auto *quick = qvariant_cast<QQuickTextDocument *>(editor->property("textDocument")); QVERIFY(quick);
    const auto canonical = [&] { QString t = quick->textDocument()->toRawText(); return t.replace(QChar(QChar::ParagraphSeparator), QLatin1Char('\n')); };
    const QString original = QString::fromUtf8("h\xC3\xA9llo \xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9 w\xC3\xB6rld\nsecond line\n");
    editor->setProperty("text", original);
    QCOMPARE(canonical(), original);
    auto *bridge = backend.editorBridge(); QVERIFY(bridge);
    QSignalSpy revision(bridge, SIGNAL(revisionChanged()));

    // Three changes in one transaction: replace, insert inside a grapheme-safe
    // position, and a newline insertion, exactly as the page would send them.
    const QJsonArray changes{QJsonObject{{"from", 0}, {"to", 1}, {"insert", "H"}},
                             QJsonObject{{"from", 6}, {"to", 6}, {"insert", QString::fromUtf8("\xE2\x9C\xA8 ")}},
                             QJsonObject{{"from", int(original.size()) - 1}, {"to", int(original.size()) - 1}, {"insert", "\nthird"}}};
    const QString json = QString::fromUtf8(QJsonDocument(changes).toJson(QJsonDocument::Compact));
    QVERIFY(QMetaObject::invokeMethod(bridge, "documentChanged", Q_ARG(QString, json), Q_ARG(int, 7)));
    QCOMPARE(canonical(), cycle134ApplyReference(original, changes));
    QCOMPARE(bridge->property("revision").toInt(), 7);
    QVERIFY(backend.modified());
    QCOMPARE(revision.count(), 1);

    // Out-of-range and malformed lists are refused as a whole; nothing changes.
    const QString before = canonical();
    QSignalSpy logged(bridge, SIGNAL(messageLogged(QString)));
    for (const QString &bad : {QStringLiteral("[{\"from\":0,\"to\":99999,\"insert\":\"x\"}]"),
                               QStringLiteral("[{\"from\":5,\"to\":2,\"insert\":\"x\"}]"),
                               QStringLiteral("not json"),
                               QStringLiteral("[{\"from\":-1,\"to\":0,\"insert\":\"x\"}]")}) {
        QVERIFY(QMetaObject::invokeMethod(bridge, "documentChanged", Q_ARG(QString, bad), Q_ARG(int, 8)));
        QCOMPARE(canonical(), before);
    }
    QVERIFY(logged.count() >= 4);

    // Undo on the document side still works for mirrored edits (the editor's
    // own history is authoritative while Live is active; this is the fallback).
    QVERIFY(QMetaObject::invokeMethod(editor.data(), "undo"));
    QCOMPARE(canonical(), original);
    backend.discardRecovery();
}

void FomawriteTest::liveBridgeReplaysStateToLatePage() {
    EditorBridge bridge;
    QSignalSpy document(&bridge, &EditorBridge::setDocument), mode(&bridge, &EditorBridge::setMode),
               theme(&bridge, &EditorBridge::setTheme), appearance(&bridge, &EditorBridge::setAppearance);
    bridge.loadDocument("# Late\n");
    bridge.selectMode("source");
    bridge.applyTheme("#write{color:red}");
    bridge.applyAppearance("{\"fontSize\":17}");
    // Nothing reaches a page that is not connected yet.
    QCOMPARE(document.count(), 0); QCOMPARE(mode.count(), 0); QCOMPARE(theme.count(), 0); QCOMPARE(appearance.count(), 0);
    QVERIFY(!bridge.isReady());
    bridge.ready();
    QVERIFY(bridge.isReady());
    QCOMPARE(document.count(), 1); QCOMPARE(document.first().at(0).toString(), QString("# Late\n"));
    QCOMPARE(document.first().at(1).toInt(), bridge.revision());
    QCOMPARE(mode.count(), 1); QCOMPARE(mode.first().at(0).toString(), QString("source"));
    QCOMPARE(theme.count(), 1); QCOMPARE(appearance.count(), 1);
    // Once connected, pushes go straight through.
    bridge.loadDocument("# Now\n");
    QCOMPARE(document.count(), 2);
    const int token = bridge.fetchText();
    QSignalSpy requested(&bridge, &EditorBridge::requestText);
    QVERIFY(token > 0);
    QSignalSpy received(&bridge, &EditorBridge::textReceived);
    bridge.textReply(token, "# Now\n");
    QCOMPARE(received.count(), 1); QCOMPARE(received.first().at(0).toInt(), token);
}
