// Persistence must follow the page's earlier commands, not the lagging mirror.
void FomawriteTest::liveUndoSaveWaitsForCurrentPage() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    const QString path = directory.filePath("ordered.md");
    const QString original = QString::fromUtf8("# Ordered café\n\nOriginal paragraph.\n");
    const QString changed = original + QStringLiteral("Added paragraph.\n");
    QVERIFY(cycle103Write(path, original.toUtf8()));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "setLiveEditing", Q_ARG(QVariant, true)));
    auto *bridge = f.backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 15000);
    QSignalSpy replies(bridge, &EditorBridge::textReceived);
    const int initial = bridge->fetchText();
    QTRY_VERIFY_WITH_TIMEOUT(!replies.isEmpty() && replies.last().at(0).toInt() == initial, 5000);
    QCOMPARE(replies.last().at(1).toString(), original);
    bridge->injectUserChanges(QStringLiteral("[{\"from\":%1,\"to\":%1,\"insert\":\"Added paragraph.\\n\"}]").arg(original.size()));
    QTRY_COMPARE(f.editor->property("text").toString(), changed);
    f.backend.save(); QTRY_COMPARE(cycle103Read(path), changed.toUtf8());
    QTRY_VERIFY(!f.backend.modified());
    // Same host turn: no event processing or page-text fetch between Undo/Save.
    bridge->requestUndo();
    f.backend.save();
    QTRY_COMPARE(f.editor->property("text").toString(), original);
    QTRY_COMPARE_WITH_TIMEOUT(cycle103Read(path), original.toUtf8(), 5000);
    QVERIFY(!f.backend.modified());
    bridge->requestRedo();
    f.backend.save();
    QTRY_COMPARE_WITH_TIMEOUT(cycle103Read(path), changed.toUtf8(), 5000);
    QTRY_COMPARE(f.editor->property("text").toString(), changed);
    QVERIFY(!f.backend.modified());
}

namespace {
bool livePersistenceStart(Cycle117Window &f, const QString &path, const QString &text) {
    if (!cycle103Write(path, text.toUtf8()) || !f.start() || !f.backend.open(QUrl::fromLocalFile(path))) return false;
    if (!QMetaObject::invokeMethod(f.window.data(), "setLiveEditing", Q_ARG(QVariant, true))) return false;
    if (!QTest::qWaitFor([&] { return f.backend.editorBridge()->isReady(); }, 15000)) return false;
    return cycle144PageText(f.backend.editorBridge(), 5000) == text;
}
QQuickItem *livePersistenceDialogButton(QObject *dialog, const QString &text) {
    for (auto *item : dialog->findChildren<QQuickItem *>())
        if (item->property("text").toString() == text) return item;
    return nullptr;
}
}

void FomawriteTest::liveSaveAsAndSelectionCommandsPreserveOrdering() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    const QString original = QStringLiteral("A paragraph.\n");
    const QString path = directory.filePath("original.md"), copy = directory.filePath("copy.md");
    Cycle117Window f; QVERIFY2(livePersistenceStart(f, path, original), qPrintable(f.error));
    auto *bridge = f.backend.editorBridge();
    bridge->injectUserChanges(QStringLiteral("[{\"from\":0,\"to\":1,\"insert\":\"Another\"}]"));
    const QString changed = QStringLiteral("Another paragraph.\n");
    QTRY_COMPARE(f.editor->property("text").toString(), changed);
    f.backend.save(); QTRY_COMPARE(cycle103Read(path), changed.toUtf8());
    bridge->requestUndo(); f.backend.saveAs(QUrl::fromLocalFile(copy));
    QTRY_COMPARE(cycle103Read(copy), original.toUtf8());
    QCOMPARE(cycle103Read(path), changed.toUtf8());
    QCOMPARE(f.backend.fileUrl(), QUrl::fromLocalFile(copy));
    auto *view = f.window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QCOMPARE(cycle143PageResult(view, "(()=>{let v=window.fomawriteEditor.view;v.dispatch({selection:{anchor:2,head:11}});return v.state.selection.main.to;})()", 0).toInt(), 11);
    // Save must wait for the earlier asynchronous selection command as well.
    QVERIFY(f.backend.liveWrapSelection("**", "**"));
    f.backend.save();
    const QString formatted = QStringLiteral("A **paragraph**.\n");
    QTRY_COMPARE(cycle103Read(copy), formatted.toUtf8());
    QCOMPARE(cycle144PageText(bridge), formatted);
    QVERIFY(!f.backend.modified());
}

void FomawriteTest::liveCloseWaitsBeforeDirtyDecisionAndSave() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    const QString path = directory.filePath("close.md"), original = QStringLiteral("Original paragraph.\n");
    Cycle117Window f; QVERIFY2(livePersistenceStart(f, path, original), qPrintable(f.error));
    auto *bridge = f.backend.editorBridge();
    bridge->injectUserChanges(QStringLiteral("[{\"from\":0,\"to\":8,\"insert\":\"Changed\"}]"));
    const QString changed = QStringLiteral("Changed paragraph.\n");
    QTRY_COMPARE(f.editor->property("text").toString(), changed);
    f.backend.save(); QTRY_COMPARE(cycle103Read(path), changed.toUtf8()); QTRY_VERIFY(!f.backend.modified());
    auto *prompt = f.window->findChild<QObject *>("unsavedChangesPrompt"); QVERIFY(prompt);
    bridge->requestUndo(); f.quick->close(); // Mirror still clean in this host turn.
    QVERIFY(f.quick->isVisible());
    QVERIFY(!f.window->findChild<QQuickItem *>("editorPane")->isEnabled());
    QTRY_VERIFY(prompt->property("opened").toBool());
    QCOMPARE(f.editor->property("text").toString(), original);
    QCOMPARE(cycle103Read(path), changed.toUtf8());
    QVERIFY(QMetaObject::invokeMethod(prompt, "reject"));
    QTRY_VERIFY(!prompt->property("opened").toBool());
    QVERIFY(f.quick->isVisible()); QVERIFY(f.backend.modified());
    f.quick->close(); QTRY_VERIFY(prompt->property("opened").toBool());
    auto *save = livePersistenceDialogButton(prompt, "Save"); QVERIFY(save);
    QVERIFY(cycle102Click(f.quick, save));
    QTRY_VERIFY(!f.quick->isVisible());
    QCOMPARE(cycle103Read(path), original.toUtf8());
    QVERIFY(!f.backend.modified());
}

void FomawriteTest::livePersistenceRejectsStaleDocumentsAndRendererLoss() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    const QString path = directory.filePath("first.md"), second = directory.filePath("second.md");
    const QString firstText = QStringLiteral("First document.\n"), secondText = QStringLiteral("Second document.\n");
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QVERIFY(cycle103Write(path, firstText.toUtf8())); QVERIFY(cycle103Write(second, secondText.toUtf8()));
    QVERIFY(page.backend.open(QUrl::fromLocalFile(path)));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    page.backend.syncLiveEditor(); QCOMPARE(page.pageText(), firstText);
    page.backend.setLiveEditingActive(true);
    QSignalSpy failed(&page.backend, &Backend::saveFailed), saved(&page.backend, &Backend::saveSucceeded);
    // The real page may reply later, but this operation belongs to the old doc.
    page.backend.save(); QVERIFY(page.backend.open(QUrl::fromLocalFile(second)));
    QTRY_COMPARE(failed.count(), 1);
    page.backend.syncLiveEditor(); QCOMPARE(page.pageText(), secondText);
    QVERIFY(!page.backend.modified()); // Old failure must not dirty the new doc.
    QCOMPARE(saved.count(), 0); QCOMPARE(cycle103Read(path), firstText.toUtf8());
    QCOMPARE(cycle103Read(second), secondText.toUtf8());
    // A dialog acceptance must retain the document identity of its opening.
    QSignalSpy dialogs(&page.backend, &Backend::saveDialogRequested);
    page.backend.saveAsDialog(); QTRY_COMPARE(dialogs.count(), 1);
    QVERIFY(page.backend.open(QUrl::fromLocalFile(path)));
    const QString staleCopy = directory.filePath("stale-copy.md");
    page.backend.saveAs(QUrl::fromLocalFile(staleCopy));
    QCOMPARE(failed.count(), 2); QVERIFY(!QFileInfo::exists(staleCopy));
    page.backend.syncLiveEditor(); QCOMPARE(page.pageText(), firstText);
    // Exercise the actual pane's renderer-termination handler. The renderer
    // itself is not killed: readiness invalidation, recovery notice and retry
    // are the same lifecycle used when Chromium reports a terminated process.
    page.backend.save();
    auto *pane = page.root->findChild<QObject *>("liveEditorPane"); QVERIFY(pane);
    QVERIFY(QMetaObject::invokeMethod(pane, "rendererStopped"));
    QTRY_COMPARE(failed.count(), 3);
    QVERIFY(!page.bridge()->isReady()); QVERIFY(page.backend.modified());
    QCOMPARE(saved.count(), 0); QVERIFY(page.window->isVisible());
    QVERIFY(page.backend.status().contains("Live"));
    QCOMPARE(cycle103Read(path), firstText.toUtf8());
    QVERIFY(QMetaObject::invokeMethod(pane, "retryLiveEditor"));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 10000);
    QCOMPARE(page.pageText(), firstText);
    page.bridge()->injectUserChanges(QStringLiteral("[{\"from\":0,\"to\":5,\"insert\":\"Recovered\"}]"));
    const QString recovered = QStringLiteral("Recovered document.\n");
    QTRY_COMPARE(page.canonical(), recovered);
    page.backend.save(); QTRY_COMPARE(saved.count(), 1);
    QCOMPARE(cycle103Read(path), recovered.toUtf8()); QVERIFY(!page.backend.modified());
    page.backend.save();
    QVERIFY(page.backend.renameDocument("renamed.md"));
    const QString renamed = directory.filePath("renamed.md");
    QTRY_COMPARE(failed.count(), 4);
    QCOMPARE(page.backend.fileUrl(), QUrl::fromLocalFile(renamed));
    QVERIFY(!QFileInfo::exists(path)); QCOMPARE(cycle103Read(renamed), recovered.toUtf8());
    QVERIFY(!page.backend.modified());
}

void FomawriteTest::livePersistenceMismatchAndTimeoutFailClosed() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    const QString path = directory.filePath("timeout.md"), original = QStringLiteral("Known host text.\n");
    Cycle117Window f; QVERIFY2(livePersistenceStart(f, path, original), qPrintable(f.error));
    auto *view = f.window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QSignalSpy failed(&f.backend, &Backend::saveFailed), saved(&f.backend, &Backend::saveSucceeded);
    // Deliberately desynchronize the real page state without emitting a delta.
    // The barrier must reject its genuine full-text reply, not replace Source.
    QCOMPARE(cycle143PageResult(view, "(()=>{window.fomawriteEditor.setText('Different page text.');return window.fomawriteEditor.getText();})()", 0).toString(), QStringLiteral("Different page text."));
    f.backend.save(); QTRY_COMPARE(failed.count(), 1);
    QCOMPARE(f.editor->property("text").toString(), original);
    QCOMPARE(cycle103Read(path), original.toUtf8()); QCOMPARE(saved.count(), 0);
    QVERIFY(f.backend.modified()); QVERIFY(f.quick->isVisible());
    f.backend.syncLiveEditor(); QCOMPARE(cycle144PageText(f.backend.editorBridge()), original);
    // Real Chromium executes a bounded busy task; host event processing stays
    // asynchronous. No fake readiness or fabricated text reply satisfies Save.
    auto *engine = qmlEngine(view); QVERIFY(engine);
    const QJSValue callback = engine->evaluate("(function(value) {})");
    QVERIFY(QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString,
        QStringLiteral("(()=>{const until=performance.now()+%1;while(performance.now()<until){};return true;})()").arg(Backend::LivePersistenceTimeout + 1500)), Q_ARG(uint, 0), Q_ARG(QJSValue, callback)));
    f.backend.save();
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 2, Backend::LivePersistenceTimeout + 1000);
    QCOMPARE(saved.count(), 0); QCOMPARE(cycle103Read(path), original.toUtf8());
    QVERIFY(f.backend.modified()); QVERIFY(f.quick->isVisible());
    QVERIFY(f.backend.status().contains("did not respond"));
    // The delayed genuine reply is ignored after the request was canceled.
    QCOMPARE(cycle144PageText(f.backend.editorBridge(), 5000), original);
    QCOMPARE(saved.count(), 0); QCOMPARE(cycle103Read(path), original.toUtf8());
    // An explicit switch to Source must remain usable after renderer failure.
    auto *pane = f.window->findChild<QObject *>("liveEditorPane"); QVERIFY(pane);
    QVERIFY(QMetaObject::invokeMethod(pane, "rendererStopped"));
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "setSourceEditing"));
    QTRY_VERIFY(!f.backend.liveEditingActive());
    QVERIFY(!f.window->property("documentBarrierCallback").value<QJSValue>().isCallable());
    const QString source = QStringLiteral("Source recovery remains writable.\n");
    QVERIFY(f.editor->setProperty("text", source));
    f.backend.save(); // Source save remains synchronous, even without Live.
    QCOMPARE(cycle103Read(path), source.toUtf8());
    QCOMPARE(saved.count(), 1); QVERIFY(!f.backend.modified());
    // The invalidated old page can still send queued channel calls, but its
    // real delta must not overwrite Source before a fresh ready handshake.
    QSignalSpy oldPageCursor(f.backend.editorBridge(), &EditorBridge::cursorMoved);
    QCOMPARE(cycle143PageResult(view, "(()=>{window.fomawriteEditor.simulateUserChanges('[{\"from\":0,\"to\":0,\"insert\":\"late old renderer \"}]');window.fomawriteEditor.view.dispatch({selection:{anchor:'late old renderer '.length}});return window.fomawriteEditor.getText();})()", 0).toString(), QStringLiteral("late old renderer ") + original);
    // The cursor report is sent after the real delta on the same channel;
    // witness delivery before asserting that the invalidated delta was ignored.
    QTRY_VERIFY(std::any_of(oldPageCursor.cbegin(), oldPageCursor.cend(), [](const auto &args) {
        return args.at(1).toInt() == QStringLiteral("late old renderer ").size();
    }));
    QCOMPARE(f.editor->property("text").toString(), source);
    QVERIFY(!f.backend.modified());
}

void FomawriteTest::liveCloseBlocksPostSnapshotMutations() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    const QString path = directory.filePath("snapshot.md"), original = QStringLiteral("Original paragraph.\n");
    Cycle117Window f; QVERIFY2(livePersistenceStart(f, path, original), qPrintable(f.error));
    auto *bridge = f.backend.editorBridge();
    bridge->injectUserChanges(QStringLiteral("[{\"from\":0,\"to\":8,\"insert\":\"Changed\"}]"));
    const QString changed = QStringLiteral("Changed paragraph.\n");
    QTRY_COMPARE(f.editor->property("text").toString(), changed);
    f.backend.save(); QTRY_COMPARE(cycle103Read(path), changed.toUtf8()); QTRY_VERIFY(!f.backend.modified());
    bool attempted = false, inputSuspended = false;
    // The channel has sent requestText before this observer runs. A new Undo
    // sent now would follow its snapshot and could arrive after a clean Close.
    connect(bridge, &EditorBridge::requestText, f.window.data(), [&](int) {
        if (attempted) return;
        attempted = true;
        inputSuspended = !f.window->findChild<QQuickItem *>("editorPane")->isEnabled();
        QMetaObject::invokeMethod(f.window.data(), "undoEditing");
        QTest::keyClick(f.quick, Qt::Key_X);
    });
    f.quick->close();
    QTRY_VERIFY(!f.quick->isVisible());
    QVERIFY(attempted); QVERIFY(inputSuspended);
    QCOMPARE(f.editor->property("text").toString(), changed);
    QCOMPARE(cycle144PageText(bridge), changed);
    QCOMPARE(cycle103Read(path), changed.toUtf8());
    QVERIFY(!f.backend.modified());
    auto *prompt = f.window->findChild<QObject *>("unsavedChangesPrompt"); QVERIFY(prompt);
    QVERIFY(!prompt->property("opened").toBool());
}

void FomawriteTest::liveQuitPreparationWaitsAndGlobalCancelRestoresWindows() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    const QString path = directory.filePath("first-window.md"), original = QStringLiteral("First clean document.\n");
    Cycle117Window first; QVERIFY2(livePersistenceStart(first, path, original), qPrintable(first.error));
    Cycle117Window second; QVERIFY2(second.start(), qPrintable(second.error));
    QVERIFY(second.editor->setProperty("text", QStringLiteral("Second unsaved document.")));
    auto *firstPane = first.window->findChild<QQuickItem *>("editorPane"); QVERIFY(firstPane);
    QSignalSpy ready(&first.backend, &Backend::quitReady);
    QVERIFY(QMetaObject::invokeMethod(first.window.data(), "prepareQuit"));
    QTRY_COMPARE(ready.count(), 1);
    QVERIFY(first.window->property("quitPrepared").toBool()); QVERIFY(!firstPane->isEnabled());
    QVERIFY(first.quick->isVisible());
    QVERIFY(QMetaObject::invokeMethod(second.window.data(), "prepareQuit"));
    auto *prompt = second.window->findChild<QObject *>("unsavedChangesPrompt"); QVERIFY(prompt);
    QTRY_VERIFY(prompt->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(prompt, "reject"));
    QTRY_VERIFY(!first.window->property("quitPrepared").toBool());
    QVERIFY(firstPane->isEnabled()); QVERIFY(first.quick->isVisible()); QVERIFY(second.quick->isVisible());
    QVERIFY(second.backend.modified());
    // Cancellation can also beat the first window's async acknowledgement.
    QVERIFY(QMetaObject::invokeMethod(first.window.data(), "prepareQuit"));
    second.backend.cancelQuit();
    QCOMPARE(cycle144PageText(first.backend.editorBridge()), original);
    QCOMPARE(ready.count(), 1);
    QVERIFY(!first.window->property("quitPrepared").toBool()); QVERIFY(firstPane->isEnabled());
    first.backend.editorBridge()->injectUserChanges(QStringLiteral("[{\"from\":0,\"to\":5,\"insert\":\"Resumed\"}]"));
    const QString resumed = QStringLiteral("Resumed clean document.\n");
    QTRY_COMPARE(first.editor->property("text").toString(), resumed);
    first.backend.save(); QTRY_COMPARE(cycle103Read(path), resumed.toUtf8());
    QVERIFY(!first.backend.modified());
}
