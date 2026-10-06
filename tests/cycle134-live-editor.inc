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

void FomawriteTest::liveImagesResolveThroughTheHost() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    QImage image(6, 4, QImage::Format_RGB32); image.fill(Qt::green);
    QVERIFY(image.save(directory.filePath("pic.png")));
    QFile doc(directory.filePath("doc.md")); QVERIFY(doc.open(QIODevice::WriteOnly)); doc.write("![Pic](pic.png)\n"); doc.close();
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
    QScopedPointer<QObject> editor(component.create()); QVERIFY2(editor, qPrintable(component.errorString()));
    QVERIFY(backend.open(QUrl::fromLocalFile(directory.filePath("doc.md"))));
    auto *bridge = backend.editorBridge();
    QSignalSpy replies(bridge, &EditorBridge::imageReply);
    QVERIFY(QMetaObject::invokeMethod(bridge, "requestImage", Q_ARG(int, 1), Q_ARG(QString, "pic.png")));
    QCOMPARE(replies.count(), 1);
    QCOMPARE(replies.at(0).at(0).toInt(), 1);
    QVERIFY(replies.at(0).at(1).toString().startsWith("data:image/png;base64,"));
    QVERIFY(replies.at(0).at(2).toString().isEmpty());
    QVERIFY(QMetaObject::invokeMethod(bridge, "requestImage", Q_ARG(int, 2), Q_ARG(QString, "missing.png")));
    QCOMPARE(replies.at(1).at(1).toString(), QString());
    QVERIFY(!replies.at(1).at(2).toString().isEmpty());
    QVERIFY(QMetaObject::invokeMethod(bridge, "requestImage", Q_ARG(int, 3), Q_ARG(QString, "https://example.com/x.png")));
    QVERIFY(replies.at(2).at(2).toString().contains("remote"));
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

// Part 2 (real browser): the editor page inside a WebEngineView, driven over
// the bridge exactly as the app drives it.

namespace {
struct Cycle134Page {
    Backend backend;
    QQmlEngine engine;
    QScopedPointer<QObject> editor;   // the attached QTextDocument owner
    QScopedPointer<QObject> root;     // the LiveEditorPane harness
    QQuickWindow *window = nullptr;
    QString error;
    bool create() {
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent textComponent(&engine);
        textComponent.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
        editor.reset(textComponent.create());
        if (!editor) { error = textComponent.errorString(); return false; }
        QQmlComponent component(&engine);
        component.setData(R"QML(
import QtQuick
import QtQuick.Window
import "../src" as Fomawrite
Window {
    width: 900; height: 700; visible: true
    Fomawrite.LiveEditorPane { anchors.fill: parent; bridge: backend.editorBridge }
}
)QML", QUrl::fromLocalFile(QFINDTESTDATA("tst_fomawrite.cpp")));
        root.reset(component.create());
        if (!root) { error = component.errorString(); return false; }
        window = qobject_cast<QQuickWindow *>(root.data());
        return window != nullptr;
    }
    EditorBridge *bridge() { return backend.editorBridge(); }
    QString canonical() const {
        auto *quick = qvariant_cast<QQuickTextDocument *>(editor->property("textDocument"));
        QString t = quick->textDocument()->toRawText(); return t.replace(QChar(QChar::ParagraphSeparator), QLatin1Char('\n'));
    }
    QString pageText(int timeoutMs = 5000) {
        QSignalSpy received(bridge(), &EditorBridge::textReceived);
        const int token = bridge()->fetchText();
        QElapsedTimer timer; timer.start();
        while (timer.elapsed() < timeoutMs) {
            QTest::qWait(10);
            for (const auto &args : received) if (args.at(0).toInt() == token) return args.at(1).toString();
        }
        return QStringLiteral("<timeout>");
    }
};
}

void FomawriteTest::liveEditorPageMirrorsUserEditsByteExactly() {
    Cycle103SettingsSnapshot settings;
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QSignalSpy metrics(page.bridge(), &EditorBridge::metricRecorded);
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    const QString original = QString::fromUtf8("# Live\n\nh\xC3\xA9llo \xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9 **bold** and `code`\n\n- item\n");
    page.editor->setProperty("text", original);
    page.backend.syncLiveEditor();
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), original, 10000);
    // A user edit in the page: three changes in one transaction.
    QSignalSpy changes(page.bridge(), &EditorBridge::changesReceived);
    QElapsedTimer roundTrip; roundTrip.start();
    page.bridge()->injectUserChanges(QStringLiteral("[{\"from\":2,\"to\":6,\"insert\":\"Spike\"},{\"from\":%1,\"to\":%1,\"insert\":\"\\n- second \\u2728\"}]").arg(original.size() - 1));
    QTRY_VERIFY_WITH_TIMEOUT(changes.count() >= 1, 10000);
    const qint64 mirrorMs = roundTrip.elapsed();
    const QString expected = QStringLiteral("# Spike\n\n") + original.mid(8, original.size() - 9) + QStringLiteral("\n- second \u2728\n");
    QCOMPARE(page.canonical(), expected);
    QCOMPARE(page.pageText(), expected);
    qInfo("perf live: inject -> C++ mirror round trip %lld ms", static_cast<long long>(mirrorMs));
    // Undo in the page mirrors back; Source/Live switches keep one text.
    page.bridge()->requestUndo();
    QTRY_COMPARE_WITH_TIMEOUT(page.canonical(), original, 10000);
    QCOMPARE(page.pageText(), original);
    page.bridge()->selectMode("source");
    QTest::qWait(100);
    QCOMPARE(page.pageText(), original);
    page.bridge()->selectMode("live");
    QTest::qWait(100);
    QCOMPARE(page.pageText(), original);
    for (const auto &m : metrics) qInfo("perf live page metric %s = %.2f ms", qPrintable(m.at(0).toString()), m.at(1).toDouble());
    page.backend.discardRecovery();
}

void FomawriteTest::liveEditorRoundTripsPersistenceFixturesThroughThePage() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    struct Fixture { const char *name; QByteArray bytes; QByteArray eol; bool bom; };
    const QList<Fixture> fixtures = {
        {"crlf.md", "# Title\r\n\r\nline one\r\nline two\r\n", "\r\n", false},
        {"bom.md", "\xEF\xBB\xBF# Title\n\nbody\n", "\n", true},
        {"nbsp.md", "10\xC2\xA0km and \xC2\xAB quoted \xC2\xBB\n", "\n", false},
        {"graphemes.md", "e\xCC\x81 \xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x92\xBB \xE4\xBD\xA0\xE5\xA5\xBD\n", "\n", false},
    };
    for (const Fixture &fixture : fixtures) {
        const QString path = directory.filePath(QString::fromLatin1(fixture.name));
        { QFile f(path); QVERIFY(f.open(QIODevice::WriteOnly)); f.write(fixture.bytes); }
        QVERIFY2(page.backend.open(QUrl::fromLocalFile(path)), fixture.name);
        page.backend.syncLiveEditor();
        const QString loaded = page.canonical();
        QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), loaded, 10000);
        // Edit in the page, save from C++, compare bytes with the file's own conventions.
        QSignalSpy changes(page.bridge(), &EditorBridge::changesReceived);
        page.bridge()->injectUserChanges(QStringLiteral("[{\"from\":0,\"to\":0,\"insert\":\"Edited\\n\"}]"));
        QTRY_VERIFY_WITH_TIMEOUT(changes.count() >= 1, 10000);
        page.backend.save();
        QVERIFY2(!page.backend.modified(), qPrintable(page.backend.status()));
        QByteArray expected = fixture.bytes;
        if (fixture.bom) expected.remove(0, 3);
        expected.prepend("Edited" + fixture.eol);
        if (fixture.bom) expected.prepend("\xEF\xBB\xBF");
        QFile f(path); QVERIFY(f.open(QIODevice::ReadOnly)); QCOMPARE(f.readAll(), expected);
    }
    page.backend.discardRecovery();
}

void FomawriteTest::liveEditorLoadsLargeDocumentQuickly() {
    Cycle103SettingsSnapshot settings;
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    QString big; big.reserve(1100000);
    for (int i = 0; big.size() < 1048576; ++i) big += QStringLiteral("## Section %1\n\nParagraph %1 with **bold**, *em*, `code` and a [link](https://example.com/%1).\n\n").arg(i);
    page.editor->setProperty("text", big);
    QSignalSpy metrics(page.bridge(), &EditorBridge::metricRecorded);
    QElapsedTimer timer; timer.start();
    page.backend.syncLiveEditor();
    QTRY_VERIFY_WITH_TIMEOUT(([&] { for (const auto &m : metrics) if (m.at(0).toString() == "setDocument") return true; return false; })(), 30000);
    qInfo("perf live: 1 MiB document pushed and loaded in %lld ms (wall, includes channel transfer)", static_cast<long long>(timer.elapsed()));
    for (const auto &m : metrics) qInfo("perf live page metric %s = %.2f ms", qPrintable(m.at(0).toString()), m.at(1).toDouble());
    QSignalSpy changes(page.bridge(), &EditorBridge::changesReceived);
    QElapsedTimer edit; edit.start();
    page.bridge()->injectUserChanges(QStringLiteral("[{\"from\":0,\"to\":0,\"insert\":\"x\"}]"));
    QTRY_VERIFY_WITH_TIMEOUT(changes.count() >= 1, 10000);
    qInfo("perf live: one keystroke on 1 MiB mirrored in %lld ms", static_cast<long long>(edit.elapsed()));
    QVERIFY(page.canonical().startsWith("x## Section 0"));
    page.backend.discardRecovery();
}
