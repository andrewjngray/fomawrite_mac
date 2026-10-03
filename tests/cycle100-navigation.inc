// Definitions are outside FomawriteTest so moc sees ordinary slot declarations.
void FomawriteTest::navigationFragmentsKeepSnapshotsAndReportMissing() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto write = [&](const QString &name, const QString &text) {
        QFile file(directory.filePath(name));
        if (!file.open(QIODevice::WriteOnly)) return QUrl();
        if (file.write(text.toUtf8()) != text.toUtf8().size()) return QUrl();
        return QUrl::fromLocalFile(file.fileName());
    };
    const QString sourceText = "# Source\n\nKeep this document and its selection.\n";
    const QString targetText = QString::fromUtf8("# Same\n\nFirst.\n\n# Same\n\nSecond.\n\n# Café 你好\n\nUnicode.\n");
    const QUrl sourceUrl = write("Source.md", sourceText);
    const QUrl targetUrl = write("Target.md", targetText);
    QVERIFY(!sourceUrl.isEmpty() && !targetUrl.isEmpty());
    Backend source;
    Backend target;
    QQmlEngine sourceEngine;
    QQmlEngine targetEngine;
    sourceEngine.rootContext()->setContextProperty("backend", &source);
    targetEngine.rootContext()->setContextProperty("backend", &target);
    QQmlComponent sourceComponent(&sourceEngine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QQmlComponent targetComponent(&targetEngine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> sourceWindow(sourceComponent.create());
    QScopedPointer<QObject> targetWindow(targetComponent.create());
    QVERIFY2(sourceWindow, qPrintable(sourceComponent.errorString()));
    QVERIFY2(targetWindow, qPrintable(targetComponent.errorString()));
    auto *sourceEditor = sourceWindow->findChild<QObject *>("sourceEditor");
    auto *targetEditor = targetWindow->findChild<QObject *>("sourceEditor");
    auto *notice = targetWindow->findChild<QObject *>("navigationNotice");
    QVERIFY(sourceEditor && targetEditor && notice);
    QVERIFY(source.open(sourceUrl));
    QVERIFY(target.open(targetUrl));
    // documentLoaded resets the caret in Qt.callLater. Let that normal open
    // lifecycle finish before establishing the selection this test preserves.
    sourceEditor->setProperty("cursorPosition", sourceText.size());
    targetEditor->setProperty("cursorPosition", targetText.size());
    QTRY_COMPARE(sourceEditor->property("cursorPosition").toInt(), 0);
    QTRY_COMPARE(targetEditor->property("cursorPosition").toInt(), 0);
    QVERIFY(QMetaObject::invokeMethod(sourceEditor, "select", Q_ARG(int, 4), Q_ARG(int, 10)));
    const QString dirtyTarget = targetText + "\n# Live unsaved heading\n";
    QVERIFY(QMetaObject::invokeMethod(targetEditor, "insert", Q_ARG(int, targetText.size()),
                                     Q_ARG(QString, QStringLiteral("\n# Live unsaved heading\n"))));
    QVERIFY(target.modified());
    int forwards = 0;
    source.focusExistingDocument = [&](const QUrl &url) {
        if (url.toLocalFile() != targetUrl.toLocalFile()) return false;
        ++forwards;
        return QMetaObject::invokeMethod(targetWindow.data(), "navigateDocumentFragment",
            Q_ARG(QVariant, url.fragment(QUrl::FullyEncoded)));
    };
    QUrl linked = targetUrl;
    linked.setFragment("same-1");
    QVERIFY(QMetaObject::invokeMethod(sourceWindow.data(), "requestOpen", Q_ARG(QVariant, linked)));
    QCOMPARE(forwards, 1);
    QTRY_COMPARE(targetEditor->property("cursorPosition").toInt(), targetText.lastIndexOf("# Same"));
    QCOMPARE(source.fileUrl(), sourceUrl);
    QCOMPARE(sourceEditor->property("text").toString(), sourceText);
    QCOMPARE(sourceEditor->property("selectionStart").toInt(), 4);
    QCOMPARE(sourceEditor->property("selectionEnd").toInt(), 10);
    QCOMPARE(targetEditor->property("text").toString(), dirtyTarget);
    QVERIFY(target.modified());
    QVERIFY(targetEditor->property("canUndo").toBool());
    QVERIFY2(!notice->property("visible").toBool(), qPrintable(targetWindow->property("navigationNotice").toString()));

    linked.setFragment(QString::fromUtf8("café-你好"));
    QVERIFY(QMetaObject::invokeMethod(sourceWindow.data(), "requestOpen", Q_ARG(QVariant, linked)));
    QTRY_COMPARE(targetEditor->property("cursorPosition").toInt(), targetText.indexOf(QString::fromUtf8("# Café 你好")));
    linked.setFragment("live-unsaved-heading");
    QVERIFY(QMetaObject::invokeMethod(sourceWindow.data(), "requestOpen", Q_ARG(QVariant, linked)));
    QTRY_COMPARE(targetEditor->property("cursorPosition").toInt(), dirtyTarget.indexOf("# Live"));
    const int targetCursor = targetEditor->property("cursorPosition").toInt();
    linked.setFragment("missing-heading");
    QVERIFY(QMetaObject::invokeMethod(sourceWindow.data(), "requestOpen", Q_ARG(QVariant, linked)));
    QTRY_VERIFY(notice->property("visible").toBool());
    QVERIFY(targetWindow->property("navigationNotice").toString().contains("missing-heading"));
    QCOMPARE(targetEditor->property("cursorPosition").toInt(), targetCursor);
    QCOMPARE(targetEditor->property("text").toString(), dirtyTarget);

    // Invalid addresses produce visible feedback and never reposition the caret.
    QVERIFY(QMetaObject::invokeMethod(targetWindow.data(), "navigateDocumentFragment",
                                     Q_ARG(QVariant, QStringLiteral("bad%ZZ"))));
    QVERIFY(targetWindow->property("navigationNotice").toString().contains("invalid"));
    QCOMPARE(targetEditor->property("cursorPosition").toInt(), targetCursor);
    QVERIFY(QMetaObject::invokeMethod(targetEditor, "undo"));
    QCOMPARE(targetEditor->property("text").toString(), targetText);

    // A dirty source must ask first; Cancel neither activates nor navigates away.
    QVERIFY(QMetaObject::invokeMethod(sourceEditor, "insert", Q_ARG(int, sourceText.size()),
                                     Q_ARG(QString, QStringLiteral("Draft."))));
    const QString dirtySource = sourceEditor->property("text").toString();
    const int beforeCancel = forwards;
    linked.setFragment("same-1");
    QVERIFY(QMetaObject::invokeMethod(sourceWindow.data(), "requestOpen", Q_ARG(QVariant, linked)));
    QCOMPARE(forwards, beforeCancel);
    QCOMPARE(sourceWindow->property("pendingAction").toString(), QStringLiteral("open"));
    QVERIFY(QMetaObject::invokeMethod(sourceWindow->findChild<QObject *>("unsavedChangesPrompt"), "cancelRequested"));
    QCOMPARE(forwards, beforeCancel);
    QCOMPARE(sourceEditor->property("text").toString(), dirtySource);
    QVERIFY(source.modified());
    source.discardRecovery();
    target.discardRecovery();
}

void FomawriteTest::explicitDocumentViewsPreserveFragments() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QFile file(directory.filePath("Target.md"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("# Target\n");
    file.close();
    Backend backend;
    QSignalSpy newTab(&backend, &Backend::newTabRequested);
    QSignalSpy newWindow(&backend, &Backend::newWindowRequested);
    QUrl url = QUrl::fromLocalFile(file.fileName());
    url.setFragment(QString::fromUtf8("café-你好-100%"), QUrl::DecodedMode);
    QVERIFY(backend.openInNewTab(url));
    QVERIFY(backend.openInNewWindow(url));
    QCOMPARE(newTab.size(), 1);
    QCOMPARE(newWindow.size(), 1);
    const QUrl tabUrl = newTab.takeFirst().first().toUrl();
    const QUrl windowUrl = newWindow.takeFirst().first().toUrl();
    QCOMPARE(tabUrl.fragment(QUrl::FullyDecoded), url.fragment(QUrl::FullyDecoded));
    QCOMPARE(windowUrl, tabUrl);
    QCOMPARE(QFileInfo(tabUrl.toLocalFile()).canonicalFilePath(), QFileInfo(file.fileName()).canonicalFilePath());
}
