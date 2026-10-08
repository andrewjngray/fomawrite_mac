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

void FomawriteTest::liveImagesSaveBesideTheDocument() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
    QScopedPointer<QObject> editor(component.create()); QVERIFY2(editor, qPrintable(component.errorString()));
    QImage image(10, 6, QImage::Format_RGB32); image.fill(Qt::magenta);
    QByteArray png; { QBuffer buffer(&png); buffer.open(QIODevice::WriteOnly); QVERIFY(image.save(&buffer, "PNG")); }
    auto *bridge = backend.editorBridge();
    QSignalSpy saved(bridge, &EditorBridge::imageSaved);
    // Untitled document: refused with a reason.
    QVERIFY(QMetaObject::invokeMethod(bridge, "saveImage", Q_ARG(int, 1), Q_ARG(QString, "shot.png"), Q_ARG(QString, "image/png"), Q_ARG(QString, QString::fromLatin1(png.toBase64()))));
    QCOMPARE(saved.count(), 1); QVERIFY(saved.at(0).at(1).toString().isEmpty()); QVERIFY(saved.at(0).at(2).toString().contains("Save the document"));
    // Saved document: stored under images/ with a unique, safe name.
    editor->setProperty("text", "# Doc\n");
    backend.saveAs(QUrl::fromLocalFile(directory.filePath("doc.md")));
    QVERIFY(!backend.modified());
    QVERIFY(QMetaObject::invokeMethod(bridge, "saveImage", Q_ARG(int, 2), Q_ARG(QString, "My Shot (1).png"), Q_ARG(QString, "image/png"), Q_ARG(QString, QString::fromLatin1(png.toBase64()))));
    QCOMPARE(saved.count(), 2);
    QCOMPARE(saved.at(1).at(1).toString(), QString("images/My-Shot-1.png"));
    QVERIFY(saved.at(1).at(2).toString().isEmpty());
    QVERIFY(QFileInfo::exists(directory.filePath("images/My-Shot-1.png")));
    QVERIFY(QMetaObject::invokeMethod(bridge, "saveImage", Q_ARG(int, 3), Q_ARG(QString, "My Shot (1).png"), Q_ARG(QString, "image/png"), Q_ARG(QString, QString::fromLatin1(png.toBase64()))));
    QCOMPARE(saved.at(2).at(1).toString(), QString("images/My-Shot-1-2.png"));
    // Bad data and bad types are refused and nothing is written.
    QVERIFY(QMetaObject::invokeMethod(bridge, "saveImage", Q_ARG(int, 4), Q_ARG(QString, "x.png"), Q_ARG(QString, "image/png"), Q_ARG(QString, QString::fromLatin1(QByteArray("not an image").toBase64()))));
    QVERIFY(!saved.at(3).at(2).toString().isEmpty());
    QVERIFY(QMetaObject::invokeMethod(bridge, "saveImage", Q_ARG(int, 5), Q_ARG(QString, "x.svg"), Q_ARG(QString, "image/svg+xml"), Q_ARG(QString, QString::fromLatin1(png.toBase64()))));
    QVERIFY(!saved.at(4).at(2).toString().isEmpty());
    QCOMPARE(QDir(directory.filePath("images")).entryList(QDir::Files).size(), 2);
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
    QElapsedTimer pageStart; pageStart.start();
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QSignalSpy metrics(page.bridge(), &EditorBridge::metricRecorded);
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    qInfo("perf live: editor page created, loaded and connected in %lld ms (first page in this process)", static_cast<long long>(pageStart.elapsed()));
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

void FomawriteTest::liveEditorFollowsHostEdits() {
    Cycle103SettingsSnapshot settings;
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    const QString original = QString::fromUtf8("# Host\n\nalpha beta gamma\n");
    page.editor->setProperty("text", original);
    page.backend.syncLiveEditor();
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), original, 10000);
    QSignalSpy pushed(page.bridge(), &EditorBridge::applyChanges);
    // A change made on the C++ side, as a Format command or Replace would make it.
    auto *quick = qvariant_cast<QQuickTextDocument *>(page.editor->property("textDocument"));
    QTextCursor cursor(quick->textDocument());
    cursor.setPosition(8); cursor.setPosition(13, QTextCursor::KeepAnchor); cursor.insertText("**alpha**");
    const QString expected = QString::fromUtf8("# Host\n\n**alpha** beta gamma\n");
    QCOMPARE(page.canonical(), expected);
    QTRY_COMPARE_WITH_TIMEOUT(pushed.count(), 1, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), expected, 10000);
    // A format-only change (authorship label) must not be pushed as text.
    QTextCursor label(quick->textDocument()); label.setPosition(10); label.setPosition(15, QTextCursor::KeepAnchor);
    QTextCharFormat format; format.setFontItalic(true); label.mergeCharFormat(format);
    QTest::qWait(300);
    QCOMPARE(pushed.count(), 1);
    QCOMPARE(page.pageText(), expected);
    // And a page edit after a host edit still mirrors (offsets agree).
    QSignalSpy changes(page.bridge(), &EditorBridge::changesReceived);
    page.bridge()->injectUserChanges(QStringLiteral("[{\"from\":0,\"to\":0,\"insert\":\"> \"}]"));
    QTRY_VERIFY_WITH_TIMEOUT(changes.count() >= 1, 10000);
    QCOMPARE(page.canonical(), "> " + expected);
    QCOMPARE(page.pageText(), "> " + expected);
    page.backend.discardRecovery();
}

void FomawriteTest::liveEditorPlacesCursorAndRestylesWithoutReload() {
    Cycle103SettingsSnapshot settings;
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    const QString text = QString::fromUtf8("# One\n\ntwo three four\n\nfive\n");
    page.editor->setProperty("text", text);
    QSignalSpy cursor(page.bridge(), &EditorBridge::cursorMoved);
    page.backend.syncLiveEditor(12);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), text, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->lastCursor() == 12, 5000);
    QCOMPARE(page.backend.liveCursor(), 12);
    // A theme change re-styles the page but must not reload the document.
    QSignalSpy documents(page.bridge(), &EditorBridge::setDocument), themes(page.bridge(), &EditorBridge::setTheme);
    QVERIFY(page.backend.selectPublishingTheme("claude-like"));
    page.backend.pushLiveTheme();
    QCOMPARE(documents.count(), 0);
    // The pane re-styles on publishingCssChanged and the explicit push adds one more; never a reload.
    QVERIFY(themes.count() >= 1);
    QVERIFY(themes.first().at(0).toString().contains("#write"));
    QCOMPARE(page.bridge()->lastCursor(), 12);
    page.backend.discardRecovery();
}

void FomawriteTest::liveEditorAppliesFormatCommandsAtThePageSelection() {
    Cycle103SettingsSnapshot settings;
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    const QString text = QString::fromUtf8("Title line\n\nalpha beta gamma\n");
    page.editor->setProperty("text", text);
    page.backend.syncLiveEditor(12); // caret at "alpha|"? no: offset 12 = start of "alpha"
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), text, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->lastCursor() == 12, 5000);
    // Wrap at a collapsed selection: markers inserted, caret between them, page follows.
    QVERIFY(page.backend.liveWrapSelection("**", "**")); // accepted; the edit follows the page's reply
    const QString wrapped = QString::fromUtf8("Title line\n\n****alpha beta gamma\n");
    QTRY_COMPARE_WITH_TIMEOUT(page.canonical(), wrapped, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), wrapped, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->lastCursor() == 14, 5000);
    // A block command on the first line.
    page.bridge()->placeCursor(2);
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->lastCursor() == 2, 5000);
    QVERIFY(page.backend.liveEditMarkdown("heading2"));
    QTRY_VERIFY_WITH_TIMEOUT(page.canonical().startsWith("## Title line"), 5000);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), page.canonical(), 10000);
    // Snippet replacement at the caret.
    QVERIFY(page.backend.liveReplaceSelection("[snippet]"));
    QTRY_VERIFY_WITH_TIMEOUT(page.canonical().contains("[snippet]"), 5000);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), page.canonical(), 10000);
    // Without a known selection nothing happens.
    EditorBridge fresh; Q_UNUSED(fresh);
    page.backend.discardRecovery();
}

// The page reports its selection 30 ms after it changes; a Format command in
// that window used to act on the previous selection. Now it asks the page.
void FomawriteTest::liveFormatCommandUsesTheSelectionMadeJustBefore() {
    Cycle103SettingsSnapshot settings;
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    const QString text = QStringLiteral("one two three");
    page.editor->setProperty("text", text);
    page.backend.syncLiveEditor(0);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), text, 10000);
    QTest::qWait(200); // let the setDocument/setCursor reports settle on the caret at 0
    QCOMPARE(page.bridge()->lastSelectionStart(), 0);
    QCOMPARE(page.bridge()->lastSelectionEnd(), 0);
    auto *view = page.root->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QVERIFY(QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, QStringLiteral(
        "window.fomawriteEditor.view.dispatch({selection:{anchor:4,head:7}})"))));
    // No wait: the page's debounced report has not reached the host yet.
    QVERIFY(page.backend.liveWrapSelection("**", "**"));
    const QString wrapped = QStringLiteral("one **two** three");
    QTRY_COMPARE_WITH_TIMEOUT(page.canonical(), wrapped, 10000);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), wrapped, 10000);
    page.backend.discardRecovery();
}

// A page that never answers must not swallow the command: after a second the
// last reported selection is used. (The page is stood in for by a bare bridge
// that is ready but silent.)
void FomawriteTest::liveSelectionRequestFallsBackWhenThePageDoesNotAnswer() {
    Cycle103SettingsSnapshot settings;
    Backend backend;
    QQmlEngine engine;
    QQmlComponent textComponent(&engine);
    engine.rootContext()->setContextProperty("backend", &backend);
    textComponent.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
    QScopedPointer<QObject> editor(textComponent.create()); QVERIFY2(editor, qPrintable(textComponent.errorString()));
    editor->setProperty("text", QStringLiteral("one two three"));
    auto *bridge = backend.editorBridge();
    QSignalSpy asked(bridge, &EditorBridge::requestSelection);
    bridge->ready(); // connected, but nothing answers requestSelection
    backend.syncLiveEditor(0);
    bridge->cursorChanged(4, 7, true);
    QVERIFY(backend.liveWrapSelection("**", "**"));
    QCOMPARE(asked.count(), 1);
    QCOMPARE(editor->property("text").toString(), QStringLiteral("one two three")); // not applied yet
    QTRY_COMPARE_WITH_TIMEOUT(editor->property("text").toString(), QStringLiteral("one **two** three"), 3000);
    backend.discardRecovery();
}

// Whole stack: a synthetic paste of a real PNG in the page goes to the host,
// is written beside the document, and comes back as a Markdown image.
void FomawriteTest::liveEditorPastesImagesThroughTheHost() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    page.editor->setProperty("text", QStringLiteral("before after\n"));
    page.backend.saveAs(QUrl::fromLocalFile(directory.filePath("notes.md")));
    QVERIFY(!page.backend.modified());
    page.backend.syncLiveEditor(0);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), QStringLiteral("before after\n"), 10000);
    QImage image(12, 8, QImage::Format_RGB32); image.fill(Qt::darkCyan);
    QByteArray png; { QBuffer buffer(&png); buffer.open(QIODevice::WriteOnly); QVERIFY(image.save(&buffer, "PNG")); }
    auto *view = page.root->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QSignalSpy saved(page.bridge(), &EditorBridge::imageSaved);
    // Place the caret after "before " then dispatch a paste carrying the file.
    page.bridge()->placeCursor(7);
    QTRY_COMPARE_WITH_TIMEOUT(page.bridge()->lastCursor(), 7, 5000);
    const QString script = QStringLiteral(R"JS(
        (function() {
            const bytes = Uint8Array.from(atob("%1"), c => c.charCodeAt(0));
            const file = new File([bytes], "Screen Shot.png", { type: "image/png" });
            const dt = new DataTransfer(); dt.items.add(file);
            const target = document.querySelector(".cm-content");
            target.dispatchEvent(new ClipboardEvent("paste", { clipboardData: dt, bubbles: true, cancelable: true }));
            return "dispatched";
        })())JS").arg(QString::fromLatin1(png.toBase64()));
    QVERIFY(QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, script)));
    QTRY_COMPARE_WITH_TIMEOUT(saved.count(), 1, 10000);
    QCOMPARE(saved.at(0).at(1).toString(), QStringLiteral("images/Screen-Shot.png"));
    QVERIFY(saved.at(0).at(2).toString().isEmpty());
    QVERIFY(QFileInfo::exists(directory.filePath("images/Screen-Shot.png")));
    const QString expected = QStringLiteral("before ![Screen-Shot](images/Screen-Shot.png)after\n");
    QTRY_COMPARE_WITH_TIMEOUT(page.canonical(), expected, 10000);
    QCOMPARE(page.pageText(), expected);
    // The host resolves the new relative path for the page's inline preview.
    QSignalSpy replied(page.bridge(), &EditorBridge::imageReply);
    QVERIFY(QMetaObject::invokeMethod(page.bridge(), "requestImage", Q_ARG(int, 77), Q_ARG(QString, "images/Screen-Shot.png")));
    QTRY_COMPARE_WITH_TIMEOUT(replied.count(), 1, 5000);
    QVERIFY(replied.at(0).at(1).toString().startsWith("data:image/png;base64,"));
    page.backend.discardRecovery();
}

// Insert Link in Live opens the page's own link panel; filling it writes a
// Markdown link into the canonical document.
void FomawriteTest::liveEditorInsertsLinksThroughThePagePanel() {
    Cycle103SettingsSnapshot settings;
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    const QString text = QStringLiteral("see docs now\n");
    page.editor->setProperty("text", text);
    page.backend.syncLiveEditor(0);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), text, 10000);
    auto *view = page.root->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    // Caret before "docs": open the panel through the host command, fill it and apply.
    QVERIFY(QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, QStringLiteral(
        "document.querySelector('.cm-content').focus();"))));
    page.bridge()->placeCursor(4);
    QTRY_COMPARE_WITH_TIMEOUT(page.bridge()->lastCursor(), 4, 5000);
    page.bridge()->runCommand("link");
    const QString fill = QStringLiteral(R"JS(
        (function() {
            const panel = document.querySelector('.fw-link-panel'); if (!panel) return 'no panel';
            const set = (name, value) => { const i = panel.querySelector('input[name=' + name + ']'); i.value = value; i.dispatchEvent(new Event('input', {bubbles: true})); };
            set('label', 'the docs'); set('url', 'https://example.org/a b'); set('title', 'Docs');
            panel.querySelector('button[name=apply]').click();
            return 'applied';
        })())JS");
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, fill)); QTest::qWait(100);
        return page.canonical().contains("](https://example.org/a%20b");
    })(), 10000);
    QCOMPARE(page.canonical(), QStringLiteral("see [the docs](https://example.org/a%20b \"Docs\")docs now\n"));
    QCOMPARE(page.pageText(), page.canonical());
    // The panel is gone and the page's undo removes the link as one step.
    page.bridge()->requestUndo();
    QTRY_COMPARE_WITH_TIMEOUT(page.canonical(), text, 5000);
    page.backend.discardRecovery();
}

// QTextDocument reports replacing an empty document as removing one character
// (its final block). Forwarding that literally made the page refuse the change
// and keep stale text until the next full sync. Red on the old code.
void FomawriteTest::liveEditorRecoversFromOutOfRangeHostChanges() {
    Cycle103SettingsSnapshot settings;
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    page.backend.syncLiveEditor(0);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), QString(), 10000);
    QSignalSpy logged(page.bridge(), &EditorBridge::messageLogged);
    page.editor->setProperty("text", QStringLiteral("hello"));
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), QStringLiteral("hello"), 10000);
    for (const auto &args : logged) QVERIFY2(!args.at(0).toString().contains("applyChanges failed"), qPrintable(args.at(0).toString()));
    // Offsets still agree afterwards: a page edit lands at the right place.
    QVERIFY(QMetaObject::invokeMethod(page.bridge(), "documentChanged", Q_ARG(QString, QStringLiteral("[{\"from\":5,\"to\":5,\"insert\":\" there\"}]")), Q_ARG(int, 1)));
    QTRY_COMPARE_WITH_TIMEOUT(page.canonical(), QStringLiteral("hello there"), 5000);
    page.backend.discardRecovery();
}

// Copy Markdown/HTML in Live use the page's selection; nothing is copied without one.
void FomawriteTest::liveEditorCopiesThePageSelection() {
    Cycle103SettingsSnapshot settings;
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    const QString text = QStringLiteral("# Title\n\nsome *words*\n");
    page.editor->setProperty("text", text);
    page.backend.syncLiveEditor(0);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), text, 10000);
    QGuiApplication::clipboard()->setText(QStringLiteral("untouched"));
    // The request is accepted and answered by the page; a collapsed selection copies nothing.
    QSignalSpy replies(page.bridge(), &EditorBridge::selectionReceived);
    QVERIFY(page.backend.liveCopySelection("markdown"));
    QTRY_COMPARE_WITH_TIMEOUT(replies.count(), 1, 5000);
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("untouched"));
    page.bridge()->runCommand("selectAll");
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->lastSelectionEnd() == text.size(), 5000);
    QVERIFY(page.backend.liveCopySelection("markdown"));
    QTRY_COMPARE_WITH_TIMEOUT(QGuiApplication::clipboard()->text(), text, 5000);
    QVERIFY(page.backend.liveCopySelection("html"));
    QTRY_VERIFY_WITH_TIMEOUT(QGuiApplication::clipboard()->text().contains("italic"), 5000);
    QVERIFY(page.backend.liveCopySelection("formatted"));
    QTRY_VERIFY_WITH_TIMEOUT(QGuiApplication::clipboard()->mimeData()->hasHtml(), 5000);
    QCOMPARE(QGuiApplication::clipboard()->mimeData()->text().trimmed(), QStringLiteral("Title\nsome words"));
    page.backend.discardRecovery();
}

void FomawriteTest::liveEditorRunsHostCommands() {
    Cycle103SettingsSnapshot settings;
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    const QString text = QString::fromUtf8("one two three\n");
    page.editor->setProperty("text", text);
    page.backend.syncLiveEditor(0);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), text, 10000);
    // selectAll: the page reports the whole document as the selection.
    QSignalSpy cursor(page.bridge(), &EditorBridge::cursorMoved);
    page.bridge()->runCommand("selectAll");
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->lastSelectionStart() == 0 && page.bridge()->lastSelectionEnd() == text.size(), 5000);
    // find: must not throw or change the document; an unknown command is logged.
    QSignalSpy logged(page.bridge(), &EditorBridge::messageLogged);
    page.bridge()->runCommand("find");
    page.bridge()->runCommand("no-such-command");
    QTRY_VERIFY_WITH_TIMEOUT(logged.count() >= 1, 5000);
    QCOMPARE(page.canonical(), text);
    QCOMPARE(page.pageText(), text);
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

// Visual captures: render a sample document in each presentation inside the
// real WebEngineView and save PNGs under research/cycle-134/captures/ for
// review. Not an assertion of pixels; the test only checks that frames were
// produced and are not uniform (a blank or unpainted view).

namespace {
QString cycle134CapturesDirectory() {
    const QString tests = QFileInfo(QFINDTESTDATA("tst_fomawrite.cpp")).absolutePath();
    const QString directory = QDir(tests).filePath("../research/cycle-134/captures");
    QDir().mkpath(directory);
    return QDir(directory).canonicalPath();
}
bool cycle134SaveFrame(QQuickWindow *window, const QString &path, QString *why) {
    QTest::qWait(900);
    const QImage image = window->grabWindow();
    if (image.isNull()) { *why = "grabWindow returned a null image"; return false; }
    const QRgb first = image.pixel(2, 2);
    bool uniform = true;
    for (int y = 0; y < image.height() && uniform; y += 7)
        for (int x = 0; x < image.width(); x += 7)
            if (image.pixel(x, y) != first) { uniform = false; break; }
    if (!image.save(path)) { *why = "could not save " + path; return false; }
    if (uniform) { *why = "frame is a single colour: " + path; return false; }
    return true;
}
}

void FomawriteTest::liveEditorCapturesEachPresentation() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    QImage picture(320, 120, QImage::Format_RGB32); picture.fill(QColor("#4a7bd0"));
    QPainter painter(&picture); painter.setPen(Qt::white); painter.setFont(QFont("Helvetica", 28)); painter.drawText(picture.rect(), Qt::AlignCenter, "Local image"); painter.end();
    QVERIFY(picture.save(directory.filePath("picture.png")));
    const QString sample = QString::fromUtf8(
        "# Live editing in Fomawrite\n\n"
        "The **Markdown** stays the source of truth; what you see is *painted on top*. Click into a heading and its `#` reappears.\n\n"
        "## What renders\n\n"
        "- Inline **bold**, *emphasis*, `code` and a [link](https://typora.io)\n"
        "- [x] A finished task\n"
        "- [ ] An open task\n\n"
        "> Quotes keep their bar, nested quotes indent.\n\n"
        "![A local image](picture.png)\n\n"
        "| Feature | Status |\n|:--|--:|\n| Headings | done |\n| Tables | grid |\n\n"
        "---\n\n"
        "```python\ndef hello(name):\n    return f\"Hello, {name}\"\n```\n\n"
        "Inline math $E = mc^2$ and a block:\n\n$$\\int_0^1 x^2\\,dx = \\frac{1}{3}$$\n\n"
        "Caf\xC3\xA9, na\xC3\xAFve, \xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x92\xBB \xE4\xBD\xA0\xE5\xA5\xBD.\n");
    QFile doc(directory.filePath("sample.md")); QVERIFY(doc.open(QIODevice::WriteOnly)); doc.write(sample.toUtf8()); doc.close();
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    page.window->resize(960, 820);
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    QVERIFY(page.backend.open(QUrl::fromLocalFile(directory.filePath("sample.md"))));
    const QString captures = cycle134CapturesDirectory();
    QString why;
    struct Shot { const char *file; const char *theme; const char *mode; const char *appearance; bool dark; };
    const QList<Shot> shots = {
        {"live-basic.png", "", "live", "manuscript", false},
        {"live-claude-like.png", "claude-like", "live", "manuscript", false},
        {"live-claude-like-dark.png", "claude-like", "live", "manuscript", true},
        {"source-manuscript.png", "", "source", "manuscript", false},
        {"source-editorial.png", "", "source", "editorial", false},
        {"source-code.png", "", "source", "code", false},
    };
    for (const Shot &shot : shots) {
        if (QString::fromLatin1(shot.theme).isEmpty()) page.backend.setOutputStyle(0);
        else QVERIFY2(page.backend.selectPublishingTheme(shot.theme), qPrintable(page.backend.status()));
        page.backend.syncLiveEditor();
        page.bridge()->applyAppearance(QStringLiteral("{\"appearance\":\"%1\",\"dark\":%2,\"fontSize\":17,\"fontFamily\":\"%3\",\"liveThemeFilter\":true,\"palette\":{\"background\":\"%4\",\"text\":\"%5\"}}")
            .arg(shot.appearance).arg(shot.dark ? "true" : "false").arg(QStringLiteral(".AppleSystemUIFont")).arg(shot.dark ? "#1f1f1f" : "#ffffff").arg(shot.dark ? "#e6e6e6" : "#222222"));
        page.bridge()->selectMode(shot.mode);
        QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), page.canonical(), 10000);
        QVERIFY2(cycle134SaveFrame(page.window, QDir(captures).filePath(shot.file), &why), qPrintable(why));
        qInfo("capture %s", qPrintable(QDir(captures).filePath(shot.file)));
    }
    // A short document so math and a table are visible in one frame.
    const QString mathSample = QString::fromUtf8(
        "## Math and tables\n\n"
        "Inline $E = mc^2$ and $\\alpha + \\beta = \\gamma$ stay in the line of prose.\n\n"
        "$$\n\\int_0^1 x^2\\,dx = \\frac{1}{3}\n$$\n\n"
        "| Symbol | Meaning | Value |\n|:--|:--:|--:|\n| $\\pi$ | ratio | 3.14159 |\n| $e$ | Euler | 2.71828 |\n\n"
        "A $5 coffee and a $10 lunch are prices, not math.\n");
    QFile mathDoc(directory.filePath("math.md")); QVERIFY(mathDoc.open(QIODevice::WriteOnly)); mathDoc.write(mathSample.toUtf8()); mathDoc.close();
    QVERIFY(page.backend.open(QUrl::fromLocalFile(directory.filePath("math.md"))));
    QVERIFY(page.backend.selectPublishingTheme("claude-like"));
    page.backend.syncLiveEditor();
    page.bridge()->applyAppearance(QStringLiteral("{\"appearance\":\"manuscript\",\"dark\":false,\"fontSize\":17}"));
    page.bridge()->selectMode("live");
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), page.canonical(), 10000);
    QVERIFY2(cycle134SaveFrame(page.window, QDir(captures).filePath("live-math.png"), &why), qPrintable(why));
    qInfo("capture %s", qPrintable(QDir(captures).filePath("live-math.png")));
    // Cycle 136: a deliberately layout-heavy theme. Filtered Live keeps its
    // type (face, colours, heading rule) and drops its geometry; the exact
    // switch brings the geometry back. Reviewed by eye from the two captures.
    QVERIFY(page.backend.open(QUrl::fromLocalFile(directory.filePath("sample.md"))));
    const QString heavy = QStringLiteral(
        "#write { max-width: 420px; margin: 40px auto; padding: 60px 90px; font-family: Georgia, serif; color: #3b2a1a; background: #f6efe3;"
        " background-image: linear-gradient(135deg, #f6efe3, #e6d3b3); box-shadow: 0 12px 40px rgba(0,0,0,.35); columns: 2; transform: rotate(-1deg); }"
        " #write::before { content: 'DRAFT'; position: absolute; top: 8px; left: 8px; font-size: 40px; color: rgba(200,0,0,.4); }"
        " #write h1, #write h2 { color: #8b1a1a; font-family: 'Palatino', serif; border-bottom: 2px solid #8b1a1a; }"
        " #write blockquote { color: #6a5a4a; font-style: italic; } #write code { color: #5b2d8e; } #write a { color: #0b6e4f; }");
    page.backend.syncLiveEditor();
    page.bridge()->applyTheme(heavy);
    page.bridge()->applyAppearance(QStringLiteral("{\"appearance\":\"manuscript\",\"dark\":false,\"fontSize\":17,\"liveThemeFilter\":true,\"palette\":{\"background\":\"#ffffff\",\"text\":\"#222222\"}}"));
    page.bridge()->selectMode("live");
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), page.canonical(), 10000);
    QVERIFY2(cycle134SaveFrame(page.window, QDir(captures).filePath("live-heavy-theme-filtered.png"), &why), qPrintable(why));
    page.bridge()->applyAppearance(QStringLiteral("{\"appearance\":\"manuscript\",\"dark\":false,\"fontSize\":17,\"liveThemeFilter\":false,\"palette\":{\"background\":\"#ffffff\",\"text\":\"#222222\"}}"));
    QTest::qWait(150);
    QVERIFY2(cycle134SaveFrame(page.window, QDir(captures).filePath("live-heavy-theme-exact.png"), &why), qPrintable(why));
    // The two frames must differ (the geometry came back) and neither is blank.
    const QImage filtered(QDir(captures).filePath("live-heavy-theme-filtered.png")), exact(QDir(captures).filePath("live-heavy-theme-exact.png"));
    QVERIFY(!filtered.isNull() && !exact.isNull());
    QVERIFY(filtered != exact);
    qInfo("capture %s", qPrintable(QDir(captures).filePath("live-heavy-theme-filtered.png")));
    page.backend.discardRecovery();
}

// The viewport transition that follows a mode switch used to restore editing
// focus 16 ms later even if the writer had already moved into an auxiliary
// field (the CI runner hit this in the link editor). Red on the old code.
void FomawriteTest::viewportTransitionKeepsAuxiliaryFocus() {
    Cycle103SettingsSnapshot settings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    auto *editor = window->findChild<QQuickItem *>("sourceEditor"); QVERIFY(editor);
    auto *layout = window->findChild<QObject *>("workspaceLayout"); QVERIFY(layout);
    quick->resize(1200, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    if (QGuiApplication::platformName() != "cocoa") { quick->requestActivate(); QTRY_VERIFY(quick->isActive()); }
    editor->setProperty("text", QStringLiteral("# Focus\n\nalpha beta gamma\n"));
    editor->forceActiveFocus();
    QTRY_COMPARE(quick->activeFocusItem(), editor);
    layout->setProperty("filesVisible", true);
    auto *library = window->findChild<QObject *>("libraryPane"); QVERIFY(library);
    library->setProperty("showFilterBar", true);
    auto *filter = window->findChild<QQuickItem *>("libraryFilter"); QVERIFY(filter);
    QTRY_VERIFY(filter->isVisible());
    for (const char *mode : {"preview", "source"}) {
        // Start a transition, then immediately move into the auxiliary field.
        QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QString::fromLatin1(mode))));
        QVERIFY(window->property("changingDocumentView").toBool());
        filter->forceActiveFocus();
        QCOMPARE(quick->activeFocusItem(), filter);
        QTRY_VERIFY_WITH_TIMEOUT(!window->property("changingDocumentView").toBool(), 5000);
        QTest::qWait(60);
        QVERIFY2(quick->activeFocusItem() == filter, mode);
    }
    // Without an auxiliary field, the transition still restores editing focus.
    editor->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("preview"))));
    QTRY_VERIFY_WITH_TIMEOUT(!window->property("changingDocumentView").toBool(), 5000);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("source"))));
    QTRY_VERIFY_WITH_TIMEOUT(!window->property("changingDocumentView").toBool(), 5000);
    QTRY_COMPARE_WITH_TIMEOUT(quick->activeFocusItem(), editor, 2000);
    backend.discardRecovery();
}

// The whole window: switching to Live through the real command loads the page
// inside Main.qml's Loader, hands it the document, checks the footer option,
// routes an outline jump to the page and carries the caret back to Source.
void FomawriteTest::liveModeInMainWindowLoadsPageAndNavigatesOutline() {
    Cycle103SettingsSnapshot settings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    auto *editor = window->findChild<QQuickItem *>("sourceEditor"); QVERIFY(editor);
    auto *layout = window->findChild<QObject *>("workspaceLayout"); QVERIFY(layout);
    quick->resize(1200, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    const QString text = QString::fromUtf8("# One\n\nalpha\n\n## Two\n\nbeta\n");
    editor->setProperty("text", text);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    QVERIFY(layout->property("liveEditEnabled").toBool());
    auto *bridge = backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 20000);
    QSignalSpy received(bridge, &EditorBridge::textReceived);
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        const int token = bridge->fetchText(); QTest::qWait(60);
        for (const auto &args : received) if (args.at(0).toInt() == token && args.at(1).toString() == text) return true;
        return false;
    })(), 15000);
    auto *liveToggle = window->findChild<QQuickItem *>("liveEditToggle"); QVERIFY(liveToggle);
    QTRY_VERIFY(liveToggle->property("checked").toBool());
    // Edit > Select All while the page has focus acts on the page.
    auto *pane = window->findChild<QObject *>("liveEditorPane"); QVERIFY(pane);
    QVERIFY(QMetaObject::invokeMethod(pane, "focusLive"));
    QTRY_VERIFY_WITH_TIMEOUT(pane->property("hasLiveFocus").toBool(), 5000);
    QTest::qWait(200);
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        QMetaObject::invokeMethod(window.data(), "liveWebAction", Q_ARG(QVariant, QStringLiteral("SelectAll"))); QTest::qWait(150);
        return bridge->lastSelectionStart() == 0 && bridge->lastSelectionEnd() == text.size();
    })(), 8000);
    auto *outline = window->findChild<QObject *>("documentOutline"); QVERIFY(outline);
    const int two = text.indexOf("## Two");
    QVERIFY(QMetaObject::invokeMethod(outline, "jumpRequested", Q_ARG(int, two)));
    QVERIFY(layout->property("liveEditEnabled").toBool());
    QTRY_COMPARE_WITH_TIMEOUT(backend.liveCursor(), two, 5000);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("source"))));
    QVERIFY(!layout->property("liveEditEnabled").toBool());
    QCOMPARE(editor->property("cursorPosition").toInt(), two);
    QVERIFY(!liveToggle->property("checked").toBool());
    backend.discardRecovery();
}

// An image pasted into an untitled document cannot be stored beside it; the
// page's refusal reaches the writer as the window's notice banner.
void FomawriteTest::liveModeShowsImageRefusalNotice() {
    Cycle103SettingsSnapshot settings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    auto *editor = window->findChild<QQuickItem *>("sourceEditor"); QVERIFY(editor);
    quick->resize(1200, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    editor->setProperty("text", QStringLiteral("untitled\n"));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    auto *bridge = backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 20000);
    auto *view = window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QTRY_VERIFY_WITH_TIMEOUT(!view->property("loading").toBool(), 15000);
    QImage image(4, 4, QImage::Format_RGB32); image.fill(Qt::red);
    QByteArray png; { QBuffer buffer(&png); buffer.open(QIODevice::WriteOnly); QVERIFY(image.save(&buffer, "PNG")); }
    QSignalSpy saved(bridge, &EditorBridge::imageSaved);
    const QString script = QStringLiteral(R"JS(
        (function() {
            const bytes = Uint8Array.from(atob("%1"), c => c.charCodeAt(0));
            const dt = new DataTransfer(); dt.items.add(new File([bytes], "shot.png", { type: "image/png" }));
            document.querySelector(".cm-content").dispatchEvent(new ClipboardEvent("paste", { clipboardData: dt, bubbles: true, cancelable: true }));
        })())JS").arg(QString::fromLatin1(png.toBase64()));
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, script)); QTest::qWait(150);
        return saved.count() > 0;
    })(), 15000);
    QVERIFY(!saved.at(0).at(2).toString().isEmpty());
    QTRY_VERIFY_WITH_TIMEOUT(window->property("navigationNotice").toString().contains("Save the document first"), 5000);
    QCOMPARE(editor->property("text").toString(), QStringLiteral("untitled\n"));
    backend.discardRecovery();
}

// Cycle 136: in Live the left control shows the Output Style instead of the
// Source appearances, and View > Live Follows Theme Exactly reaches the page.
void FomawriteTest::liveLeftControlShowsThemeAndExactSwitchReachesPage() {
    Cycle103SettingsSnapshot settings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    auto *editor = window->findChild<QQuickItem *>("sourceEditor"); QVERIFY(editor);
    auto *layout = window->findChild<QObject *>("workspaceLayout"); QVERIFY(layout);
    auto *prefs = window->findChild<QObject *>("workspaceSettings"); QVERIFY(prefs);
    auto *footer = window->findChild<QQuickItem *>("documentFooter"); QVERIFY(footer);
    auto *appearanceMenu = window->findChild<QObject *>("sourceAppearanceMenu"); QVERIFY(appearanceMenu);
    auto *templateMenu = window->findChild<QObject *>("previewTemplateMenu"); QVERIFY(templateMenu);
    auto *appearanceButton = footer->findChild<QQuickItem *>("sourceAppearanceButton"); QVERIFY(appearanceButton);
    quick->resize(1200, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    editor->setProperty("text", QStringLiteral("# Theme\n\nbody\n"));
    const auto closeMenus = [&] {
        for (QObject *m : {appearanceMenu, templateMenu}) { QMetaObject::invokeMethod(m, "close"); }
        QTRY_VERIFY(!appearanceMenu->property("visible").toBool() && !templateMenu->property("visible").toBool());
    };
    // Source: the control offers the writing appearances.
    QVERIFY(QMetaObject::invokeMethod(footer, "appearanceMenuRequested", Q_ARG(QVariant, QVariant::fromValue<QObject *>(appearanceButton))));
    QTRY_VERIFY(appearanceMenu->property("opened").toBool());
    QVERIFY(!templateMenu->property("opened").toBool());
    closeMenus();
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    QTRY_VERIFY(layout->property("liveEditEnabled").toBool());
    auto *bridge = backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 20000);
    // Live: the control names the Output Style and opens the theme menu instead.
    QCOMPARE(footer->property("writingAppearance").toString(), backend.publishingThemeName());
    QVERIFY(QMetaObject::invokeMethod(footer, "appearanceMenuRequested", Q_ARG(QVariant, QVariant::fromValue<QObject *>(appearanceButton))));
    QTRY_VERIFY(templateMenu->property("opened").toBool());
    QVERIFY(!appearanceMenu->property("opened").toBool());
    closeMenus();
    // The exact-theme switch is a command that reaches the page's appearance JSON.
    QSignalSpy pushed(bridge, &EditorBridge::setAppearance);
    auto *commands = window->findChild<QObject *>("workspaceCommands"); QVERIFY(commands);
    QVariant enabled;
    QVERIFY(QMetaObject::invokeMethod(commands, "isEnabled", Q_RETURN_ARG(QVariant, enabled), Q_ARG(QVariant, QStringLiteral("liveThemeExact"))));
    QVERIFY(enabled.toBool());
    auto *codeItem = window->findChild<QQuickItem *>("sourceAppearanceCode"); QVERIFY(codeItem);
    prefs->setProperty("writingAppearance", QStringLiteral("code"));
    QVERIFY(!codeItem->property("checked").toBool()); // Code is never shown as the Live appearance
    prefs->setProperty("writingAppearance", QStringLiteral("editorial"));
    QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("liveThemeExact"))));
    QVERIFY(prefs->property("liveThemeExact").toBool());
    QTRY_VERIFY(pushed.count() >= 1);
    const QString json = pushed.last().at(0).toString();
    QVERIFY2(json.contains("\"liveThemeFilter\":false"), qPrintable(json));
    QVERIFY2(json.contains("\"palette\":{\"background\":\"") && json.contains("\"text\":\""), qPrintable(json));
    QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("liveThemeExact"))));
    QTRY_VERIFY(pushed.last().at(0).toString().contains("\"liveThemeFilter\":true"));
    // Back in Source the appearances return.
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("source"))));
    QTRY_VERIFY(!layout->property("liveEditEnabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(footer, "appearanceMenuRequested", Q_ARG(QVariant, QVariant::fromValue<QObject *>(appearanceButton))));
    QTRY_VERIFY(appearanceMenu->property("opened").toBool());
    closeMenus();
    QCOMPARE(footer->property("writingAppearance").toString(), QStringLiteral("Editorial"));
    // In Source the exact-theme command is disabled and does nothing.
    QVERIFY(QMetaObject::invokeMethod(commands, "isEnabled", Q_RETURN_ARG(QVariant, enabled), Q_ARG(QVariant, QStringLiteral("liveThemeExact"))));
    QVERIFY(!enabled.toBool());
    const bool exactBefore = prefs->property("liveThemeExact").toBool();
    QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("liveThemeExact"))));
    QCOMPARE(prefs->property("liveThemeExact").toBool(), exactBefore);
    backend.discardRecovery();
}

// WCAG contrast between the background and the text of a captured region: the
// most common colour is the background; the text colour is the most contrasting
// colour that covers at least 0.5% of the region.
double cycle137TextContrast(const QImage &image, const QRect &region) {
    const auto luminance = [](QRgb c) {
        const auto channel = [](int v) { const double s = v / 255.0; return s <= 0.03928 ? s / 12.92 : std::pow((s + 0.055) / 1.055, 2.4); };
        return 0.2126 * channel(qRed(c)) + 0.7152 * channel(qGreen(c)) + 0.0722 * channel(qBlue(c));
    };
    QHash<QRgb, int> counts;
    const QRect r = region.intersected(image.rect());
    for (int y = r.top(); y <= r.bottom(); ++y) for (int x = r.left(); x <= r.right(); ++x) ++counts[image.pixel(x, y) & 0xF0F0F0F0];
    QRgb background = 0; int best = -1;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) if (it.value() > best) { best = it.value(); background = it.key(); }
    const double total = double(r.width()) * r.height(), bgL = luminance(background);
    double contrast = 1.0;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        if (it.value() < total * 0.005 || it.key() == background) continue;
        const double l = luminance(it.key());
        contrast = qMax(contrast, (qMax(l, bgL) + 0.05) / (qMin(l, bgL) + 0.05));
    }
    return contrast;
}

// Cycle 137: a gallery of every bundled Output Style in filtered Live, light
// and dark, for review by eye. Every frame's heading and body region must
// read at WCAG 3:1 or better (a dark-on-dark or light-on-light render fails),
// light and dark differ, and the styles do not all render the same.
void FomawriteTest::liveThemeGalleryCapturesEveryBundledStyle() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString sample = QString::fromUtf8(
        "# Gallery heading\n\n"
        "Body text with **bold**, *emphasis*, `code` and a [link](https://example.org). Caf\xC3\xA9 \xE4\xBD\xA0\xE5\xA5\xBD.\n\n"
        "## Second level\n\n"
        "- A bullet item\n- Another with `code`\n\n1. First numbered\n2. Second numbered\n\n"
        "> A quotation line that is long enough to wrap when the measure is narrow, so the bar and indent show.\n\n"
        "| Column | Value |\n|:--|--:|\n| one | 1 |\n| two | 2 |\n\n"
        "```js\nconst answer = 42;\n```\n\n---\n\nLast paragraph.\n");
    QFile doc(directory.filePath("gallery.md")); QVERIFY(doc.open(QIODevice::WriteOnly)); doc.write(sample.toUtf8()); doc.close();
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    page.window->resize(960, 820);
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    QVERIFY(page.backend.open(QUrl::fromLocalFile(directory.filePath("gallery.md"))));
    const QString tests = QFileInfo(QFINDTESTDATA("tst_fomawrite.cpp")).absolutePath();
    const QString captures = QDir(tests).filePath("../research/cycle-137/captures"); QDir().mkpath(captures);
    QString why; QList<QImage> frames;
    struct Style { QString name; int preset; QString theme; };
    QList<Style> styles;
    for (int preset = 0; preset <= 6; ++preset) styles.append({QStringLiteral("preset-%1").arg(preset), preset, QString()});
    styles.append({QStringLiteral("claude-like"), -1, QStringLiteral("claude-like")});
    for (const Style &style : styles) {
        if (style.theme.isEmpty()) page.backend.setOutputStyle(style.preset);
        else QVERIFY2(page.backend.selectPublishingTheme(style.theme), qPrintable(page.backend.status()));
        for (bool dark : {false, true}) {
            page.backend.syncLiveEditor();
            page.bridge()->applyAppearance(QStringLiteral("{\"appearance\":\"manuscript\",\"dark\":%1,\"fontSize\":17,\"liveThemeFilter\":true,\"palette\":{\"background\":\"%2\",\"text\":\"%3\"}}")
                .arg(dark ? "true" : "false").arg(dark ? "#1f1f1f" : "#ffffff").arg(dark ? "#e6e6e6" : "#222222"));
            page.bridge()->selectMode("live");
            QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), page.canonical(), 10000);
            QTest::qWait(120);
            const QString file = QDir(captures).filePath(QStringLiteral("live-gallery-%1-%2.png").arg(style.name, dark ? "dark" : "light"));
            QVERIFY2(cycle134SaveFrame(page.window, file, &why), qPrintable(why));
            QImage frame(file); QVERIFY(!frame.isNull());
            frames.append(frame);
            const double heading = cycle137TextContrast(frame, QRect(130, 50, 700, 70)), body = cycle137TextContrast(frame, QRect(130, 120, 700, 60));
            qInfo("capture %s heading contrast %.2f body contrast %.2f", qPrintable(file), heading, body);
            QVERIFY2(heading >= 3.0 && body >= 3.0, qPrintable(QStringLiteral("%1: unreadable (heading %2, body %3)").arg(file).arg(heading).arg(body)));
        }
    }
    // Light and dark of the same style differ, and the styles do not all render the same.
    for (int i = 0; i + 1 < frames.size(); i += 2) QVERIFY(frames[i] != frames[i + 1]);
    int distinctLight = 1;
    for (int i = 2; i < frames.size(); i += 2) if (frames[i] != frames[0]) ++distinctLight;
    QVERIFY2(distinctLight >= 2, "every bundled style rendered identically in Live");
    page.backend.discardRecovery();
}
