// Deliver the real WebEngine signal without crashing a shared renderer process
// or depending on OS crash injection. Its normal QML handler performs real reloads.
static bool signalLiveRendererCrash(QObject *view) {
    for (int i = 0; i < view->metaObject()->methodCount(); ++i) {
        const auto method = view->metaObject()->method(i);
        if (method.name() != "renderProcessTerminated") continue;
        const auto types = method.parameterTypes();
        if (types.size() != 2) continue;
        const int crashed = 2; // QWebEnginePage::CrashedTerminationStatus
        const int exitCode = 1;
        return method.invoke(view, Qt::DirectConnection,
            QGenericArgument(types.at(0).constData(), &crashed),
            QGenericArgument(types.at(1).constData(), &exitCode));
    }
    return false;
}

void FomawriteTest::liveEditorRendererRecoveryIsBoundedAndPreservesCanonicalEdits() {
    Cycle103SettingsSnapshot settings;
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    auto *pane = page.root->findChild<QObject *>("liveEditorPane"); QVERIFY(pane);
    auto *view = page.root->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    const QString original = QString::fromUtf8("# Recovery\n\nCafé stays.\n");
    page.editor->setProperty("text", original);
    page.backend.syncLiveEditor();
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), original, 10000);
    const QString edited = original + QStringLiteral("Unsaved Live edit.\n");
    page.bridge()->injectUserChanges(QStringLiteral("[{\"from\":%1,\"to\":%1,\"insert\":\"Unsaved Live edit.\\n\"}]").arg(original.size()));
    QTRY_COMPARE_WITH_TIMEOUT(page.canonical(), edited, 10000);
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), edited, 10000);
    const int documentRevision = page.backend.documentRevision();
    QSignalSpy ready(page.bridge(), &EditorBridge::readyChanged);
    QSignalSpy notices(pane, SIGNAL(noticeRequested(QString)));
    QVERIFY(signalLiveRendererCrash(view));
    QTest::qWait(200);
    QCOMPARE(ready.count(), 0); // Give the UI time to recover before retrying.
    auto *automaticRetry = page.root->findChild<QObject *>("liveEditorAutomaticRetry");
    QVERIFY(automaticRetry);
    QVERIFY(automaticRetry->property("running").toBool());
    // Duplicate reports during the delay must not queue a second reload.
    QVERIFY(signalLiveRendererCrash(view));
    QTRY_COMPARE_WITH_TIMEOUT(ready.count(), 1, 15000);
    QVERIFY(!automaticRetry->property("running").toBool());
    pane->setProperty("visible", false); // Failure of a Source-hidden prewarm.
    QVERIFY(signalLiveRendererCrash(view));
    QTest::qWait(1600); // Beyond the retry delay: a second crash must stay stopped.
    QCOMPARE(ready.count(), 1);
    QVERIFY(!pane->property("loadError").toString().isEmpty());
    QVERIFY(!notices.isEmpty()); // Source receives the nonmodal workspace notice.
    pane->setProperty("visible", true); // Selecting Live exposes its inline error.
    auto *error = page.root->findChild<QQuickItem *>("liveEditorError"); QVERIFY(error);
    auto *retry = page.root->findChild<QQuickItem *>("liveEditorRetry"); QVERIFY(retry);
    QVERIFY(error->isVisible()); QVERIFY(retry->isVisible());
    QVERIFY(!notices.isEmpty());
    QCOMPARE(page.canonical(), edited);
    QCOMPARE(page.backend.documentRevision(), documentRevision);
    QCOMPARE(page.pageText(), edited);
    // Repeated crash reports cannot extend a retry timer or queue more reloads.
    QVERIFY(signalLiveRendererCrash(view));
    QTest::qWait(1200);
    QCOMPARE(ready.count(), 1);
    QCOMPARE(page.canonical(), edited);
    QVERIFY(cycle102Click(page.window, retry));
    QTRY_COMPARE_WITH_TIMEOUT(ready.count(), 2, 15000);
    QTRY_VERIFY(pane->property("loadError").toString().isEmpty());
    QVERIFY(!error->isVisible());
    QCOMPARE(page.pageText(), edited);
    QCOMPARE(page.canonical(), edited);
    QCOMPARE(page.backend.documentRevision(), documentRevision);
    // Live editing still mirrors after the explicit retry.
    page.bridge()->injectUserChanges(QStringLiteral("[{\"from\":0,\"to\":0,\"insert\":\"Recovered\\n\"}]"));
    QTRY_COMPARE_WITH_TIMEOUT(page.canonical(), QStringLiteral("Recovered\n") + edited, 10000);
    page.backend.discardRecovery();
}
