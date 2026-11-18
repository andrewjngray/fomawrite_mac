// Phase 0 spikes for docs/pdf-plan.md. Usage:
//   pdfspike <fixtures dir> <results.json>        S1 annotations, S2 geometry, S4 qpdf, S5 performance, S3 Quick menus
//   pdfspike --native-menus <results.json>        S3 native macOS menu bar (needs the cocoa platform)
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#include <QPdfSelection>
#include <QPainter>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickPaintedItem>
#include <QMutex>
#include <QQuickWindow>
#include <QTimer>
#include <QtMath>
#include <functional>

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>
#include <qpdf/QPDFWriter.hh>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#else
#include <mach/mach.h>
#include <malloc/malloc.h>
#include <sys/resource.h>
#endif

QStringList nativeMenuTitles(); // pdfspike_mac.mm (macOS only)

static double rssMB() {
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS c{};
    GetProcessMemoryInfo(GetCurrentProcess(), &c, sizeof(c));
    return c.WorkingSetSize / 1048576.0;
#else
    mach_task_basic_info info{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count);
    return info.resident_size / 1048576.0;
#endif
}

static double peakMB() {
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS c{};
    GetProcessMemoryInfo(GetCurrentProcess(), &c, sizeof(c));
    return c.PeakWorkingSetSize / 1048576.0;
#else
    rusage u{};
    getrusage(RUSAGE_SELF, &u);
    return u.ru_maxrss / 1048576.0; // bytes on macOS
#endif
}

static void wait(int ms) {
    QElapsedTimer t; t.start();
    while (t.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

static bool waitFor(const std::function<bool()> &done, int ms) {
    QElapsedTimer t; t.start();
    while (!done() && t.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    return done();
}

// Fraction of pixels in a PDF user-space rectangle (unrotated, uncropped page)
// that differ between two renders at `scale` pixels per point, and their mean colour.
static QJsonObject regionChange(const QImage &a, const QImage &b, QRectF user, double scale, double pageHeight) {
    const QRect r = QRectF(user.left() * scale, (pageHeight - user.bottom()) * scale,
                           user.width() * scale, user.height() * scale).toAlignedRect().intersected(a.rect());
    long changed = 0, total = 0; double rs = 0, gs = 0, bs = 0;
    for (int y = r.top(); y <= r.bottom(); ++y)
        for (int x = r.left(); x <= r.right(); ++x) {
            ++total;
            const QColor ca = a.pixelColor(x, y), cb = b.pixelColor(x, y);
            if (qAbs(ca.red() - cb.red()) + qAbs(ca.green() - cb.green()) + qAbs(ca.blue() - cb.blue()) > 30) {
                ++changed; rs += cb.red(); gs += cb.green(); bs += cb.blue();
            }
        }
    QJsonObject o{{"changedFraction", total ? double(changed) / total : 0.0}};
    if (changed) o["meanChangedRGB"] = QJsonArray{int(rs / changed), int(gs / changed), int(bs / changed)};
    return o;
}

static QImage render(QPdfDocument &doc, int pageIndex, double scale, bool annotations) {
    QPdfDocumentRenderOptions options;
    options.setRenderFlags(annotations ? QPdfDocumentRenderOptions::RenderFlag::Annotations
                                       : QPdfDocumentRenderOptions::RenderFlag::None);
    const QSizeF points = doc.pagePointSize(pageIndex);
    QImage page = doc.render(pageIndex, (points * scale).toSize(), options);
    QImage white(page.size(), QImage::Format_ARGB32);
    white.fill(Qt::white);
    QPainter painter(&white);
    painter.drawImage(0, 0, page);
    return white;
}

// Same regions, rendered by the QML PdfPageImage that a Fomawrite page view would use.
static QImage renderWithQml(const QString &file, int widthPx) {
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("spikeSource", QUrl::fromLocalFile(file));
    engine.rootContext()->setContextProperty("spikeWidth", widthPx);
    engine.loadData(R"(
        import QtQuick
        import QtQuick.Pdf
        Window {
            width: spikeWidth; height: spikeWidth * 792 / 612; visible: true; color: "white"
            property alias status: image.status
            PdfDocument { id: doc; source: spikeSource }
            PdfPageImage { id: image; objectName: "image"; document: doc; currentFrame: 0
                           anchors.fill: parent; sourceSize.width: spikeWidth; fillMode: Image.Stretch }
        })");
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) return {};
    waitFor([&] { return window->property("status").toInt() == 1; }, 10000); // Image.Ready
    wait(300);
    const QImage grabbed = window->grabWindow();
    grabbed.save(file + ".qml-grab.png");
    return grabbed.convertToFormat(QImage::Format_ARGB32).scaled(widthPx, qRound(widthPx * 792.0 / 612));
}

// Fomawrite's own page renderer: QPdfDocument::render with annotations drawn,
// on Qt's image loader thread. Id: "<absolute path>#<page index>".
class PdfPageProvider : public QQuickImageProvider {
public:
    PdfPageProvider() : QQuickImageProvider(QQuickImageProvider::Image, QQuickImageProvider::ForceAsynchronousImageLoading) {}
    QImage requestImage(const QString &id, QSize *size, const QSize &requested) override {
        QMutexLocker lock(&m_mutex);
        const int hash = id.lastIndexOf('#');
        const QString path = id.left(hash);
        const int page = id.mid(hash + 1).toInt();
        if (m_path != path) { m_doc.load(path); m_path = path; }
        const QSizeF points = m_doc.pagePointSize(page);
        const QSize px = requested.width() > 0 ? QSize(requested.width(), qRound(requested.width() * points.height() / points.width()))
                                               : points.toSize();
        QPdfDocumentRenderOptions options;
        options.setRenderFlags(QPdfDocumentRenderOptions::RenderFlag::Annotations);
        QImage image = m_doc.render(page, px, options);
        if (size) *size = image.size();
        return image;
    }
private:
    QMutex m_mutex;
    QPdfDocument m_doc;
    QString m_path;
};

// A page item that renders its own page (annotations on) at the item's size
// times the screen's device pixel ratio, and owns nothing after destruction.
class SpikePage : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(int page MEMBER m_page NOTIFY changed)
    Q_PROPERTY(QString path MEMBER m_path NOTIFY changed)
public:
    SpikePage() { connect(this, &SpikePage::changed, this, [this] { update(); }); }
    void paint(QPainter *painter) override {
        static QPdfDocument doc; static QString loaded;
        if (loaded != m_path) { doc.load(m_path); loaded = m_path; }
        const qreal dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
        QPdfDocumentRenderOptions options;
        options.setRenderFlags(QPdfDocumentRenderOptions::RenderFlag::Annotations);
        const QImage image = doc.render(m_page, (size() * dpr).toSize(), options);
        painter->fillRect(boundingRect(), Qt::white);
        painter->drawImage(boundingRect(), image);
    }
signals:
    void changed();
private:
    int m_page = 0;
    QString m_path;
};

static QImage renderWithProvider(const QString &file, int widthPx) {
    QQmlApplicationEngine engine;
    engine.addImageProvider("fwpdf", new PdfPageProvider);
    engine.rootContext()->setContextProperty("spikePath", file);
    engine.rootContext()->setContextProperty("spikeWidth", widthPx);
    engine.loadData(R"(
        import QtQuick
        Window {
            width: spikeWidth; height: spikeWidth * 792 / 612; visible: true; color: "white"
            property alias status: image.status
            Image { id: image; anchors.fill: parent; source: "image://fwpdf/" + spikePath + "#0"
                    sourceSize.width: spikeWidth; asynchronous: true; cache: false }
        })");
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) return {};
    waitFor([&] { return window->property("status").toInt() == 1; }, 10000);
    wait(300);
    return window->grabWindow().convertToFormat(QImage::Format_ARGB32).scaled(widthPx, qRound(widthPx * 792.0 / 612));
}

static QJsonObject spikeAnnotations(const QString &dir) {
    const double scale = 2.0, height = 792;
    const QList<QPair<QString, QRectF>> regions = {
        {"highlight", QRectF(100, 594.5, 78.7, 24)}, {"underline+strike", QRectF(300, 192, 89.4, 28)},
        {"note", QRectF(448, 648, 24, 24)}};
    QJsonObject result;
    QPdfDocument plainDoc; plainDoc.load(dir + "/geometry.pdf");
    const QImage plain = render(plainDoc, 0, scale, false);
    for (const QString name : {"annots.pdf", "annots-ap.pdf", "annots-pdfkit.pdf"}) {
        QPdfDocument doc;
        if (doc.load(dir + "/" + name) != QPdfDocument::Error::None) { result[name] = "could not load"; continue; }
        const QImage none = render(doc, 0, scale, false), with = render(doc, 0, scale, true);
        const QImage qml = renderWithQml(dir + "/" + name, int(612 * scale));
        const QImage provided = renderWithProvider(dir + "/" + name, int(612 * scale));
        QJsonObject file;
        for (const auto &region : regions)
            file[region.first] = QJsonObject{
                {"renderWithoutFlag", regionChange(plain, none, region.second, scale, height)},
                {"renderWithAnnotationsFlag", regionChange(plain, with, region.second, scale, height)},
                {"qmlPdfPageImage", qml.isNull() ? QJsonObject{{"error", "no QML render"}}
                                                  : regionChange(plain, qml, region.second, scale, height)},
                {"fomawriteProvider", provided.isNull() ? QJsonObject{{"error", "no provider render"}}
                                                        : regionChange(plain, provided, region.second, scale, height)}};
        result[name] = file;
    }
    return result;
}

// Predicted view rectangle (top-left origin, after /Rotate, inside the CropBox)
// for a PDF user-space rectangle; compared with Qt PDF's selection bounds.
static QRectF userToView(QRectF u, int rotate, QRectF crop) {
    auto map = [&](QPointF p) -> QPointF {
        const double x = p.x(), y = p.y();
        switch (rotate) {
        case 90: return {y - crop.top(), x - crop.left()};
        case 180: return {crop.right() - x, y - crop.top()};
        case 270: return {crop.bottom() - y, crop.right() - x};
        default: return {x - crop.left(), crop.bottom() - y};
        }
    };
    // crop here is (left, bottom) .. (right, top) in user space, stored as top()=bottom-y, bottom()=top-y
    const QPointF a = map(u.topLeft()), b = map(u.bottomRight());
    return QRectF(a, b).normalized();
}

static QJsonObject spikeGeometry(const QString &dir) {
    QPdfDocument doc; doc.load(dir + "/geometry.pdf");
    const int rotations[] = {0, 90, 180, 270, 0};
    QJsonArray pages;
    for (int p = 0; p < doc.pageCount(); ++p) {
        const QRectF crop = p == 4 ? QRectF(QPointF(50, 100), QPointF(562, 742)) : QRectF(QPointF(0, 0), QPointF(612, 792));
        const QString text = doc.getAllText(p).text();
        QJsonObject page{{"rotate", rotations[p]}, {"pagePointSize", QJsonArray{doc.pagePointSize(p).width(), doc.pagePointSize(p).height()}},
                         {"text", text.simplified()}};
        const QList<QPair<QString, QRectF>> words = {{"ALPHA", QRectF(QPointF(100, 594.48), QPointF(178.70, 618.48))},
                                                     {"OMEGA", QRectF(QPointF(300, 194.48), QPointF(389.35, 218.48))}};
        for (const auto &w : words) {
            const int index = text.indexOf(w.first);
            if (index < 0) { page[w.first] = "not found"; continue; }
            const QRectF rawQt = doc.getSelectionAtIndex(p, index, w.first.size()).boundingRectangle();
            const QRectF qt = rawQt.normalized();
            const QRectF predicted = userToView(w.second, rotations[p], crop);
            const QRectF overlap = qt.intersected(predicted);
            const double iou = overlap.width() * overlap.height()
                / (qt.width() * qt.height() + predicted.width() * predicted.height() - overlap.width() * overlap.height());
            page[w.first] = QJsonObject{{"qtSelectionRaw", QJsonArray{rawQt.x(), rawQt.y(), rawQt.width(), rawQt.height()}},
                                        {"qtSelection", QJsonArray{qt.x(), qt.y(), qt.width(), qt.height()}},
                                        {"predictedFromUserSpace", QJsonArray{predicted.x(), predicted.y(), predicted.width(), predicted.height()}},
                                        {"centreDistancePt", QLineF(qt.center(), predicted.center()).length()},
                                        {"intersectionOverUnion", iou}};
        }
        // Do Qt's selection rectangles cover the rendered ink? (selection space == rendered view space)
        const QImage img = render(doc, p, 1.0, false);
        long ink = 0, inside = 0;
        for (int y = 0; y < img.height(); ++y)
            for (int x = 0; x < img.width(); ++x)
                if (qGray(img.pixel(x, y)) < 128) {
                    ++ink;
                    for (const auto &w : words) {
                        const int index = text.indexOf(w.first);
                        if (index >= 0 && doc.getSelectionAtIndex(p, index, w.first.size()).boundingRectangle().normalized().adjusted(-1, -1, 1, 1).contains(QPointF(x + 0.5, y + 0.5))) { ++inside; break; }
                    }
                }
        page["inkInsideSelections"] = ink ? double(inside) / ink : 0.0;
        pages.append(page);
    }
    return QJsonObject{{"pages", pages}};
}

static QString pageTexts(const QString &file) {
    QPdfDocument doc; doc.load(file);
    QStringList out;
    for (int p = 0; p < doc.pageCount(); ++p) {
        const QSizeF s = doc.pagePointSize(p);
        out << QString("%1%2").arg(doc.getAllText(p).text().simplified().left(12), s.width() > s.height() ? "[landscape]" : "");
    }
    return out.join(" | ");
}

static QJsonObject spikeQpdf(const QString &dir, const QString &work) {
    QJsonObject result{{"qpdfVersion", QString::fromStdString(QPDF::QPDFVersion())}};
    const std::string geometry = (dir + "/geometry.pdf").toStdString();
    auto timed = [&](const QString &name, const std::function<void(QPDF &, QPDFPageDocumentHelper &)> &edit, const std::string &source) {
        QElapsedTimer t; t.start();
        QPDF pdf; pdf.processFile(source.c_str());
        QPDFPageDocumentHelper pages(pdf);
        edit(pdf, pages);
        const QString out = work + "/" + name + ".pdf";
        QPDFWriter writer(pdf, out.toStdString().c_str());
        writer.write();
        const qint64 ms = t.elapsed();
        QPdfDocument check; check.load(out);
        result[name] = QJsonObject{{"ms", ms}, {"pages", check.pageCount()}, {"pageTextsAfter", pageTexts(out)}};
    };
    result["original"] = pageTexts(dir + "/geometry.pdf");
    timed("rotate-page1-plus90", [](QPDF &, QPDFPageDocumentHelper &pages) { pages.getAllPages().at(0).rotatePage(90, true); }, geometry);
    timed("delete-page3", [](QPDF &, QPDFPageDocumentHelper &pages) { pages.removePage(pages.getAllPages().at(2)); }, geometry);
    timed("move-last-to-first", [](QPDF &, QPDFPageDocumentHelper &pages) {
        auto all = pages.getAllPages(); auto last = all.back();
        pages.removePage(last); pages.addPageAt(last, true, pages.getAllPages().front()); }, geometry);
    timed("append-pages-300", [&](QPDF &, QPDFPageDocumentHelper &pages) {
        static QPDF other; other.processFile((dir + "/pages-300.pdf").toStdString().c_str());
        for (auto &page : QPDFPageDocumentHelper(other).getAllPages()) pages.addPage(page, false); }, geometry);
    timed("reverse-300-pages", [](QPDF &, QPDFPageDocumentHelper &pages) {
        auto all = pages.getAllPages();
        for (auto &p : all) pages.removePage(p);
        for (auto it = all.rbegin(); it != all.rend(); ++it) pages.addPage(*it, false); }, (dir + "/pages-300.pdf").toStdString());
    // A highlight written by qpdf as a bare annotation dictionary (no /AP):
    // does Qt PDF (PDFium) draw it?
    timed("qpdf-written-highlight", [](QPDF &pdf, QPDFPageDocumentHelper &pages) {
        auto page = pages.getAllPages().at(0).getObjectHandle();
        auto annot = pdf.makeIndirectObject(QPDFObjectHandle::parse(
            "<< /Type /Annot /Subtype /Highlight /Rect [100 594.48 178.7 618.48] /C [1 0.85 0] /F 4 "
            "/QuadPoints [100 618.48 178.7 618.48 100 594.48 178.7 594.48] /Contents (Spike) >>"));
        if (!page.hasKey("/Annots")) page.replaceKey("/Annots", QPDFObjectHandle::newArray());
        page.getKey("/Annots").appendItem(annot); }, geometry);
    QPdfDocument plainDoc; plainDoc.load(dir + "/geometry.pdf");
    QPdfDocument written; written.load(work + "/qpdf-written-highlight.pdf");
    result["qpdf-written-highlight"] = QJsonObject{{"drawnByQtPdfWithAnnotationsFlag",
        regionChange(render(plainDoc, 0, 2, false), render(written, 0, 2, true), QRectF(100, 594.5, 78.7, 24), 2, 792)}};
    return result;
}

static QJsonObject spikePerformance(const QString &dir) {
    QJsonObject result;
    for (const QString name : {"pages-300.pdf", "images-50mb.pdf"}) {
        if (qEnvironmentVariableIsSet("SPIKE_SCROLL")) break;
        const double before = rssMB();
        QElapsedTimer t; t.start();
        QPdfDocument doc; doc.load(dir + "/" + name);
        const qint64 openMs = t.elapsed();
        const double afterOpen = rssMB();
        t.restart();
        double maxRss = afterOpen;
        for (int p = 0; p < doc.pageCount(); ++p) {
            const QImage image = render(doc, p, 2.0, true);
            maxRss = qMax(maxRss, rssMB());
        }
        const qint64 renderMs = t.elapsed();
        result[name] = QJsonObject{{"pages", doc.pageCount()}, {"openMs", openMs}, {"rssBeforeMB", before},
                                   {"rssAfterOpenMB", afterOpen}, {"renderAllAt2xMs", renderMs},
                                   {"renderMsPerPage", double(renderMs) / qMax(1, doc.pageCount())},
                                   {"maxRssWhileRenderingMB", maxRss}};
    }
    // QML: a ListView of 300 pages scrolled end to end, as a page view would
    // be: Qt's PdfPageImage (cache off; half size) and Fomawrite's own provider.
    const QList<QPair<QString, QString>> variants = {
        {"pdfPageImage_1800px_cacheOff", "PdfPageImage { required property int index; document: doc; currentFrame: index; width: 900; height: 1165; sourceSize.width: 1800; asynchronous: true; cache: false }"},
        {"pdfPageImage_900px_cacheOff", "PdfPageImage { required property int index; document: doc; currentFrame: index; width: 900; height: 1165; sourceSize.width: 900; asynchronous: true; cache: false }"},
        {"provider_1800px_cacheOff", "Image { required property int index; source: 'image://fwpdf/' + spikePath + '#' + index; width: 900; height: 1165; sourceSize.width: 1800; asynchronous: true; cache: false }"},
        {"provider_sync", "Image { required property int index; source: 'image://fwpdf/' + spikePath + '#' + index; width: 900; height: 1165; sourceSize.width: 1800; asynchronous: false; cache: false }"},
        {"provider_reuseItems", "Image { required property int index; source: 'image://fwpdf/' + spikePath + '#' + index; width: 900; height: 1165; sourceSize.width: 1800; asynchronous: true; cache: false }"},
        {"paintedPageItem", "SpikePage { required property int index; page: index; path: spikePath; width: 900; height: 1165 }"}};
    const QString only = qEnvironmentVariable("SPIKE_SCROLL");
    for (const auto &variant : variants) {
        if (!only.isEmpty() && variant.first != only) continue;
        QQmlApplicationEngine engine;
        engine.addImageProvider("fwpdf", new PdfPageProvider);
        engine.rootContext()->setContextProperty("spikeSource", QUrl::fromLocalFile(dir + "/pages-300.pdf"));
        engine.rootContext()->setContextProperty("spikePath", dir + "/pages-300.pdf");
        engine.rootContext()->setContextProperty("spikeReuse", variant.first == "provider_reuseItems");
        engine.loadData(QString(R"(
            import QtQuick
            import QtQuick.Pdf
            import Spike
            Window {
                width: 900; height: 1100; visible: true
                property alias view: list
                PdfDocument { id: doc; source: spikeSource }
                ListView { id: list; anchors.fill: parent; model: doc.pageCount; cacheBuffer: 0; spacing: 8; reuseItems: spikeReuse
                    delegate: %1 }
            })").arg(variant.second).toUtf8());
        auto *window = engine.rootObjects().value(0);
        auto *list = window ? window->property("view").value<QQuickItem *>() : nullptr;
        if (!list) { result[variant.first] = "QML failed"; continue; }
        waitFor([&] { return list->property("count").toInt() == 300; }, 10000);
        const double start = rssMB(); double maxRss = start;
        QElapsedTimer t; t.start();
        const int step = qEnvironmentVariableIntValue("SPIKE_WAIT_MS") > 0 ? qEnvironmentVariableIntValue("SPIKE_WAIT_MS") : 15;
        for (int i = 0; i < 300; i += 3) {
            QMetaObject::invokeMethod(list, "positionViewAtIndex", Q_ARG(int, i), Q_ARG(int, 0));
            wait(step);
            maxRss = qMax(maxRss, rssMB());
        }
        wait(500);
        const double end = rssMB();
        engine.collectGarbage();
        wait(2000);
        const double afterGc = rssMB();
#ifdef Q_OS_MACOS
        malloc_zone_pressure_relief(nullptr, 0);
#endif
        wait(200);
        result[variant.first] = QJsonObject{{"ms", t.elapsed()}, {"waitPerStepMs", step}, {"rssStartMB", start}, {"maxRssMB", maxRss},
                                            {"rssEndMB", end}, {"rssAfterGcMB", afterGc}, {"rssAfterMallocReliefMB", rssMB()},
                                            {"platform", QGuiApplication::platformName()},
                                            {"delegatesAlive", list->property("contentItem").value<QQuickItem *>()->childItems().size()},
                                            {"imagesAlive", int(window->findChildren<QQuickItem *>().size())}};
    }
    result["peakRssProcessMB"] = peakMB();
    return result;
}

// Quick Controls menu bar (the path Fomawrite's Windows menu uses): add and
// remove a PDF menu at runtime.
static QJsonObject spikeQuickMenus() {
    QQmlApplicationEngine engine;
    engine.loadData(R"(
        import QtQuick
        import QtQuick.Controls
        ApplicationWindow {
            width: 600; height: 300; visible: true
            menuBar: MenuBar { id: bar; Menu { title: "File" } Menu { title: "Format" } Menu { title: "Help" } }
            Component { id: pdfMenu; Menu { title: "PDF"; MenuItem { text: "Highlight Text" } } }
            function titles() { var t = []; for (var i = 0; i < bar.count; ++i) t.push(bar.menuAt(i).title); return t.join(","); }
            property var added: null
            function addPdf() { added = pdfMenu.createObject(bar); bar.insertMenu(2, added); }
            function removePdf() { bar.removeMenu(added); added.destroy(); added = null; }
            function hideFormat(hidden) { bar.menuAt(1).enabled = !hidden; }
        })");
    QObject *w = engine.rootObjects().value(0);
    if (!w) return {{"error", "QML failed"}};
    auto titles = [&] { QVariant r; QMetaObject::invokeMethod(w, "titles", Q_RETURN_ARG(QVariant, r)); return r.toString(); };
    QJsonObject o{{"initial", titles()}};
    QMetaObject::invokeMethod(w, "addPdf"); wait(100); o["afterAdd"] = titles();
    QMetaObject::invokeMethod(w, "removePdf"); wait(100); o["afterRemove"] = titles();
    return o;
}

static QJsonObject spikeNativeMenus() {
#ifdef Q_OS_MACOS
    QQmlApplicationEngine engine;
    engine.loadData(R"(
        import QtQuick
        import Qt.labs.platform as Platform
        Window {
            width: 300; height: 200; visible: true; x: -2000; y: -2000
            Platform.MenuBar { id: bar
                Platform.Menu { title: "File"; Platform.MenuItem { text: "Open" } }
                Platform.Menu { title: "Format"; Platform.MenuItem { text: "Bold" } }
                Platform.Menu { title: "Help"; Platform.MenuItem { text: "About" } } }
            Component { id: pdfMenu; Platform.Menu { title: "PDF"; Platform.MenuItem { text: "Highlight Text" } } }
            property var added: null
            function addPdf() { added = pdfMenu.createObject(bar); bar.insertMenu(2, added); }
            function removePdf() { bar.removeMenu(added); added.destroy(); added = null; }
            function setFormatVisible(v) { bar.menus[1].visible = v; }
        })");
    QObject *w = engine.rootObjects().value(0);
    if (!w) return {{"error", "QML failed"}};
    wait(800);
    QJsonObject o{{"initial", nativeMenuTitles().join(",")}};
    QMetaObject::invokeMethod(w, "addPdf"); wait(400); o["afterAdd"] = nativeMenuTitles().join(",");
    QMetaObject::invokeMethod(w, "setFormatVisible", Q_ARG(QVariant, false)); wait(400); o["formatHidden"] = nativeMenuTitles().join(",");
    QMetaObject::invokeMethod(w, "setFormatVisible", Q_ARG(QVariant, true)); wait(400); o["formatShown"] = nativeMenuTitles().join(",");
    QMetaObject::invokeMethod(w, "removePdf"); wait(400); o["afterRemove"] = nativeMenuTitles().join(",");
    return o;
#else
    return {{"skipped", "not macOS"}};
#endif
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    qmlRegisterType<SpikePage>("Spike", 1, 0, "SpikePage");
    const QStringList args = app.arguments();
    QJsonObject results{{"qtVersion", qVersion()}, {"platform", QGuiApplication::platformName()},
                        {"os", QSysInfo::prettyProductName()}, {"cpu", QSysInfo::currentCpuArchitecture()}};
    if (args.value(1) == "--native-menus") {
        results["S3_nativeMenus"] = spikeNativeMenus();
    } else {
        const QString dir = args.value(1), work = QDir(dir).filePath("work");
        QDir().mkpath(work);
        if (qEnvironmentVariableIsSet("SPIKE_SCROLL")) {
            results["S5_performance"] = spikePerformance(dir);
            QFile out(args.last()); out.open(QIODevice::WriteOnly); out.write(QJsonDocument(results).toJson());
            return 0;
        }
        results["S1_annotations"] = spikeAnnotations(dir);
        results["S2_geometry"] = spikeGeometry(dir);
        results["S3_quickMenus"] = spikeQuickMenus();
        try { results["S4_qpdf"] = spikeQpdf(dir, work); }
        catch (const std::exception &e) { results["S4_qpdf"] = QJsonObject{{"exception", e.what()}}; }
        results["S5_performance"] = spikePerformance(dir);
    }
    QFile out(args.last());
    out.open(QIODevice::WriteOnly);
    out.write(QJsonDocument(results).toJson());
    printf("%s\n", QJsonDocument(results).toJson().constData());
    return 0;
}

#include "pdfspike.moc"
