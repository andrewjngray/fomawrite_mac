#include <QPdfDocument>

void FomawriteTest::publishingPreviewMatchesExportsWithoutChangingDraft() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
    QScopedPointer<QObject> editor(component.create()); QVERIFY2(editor, qPrintable(component.errorString()));
    QImage image(24, 24, QImage::Format_RGB32); image.fill(Qt::blue);
    QVERIFY(image.save(directory.filePath("image.png")));
    const QString source = QString::fromUtf8("# Publishing café\n\n![Local](image.png)\n\n[Local note](next.md)\n\n<!-- pagebreak -->\n\n# Publishing café\n\nSecond page, 你好.\n");
    editor->setProperty("text", source);
    const QUrl sourceUrl = QUrl::fromLocalFile(directory.filePath("source.md"));
    backend.saveAs(sourceUrl);
    QVERIFY(QMetaObject::invokeMethod(editor.data(), "insert", Q_ARG(int, source.size()), Q_ARG(QString, QStringLiteral("Unsaved ending."))));
    const QString draft = editor->property("text").toString();
    QVERIFY(backend.modified());
    const QString statusBefore = backend.status();
    QVERIFY(backend.setExportPaperSize("letter"));
    QVERIFY(backend.setExportOrientation("landscape"));
    backend.setOutputStyle(1);
    const auto html = backend.publishingPreview("html"); QVERIFY2(html["ok"].toBool(), qPrintable(html["error"].toString()));
    const auto pdf = backend.publishingPreview("pdf"); QVERIFY2(pdf["ok"].toBool(), qPrintable(pdf["error"].toString()));
    QCOMPARE(backend.status(), statusBefore);
    QCOMPARE(editor->property("text").toString(), draft); QVERIFY(backend.modified());
    QFile sourceFile(sourceUrl.toLocalFile()); QVERIFY(sourceFile.open(QIODevice::ReadOnly)); QCOMPARE(sourceFile.readAll(), source.toUtf8());
    QFile previewHtml(html["url"].toUrl().toLocalFile()); QVERIFY(previewHtml.open(QIODevice::ReadOnly));
    QString previewMarkup = QString::fromUtf8(previewHtml.readAll());
    QVERIFY(previewMarkup.contains("data:image/png;base64,"));
    QVERIFY(previewMarkup.contains("next.md"));
    QVERIFY(previewMarkup.contains(QString::fromUtf8("publishing-café-1")));
    previewMarkup.remove(QRegularExpression("<base href=\"[^\"]*\">"));
    const QUrl exportHtml = QUrl::fromLocalFile(directory.filePath("export.html"));
    QVERIFY(backend.exportDocument(exportHtml, "html"));
    QFile exportedHtml(exportHtml.toLocalFile()); QVERIFY(exportedHtml.open(QIODevice::ReadOnly));
    QCOMPARE(previewMarkup.toUtf8(), exportedHtml.readAll());
    const QUrl exportPdf = QUrl::fromLocalFile(directory.filePath("export.pdf"));
    QVERIFY(backend.exportDocument(exportPdf, "pdf"));
    QPdfDocument previewDocument, exportDocument;
    QCOMPARE(previewDocument.load(pdf["url"].toUrl().toLocalFile()), QPdfDocument::Error::None);
    QCOMPARE(exportDocument.load(exportPdf.toLocalFile()), QPdfDocument::Error::None);
    QVERIFY(previewDocument.pageCount() >= 2);
    QCOMPARE(previewDocument.pageCount(), exportDocument.pageCount());
    for (int page = 0; page < previewDocument.pageCount(); ++page) {
        QCOMPARE(previewDocument.pagePointSize(page), exportDocument.pagePointSize(page));
        QVERIFY(previewDocument.pagePointSize(page).width() > previewDocument.pagePointSize(page).height());
        const QImage previewImage = previewDocument.render(page, QSize(640, 480)); QVERIFY(!previewImage.isNull());
        QCOMPARE(previewImage, exportDocument.render(page, QSize(640, 480)));
    }
    const auto anchors = pdf["anchors"].toMap();
    QVERIFY(anchors.contains(QString::fromUtf8("publishing-café")));
    QVERIFY(anchors.contains(QString::fromUtf8("publishing-café-1")));
    QVERIFY(anchors[QString::fromUtf8("publishing-café-1")].toInt() > anchors[QString::fromUtf8("publishing-café")].toInt());
    QCOMPARE(editor->property("text").toString(), draft);
    backend.discardRecovery();
}

void FomawriteTest::publishingPreviewOwnsTemporaryFilesAndReportsErrors() {
    Cycle103SettingsSnapshot isolatedSettings;
    QString lastPreview;
    {
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
        QScopedPointer<QObject> editor(component.create()); QVERIFY2(editor, qPrintable(component.errorString()));
        editor->setProperty("text", "# Private preview\n\nDraft.");
        const QString draft = editor->property("text").toString();
        const QString status = backend.status();
        QVERIFY(!backend.publishingPreview("invalid")["ok"].toBool());
        QString firstPreview;
        for (int index = 0; index < 6; ++index) {
            const auto result = backend.publishingPreview("html"); QVERIFY(result["ok"].toBool());
            lastPreview = result["url"].toUrl().toLocalFile();
            if (!index) firstPreview = lastPreview;
            QVERIFY(QFileInfo::exists(lastPreview));
        }
        QVERIFY(!QFileInfo::exists(firstPreview));
        QCOMPARE(editor->property("text").toString(), draft);
        QCOMPARE(backend.status(), status);
        editor->setProperty("text", "![Missing](fomawrite-missing-publishing-image.png)");
        const QString changedStatus = backend.status();
        const auto failed = backend.publishingPreview("html");
        QVERIFY(!failed["ok"].toBool()); QVERIFY(!failed["error"].toString().isEmpty());
        QCOMPARE(backend.status(), changedStatus);
        backend.discardRecovery();
    }
    QVERIFY(!QFileInfo::exists(lastPreview));
}
