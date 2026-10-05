#include "publishinghtml.h"
#include "publishingthemes.h"
#include "publishingpdf.h"
#include <QPdfSelection>

void FomawriteTest::publishingSemanticHtmlPreservesStructureAndEscapesContent() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    QFile csv(directory.filePath("cells.csv")); QVERIFY(csv.open(QIODevice::WriteOnly));
    csv.write("Name,Value\n\"<script>\",\"first\nsecond & third\"\n"); csv.close();
    const QString source = QString::fromUtf8(
        "# Café *title*\n# Café title\n# Café title-1\n\n"
        "> Outer\n>\n> > Inner\n\n- First\n  - Nested\n\n"
        "| Name | Value |\n|---|---:|\n| Alpha | 42 |\n\n"
        "```cpp\n<script>alert(1)</script>\n<!-- pagebreak -->\n```\n\n"
        "`<inline>`\n\n---\n\n<!-- pagebreak -->\n\n"
        "[unsafe](jav&#x61;script:alert%281%29) ![unsafe](data:text/html,boom)\n\n"
        "<img src=x onerror=alert(1)>\n\n"
        "Note[^a] and ==highlight==.\n\n[^a]: Note body.\n\n/cells.csv\n");
    QString error;
    const auto html = PublishingHtml::body(expandedMarkdown(source, QUrl::fromLocalFile(directory.path() + '/')), &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(html.contains(QString::fromUtf8("<h1 id=\"café-title\">Café <em>title</em></h1>")));
    QVERIFY(html.contains(QString::fromUtf8("id=\"café-title-1-1\"")));
    QCOMPARE(html.count("<blockquote>"), 2); QCOMPARE(html.count("<ul>"), 2);
    QVERIFY(html.contains("<thead>")); QVERIFY(html.contains("<tbody>"));
    QVERIFY(html.contains("<th style=\"text-align:right\">Value</th>"));
    QVERIFY(html.contains("<pre><code class=\"language-cpp\">&lt;script&gt;"));
    QVERIFY(html.contains("<code>&lt;inline&gt;</code>")); QVERIFY(html.contains("<hr>"));
    QCOMPARE(html.count("class=\"pagebreak\""), 1);
    QVERIFY(html.contains("id=\"ow-note-0-a\"")); QVERIFY(html.contains("href=\"#ow-note-0-a-ref-1\""));
    QVERIFY(html.contains("<span class=\"highlight\">highlight</span>"));
    QVERIFY(html.contains("<td>&lt;script&gt;</td>"));
    QVERIFY(html.contains("first\nsecond &amp; third"));
    QVERIFY(!html.contains("href=\"javascript:")); QVERIFY(!html.contains("src=\"data:"));
    QVERIFY(!html.contains("<img src=x")); QVERIFY(!html.contains("<script>"));
    QVERIFY(!PublishingHtml::body("inline <!-- pagebreak --> inline").contains("class=\"pagebreak\""));
}

void FomawriteTest::publishingThemeImportEmbedsLocalAssetsAndRejectsNetwork() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    QImage image(8, 8, QImage::Format_RGB32); image.fill(Qt::red);
    QVERIFY(image.save(directory.filePath("paper.png")));
    QVERIFY(QFile::copy(QFINDTESTDATA("../fonts/iAWriterMonoS-Regular.ttf"), directory.filePath("body.ttf")));
    QFile css(directory.filePath("fixture.css")); QVERIFY(css.open(QIODevice::WriteOnly));
    css.write("@font-face{font-family:Fixture;src:url(body.ttf)} #write{background-image:url(paper.png);color:#1177cc}"); css.close();
    QString id, error;
    QVERIFY2(PublishingThemes::importTheme(QUrl::fromLocalFile(css.fileName()), &id, &error), qPrintable(error));
    const QString managed = QDir(PublishingThemes::directory()).filePath(id + ".css");
    const auto cleanup = qScopeGuard([&] { QFile::remove(managed); });
    const auto embedded = PublishingThemes::css(id, &error); QVERIFY2(embedded.has_value(), qPrintable(error));
    QVERIFY(embedded->contains("data:image/png;base64,")); QVERIFY(embedded->contains("data:font/ttf;base64,"));
    QVERIFY(!embedded->contains("url(body.ttf)")); QVERIFY(!embedded->contains("url(paper.png)"));
    for (const QByteArray &unsafe : {QByteArray("#write{background:url(https://example.com/a.png)}"),
                                  QByteArray("@import url(https://example.com/theme.css);"),
                                  QByteArray("#write{background:url(../outside.png)}")}) {
        QVERIFY(css.open(QIODevice::WriteOnly | QIODevice::Truncate)); css.write(unsafe); css.close();
        QString rejectedId;
        QVERIFY(!PublishingThemes::importTheme(QUrl::fromLocalFile(css.fileName()), &rejectedId, &error));
        QVERIFY(rejectedId.isEmpty()); QVERIFY(!error.isEmpty());
    }
}

void FomawriteTest::publishingSharedThemePdfPreservesPaperBreaksDraftAndUndo() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    QFile css(directory.filePath("blue-fixture.css")); QVERIFY(css.open(QIODevice::WriteOnly));
    css.write("#write h1 {background:#1177cc !important;color:white !important;padding:20px;} @media print{*{-webkit-print-color-adjust:exact;print-color-adjust:exact;}}"); css.close();
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
    QScopedPointer<QObject> editor(component.create()); QVERIFY2(editor, qPrintable(component.errorString()));
    const QString source = "# First theme page\n\nBody one.\n\n<!-- pagebreak -->\n\n# Second theme page\n\nBody two.\n";
    editor->setProperty("text", source);
    const QUrl sourceUrl = QUrl::fromLocalFile(directory.filePath("source.md")); backend.saveAs(sourceUrl);
    QVERIFY(QMetaObject::invokeMethod(editor.data(), "insert", Q_ARG(int, source.size()), Q_ARG(QString, QString("Unsaved ending."))));
    const auto draft = editor->property("text").toString(); QVERIFY(backend.modified());
    QVERIFY(backend.importPublishingTheme(QUrl::fromLocalFile(css.fileName())));
    const QString themeId = backend.publishingThemeId();
    const auto cleanup = qScopeGuard([&] { QFile::remove(QDir(PublishingThemes::directory()).filePath(themeId + ".css")); });
    QVERIFY(backend.setExportPaperSize("letter")); QVERIFY(backend.setExportOrientation("landscape"));
    const auto web = backend.publishingPreview("html"); QVERIFY2(web["ok"].toBool(), qPrintable(web["error"].toString()));
    QFile htmlFile(web["url"].toUrl().toLocalFile()); QVERIFY(htmlFile.open(QIODevice::ReadOnly));
    const auto html = htmlFile.readAll(); QVERIFY(html.contains("#1177cc")); QVERIFY(html.contains("<article id=\"write\">"));
    QVERIFY(html.contains("class=\"pagebreak\""));
    const auto pdf = backend.publishingPreview("pdf"); QVERIFY2(pdf["ok"].toBool(), qPrintable(pdf["error"].toString()));
    QPdfDocument document; QCOMPARE(document.load(pdf["url"].toUrl().toLocalFile()), QPdfDocument::Error::None);
    QCOMPARE(document.pageCount(), 2);
    for (int page = 0; page < document.pageCount(); ++page) {
        const auto paper = document.pagePointSize(page);
        QVERIFY(qAbs(paper.width() - 792) < 2); QVERIFY(qAbs(paper.height() - 612) < 2);
        QVERIFY(document.getAllText(page).text().contains(page ? "Second theme page" : "First theme page"));
        const auto pixels = document.render(page, QSize(792, 612)); QVERIFY(!pixels.isNull());
        int blue = 0;
        for (int y = 0; y < pixels.height(); ++y) for (int x = 0; x < pixels.width(); ++x) {
            const QColor c = pixels.pixelColor(x, y);
            if (qAbs(c.red() - 17) < 5 && qAbs(c.green() - 119) < 5 && qAbs(c.blue() - 204) < 5) ++blue;
        }
        QVERIFY2(blue > 500, "Imported CSS heading background must reach the actual browser PDF.");
    }
    QCOMPARE(editor->property("text").toString(), draft); QVERIFY(backend.modified());
    QFile saved(sourceUrl.toLocalFile()); QVERIFY(saved.open(QIODevice::ReadOnly)); QCOMPARE(saved.readAll(), source.toUtf8());
    QVERIFY(QMetaObject::invokeMethod(editor.data(), "undo")); QCOMPARE(editor->property("text").toString(), source);
    QVERIFY(QMetaObject::invokeMethod(editor.data(), "redo")); QCOMPARE(editor->property("text").toString(), draft);
    // Page furniture is CSS text: an HTML style terminator must never escape it.
    QFile furniture(directory.filePath("furniture.json")); QVERIFY(furniture.open(QIODevice::WriteOnly));
    furniture.write(QJsonDocument(QJsonObject{{"fontFamily", "Georgia"}, {"pointSize", 12},
        {"header", "</style><script>alert(1)</script>"}, {"footer", "</style><img src=x onerror=alert(1)>"}}).toJson());
    furniture.close(); QVERIFY(backend.loadOutputStyle(QUrl::fromLocalFile(furniture.fileName())));
    const auto protectedWeb = backend.publishingPreview("html"); QVERIFY(protectedWeb["ok"].toBool());
    QFile protectedFile(protectedWeb["url"].toUrl().toLocalFile()); QVERIFY(protectedFile.open(QIODevice::ReadOnly));
    const auto protectedHtml = protectedFile.readAll();
    QCOMPARE(protectedHtml.count("</style>"), 1); QVERIFY(!protectedHtml.contains("<script>"));
    QVERIFY(!protectedHtml.contains("<img src=x"));
    QCOMPARE(editor->property("text").toString(), draft);
    backend.discardRecovery();
}

void FomawriteTest::publishingAsyncPreviewKeepsLatestRequest() {
    Cycle103SettingsSnapshot settings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
    QScopedPointer<QObject> editor(component.create()); QVERIFY2(editor, qPrintable(component.errorString()));
    QObject consumer;
    QSignalSpy ready(&backend, &Backend::publishingPreviewReady);
    editor->setProperty("text", "# Superseded draft\n");
    const auto first = backend.requestPublishingPreview("pdf", &consumer); QVERIFY(first["ok"].toBool());
    editor->setProperty("text", "# Latest draft\n\n![Missing](fomawrite-async-missing-image.png)\n");
    const auto latest = backend.requestPublishingPreview("pdf", &consumer); QVERIFY(latest["ok"].toBool());
    const quint64 latestId = latest["requestId"].toULongLong(); QVERIFY(latestId > first["requestId"].toULongLong());
    QVariantMap result;
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        for (const auto &arguments : ready) if (arguments[0].toULongLong() == latestId) {
            result = arguments[1].toMap(); return true;
        }
        return false;
    })(), 35000);
    QVERIFY2(result["ok"].toBool(), qPrintable(result["error"].toString()));
    QPdfDocument document; QCOMPARE(document.load(result["url"].toUrl().toLocalFile()), QPdfDocument::Error::None);
    QVERIFY(!result["warning"].toString().isEmpty());
    QVERIFY(document.getAllText(0).text().contains("Latest draft"));
    QVERIFY(document.getAllText(0).text().contains("Image unavailable: Missing"));
    QVERIFY(!document.getAllText(0).text().contains("Superseded draft"));
    for (const auto &arguments : ready) if (arguments[0].toULongLong() != latestId)
        QVERIFY(!arguments[1].toMap()["ok"].toBool());
    QCOMPARE(editor->property("text").toString(), QString("# Latest draft\n\n![Missing](fomawrite-async-missing-image.png)\n"));
    backend.discardRecovery();
}

void FomawriteTest::publishingImagesKeepIncompletePreviewsReadableAndExportsStrict() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    QImage image(8, 8, QImage::Format_RGB32); image.fill(Qt::red);
    QVERIFY(image.save(directory.filePath("local & valid.png")));
    QFile corrupt(directory.filePath("corrupt.png")); QVERIFY(corrupt.open(QIODevice::WriteOnly));
    corrupt.write(QByteArray::fromHex("89504e470d0a1a0a")); corrupt.close();
    QFile oversized(directory.filePath("oversized.png")); QVERIFY(oversized.open(QIODevice::WriteOnly));
    QVERIFY(oversized.resize(5 * 1024 * 1024 + 1)); oversized.close();
    const QString source = "# Still readable\n\nBefore images.\n\n![Local](<local & valid.png>)\n\n"
        "![Missing & <unsafe>](missing.png)\n\n![Remote](https://example.com/image.png)\n\n"
        "![Corrupt](corrupt.png)\n\n![Oversized](oversized.png)\n\nAfter images.\n";
    QString error, warning;
    const auto baseUrl = QUrl::fromLocalFile(directory.path() + '/');
    const auto rendered = PublishingHtml::body(source, &error); QVERIFY(error.isEmpty());
    const auto embedded = PublishingHtml::embedImages(rendered, baseUrl, true, &error, &warning);
    QVERIFY2(error.isEmpty(), qPrintable(error)); QVERIFY(!warning.isEmpty());
    QVERIFY(embedded.contains("Before images.")); QVERIFY(embedded.contains("After images."));
    QVERIFY(embedded.contains("data:image/png;base64,"));
    QCOMPARE(embedded.count("class=\"fomawrite-image-placeholder\""), 4);
    QVERIFY(embedded.contains("Missing &amp; &lt;unsafe&gt;")); QVERIFY(!embedded.contains("<unsafe>"));
    QVERIFY(!embedded.contains("https://example.com/image.png"));
    QVERIFY(PublishingHtml::embedImages(rendered, baseUrl, false, &error).isEmpty()); QVERIFY(!error.isEmpty());
    const auto valid = PublishingHtml::embedImages(PublishingHtml::body("![Local](<local & valid.png>)"), baseUrl, false, &error, &warning);
    QVERIFY(error.isEmpty()); QVERIFY(warning.isEmpty()); QVERIFY(valid.contains("data:image/png;base64,"));
    // The total limit also leaves later text visible; repeated bounded PNG data
    // triggers it without allocating or decoding an oversized bitmap.
    QFile padded(directory.filePath("padded.png")); QVERIFY(padded.open(QIODevice::WriteOnly));
    QFile local(directory.filePath("local & valid.png")); QVERIFY(local.open(QIODevice::ReadOnly));
    padded.write(local.readAll()); QVERIFY(padded.resize(5 * 1024 * 1024)); padded.close();
    const auto totalMarkup = PublishingHtml::body(QString("![Repeated](padded.png)\n\n").repeated(5) + "After total limit.");
    const auto totalPreview = PublishingHtml::embedImages(totalMarkup, baseUrl, true, &error, &warning);
    QVERIFY(error.isEmpty()); QVERIFY(!warning.isEmpty());
    QCOMPARE(totalPreview.count("data:image/png;base64,"), 4);
    QCOMPARE(totalPreview.count("fomawrite-image-placeholder"), 1);
    QVERIFY(totalPreview.contains("After total limit."));
    QVERIFY(PublishingHtml::embedImages(totalMarkup, baseUrl, false, &error).isEmpty()); QVERIFY(!error.isEmpty());

    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
    QScopedPointer<QObject> editor(component.create()); QVERIFY2(editor, qPrintable(component.errorString()));
    editor->setProperty("text", source);
    backend.saveAs(QUrl::fromLocalFile(directory.filePath("source.md")));
    const auto web = backend.publishingPreview("html"); QVERIFY(web["ok"].toBool()); QVERIFY(!web["warning"].toString().isEmpty());
    QFile webFile(web["url"].toUrl().toLocalFile()); QVERIFY(webFile.open(QIODevice::ReadOnly));
    const auto webHtml = webFile.readAll(); QVERIFY(webHtml.contains("Still readable")); QVERIFY(webHtml.contains("After images."));
    const auto pdf = backend.publishingPreview("pdf"); QVERIFY2(pdf["ok"].toBool(), qPrintable(pdf["error"].toString()));
    QVERIFY(!pdf["warning"].toString().isEmpty());
    QPdfDocument pdfDocument; QCOMPARE(pdfDocument.load(pdf["url"].toUrl().toLocalFile()), QPdfDocument::Error::None);
    QVERIFY(pdfDocument.getAllText(0).text().contains("After images."));
    QVERIFY(!backend.exportDocument(QUrl::fromLocalFile(directory.filePath("strict.html")), "html"));
    QVERIFY(!backend.exportDocument(QUrl::fromLocalFile(directory.filePath("strict.pdf")), "pdf"));
    QCOMPARE(editor->property("text").toString(), source);
    backend.discardRecovery();
}

void FomawriteTest::publishingPreviewCacheTracksAssetsAndConsumerLifetime() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    QImage image(8, 8, QImage::Format_RGB32); image.fill(Qt::red);
    const auto imagePath = directory.filePath("asset.png"); QVERIFY(image.save(imagePath));
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
    QScopedPointer<QObject> editor(component.create()); QVERIFY2(editor, qPrintable(component.errorString()));
    const QString source = "# Cached draft\n\n![Asset](asset.png)\n";
    editor->setProperty("text", source); backend.saveAs(QUrl::fromLocalFile(directory.filePath("source.md")));
    auto *webConsumer = new QObject;
    const auto firstWeb = backend.requestPublishingPreview("html", webConsumer); QVERIFY(firstWeb["ok"].toBool());
    const QUrl firstWebUrl = firstWeb["url"].toUrl();
    for (int i = 0; i < 5; ++i)
        QCOMPARE(backend.requestPublishingPreview("html", webConsumer)["url"].toUrl(), firstWebUrl);
    image.fill(Qt::blue); QVERIFY(image.save(imagePath));
    const auto changedAsset = backend.requestPublishingPreview("html", webConsumer); QVERIFY(changedAsset["ok"].toBool());
    QVERIFY(changedAsset["url"].toUrl() != firstWebUrl);
    QFile changedFile(changedAsset["url"].toUrl().toLocalFile()); QVERIFY(changedFile.open(QIODevice::ReadOnly));
    QFile assetFile(imagePath); QVERIFY(assetFile.open(QIODevice::ReadOnly));
    QVERIFY(changedFile.readAll().contains(assetFile.readAll().toBase64()));
    editor->setProperty("text", source + "Changed document.");
    const auto changedDocument = backend.requestPublishingPreview("html", webConsumer);
    QVERIFY(changedDocument["url"].toUrl() != changedAsset["url"].toUrl());
    QFile documentFile(changedDocument["url"].toUrl().toLocalFile()); QVERIFY(documentFile.open(QIODevice::ReadOnly));
    QVERIFY(documentFile.readAll().contains("Changed document."));

    QObject pdfConsumer;
    QSignalSpy ready(&backend, &Backend::publishingPreviewReady);
    const auto firstPdf = backend.requestPublishingPreview("pdf", &pdfConsumer); QVERIFY(firstPdf["pending"].toBool());
    QCOMPARE(backend.requestPublishingPreview("pdf", &pdfConsumer)["requestId"], firstPdf["requestId"]);
    QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 35000);
    const auto pdfResult = ready.last()[1].toMap(); QVERIFY2(pdfResult["ok"].toBool(), qPrintable(pdfResult["error"].toString()));
    const auto cachedPdf = backend.requestPublishingPreview("pdf", &pdfConsumer);
    QVERIFY(!cachedPdf["pending"].toBool()); QCOMPARE(cachedPdf["url"], pdfResult["url"]);
    image.fill(Qt::green); QVERIFY(image.save(imagePath));
    const auto changedPdfAsset = backend.requestPublishingPreview("pdf", &pdfConsumer); QVERIFY(changedPdfAsset["pending"].toBool());
    QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 2, 35000);
    const auto newPdfResult = ready.last()[1].toMap(); QVERIFY(newPdfResult["ok"].toBool());
    QVERIFY(newPdfResult["url"] != pdfResult["url"]);
    const QString cachedSource = editor->property("text").toString();
    editor->setProperty("text", "# Superseded cache miss\n");
    const auto superseded = backend.requestPublishingPreview("pdf", &pdfConsumer); QVERIFY(superseded["pending"].toBool());
    editor->setProperty("text", cachedSource);
    QCOMPARE(backend.requestPublishingPreview("pdf", &pdfConsumer)["url"], newPdfResult["url"]);
    QTest::qWait(200);
    QCOMPARE(ready.size(), 2); // Cache hit cancels the superseded request.
    editor->setProperty("text", "# Cancelled by error\n");
    QVERIFY(backend.requestPublishingPreview("pdf", &pdfConsumer)["pending"].toBool());
    QVERIFY(!backend.requestPublishingPreview("invalid", &pdfConsumer)["ok"].toBool());
    QTest::qWait(200); QCOMPARE(ready.size(), 2);
    editor->setProperty("text", cachedSource);
    QCOMPARE(backend.requestPublishingPreview("pdf", &pdfConsumer)["url"], newPdfResult["url"]);
    QVERIFY(backend.setExportPaperSize(backend.exportPaperSize() == "letter" ? "a4" : "letter"));
    QVERIFY(backend.requestPublishingPreview("pdf", &pdfConsumer)["pending"].toBool());
    QVERIFY(!backend.requestPublishingPreview("invalid", &pdfConsumer)["ok"].toBool());

    // A web-only consumer pins its file while alive and releases it on teardown.
    const auto pinnedUrl = backend.requestPublishingPreview("html", webConsumer)["url"].toUrl();
    for (int i = 0; i < 6; ++i) QVERIFY(backend.publishingPreview("html")["ok"].toBool());
    QVERIFY(QFileInfo::exists(pinnedUrl.toLocalFile()));
    delete webConsumer;
    for (int i = 0; i < 6; ++i) QVERIFY(backend.publishingPreview("html")["ok"].toBool());
    QVERIFY(!QFileInfo::exists(pinnedUrl.toLocalFile()));
    QObject transientConsumer;
    QFile css(directory.filePath("transient.css")); QVERIFY(css.open(QIODevice::WriteOnly));
    css.write("#write { color: #112233; }"); css.close();
    QVERIFY(backend.loadOutputCss(QUrl::fromLocalFile(css.fileName())));
    const auto styled = backend.requestPublishingPreview("html", &transientConsumer); QVERIFY(styled["ok"].toBool());
    QVERIFY(css.remove());
    const auto failed = backend.requestPublishingPreview("html", &transientConsumer);
    QVERIFY(!failed["ok"].toBool()); QVERIFY(!failed["error"].toString().isEmpty());
    QVERIFY(css.open(QIODevice::WriteOnly)); css.write("#write { color: #445566; }"); css.close();
    const auto recovered = backend.requestPublishingPreview("html", &transientConsumer);
    QVERIFY(recovered["ok"].toBool()); QVERIFY(recovered["url"] != styled["url"]);
    QCOMPARE(editor->property("text").toString(), cachedSource);
    backend.discardRecovery();
}
