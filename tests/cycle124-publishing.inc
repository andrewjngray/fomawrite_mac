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
    editor->setProperty("text", "# Latest draft\n");
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
    QVERIFY(document.getAllText(0).text().contains("Latest draft"));
    QVERIFY(!document.getAllText(0).text().contains("Superseded draft"));
    for (const auto &arguments : ready) if (arguments[0].toULongLong() != latestId)
        QVERIFY(!arguments[1].toMap()["ok"].toBool());
    QCOMPARE(editor->property("text").toString(), QString("# Latest draft\n"));
    backend.discardRecovery();
}
