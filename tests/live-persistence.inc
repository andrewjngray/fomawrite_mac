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
    QSignalSpy selectionRequests(bridge, &EditorBridge::requestSelection), selectionReplies(bridge, &EditorBridge::selectionReceived);
    const auto failedDiagnostic = QObject::connect(&f.backend, &Backend::saveFailed, f.window.data(), [&] {
        qWarning() << "SaveAs/selection ordering failure" << f.backend.status() << "ready" << bridge->isReady()
                   << "page generation" << bridge->pageGeneration() << "selection requests/replies" << selectionRequests.count() << selectionReplies.count();
    });
    const auto diagnosticCleanup = qScopeGuard([&] { QObject::disconnect(failedDiagnostic); });
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

// A busy but responsive real page must not lose a preceding Format command
// merely because selection takes longer than the standalone fallback budget.
void FomawriteTest::liveSaveWaitsForDelayedSelectionReply_data() {
    QTest::addColumn<int>("stallMs");
    QTest::addColumn<bool>("shouldSave");
    QTest::newRow("responsive-after-standalone-deadline") << 1800 << true;
    QTest::newRow("beyond-persistence-deadline") << 4200 << false;
}

void FomawriteTest::liveSaveWaitsForDelayedSelectionReply() {
    QFETCH(int, stallMs); QFETCH(bool, shouldSave);
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    const QString original = QStringLiteral("A paragraph.\n");
    const QString path = directory.filePath("original.md"), copy = directory.filePath("copy.md");
    Cycle117Window f; QVERIFY2(livePersistenceStart(f, path, original), qPrintable(f.error));
    auto *bridge = f.backend.editorBridge();
    auto *view = f.window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QSignalSpy saved(&f.backend, &Backend::saveSucceeded), failed(&f.backend, &Backend::saveFailed);
    QSignalSpy requested(bridge, &EditorBridge::requestSelection), replied(bridge, &EditorBridge::selectionReceived);
    f.backend.saveAs(QUrl::fromLocalFile(copy)); QTRY_COMPARE(saved.count(), 1);
    QCOMPARE(cycle103Read(copy), original.toUtf8());
    QCOMPARE(f.backend.fileUrl(), QUrl::fromLocalFile(copy));
    QCOMPARE(cycle144PageText(bridge), original);
    const int generation = bridge->pageGeneration();
    auto *engine = qmlEngine(view); QVERIFY(engine);
    engine->globalObject().setProperty("liveDelayedSelectionDone", QJSValue(QJSValue::UndefinedValue));
    const QJSValue callback = engine->evaluate("(function(value) { liveDelayedSelectionDone = value; })");
    // The genuine CM delta reaches the host before this task blocks Chromium.
    // It provides a start witness without fabricating a selection/text reply.
    QElapsedTimer queued; queued.start();
    QVERIFY(QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, QStringLiteral(R"JS(
        (()=>{
            const v=window.fomawriteEditor.view;
            v.dispatch({changes:{from:0,to:1,insert:'B'},selection:{anchor:2,head:11},userEvent:'input'});
            const until=performance.now()+%1;
            while(performance.now()<until){}
            return true;
        })()
    )JS").arg(stallMs)), Q_ARG(uint, 0), Q_ARG(QJSValue, callback)));
    const QString marked = QStringLiteral("B paragraph.\n");
    QTRY_COMPARE_WITH_TIMEOUT(f.editor->property("text").toString(), marked, 5000);
    const qint64 startAcknowledgedMs = queued.elapsed();
    QVERIFY2(engine->globalObject().property("liveDelayedSelectionDone").isUndefined(), "Timing precondition: genuine page stall must still be running after its delta acknowledgment");
    QElapsedTimer selectionDelay; selectionDelay.start();
    qint64 selectionReplyMs = -1;
    const auto timing = QObject::connect(bridge, &EditorBridge::selectionReceived, f.window.data(), [&](int, int, int) { selectionReplyMs = selectionDelay.elapsed(); });
    const auto timingCleanup = qScopeGuard([&] { QObject::disconnect(timing); });
    QVERIFY(f.backend.liveWrapSelection("**", "**"));
    f.backend.save(); // Same host turn; no wait for the selection response.
    QCOMPARE(requested.count(), 1);
    QCOMPARE(replied.count(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(saved.count() == 2 || failed.count() != 0, Backend::LivePersistenceTimeout + 1000);
    const bool taskFinished = !engine->globalObject().property("liveDelayedSelectionDone").isUndefined();
    qInfo() << "Delayed real selection/save" << "saved" << saved.count() << "failed" << failed.count()
            << "requests" << requested.count() << "replies" << replied.count() << "status" << f.backend.status()
            << "page generation" << generation << bridge->pageGeneration() << "start acknowledged ms" << startAcknowledgedMs
            << "selection reply ms" << selectionReplyMs << "page task finished" << taskFinished;
    if (!shouldSave) {
        QCOMPARE(failed.count(), 1); QCOMPARE(saved.count(), 1);
        QVERIFY2(!taskFinished, "Timing precondition: the genuine page stall must outlast the bounded Save failure");
        QCOMPARE(cycle103Read(copy), original.toUtf8());
        QCOMPARE(f.editor->property("text").toString(), marked); QVERIFY(f.backend.modified());
        QTRY_VERIFY_WITH_TIMEOUT(!engine->globalObject().property("liveDelayedSelectionDone").isUndefined(), stallMs + 1000);
        QCOMPARE(cycle144PageText(bridge), marked); // Same-channel fence after the canceled late selection reply.
        QCOMPARE(replied.count(), 0);
        QCOMPARE(cycle103Read(copy), original.toUtf8()); QCOMPARE(saved.count(), 1);
        QCOMPARE(f.editor->property("text").toString(), marked); QVERIFY(f.backend.modified());
        f.backend.save(); QTRY_COMPARE(saved.count(), 2); // A canceled selection cannot strand future saves.
        QCOMPARE(failed.count(), 1); QCOMPARE(cycle103Read(copy), marked.toUtf8());
        QVERIFY(!f.backend.modified());
        return;
    }
    if (failed.count()) QVERIFY2(!taskFinished, "Timing witness: failed Save must have occurred while the actual page was still stalled");
    QCOMPARE(failed.count(), 0);
    QCOMPARE(saved.count(), 2);
    QTRY_COMPARE(replied.count(), 1);
    QVERIFY2(selectionReplyMs > 1000, "Timing precondition: the actual selection reply must cross the standalone one-second deadline");
    QCOMPARE(replied.first().at(1).toInt(), 2); QCOMPARE(replied.first().at(2).toInt(), 11);
    const QString formatted = QStringLiteral("B **paragraph**.\n");
    QCOMPARE(cycle103Read(copy), formatted.toUtf8());
    QCOMPARE(f.editor->property("text").toString(), formatted);
    QCOMPARE(cycle144PageText(bridge), formatted);
    QCOMPARE(cycle103Read(path), original.toUtf8());
    QCOMPARE(bridge->pageGeneration(), generation);
    QVERIFY(!f.backend.modified());
}

void FomawriteTest::liveCanceledBarrierRestoresSelectionFallback() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    const QString path = directory.filePath("cancel.md"), original = QStringLiteral("A paragraph.\n");
    Cycle117Window f; QVERIFY2(livePersistenceStart(f, path, original), qPrintable(f.error));
    auto *bridge = f.backend.editorBridge();
    auto *view = f.window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    // Establish the ordinary fallback cache through the real debounced report.
    QCOMPARE(cycle143PageResult(view, "(()=>{let v=window.fomawriteEditor.view;v.dispatch({selection:{anchor:2,head:11}});return v.state.selection.main.to;})()", 0).toInt(), 11);
    QTRY_COMPARE(bridge->lastSelectionStart(), 2); QTRY_COMPARE(bridge->lastSelectionEnd(), 11);
    auto *engine = qmlEngine(view); QVERIFY(engine);
    engine->globalObject().setProperty("liveCanceledSelectionDone", QJSValue(QJSValue::UndefinedValue));
    const QJSValue callback = engine->evaluate("(function(value) { liveCanceledSelectionDone = value; })");
    QVERIFY(QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, QStringLiteral(R"JS(
        (()=>{const v=window.fomawriteEditor.view;
            v.dispatch({changes:{from:0,to:1,insert:'B'},userEvent:'input'});
            const until=performance.now()+2600;while(performance.now()<until){};return true;})()
    )JS")), Q_ARG(uint, 0), Q_ARG(QJSValue, callback)));
    QTRY_COMPARE(f.editor->property("text").toString(), QStringLiteral("B paragraph.\n"));
    QVERIFY(engine->globalObject().property("liveCanceledSelectionDone").isUndefined());
    QSignalSpy failed(&f.backend, &Backend::saveFailed), saved(&f.backend, &Backend::saveSucceeded);
    QSignalSpy finished(&f.backend, &Backend::documentBarrierFinished);
    QVERIFY(f.backend.liveWrapSelection("**", "**"));
    const int request = f.backend.requestDocumentBarrier(); QVERIFY(request > 0);
    QTest::qWait(1200); // Cross the documented standalone fallback deadline.
    QVERIFY2(engine->globalObject().property("liveCanceledSelectionDone").isUndefined(), "Timing precondition: page must still be stalled at barrier cancellation");
    QCOMPARE(finished.count(), 0);
    f.backend.cancelDocumentBarrier(request);
    const QString formatted = QStringLiteral("B **paragraph**.\n");
    QTRY_COMPARE_WITH_TIMEOUT(f.editor->property("text").toString(), formatted, 1000);
    QCOMPARE(cycle103Read(path), original.toUtf8()); QCOMPARE(saved.count(), 0); QCOMPARE(failed.count(), 0);
    QVERIFY(f.backend.modified());
    QTRY_VERIFY_WITH_TIMEOUT(!engine->globalObject().property("liveCanceledSelectionDone").isUndefined(), 5000);
    QCOMPARE(cycle144PageText(bridge), formatted);
    QCOMPARE(finished.count(), 0);
    f.backend.save(); QTRY_COMPARE(saved.count(), 1);
    QCOMPARE(cycle103Read(path), formatted.toUtf8()); QCOMPARE(failed.count(), 0);
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
    QVERIFY(page.backend.liveWrapSelection("**", "**"));
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
    QVERIFY(page.backend.liveWrapSelection("**", "**"));
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
    // Exhaust the genuine one-retry lifecycle before checking Source recovery.
    // An exhausted renderer must not reload again behind the Source editor.
    auto *pane = f.window->findChild<QObject *>("liveEditorPane"); QVERIFY(pane);
    auto *automaticRetry = pane->findChild<QObject *>("liveEditorAutomaticRetry"); QVERIFY(automaticRetry);
    auto *bridge = f.backend.editorBridge();
    QVERIFY(QMetaObject::invokeMethod(pane, "rendererStopped"));
    QVERIFY(!bridge->isReady());
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 15000);
    QCOMPARE(cycle144PageText(bridge), original);
    QVERIFY(pane->property("automaticRetryUsed").toBool());
    QVERIFY(!automaticRetry->property("running").toBool());
    QVERIFY(QMetaObject::invokeMethod(pane, "rendererStopped"));
    QVERIFY(!bridge->isReady());
    QVERIFY(!pane->property("loadError").toString().isEmpty());
    QVERIFY(!automaticRetry->property("running").toBool());
    const int stoppedGeneration = bridge->pageGeneration();
    // An explicit switch to Source must remain usable after renderer failure.
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "setSourceEditing"));
    QTRY_VERIFY(!f.backend.liveEditingActive());
    QVERIFY(!f.window->property("documentBarrierCallback").value<QJSValue>().isCallable());
    const QString source = QStringLiteral("Source recovery remains writable.\n");
    QVERIFY(f.editor->setProperty("text", source));
    f.backend.save(); // Source save remains synchronous, even without Live.
    QCOMPARE(cycle103Read(path), source.toUtf8());
    QCOMPARE(saved.count(), 1); QVERIFY(!f.backend.modified());
    // Check that Source remains saved beyond the real automatic retry delay.
    QTest::qWait(1200); // Beyond the real retry delay: this page must stay stopped.
    QVERIFY(!bridge->isReady());
    QVERIFY(!automaticRetry->property("running").toBool());
    QVERIFY(pane->property("automaticRetryUsed").toBool());
    QCOMPARE(bridge->pageGeneration(), stoppedGeneration);
    QCOMPARE(f.editor->property("text").toString(), source);
    QCOMPARE(cycle103Read(path), source.toUtf8());
    QCOMPARE(saved.count(), 1); QVERIFY(!f.backend.modified());
    QVERIFY(!pane->property("loadError").toString().isEmpty());
}

void FomawriteTest::liveInvalidatedPageRejectsRealChannelChanges() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    const QString path = directory.filePath("invalidated-page.md");
    const QString original = QStringLiteral("Known host text.\n");
    const QString source = QStringLiteral("Source recovery remains writable.\n");
    QVERIFY(cycle103Write(path, original.toUtf8()));
    // Keep the real renderer visible independently of Main's Source/error UI.
    // Invalidating its production bridge must reject stale page traffic; it
    // must not require new JavaScript work in a hidden/stopped Chromium view.
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    auto *bridge = page.bridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 15000);
    QVERIFY(page.backend.open(QUrl::fromLocalFile(path)));
    page.backend.syncLiveEditor();
    QCOMPARE(page.pageText(), original);
    auto *view = page.root->findChild<QQuickItem *>("liveEditorView"); QVERIFY(view);
    auto *pane = page.root->findChild<QQuickItem *>("liveEditorPane"); QVERIFY(pane);
    QVERIFY(page.window->isVisible()); QVERIFY(pane->isVisible()); QVERIFY(view->isVisible());
    QVERIFY(pane->property("loadError").toString().isEmpty());
    QVERIFY(!page.backend.liveEditingActive());
    bridge->invalidatePage();
    QVERIFY(!bridge->isReady());
    const int invalidatedGeneration = bridge->pageGeneration();
    QSignalSpy saved(&page.backend, &Backend::saveSucceeded);
    QVERIFY(page.editor->setProperty("text", source));
    QCOMPARE(page.canonical(), source);
    page.backend.save();
    QCOMPARE(saved.count(), 1); QCOMPARE(cycle103Read(path), source.toUtf8());
    QVERIFY(!page.backend.modified());
    QVERIFY(view->isVisible());
    auto *engine = &page.engine;
    QSignalSpy oldPageDispatch(bridge, &EditorBridge::metricRecorded);
    QSignalSpy oldPageChanges(bridge, &EditorBridge::changesReceived);
    // Submit the mutation once in the main world. Record callback completion
    // separately from its value so timeout/undefined cannot look like empty text.
    QJSValue probe = engine->newObject();
    probe.setProperty("completed", false);
    engine->globalObject().setProperty("liveStalePageProbe", probe);
    const QJSValue probeCallback = engine->evaluate(
        "(function(value) { liveStalePageProbe.value = value; liveStalePageProbe.completed = true; })");
    QVERIFY(probeCallback.isCallable());
    QVERIFY(QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, QStringLiteral(R"JS(
        (() => {
            try {
                window.fomawriteEditor.view.dom.dispatchEvent(new Event('beforeinput'));
                window.fomawriteEditor.simulateUserChanges('[{"from":0,"to":0,"insert":"late old renderer "}]');
                return JSON.stringify({text: window.fomawriteEditor.getText()});
            } catch (error) { return JSON.stringify({error: String(error)}); }
        })()
    )JS")), Q_ARG(uint, 0), Q_ARG(QJSValue, probeCallback)));
    const bool completed = QTest::qWaitFor([&] { return probe.property("completed").toBool(); }, 5000);
    const QJSValue result = probe.property("value");
    qInfo() << "Invalidated visible-page mutation callback completed" << completed
            << "result" << result.toString() << "generation" << bridge->pageGeneration()
            << "expected generation" << invalidatedGeneration << "ready" << bridge->isReady();
    QVERIFY2(completed, "Invalidated visible-page JavaScript callback did not complete within five seconds");
    QVERIFY2(result.isString(), "Invalidated visible-page callback completed without the expected JSON string");
    QJsonParseError parseError;
    const auto resultDocument = QJsonDocument::fromJson(result.toString().toUtf8(), &parseError);
    QVERIFY2(parseError.error == QJsonParseError::NoError && resultDocument.isObject(), qPrintable(result.toString()));
    const auto resultObject = resultDocument.object();
    QVERIFY2(!resultObject.contains("error"), qPrintable(resultObject.value("error").toString()));
    QCOMPARE(resultObject.value("text").toString(), QStringLiteral("late old renderer ") + original);
    // onDocChanged sends this metric immediately after documentChanged on
    // the same channel, proving delivery before the zero-accepted-change check.
    QTRY_VERIFY(std::any_of(oldPageDispatch.cbegin(), oldPageDispatch.cend(), [](const auto &args) {
        return args.at(0).toString() == QStringLiteral("keystroke-to-dispatch");
    }));
    QCOMPARE(oldPageChanges.count(), 0);
    QCOMPARE(page.canonical(), source);
    QCOMPARE(cycle103Read(path), source.toUtf8());
    QVERIFY(!page.backend.modified());
    QVERIFY(!bridge->isReady());
    QCOMPARE(bridge->pageGeneration(), invalidatedGeneration);
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
