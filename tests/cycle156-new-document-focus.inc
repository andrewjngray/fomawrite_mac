// Cycle 156: after "+" creates a document, the keyboard is in its first line,
// in Source and in Live, not left on the "+" button.

void FomawriteTest::libraryPlusPutsTheCaretInTheNewDocument() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    cycle148Make(directory.filePath("Existing.md"));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    auto *library = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(library);
    library->setRootFolder(QUrl::fromLocalFile(directory.path()));
    f.layout->setProperty("organizerVisible", true); f.layout->setProperty("filesVisible", true);
    f.layout->setProperty("fileWidth", 288);
    QTRY_COMPARE(f.layout->property("effectiveFileWidth").toReal(), 288.0);
    auto *plus = f.window->findChild<QQuickItem *>("libraryNewButton"); QVERIFY(plus);
    auto *fileDialog = f.window->findChild<QObject *>("newLibraryFileDialog"); QVERIFY(fileDialog);
    auto *fileName = f.window->findChild<QObject *>("newLibraryFileName"); QVERIFY(fileName);
    QTRY_VERIFY(plus->isVisible());
    f.quick->requestActivate(); QTRY_VERIFY(f.quick->isActive());

    auto create = [&](const QString &name) {
        if (!cycle102Click(f.quick, plus)) return false;
        if (!QTest::qWaitFor([&] { return fileDialog->property("opened").toBool()
                                          && fileName->property("activeFocus").toBool(); }, 5000)) return false;
        fileName->setProperty("text", name);
        QTest::keyClick(f.quick, Qt::Key_Return); // the writer's own route: type a name, press Return
        return QTest::qWaitFor([&] { return !fileDialog->property("visible").toBool()
                                            && f.backend.fileName() == name; }, 5000);
    };

    // Source: the caret is in the new document's first line and typing goes there.
    f.layout->setProperty("liveEditEnabled", false);
    QVERIFY(create(QStringLiteral("Fresh.md")));
    QTRY_VERIFY(f.editor->hasActiveFocus());
    QVERIFY(!plus->hasActiveFocus());
    QCOMPARE(f.editor->property("cursorPosition").toInt(), 0);
    for (const QChar c : QStringLiteral("Hello")) QTest::keyClick(f.quick, c.toLatin1());
    QTRY_COMPARE(f.editor->property("text").toString(), QStringLiteral("Hello"));
    f.backend.save();
    QTRY_VERIFY(!f.backend.modified());

    // Live: the page holds the keyboard, with the caret at the start.
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    QVERIFY(f.layout->property("liveEditEnabled").toBool());
    auto *bridge = f.backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 45000); // the GitHub runner's first page load is slow
    auto *pane = f.window->findChild<QObject *>("liveEditorPane"); QVERIFY(pane);
    QVERIFY(create(QStringLiteral("Second.md")));
    QTRY_VERIFY_WITH_TIMEOUT(pane->property("hasLiveFocus").toBool(), 5000);
    QVERIFY(!plus->hasActiveFocus());
    QSignalSpy selection(bridge, &EditorBridge::selectionReceived);
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        const int token = bridge->fetchSelection(); QTest::qWait(60);
        for (const auto &args : selection)
            if (args.at(0).toInt() == token) return args.at(1).toInt() == 0 && args.at(2).toInt() == 0;
        return false;
    })(), 8000);
    f.backend.discardRecovery();
}
