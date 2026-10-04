
void FomawriteTest::visualObjectsKeepMarkdownUndoAndLocalImages() {
    QTemporaryDir fixtures;
    QVERIFY(fixtures.isValid());
    QImage image(120, 80, QImage::Format_ARGB32);
    image.fill(QColor("#467c92"));
    QVERIFY(image.save(fixtures.filePath("sample.png")));
    const QString source = QString::fromUtf8("# Visual objects\n\n![Coast 📝](sample.png)\n\n| Name | Value |\n| :--- | ---: |\n| Alpha | 12 |\n\n- [x] Finished\n");
    QFile file(fixtures.filePath("sample.md"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(source.toUtf8()); file.close();
    Backend backend;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create());
    QVERIFY2(window, qPrintable(component.errorString()));
    auto *editor = window->findChild<QObject *>("sourceEditor");
    auto *pane = window->findChild<QObject *>("visualEditorPane");
    auto *visual = window->findChild<QObject *>("visualEditor");
    auto *thumbnail = pane->findChild<QObject *>("visualImageThumbnail");
    auto *inspect = pane->findChild<QObject *>("visualImageInspectButton");
    QVERIFY(editor && pane && visual && thumbnail && inspect);
    window->setProperty("width", 1280); window->setProperty("height", 820);
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    QVERIFY(quick); quick->show();
    QVERIFY(backend.open(QUrl::fromLocalFile(file.fileName())));
    editor->setProperty("cursorPosition", source.size());
    QTRY_COMPARE(editor->property("cursorPosition").toInt(), 0);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("visual"))));
    QTRY_VERIFY(visual->property("text").toString().contains("Coast"));
    auto projected = backend.visualProjection();
    const QVariantList images = projected.value("images").toList();
    QCOMPARE(images.size(), 1);
    QCOMPARE(QUrl(images[0].toMap().value("previewUrl").toString()).toLocalFile(), fixtures.filePath("sample.png"));
    QTest::qWait(150);
    QVERIFY(pane->property("visible").toBool());
    QVERIFY(visual->setProperty("cursorPosition", images[0].toMap().value("visualStart").toInt() + 2));
    QCOMPARE(visual->property("cursorPosition").toInt(), images[0].toMap().value("visualStart").toInt() + 2);
    QCOMPARE(pane->property("visualImages").toList().size(), 1);
    QTRY_VERIFY(inspect->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(inspect, "clicked"));
    QTRY_COMPARE(thumbnail->property("status").toInt(), 1); // Image.Ready
    if (const QString capture = qEnvironmentVariable("FOMAWRITE_IMAGE_CAPTURE"); !capture.isEmpty()) {
        QTest::qWait(250);
        QVERIFY(quick->grabWindow().save(capture));
    }
    auto *popup = pane->findChild<QObject *>("visualImageInspector");
    QVERIFY(popup);
    QVERIFY(QMetaObject::invokeMethod(popup, "close"));
    QString next = visual->property("text").toString();
    next.replace("Coast", "Harbour");
    QVERIFY(visual->setProperty("text", next));
    QTRY_COMPARE(editor->property("text").toString(), QString(source).replace("Coast", "Harbour"));
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QTRY_COMPARE(editor->property("text").toString(), source);
    QTRY_VERIFY(visual->property("text").toString().contains("Coast"));
    next = visual->property("text").toString(); next.replace("Alpha", "Beta");
    QVERIFY(visual->setProperty("text", next));
    QTRY_COMPARE(editor->property("text").toString(), QString(source).replace("Alpha", "Beta"));
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QTRY_COMPARE(editor->property("text").toString(), source);
    projected = backend.visualProjection();
    const int end = projected.value("visualText").toString().indexOf("Finished") + 8;
    const auto broken = backend.applyVisualBreak(end, false, source);
    QVERIFY(broken.value("applied").toBool());
    QCOMPARE(editor->property("text").toString(), QString(source).replace("Finished\n", "Finished\n- [ ] \n"));
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QCOMPARE(editor->property("text").toString(), source);
    QVERIFY(!backend.applyVisualBreak(end, false, source + "stale").value("applied").toBool());
    QCOMPARE(editor->property("text").toString(), source);
    // The inspector never initiates remote image requests.
    QVERIFY(editor->setProperty("text", "![Remote](https://example.invalid/image.png)\n"));
    const auto remote = backend.visualProjection().value("images").toList();
    QCOMPARE(remote.size(), 1);
    QVERIFY(remote[0].toMap().value("previewUrl").toString().isEmpty());
    backend.discardRecovery();
}

void FomawriteTest::visualEditingKeepsCaretVisibleAndUnicodeIntact() {
    Backend backend;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create());
    QVERIFY2(window, qPrintable(component.errorString()));
    auto *editor = window->findChild<QObject *>("sourceEditor");
    auto *pane = window->findChild<QObject *>("visualEditorPane");
    auto *visual = window->findChild<QObject *>("visualEditor");
    auto *scroll = pane->findChild<QObject *>("visualEditorScroll");
    QVERIFY(editor && pane && visual && scroll);
    window->setProperty("width", 1200); window->setProperty("height", 700);
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    QVERIFY(quick); quick->show(); quick->requestActivate();
    QVERIFY(editor->setProperty("text", QString::fromUtf8("A 📝 note.\n")));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("visual"))));
    QTRY_COMPARE(visual->property("text").toString(), QString::fromUtf8("A 📝 note.\n"));
    QVERIFY(visual->setProperty("text", QString::fromUtf8("A 📚 note.\n")));
    QTRY_COMPARE(editor->property("text").toString(), QString::fromUtf8("A 📚 note.\n"));
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QTRY_COMPARE(editor->property("text").toString(), QString::fromUtf8("A 📝 note.\n"));
    for (const auto &pair : {qMakePair(QString::fromUtf8("A café."), QString::fromUtf8("A cafè.")),
                             qMakePair(QString::fromUtf8("A 👍🏻."), QString::fromUtf8("A 👍🏽."))}) {
        QVERIFY(editor->setProperty("text", pair.first));
        QTRY_COMPARE(visual->property("text").toString(), pair.first);
        QVERIFY(visual->setProperty("text", pair.second));
        QTRY_COMPARE(editor->property("text").toString(), pair.second);
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QTRY_COMPARE(editor->property("text").toString(), pair.first);
    }
    QString longSource;
    for (int i = 0; i < 100; ++i) longSource += QString("Paragraph %1 with words to read and edit.\n\n").arg(i);
    QVERIFY(editor->setProperty("text", longSource));
    QTRY_COMPARE(visual->property("text").toString(), longSource);
    QVERIFY(QMetaObject::invokeMethod(pane, "focusVisualEditor"));
    QVERIFY(visual->setProperty("cursorPosition", longSource.size() - 2));
    QTRY_VERIFY(scroll->property("contentY").toReal() > 500);
    const QRectF caret = visual->property("cursorRectangle").toRectF();
    const qreal bottom = visual->property("y").toReal() + caret.bottom();
    QVERIFY(bottom <= scroll->property("contentY").toReal() + scroll->property("height").toReal() + 1);
    if (const QString capture = qEnvironmentVariable("FOMAWRITE_VISUAL_CAPTURE"); !capture.isEmpty()) {
        QTest::qWait(200);
        QVERIFY(quick->grabWindow().save(capture));
    }
    // Rejected edits remain explained even in a narrow visual pane.
    window->setProperty("width", 540);
    QVERIFY(editor->setProperty("text", "```\nprotected\n```\n"));
    QTRY_COMPARE(visual->property("text").toString(), QString("```\nprotected\n```\n"));
    QVERIFY(visual->setProperty("text", "```\nchanged\n```\n"));
    QTRY_COMPARE(editor->property("text").toString(), QString("```\nprotected\n```\n"));
    auto *notice = pane->findChild<QObject *>("visualEditNotice");
    auto *sourceAction = pane->findChild<QObject *>("visualNoticeSourceButton");
    QVERIFY(notice && sourceAction);
    QTRY_VERIFY(notice->property("visible").toBool());
    QVERIFY(sourceAction->property("visible").toBool());
    QVERIFY(pane->property("visualNotice").toString().contains("Source"));
    // A blank new document is still a writable visual surface.
    QVERIFY(editor->setProperty("text", ""));
    QTRY_COMPARE(visual->property("text").toString(), QString());
    QVERIFY(visual->setProperty("text", "New paragraph"));
    QTRY_COMPARE(editor->property("text").toString(), QString("New paragraph"));
    backend.discardRecovery();
}
