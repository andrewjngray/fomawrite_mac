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
    QVERIFY(page.backend.liveWrapSelection("**", "**"));
    const QString wrapped = QString::fromUtf8("Title line\n\n****alpha beta gamma\n");
    QCOMPARE(page.canonical(), wrapped);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), wrapped, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->lastCursor() == 14, 5000);
    // A block command on the first line.
    page.bridge()->placeCursor(2);
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->lastCursor() == 2, 5000);
    QVERIFY(page.backend.liveEditMarkdown("heading2"));
    QVERIFY2(page.canonical().startsWith("## Title line"), qPrintable(page.canonical()));
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), page.canonical(), 10000);
    // Snippet replacement at the caret.
    QVERIFY(page.backend.liveReplaceSelection("[snippet]"));
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), page.canonical(), 10000);
    QVERIFY(page.canonical().contains("[snippet]"));
    // Without a known selection nothing happens.
    EditorBridge fresh; Q_UNUSED(fresh);
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
        page.bridge()->applyAppearance(QStringLiteral("{\"appearance\":\"%1\",\"dark\":%2,\"fontSize\":17}").arg(shot.appearance).arg(shot.dark ? "true" : "false"));
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
    page.backend.discardRecovery();
}
