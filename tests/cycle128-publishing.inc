#include <QPainter>
#include <QPdfWriter>
#include <QQuickRenderControl>

namespace {
QVariantMap cycle128PdfResult(const QUrl &url) {
    return {{"ok", true}, {"url", url}, {"anchors", QVariantMap{}}};
}
QVariantMap cycle128PendingResult(int requestId) {
    return {{"ok", true}, {"pending", true}, {"requestId", requestId}};
}
bool cycle128WritePdf(const QString &path, const QString &text) {
    {
        QPdfWriter writer(path);
        writer.setPageSize(QPageSize(QPageSize::A4));
        QPainter painter(&writer);
        painter.setFont(QFont("Helvetica", 24));
        painter.drawText(QRect(100, 100, 6000, 1000), text);
    }
    QPdfDocument document;
    return document.load(path) == QPdfDocument::Error::None
        && document.pageCount() == 1 && document.getAllText(0).text().contains(text);
}
// The renderer supplies deterministic completed/pending jobs; the component
// still uses Qt's real PdfDocument and PdfPageImage, not a fake PDF loader.
struct Cycle128Preview {
    QQmlEngine engine;
    QQuickRenderControl renderControl;
    QQuickWindow window;
    QScopedPointer<QObject> root;
    QString error;
    Cycle128Preview(const QString &qmlPath, const QVariantMap &response, bool suspendFrames = false)
        : window(suspendFrames ? &renderControl : nullptr) {
        QQmlComponent component(&engine);
        component.setData(R"QML(
import QtQuick
import "../src" as Fomawrite
Item {
    id: harness
    width: 720; height: 800
    property var response
    property alias testRenderer: fakeRenderer
    QtObject {
        id: fakeRenderer
        property var palette: ({ page: "white", panel: "white", text: "black" })
        property int requests: 0
        property string lastFormat: ""
        property var requestTokens: ({})
        function cancelPublishingPreview(consumer) {}
        signal outputStyleChanged()
        signal outputPageLayoutChanged()
        signal outputCssChanged()
        signal publishingThemesChanged()
        signal publishingPreviewReady(double requestId, var result)
        function requestPublishingPreview(format, consumer, requestIdentity) {
            ++requests; lastFormat = format;
            var result = Object.assign({}, harness.response);
            if (requestIdentity !== undefined) {
                result.requestIdentity = requestIdentity;
                if (result.pending) requestTokens[result.requestId] = requestIdentity;
            }
            return result;
        }
    }
    Fomawrite.PublishingPreview {
        anchors.fill: parent
        renderer: fakeRenderer
        publishingMode: "pdf"
        markdown: "# Fixture"
    }
}
)QML", QUrl::fromLocalFile(qmlPath));
        root.reset(component.createWithInitialProperties({{"response", response}}));
        error = component.errorString();
        if (auto *item = qobject_cast<QQuickItem *>(root.data())) {
            window.resize(720, 800);
            item->setParentItem(window.contentItem());
        }
    }
    QObject *preview() const { return root ? root->findChild<QObject *>("previewPane") : nullptr; }
    QObject *renderer() const { return root ? root->property("testRenderer").value<QObject *>() : nullptr; }
    QObject *pdf() const {
        if (!root) return nullptr;
        for (auto *child : root->findChildren<QObject *>())
            if (child->property("pageCount").isValid() && child->property("source").isValid()
                    && QByteArray(child->metaObject()->className()).contains("PdfDocument")) return child;
        return nullptr;
    }
    QUrl pdfSource() const { return pdf() ? pdf()->property("source").toUrl() : QUrl(); }
    int visiblePdfPages() const {
        // Loader items belong to the scene tree even when QObject ownership is
        // held by the QML engine rather than the surrounding component.
        const auto countPages = [](auto &&self, QQuickItem *item) -> int {
            if (!item) return 0;
            int count = item->objectName().startsWith("publishingPdfPage_")
                && item->parentItem() && item->isVisible() ? 1 : 0;
            for (auto *child : item->childItems()) count += self(self, child);
            return count;
        };
        return countPages(countPages, window.contentItem());
    }
    bool blankDisplayed() const {
        return preview() && preview()->property("outputUrl").toUrl().isEmpty()
            && !preview()->property("pdfPagesReady").toBool()
            && preview()->property("displayedDocumentIdentity").toString().isEmpty()
            && visiblePdfPages() == 0;
    }
    bool ready(const QUrl &url) const {
        return preview() && pdf() && pdfSource() == url
            && preview()->property("pdfPagesReady").toBool()
            && pdf()->property("pageCount").toInt() == 1
            && !preview()->property("viewportRefreshPending").toBool();
    }
    bool complete(int requestId, QVariantMap result) {
        const auto tokens = renderer()->property("requestTokens").value<QJSValue>();
        const auto token = tokens.property(QString::number(requestId));
        if (!token.isUndefined()) result.insert("requestIdentity", token.toVariant());
        return QMetaObject::invokeMethod(renderer(), "publishingPreviewReady",
            Q_ARG(double, double(requestId)), Q_ARG(QVariant, QVariant(result)));
    }
    void document(const QString &identity, const QString &markdown) {
        // Before the fix this is a dynamic property: the immediate stale-output
        // assertion below still demonstrates the old behavior without requiring
        // the new property to exist just to construct the component.
        preview()->setProperty("documentIdentity", identity);
        preview()->setProperty("markdown", markdown);
    }
};
}

void FomawriteTest::publishingComponentColdPdfLoadsActualDocument() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto pdfUrl = QUrl::fromLocalFile(directory.filePath("cold.pdf"));
    QVERIFY(cycle128WritePdf(pdfUrl.toLocalFile(), "Cold PDF fixture"));
    Cycle128Preview fixture(QFINDTESTDATA("tst_fomawrite.cpp"), cycle128PdfResult(pdfUrl));
    QVERIFY2(fixture.root, qPrintable(fixture.error)); QVERIFY(fixture.preview()); QVERIFY(fixture.pdf());
    fixture.document("untitled:1", "# Cold PDF fixture");
    fixture.window.show();
    QTRY_VERIFY_WITH_TIMEOUT(fixture.ready(pdfUrl), 5000);
    QCOMPARE(fixture.preview()->property("outputUrl").toUrl(), pdfUrl);
    QCOMPARE(fixture.pdfSource(), pdfUrl);
    QCOMPARE(fixture.renderer()->property("requests").toInt(), 1);
    // Bounce before Web's debounce fires. The PDF resource is still loaded,
    // so returning the same cache URL does not produce a new Ready signal.
    fixture.preview()->setProperty("publishingMode", "web");
    fixture.preview()->setProperty("publishingMode", "pdf");
    QTRY_VERIFY_WITH_TIMEOUT(fixture.ready(pdfUrl), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(fixture.visiblePdfPages() > 0, 2000);
    QCOMPARE(fixture.preview()->property("displayedDocumentIdentity").toString(), QString("untitled:1"));
    QCOMPARE(fixture.renderer()->property("lastFormat").toString(), QString("pdf"));
}

void FomawriteTest::publishingComponentReplacementLoadsWithoutCaptureFrames() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto first = QUrl::fromLocalFile(directory.filePath("first.pdf"));
    const auto replacement = QUrl::fromLocalFile(directory.filePath("replacement.pdf"));
    QVERIFY(cycle128WritePdf(first.toLocalFile(), "Original draft"));
    QVERIFY(cycle128WritePdf(replacement.toLocalFile(), "Replacement draft"));
    Cycle128Preview fixture(QFINDTESTDATA("tst_fomawrite.cpp"), cycle128PdfResult(first), true);
    QVERIFY2(fixture.root, qPrintable(fixture.error)); QVERIFY(fixture.pdf());
    fixture.document("untitled:1", "# Original draft");
    fixture.window.show();
    QTRY_VERIFY_WITH_TIMEOUT(fixture.ready(first), 5000);
    // QQuickRenderControl leaves frame submission to the caller. We never
    // submit a frame, so grabToImage cannot finish. PDF source assignment and
    // loading must still progress without a compositor or screenshot callback.
    fixture.root->setProperty("response", cycle128PdfResult(replacement));
    fixture.preview()->setProperty("markdown", "# Replacement draft");
    QTRY_VERIFY_WITH_TIMEOUT(fixture.ready(replacement), 5000);
    QCOMPARE(fixture.pdfSource(), replacement);
    QVERIFY(!fixture.preview()->property("holdingPreviousFrame").toBool());
}

void FomawriteTest::publishingComponentSwitchRejectsStalePdfResults() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto a = QUrl::fromLocalFile(directory.filePath("a.pdf"));
    const auto b = QUrl::fromLocalFile(directory.filePath("b.pdf"));
    const auto aIdentity = QUrl::fromLocalFile(directory.filePath("a.md")).toString();
    const auto bIdentity = QUrl::fromLocalFile(directory.filePath("b.md")).toString();
    QVERIFY(cycle128WritePdf(a.toLocalFile(), "Document A"));
    QVERIFY(cycle128WritePdf(b.toLocalFile(), "Document B"));
    Cycle128Preview fixture(QFINDTESTDATA("tst_fomawrite.cpp"), cycle128PdfResult(a));
    QVERIFY2(fixture.root, qPrintable(fixture.error)); QVERIFY(fixture.pdf());
    fixture.preview()->setProperty("documentBaseUrl", QUrl::fromLocalFile(directory.path() + '/'));
    fixture.document(aIdentity, "# Document A"); fixture.window.show();
    QTRY_VERIFY_WITH_TIMEOUT(fixture.ready(a), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(fixture.visiblePdfPages() > 0, 2000);
    fixture.root->setProperty("response", cycle128PendingResult(101));
    fixture.preview()->setProperty("markdown", "# Edited A");
    QTRY_COMPARE_WITH_TIMEOUT(fixture.preview()->property("pendingRequest").toInt(), 101, 3000);
    fixture.root->setProperty("response", cycle128PendingResult(102));
    fixture.document(bIdentity, "# Document B");
    // Same directory deliberately leaves documentBaseUrl unchanged. A's pages
    // and displayed identity must disappear while B is being generated; Qt may
    // retain the loaded resource after its page delegates have been removed.
    QVERIFY(fixture.preview()->property("outputUrl").toUrl().isEmpty());
    QVERIFY(!fixture.preview()->property("pdfPagesReady").toBool());
    QVERIFY(!fixture.preview()->property("holdingPreviousFrame").toBool());
    QVERIFY(fixture.preview()->property("displayedDocumentIdentity").toString().isEmpty());
    QTRY_VERIFY_WITH_TIMEOUT(fixture.blankDisplayed(), 1000);
    auto *loading = fixture.preview()->findChild<QQuickItem *>("publishingLoadingNotice");
    QVERIFY(loading);
    QTRY_VERIFY(loading->isVisible());
    QVERIFY(fixture.complete(101, cycle128PdfResult(a)));
    QVERIFY(fixture.blankDisplayed());
    QTRY_COMPARE_WITH_TIMEOUT(fixture.preview()->property("pendingRequest").toInt(), 102, 3000);
    QVERIFY(fixture.complete(101, cycle128PdfResult(a)));
    QVERIFY(fixture.blankDisplayed());
    QVERIFY(fixture.complete(102, cycle128PdfResult(b)));
    QTRY_VERIFY_WITH_TIMEOUT(fixture.ready(b), 5000);
    QCOMPARE(fixture.preview()->property("actualPdfSource").toUrl(), b);
    QCOMPARE(fixture.preview()->property("displayedDocumentIdentity").toString(), bIdentity);
    QVERIFY(fixture.complete(101, cycle128PdfResult(a)));
    QTest::qWait(250);
    QCOMPARE(fixture.pdfSource(), b);
}

void FomawriteTest::publishingComponentRapidSwitchModesAndHiddenRejectStaleResults() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto a = QUrl::fromLocalFile(directory.filePath("a.pdf"));
    const auto b = QUrl::fromLocalFile(directory.filePath("b.pdf"));
    const auto c = QUrl::fromLocalFile(directory.filePath("c.pdf"));
    QVERIFY(cycle128WritePdf(a.toLocalFile(), "Rapid A"));
    QVERIFY(cycle128WritePdf(b.toLocalFile(), "Rapid B"));
    QVERIFY(cycle128WritePdf(c.toLocalFile(), "Rapid C"));
    Cycle128Preview fixture(QFINDTESTDATA("tst_fomawrite.cpp"), cycle128PendingResult(201));
    QVERIFY2(fixture.root, qPrintable(fixture.error)); QVERIFY(fixture.pdf());
    fixture.document("untitled:1", "# Rapid A"); fixture.window.show();
    QTRY_COMPARE_WITH_TIMEOUT(fixture.preview()->property("pendingRequest").toInt(), 201, 3000);
    fixture.root->setProperty("response", cycle128PendingResult(202));
    fixture.document("untitled:2", "# Rapid B");
    QTRY_COMPARE_WITH_TIMEOUT(fixture.preview()->property("pendingRequest").toInt(), 202, 3000);
    fixture.preview()->setProperty("publishingMode", "web");
    QVERIFY(fixture.complete(202, cycle128PdfResult(b)));
    QVERIFY(fixture.complete(201, cycle128PdfResult(a)));
    QVERIFY(fixture.blankDisplayed());
    QVERIFY(!fixture.preview()->property("pdfPagesReady").toBool());
    fixture.preview()->setProperty("visible", false);
    fixture.document("untitled:3", "# Rapid C");
    QVERIFY(fixture.complete(202, cycle128PdfResult(b)));
    QVERIFY(fixture.complete(201, cycle128PdfResult(a)));
    QVERIFY(fixture.blankDisplayed());
    fixture.preview()->setProperty("publishingMode", "web");
    QVERIFY(fixture.complete(202, cycle128PdfResult(b)));
    QVERIFY(fixture.blankDisplayed());
    fixture.root->setProperty("response", cycle128PendingResult(203));
    fixture.preview()->setProperty("publishingMode", "pdf");
    fixture.preview()->setProperty("visible", true);
    QTRY_COMPARE_WITH_TIMEOUT(fixture.preview()->property("pendingRequest").toInt(), 203, 3000);
    QVERIFY(fixture.complete(201, cycle128PdfResult(a)));
    QVERIFY(fixture.complete(202, cycle128PdfResult(b)));
    QVERIFY(fixture.blankDisplayed());
    QVERIFY(fixture.complete(203, cycle128PdfResult(c)));
    QTRY_VERIFY_WITH_TIMEOUT(fixture.ready(c), 5000);
    QCOMPARE(fixture.preview()->property("actualPdfSource").toUrl(), c);
    QCOMPARE(fixture.preview()->property("displayedDocumentIdentity").toString(), QString("untitled:3"));
}

void FomawriteTest::publishingComponentTimeoutClearsStalePagesAndRecovers() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto first = QUrl::fromLocalFile(directory.filePath("first.pdf"));
    const auto abandoned = QUrl::fromLocalFile(directory.filePath("abandoned.pdf"));
    QVERIFY(cycle128WritePdf(first.toLocalFile(), "Before timeout"));
    QVERIFY(cycle128WritePdf(abandoned.toLocalFile(), "Late abandoned result"));
    Cycle128Preview fixture(QFINDTESTDATA("tst_fomawrite.cpp"), cycle128PdfResult(first));
    QVERIFY2(fixture.root, qPrintable(fixture.error)); QVERIFY(fixture.pdf());
    fixture.document("untitled:1", "# Before timeout"); fixture.window.show();
    QTRY_VERIFY_WITH_TIMEOUT(fixture.ready(first), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(fixture.visiblePdfPages() > 0, 2000);
    auto *deadline = fixture.preview()->findChild<QObject *>("publishingRefreshDeadline");
    QVERIFY(deadline); QVERIFY(deadline->setProperty("interval", 250));
    fixture.root->setProperty("response", cycle128PendingResult(301));
    QVERIFY(QMetaObject::invokeMethod(fixture.preview(), "reload"));
    QTRY_COMPARE_WITH_TIMEOUT(fixture.preview()->property("pendingRequest").toInt(), 301, 2000);
    QTRY_VERIFY_WITH_TIMEOUT(!fixture.preview()->property("errorText").toString().isEmpty(), 2000);
    QVERIFY(!fixture.preview()->property("viewportRefreshPending").toBool());
    QTRY_VERIFY_WITH_TIMEOUT(fixture.blankDisplayed(), 1000);
    QVERIFY(fixture.complete(301, cycle128PdfResult(first)));
    QVERIFY(fixture.blankDisplayed());
    QVERIFY(!fixture.preview()->property("errorText").toString().isEmpty());
    QVERIFY(deadline->setProperty("interval", 30000));
    // Retry returns the same cache URL. Qt retained that PDF resource while
    // the error removed its pages, so there will be no new Ready transition.
    fixture.root->setProperty("response", cycle128PdfResult(first));
    QVERIFY(QMetaObject::invokeMethod(fixture.preview(), "reload"));
    QTRY_VERIFY_WITH_TIMEOUT(fixture.ready(first), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(fixture.visiblePdfPages() > 0, 2000);
    QVERIFY(fixture.preview()->property("errorText").toString().isEmpty());
    QCOMPARE(fixture.preview()->property("displayedDocumentIdentity").toString(), QString("untitled:1"));
    QVERIFY(fixture.complete(301, cycle128PdfResult(abandoned)));
    QTest::qWait(250);
    QCOMPARE(fixture.pdfSource(), first);
}
