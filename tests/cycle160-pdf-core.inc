// Cycle 160 — PDF core: PdfSession, the PdfPage item, routing PDFs through
// Backend::open with a notes companion, the explorer's PDF rows and the bare
// page list in the preview slot. Fixtures: tests/pdffixtures.h.

#include "pdffixtures.h"
#include "pdfsession.h"
#include "pdfpageitem.h"
#include <QPdfDocument>

#ifdef Q_OS_MACOS
#include <mach/mach.h>
#elif defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#endif

namespace {
// Resident memory of this process, as the cycle 159 spike measured it.
double cycle160RssMB() {
#ifdef Q_OS_MACOS
    mach_task_basic_info info{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count);
    return info.resident_size / 1048576.0;
#elif defined(Q_OS_WIN)
    PROCESS_MEMORY_COUNTERS counters{};
    GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters));
    return counters.WorkingSetSize / 1048576.0;
#else
    QFile statm(QStringLiteral("/proc/self/statm"));
    if (!statm.open(QIODevice::ReadOnly)) return 0;
    return statm.readAll().split(' ').value(1).toDouble() * 4096 / 1048576.0;
#endif
}

struct Cycle160Fixtures {
    QTemporaryDir directory;
    QString path(const QString &name) const { return directory.filePath(name); }
    QUrl url(const QString &name) const { return QUrl::fromLocalFile(path(name)); }
    bool make() {
        return directory.isValid()
            && PdfFixtures::write(path("geometry.pdf"), PdfFixtures::document(PdfFixtures::geometryPages()))
            && PdfFixtures::write(path("plain.pdf"), PdfFixtures::document({PdfFixtures::geometryPages().first()}))
            && PdfFixtures::write(path("annots.pdf"), PdfFixtures::annotatedDocument())
            && PdfFixtures::write(path("navigation.pdf"), PdfFixtures::navigationDocument())
            && PdfFixtures::write(path("damaged.pdf"), PdfFixtures::damagedDocument())
            && PdfFixtures::write(path("not-a-pdf.pdf"), "# Markdown, not a PDF\n");
    }
};

// A Backend with a Source document attached, as the persistence tests use.
struct Cycle160Document {
    Backend backend;
    QQmlEngine engine;
    QScopedPointer<QObject> editor;
    bool create() {
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
        editor.reset(component.create());
        return !editor.isNull();
    }
    QString text() const { return editor->property("text").toString(); }
    bool type(const QString &value) {
        return QMetaObject::invokeMethod(editor.data(), "insert", Q_ARG(int, text().size()), Q_ARG(QString, value));
    }
    ~Cycle160Document() { backend.discardRecovery(); }
};

QByteArray cycle160Read(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

QRectF cycle160Rect(const QVariant &value) { return value.toRectF(); }

// Delegates are visual children, not QObject children, of a list.
QList<PdfPageItem *> cycle160Pages(QQuickItem *root) {
    QList<PdfPageItem *> pages;
    for (auto *child : root->childItems()) {
        if (auto *page = qobject_cast<PdfPageItem *>(child)) pages << page;
        pages << cycle160Pages(child);
    }
    return pages;
}
}

void FomawriteTest::pdfSessionLoadsRefusesAndUnlocks() {
    Cycle160Fixtures f; QVERIFY(f.make());
    PdfSession session;
    QCOMPARE(session.state(), QStringLiteral("none"));
    QVERIFY(!session.active());

    // A damaged file and a non-PDF fail cleanly with a human message.
    QVERIFY(!session.load(f.url("damaged.pdf")));
    QCOMPARE(session.state(), QStringLiteral("error"));
    QVERIFY(!session.active());
    QVERIFY2(session.errorText().contains("damaged.pdf"), qPrintable(session.errorText()));
    QVERIFY(!session.load(f.url("not-a-pdf.pdf")));
    QCOMPARE(session.state(), QStringLiteral("error"));
    QVERIFY(!session.load(f.url("missing.pdf")));
    QVERIFY(session.errorText().contains("could not be found"));

    QSignalSpy opened(&session, &PdfSession::opened);
    QSignalSpy revision(&session, &PdfSession::revisionChanged);
    QVERIFY(session.load(f.url("geometry.pdf")));
    QCOMPARE(session.state(), QStringLiteral("ready"));
    QVERIFY(session.active());
    QVERIFY(session.errorText().isEmpty());
    QCOMPARE(session.pageCount(), 5);
    QCOMPARE(session.fileName(), QStringLiteral("geometry.pdf"));
    QCOMPARE(session.notesUrl(), QUrl::fromLocalFile(f.path("geometry.pdf.md")));
    QVERIFY(!session.notesExist());
    QCOMPARE(opened.size(), 1);
    QCOMPARE(revision.size(), 1);
    QVERIFY(session.document() && session.renderer());

    // A failed load keeps the PDF that is open, unchanged.
    QVERIFY(!session.load(f.url("damaged.pdf")));
    QCOMPARE(session.state(), QStringLiteral("ready"));
    QCOMPARE(session.url(), f.url("geometry.pdf"));
    QCOMPARE(session.pageCount(), 5);
    QVERIFY(session.lastError().contains("damaged.pdf"));

    // Inspector facts.
    QVERIFY(session.load(f.url("navigation.pdf")));
    const QVariantMap info = session.info();
    QCOMPARE(info.value("fileName").toString(), QStringLiteral("navigation.pdf"));
    QCOMPARE(info.value("pdfVersion").toString(), QStringLiteral("1.7"));
    QCOMPARE(info.value("pageCount").toInt(), 2);
    QCOMPARE(info.value("encrypted").toBool(), false);
    QCOMPARE(info.value("title").toString(), QStringLiteral("Fixture Title"));
    QCOMPARE(info.value("author").toString(), QStringLiteral("Fixture Author"));
    QCOMPARE(info.value("subject").toString(), QStringLiteral("Fixture Subject"));
    QCOMPARE(info.value("keywords").toString(), QStringLiteral("alpha omega"));
    QCOMPARE(info.value("producer").toString(), QStringLiteral("Fomawrite tests"));
    QCOMPARE(info.value("creator").toString(), QStringLiteral("pdffixtures.h"));
    QVERIFY2(info.value("created").toString().startsWith("2026-11-19"), qPrintable(info.value("created").toString()));
    QVERIFY(info.contains("modified"));

    // Password: "password" until unlock succeeds. The sample was made with
    // qpdf --encrypt fomawrite-user fomawrite-owner 256 -- plain.pdf password.pdf
    const QString locked = QFINDTESTDATA("fixtures/password.pdf");
    QVERIFY(!locked.isEmpty());
    QVERIFY(session.load(QUrl::fromLocalFile(locked)));
    QCOMPARE(session.state(), QStringLiteral("password"));
    QVERIFY(session.active());
    QCOMPARE(session.pageCount(), 0);
    QVERIFY(session.info().value("encrypted").toBool());
    QVERIFY(!session.unlock(QStringLiteral("wrong")));
    QCOMPARE(session.state(), QStringLiteral("password"));
    QVERIFY(session.unlock(QStringLiteral("fomawrite-user")));
    QCOMPARE(session.state(), QStringLiteral("ready"));
    QCOMPARE(session.pageCount(), 1);
    QVERIFY(session.pageText(0).contains("ALPHA"));
    QVERIFY(session.info().value("encrypted").toBool());

    session.close();
    QVERIFY(!session.active());
    QCOMPARE(session.state(), QStringLiteral("none"));
    QCOMPARE(session.pageCount(), 0);
    QVERIFY(!session.renderer());
}

void FomawriteTest::pdfSessionReportsPageSizesLabelsAndText() {
    Cycle160Fixtures f; QVERIFY(f.make());
    PdfSession session;
    QVERIFY(session.load(f.url("geometry.pdf")));
    // Rotated pages swap width and height; the CropBox page is 512 × 642.
    const QList<QSizeF> sizes{{612, 792}, {792, 612}, {612, 792}, {792, 612}, {512, 642}};
    for (int page = 0; page < 5; ++page) {
        QCOMPARE(session.pageSize(page), sizes.at(page));
        QCOMPARE(session.pageLabel(page), QString::number(page + 1));
        const QString text = session.pageText(page);
        QVERIFY2(text.contains("ALPHA") && text.contains("OMEGA"), qPrintable(text));
    }
    QVERIFY(!session.pageSize(5).isValid());
    QVERIFY(session.pageText(-1).isEmpty());

    // #page=N lands on a page that exists.
    session.setRequestedPage(99);
    QCOMPARE(session.requestedPage(), 4);
    session.setRequestedPage(-3);
    QCOMPARE(session.requestedPage(), 0);
}

void FomawriteTest::pdfSessionSelectionGeometryMatchesTheSpikeMapping() {
    Cycle160Fixtures f; QVERIFY(f.make());
    PdfSession session;
    QVERIFY(session.load(f.url("geometry.pdf")));
    for (int page = 0; page < 5; ++page) {
        const QString text = session.pageText(page);
        const int rotate = PdfFixtures::geometryRotation[page];
        for (const auto *word : {&PdfFixtures::Alpha, &PdfFixtures::Omega}) {
            const QString name = QString::fromLatin1(word->text);
            const QString where = QStringLiteral("page %1 %2").arg(page + 1).arg(name);
            const int start = int(text.indexOf(name));
            QVERIFY2(start >= 0, qPrintable(where));
            const QVariantMap selection = session.selectionAt(page, start, int(name.size()));
            QCOMPARE(selection.value("text").toString(), name);
            QCOMPARE(selection.value("start").toInt(), start);
            QCOMPARE(selection.value("length").toInt(), int(name.size()));
            const QVariantList rects = selection.value("rects").toList();
            QCOMPARE(rects.size(), 1);
            const QRectF qt = cycle160Rect(rects.first());
            QVERIFY2(qt.width() > 0 && qt.height() > 0, qPrintable(where)); // normalised
            const QRectF predicted = PdfFixtures::userToView(PdfFixtures::userBox(*word), rotate, PdfFixtures::geometryCrop(page));
            // Along the text the edges agree within 1.5 pt; across it Qt's
            // glyph-tight box lies inside the descender-to-cap-height box.
            const bool horizontal = rotate == 0 || rotate == 180;
            const qreal startError = horizontal ? qAbs(qt.left() - predicted.left()) : qAbs(qt.top() - predicted.top());
            const qreal endError = horizontal ? qAbs(qt.right() - predicted.right()) : qAbs(qt.bottom() - predicted.bottom());
            QVERIFY2(startError <= 1.5 && endError <= 1.5,
                     qPrintable(QStringLiteral("%1: Qt %2,%3 %4x%5 vs predicted %6,%7 %8x%9").arg(where)
                         .arg(qt.x()).arg(qt.y()).arg(qt.width()).arg(qt.height())
                         .arg(predicted.x()).arg(predicted.y()).arg(predicted.width()).arg(predicted.height())));
            QVERIFY2(predicted.adjusted(-1, -1, 1, 1).contains(qt), qPrintable(where));

            // Dragging across the word selects the word.
            const QPointF from = horizontal ? QPointF(rotate == 0 ? qt.left() + 0.5 : qt.right() - 0.5, qt.center().y())
                                            : QPointF(qt.center().x(), rotate == 90 ? qt.top() + 0.5 : qt.bottom() - 0.5);
            // Release just past the last letter, as a reader's drag does.
            const QPointF to = horizontal ? QPointF(rotate == 0 ? qt.right() + 1 : qt.left() - 1, qt.center().y())
                                          : QPointF(qt.center().x(), rotate == 90 ? qt.bottom() + 1 : qt.top() - 1);
            const QVariantMap dragged = session.selectionBetween(page, from, to);
            QVERIFY2(dragged.value("text").toString() == name, qPrintable(where + ": " + dragged.value("text").toString()));
            QCOMPARE(dragged.value("start").toInt(), start);
            QCOMPARE(dragged.value("length").toInt(), int(name.size()));
            for (const QVariant &rect : dragged.value("rects").toList())
                QVERIFY(cycle160Rect(rect).width() > 0 && cycle160Rect(rect).height() > 0);

            const QVariantMap clicked = session.wordAt(page, qt.center());
            QCOMPARE(clicked.value("text").toString(), name);
            QCOMPARE(clicked.value("start").toInt(), start);
        }
        QVERIFY(session.wordAt(page, QPointF(5, 5)).isEmpty());
    }

    // Search is case-insensitive and finds the word on every page.
    const QVariantList found = session.search(QStringLiteral("omega"));
    QCOMPARE(found.size(), 5);
    for (int i = 0; i < found.size(); ++i) {
        const QVariantMap hit = found.at(i).toMap();
        QCOMPARE(hit.value("page").toInt(), i);
        QCOMPARE(session.pageText(i).mid(hit.value("start").toInt(), hit.value("length").toInt()), QStringLiteral("OMEGA"));
        QCOMPARE(hit.value("rects").toList().size(), 1);
        QVERIFY(hit.value("context").toString().contains("OMEGA"));
    }
    QVERIFY(session.search(QStringLiteral("absent")).isEmpty());
    QVERIFY(session.search(QString()).isEmpty());
}

void FomawriteTest::pdfSessionReadsLinksAndOutline() {
    Cycle160Fixtures f; QVERIFY(f.make());
    PdfSession session;
    QVERIFY(session.load(f.url("navigation.pdf")));
    // The internal link, the /URI action and the address in the page text.
    const QVariantList links = session.links(0);
    QCOMPARE(links.size(), 3);
    bool internal = false;
    QList<QUrl> web;
    for (const QVariant &value : links) {
        const QVariantMap link = value.toMap();
        const QRectF rect = link.value("rect").toRectF();
        QVERIFY(rect.width() > 0 && rect.height() > 0);
        if (link.value("url").toUrl().isEmpty()) {
            internal = true;
            QCOMPARE(link.value("page").toInt(), 1);
            QVERIFY(qAbs(rect.left() - 100) < 2 && qAbs(rect.top() - (792 - 618)) < 2);
        } else {
            web << link.value("url").toUrl();
            QCOMPARE(link.value("page").toInt(), -1);
        }
    }
    QVERIFY(internal);
    QVERIFY(web.contains(QUrl("https://example.org/omega")));
    QVERIFY(web.contains(QUrl("https://example.com/fomawrite")));
    QVERIFY(session.links(1).isEmpty());
    const QVariantList outline = session.outline();
    QCOMPARE(outline.size(), 2);
    QCOMPARE(outline.at(0).toMap().value("title").toString(), QStringLiteral("Opening"));
    QCOMPARE(outline.at(0).toMap().value("page").toInt(), 0);
    QCOMPARE(outline.at(1).toMap().value("title").toString(), QStringLiteral("Closing"));
    QCOMPARE(outline.at(1).toMap().value("page").toInt(), 1);
    QCOMPARE(outline.at(1).toMap().value("level").toInt(), outline.at(0).toMap().value("level").toInt());
}

void FomawriteTest::pdfPageDrawsTheAnnotationsInTheFile() {
    Cycle160Fixtures f; QVERIFY(f.make());
    // Fraction of yellowish pixels inside the highlight's rectangle.
    const auto yellowInHighlight = [&](const QString &file, double *fraction) -> QString {
        PdfSession session;
        if (!session.load(f.url(file))) return "could not load " + file;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("pdfSession", &session);
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport Fomawrite 1.0\nWindow { width: 612; height: 792; visible: true; color: 'black'\n"
                          "  property alias page: page\n  PdfPage { id: page; session: pdfSession; page: 0; width: 612; height: 792 } }", QUrl());
        QScopedPointer<QObject> object(component.create());
        auto *window = qobject_cast<QQuickWindow *>(object.data());
        if (!window) return component.errorString();
        auto *page = qvariant_cast<QQuickItem *>(object->property("page"));
        if (!page || !QTest::qWaitForWindowExposed(window)) return "no page";
        if (!QTest::qWaitFor([&] { return page->property("rendered").toBool(); }, 10000)) return "never rendered";
        QImage image;
        // The frame after the image arrives carries it.
        QTest::qWaitFor([&] {
            image = window->grabWindow();
            return !image.isNull() && image.pixelColor(10, 10) == QColor(Qt::white);
        }, 5000);
        if (image.isNull()) return "grab failed";
        const qreal scale = image.width() / 612.0;
        const QRect region = QRectF(PdfFixtures::highlightView().topLeft() * scale, PdfFixtures::highlightView().size() * scale)
            .toAlignedRect().intersected(image.rect());
        int yellow = 0, total = 0;
        for (int y = region.top(); y <= region.bottom(); ++y)
            for (int x = region.left(); x <= region.right(); ++x) {
                const QColor c = image.pixelColor(x, y);
                ++total;
                if (c.red() > 180 && c.green() > 140 && c.blue() < 120) ++yellow;
            }
        *fraction = total ? double(yellow) / total : 0;
        return {};
    };
    double annotated = 0, plain = 1;
    QString why = yellowInHighlight("annots.pdf", &annotated);
    QVERIFY2(why.isEmpty(), qPrintable(why));
    why = yellowInHighlight("plain.pdf", &plain);
    QVERIFY2(why.isEmpty(), qPrintable(why));
    QVERIFY2(annotated > 0.4, qPrintable(QString::number(annotated)));
    QVERIFY2(plain < 0.01, qPrintable(QString::number(plain)));
}

void FomawriteTest::pdfPageListScrollKeepsMemoryBounded() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString path = directory.filePath("pages-300.pdf");
    QVERIFY(PdfFixtures::write(path, PdfFixtures::manyPagesDocument(300)));
    PdfSession session;
    QVERIFY(session.load(QUrl::fromLocalFile(path)));
    QCOMPARE(session.pageCount(), 300);
    QQmlEngine engine;
    engine.rootContext()->setContextProperty("pdfSession", &session);
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nimport Fomawrite 1.0\nWindow { width: 900; height: 1100; visible: true\n"
                      "  property alias view: list\n"
                      "  ListView { id: list; anchors.fill: parent; model: pdfSession.pageCount; cacheBuffer: 0; spacing: 8\n"
                      "    delegate: PdfPage { required property int index; session: pdfSession; page: index; width: 900; height: 1165 } } }", QUrl());
    QScopedPointer<QObject> object(component.create());
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    QVERIFY2(window, qPrintable(component.errorString()));
    QVERIFY(QTest::qWaitForWindowExposed(window));
    auto *list = qvariant_cast<QQuickItem *>(object->property("view"));
    QVERIFY(list);
    QTRY_COMPARE(list->property("count").toInt(), 300);
    const double start = cycle160RssMB();
    double peak = start;
    int renderedSeen = 0;
    for (int i = 0; i < 300; i += 3) {
        QMetaObject::invokeMethod(list, "positionViewAtIndex", Q_ARG(int, i), Q_ARG(int, 0));
        QTest::qWait(15);
        peak = qMax(peak, cycle160RssMB());
        for (auto *page : cycle160Pages(list)) if (page->rendered()) { ++renderedSeen; break; }
    }
    QTest::qWait(500);
    const double end = cycle160RssMB();
    qInfo("PdfPage list over 300 pages: RSS start %.0f MB, peak %.0f MB, end %.0f MB (%s)",
          start, peak, end, qPrintable(QGuiApplication::platformName()));
    QVERIFY2(renderedSeen > 10, qPrintable(QString::number(renderedSeen))); // pages really rendered while scrolling
    QVERIFY2(peak - start < 400, qPrintable(QStringLiteral("grew %1 MB").arg(peak - start)));
    // Only the delegates on screen hold an image.
    QVERIFY(cycle160Pages(list).size() < 10);
}

void FomawriteTest::pdfOpenEntersPdfModeWithANotesCompanion() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    const QString markdown = f.path("Draft.md");
    QVERIFY(PdfFixtures::write(markdown, "# Draft\n"));
    Cycle160Document d; QVERIFY(d.create());
    auto *pdf = d.backend.pdf();
    QVERIFY(d.backend.open(QUrl::fromLocalFile(markdown)));
    QVERIFY(!pdf->active());
    QCOMPARE(d.backend.displayedFileUrl(), QUrl::fromLocalFile(markdown));

    // No notes yet: PDF mode with an untitled companion; nothing is written until Save.
    const QStringList before = QDir(f.directory.path()).entryList(QDir::Files);
    QVERIFY(d.backend.open(f.url("geometry.pdf")));
    QVERIFY(pdf->active());
    QCOMPARE(pdf->state(), QStringLiteral("ready"));
    QVERIFY(!pdf->notesExist());
    QVERIFY(d.backend.fileUrl().isEmpty());
    QCOMPARE(d.backend.saveTarget(), f.url("geometry.pdf.md"));
    QCOMPARE(d.backend.fileName(), QStringLiteral("geometry.pdf.md"));
    QCOMPARE(d.backend.displayedFileUrl(), f.url("geometry.pdf"));
    QCOMPARE(d.text(), QString());
    QVERIFY(!d.backend.modified());
    // Links in the notes resolve beside the PDF.
    QCOMPARE(d.backend.documentBaseUrl(), QUrl::fromLocalFile(f.directory.path() + QDir::separator()));
    QCOMPARE(d.backend.resolveDocumentLink("geometry.pdf#page=3"), QUrl(f.url("geometry.pdf").toString() + "#page=3"));
    QVERIFY(d.type("Notes on the geometry.\n"));
    QVERIFY(d.backend.modified());
    QCOMPARE(QDir(f.directory.path()).entryList(QDir::Files), before);
    QSignalSpy dialog(&d.backend, &Backend::saveDialogRequested);
    d.backend.save();
    QCOMPARE(dialog.size(), 0);
    QVERIFY(!d.backend.modified());
    QCOMPARE(cycle160Read(f.path("geometry.pdf.md")), QByteArray("Notes on the geometry.\n"));
    QCOMPARE(d.backend.fileUrl(), f.url("geometry.pdf.md"));
    QVERIFY(pdf->active());
    QVERIFY(pdf->notesExist());
    QVERIFY(d.backend.saveTarget().isEmpty());

    // An existing notes file is the companion.
    QVERIFY(d.backend.open(QUrl::fromLocalFile(markdown)));
    QVERIFY(!pdf->active());
    QVERIFY(d.backend.saveTarget().isEmpty());
    QCOMPARE(d.text(), QStringLiteral("# Draft\n"));
    QVERIFY(d.backend.open(f.url("geometry.pdf")));
    QVERIFY(pdf->active());
    QCOMPARE(d.backend.fileUrl(), f.url("geometry.pdf.md"));
    QCOMPARE(d.text(), QStringLiteral("Notes on the geometry.\n"));

    // Opening the notes file directly opens its PDF too: the same state.
    QVERIFY(d.backend.open(QUrl::fromLocalFile(markdown)));
    QVERIFY(d.backend.open(f.url("geometry.pdf.md")));
    QVERIFY(pdf->active());
    QCOMPARE(pdf->url(), f.url("geometry.pdf"));
    QCOMPARE(d.backend.fileUrl(), f.url("geometry.pdf.md"));
    QCOMPARE(d.backend.displayedFileUrl(), f.url("geometry.pdf"));

    // A notes-shaped file without its PDF is ordinary Markdown.
    QVERIFY(PdfFixtures::write(f.path("orphan.pdf.md"), "orphan\n"));
    QVERIFY(d.backend.open(f.url("orphan.pdf.md")));
    QVERIFY(!pdf->active());
    QCOMPARE(d.backend.fileUrl(), f.url("orphan.pdf.md"));

    // #page=N asks the view for that page (1-based in links), clamped.
    QVERIFY(d.backend.open(QUrl(f.url("geometry.pdf").toString() + "#page=3")));
    QCOMPARE(pdf->requestedPage(), 2);
    QVERIFY(d.backend.open(QUrl(f.url("geometry.pdf").toString() + "#page=40")));
    QCOMPARE(pdf->requestedPage(), 4);

    // New Document leaves PDF mode.
    d.backend.newDocument();
    QVERIFY(!pdf->active());
    QVERIFY(d.backend.saveTarget().isEmpty());
    QCOMPARE(d.backend.fileName(), QStringLiteral("Untitled.md"));
}

void FomawriteTest::pdfOpenFailuresHistoryAndRecentsUseThePdf() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    const QString markdown = f.path("Draft.md");
    QVERIFY(PdfFixtures::write(markdown, "# Draft\n"));
    Cycle160Document d; QVERIFY(d.create());
    auto *pdf = d.backend.pdf();
    QVERIFY(d.backend.open(QUrl::fromLocalFile(markdown)));
    QVERIFY(d.type("unsaved words"));

    // A PDF that cannot be opened changes nothing.
    QVERIFY(!d.backend.open(f.url("damaged.pdf")));
    QVERIFY(!pdf->active());
    QCOMPARE(d.backend.fileUrl(), QUrl::fromLocalFile(markdown));
    QCOMPARE(d.text(), QStringLiteral("# Draft\nunsaved words"));
    QVERIFY(d.backend.modified());
    QVERIFY2(d.backend.status().contains("damaged.pdf"), qPrintable(d.backend.status()));
    QVERIFY(PdfFixtures::write(markdown, "# Draft\n"));
    d.backend.reloadFromDisk();
    QVERIFY(!d.backend.modified());

    // In PDF mode a failed open keeps the PDF and its notes.
    QVERIFY(d.backend.open(f.url("geometry.pdf")));
    QVERIFY(d.type("pdf notes"));
    QVERIFY(!d.backend.open(f.url("not-a-pdf.pdf")));
    QVERIFY(pdf->active());
    QCOMPARE(pdf->url(), f.url("geometry.pdf"));
    QCOMPARE(d.text(), QStringLiteral("pdf notes"));
    d.backend.save();
    QVERIFY(!d.backend.modified());

    // Recents and back/forward hold the PDF, not its notes.
    const QString pdfPath = QFileInfo(f.path("geometry.pdf")).canonicalFilePath();
    const auto recents = qobject_cast<FileLibrary *>(d.backend.library())->recentFiles();
    QVERIFY(!recents.isEmpty());
    QCOMPARE(recents.first().toMap().value("url").toUrl(), QUrl::fromLocalFile(pdfPath));
    for (const QVariant &recent : recents)
        QVERIFY(!recent.toMap().value("url").toUrl().toLocalFile().endsWith(".pdf.md"));
    QVERIFY(d.backend.open(QUrl::fromLocalFile(markdown)));
    QVERIFY(!pdf->active());
    QVERIFY(d.backend.canGoBack());
    QVERIFY(d.backend.navigateHistory(-1) >= 0);
    QVERIFY(pdf->active());
    QCOMPARE(pdf->url(), f.url("geometry.pdf"));
    QCOMPARE(d.text(), QStringLiteral("pdf notes"));
    QVERIFY(d.backend.canGoForward());
    QVERIFY(d.backend.navigateHistory(1) >= 0);
    QVERIFY(!pdf->active());
    QCOMPARE(d.backend.fileUrl(), QUrl::fromLocalFile(markdown));

    // A window showing X.pdf owns X.pdf and X.pdf.md.
    QVERIFY(d.backend.navigateHistory(-1) >= 0);
    QVERIFY(d.backend.ownsDocument(pdfPath));
    QVERIFY(d.backend.ownsDocument(QFileInfo(f.path("geometry.pdf.md")).canonicalFilePath()));
    QVERIFY(!d.backend.ownsDocument(QFileInfo(markdown).canonicalFilePath()));

    // Untitled notes: the base URL is the PDF's folder before any save.
    QVERIFY(d.backend.open(f.url("plain.pdf")));
    QVERIFY(d.backend.fileUrl().isEmpty());
    QCOMPARE(d.backend.documentBaseUrl(), QUrl::fromLocalFile(f.directory.path() + QDir::separator()));
}

#ifdef Q_OS_MACOS
#include <fcntl.h>
#include <sys/param.h>
#include <unistd.h>
// How many of this process's open file descriptors refer to `path`.
static int cycle162OpenDescriptorsFor(const QString &path) {
    const QByteArray wanted = QFileInfo(path).canonicalFilePath().toLocal8Bit();
    int count = 0;
    for (int fd = 0, limit = getdtablesize(); fd < limit; ++fd) {
        char name[MAXPATHLEN];
        if (fcntl(fd, F_GETPATH, name) == 0 && QFileInfo(QString::fromLocal8Bit(name)).canonicalFilePath().toLocal8Bit() == wanted) ++count;
    }
    return count;
}
#endif

// The PDF is read into memory and its file is not held open, so it can be
// renamed, moved or replaced while shown (Windows refuses those on an open
// file; Windows CI's rename test is the evidence there).
void FomawriteTest::pdfSessionDoesNotHoldTheFileOpen() {
    Cycle160Fixtures f; QVERIFY(f.make());
    QVERIFY(PdfFixtures::write(f.path("pages-300.pdf"), PdfFixtures::manyPagesDocument(300)));
    PdfSession pdf;
    QVERIFY(pdf.load(f.url("pages-300.pdf")));
    QCOMPARE(pdf.pageCount(), 300);
    QVERIFY(pdf.pageText(249).contains("250"));
#ifdef Q_OS_MACOS
    QCOMPARE(cycle162OpenDescriptorsFor(f.path("pages-300.pdf")), 0);
#endif
    // The loaded document does not depend on the file any more.
    QVERIFY(QFile::remove(f.path("pages-300.pdf")));
    QVERIFY(pdf.pageText(299).contains("300"));
    QVERIFY(!pdf.selectionAt(120, 0, 4).value("rects").toList().isEmpty());
    // Password-protected and damaged files behave as before through this path.
    PdfSession locked;
    const QString lockedPath = QFINDTESTDATA("fixtures/password.pdf");
    QVERIFY(locked.load(QUrl::fromLocalFile(lockedPath)));
    QCOMPARE(locked.state(), QStringLiteral("password"));
    QVERIFY(locked.unlock(QStringLiteral("fomawrite-user")));
#ifdef Q_OS_MACOS
    QCOMPARE(cycle162OpenDescriptorsFor(lockedPath), 0);
#endif
    PdfSession damaged;
    QVERIFY(!damaged.load(f.url("damaged.pdf")));
    QCOMPARE(damaged.state(), QStringLiteral("error"));
}

// A notes file beside a PDF that cannot be loaded opens as plain Markdown.
void FomawriteTest::pdfNotesStillOpenWhenTheirPdfCannotBeRead() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    const QString notes = f.path("damaged.pdf.md");
    QVERIFY(PdfFixtures::write(notes, "# Kept notes\n"));
    Cycle160Document d; QVERIFY(d.create());
    QVERIFY(d.backend.open(QUrl::fromLocalFile(notes)));
    QVERIFY(!d.backend.pdf()->active());
    QCOMPARE(d.backend.fileUrl(), QUrl::fromLocalFile(notes));
    QCOMPARE(d.text(), QStringLiteral("# Kept notes\n"));
    QCOMPARE(d.backend.displayedFileUrl(), QUrl::fromLocalFile(notes));
    // From PDF mode too: the readable PDF closes and the notes open.
    QVERIFY(d.backend.open(f.url("geometry.pdf")));
    QVERIFY(d.backend.pdf()->active());
    QVERIFY(d.backend.open(QUrl::fromLocalFile(notes)));
    QVERIFY(!d.backend.pdf()->active());
    QCOMPARE(d.text(), QStringLiteral("# Kept notes\n"));
    d.backend.discardRecovery();
}

void FomawriteTest::pdfOpenFromAModifiedDocumentAsksFirst() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    const QString markdown = f.path("Draft.md");
    QVERIFY(PdfFixtures::write(markdown, "# Draft\n"));
    Cycle117Window w; QVERIFY2(w.start(), qPrintable(w.error));
    QVERIFY(w.backend.open(QUrl::fromLocalFile(markdown)));
    QVERIFY(QMetaObject::invokeMethod(w.editor, "insert", Q_ARG(int, 8), Q_ARG(QString, "Unsaved.")));
    QVERIFY(w.backend.modified());
    auto *prompt = w.window->findChild<QObject *>("unsavedChangesPrompt"); QVERIFY(prompt);
    QVERIFY(QMetaObject::invokeMethod(w.window.data(), "requestOpen", Q_ARG(QVariant, f.url("geometry.pdf"))));
    QCOMPARE(w.window->property("pendingAction").toString(), QStringLiteral("open"));
    QTRY_VERIFY(prompt->property("visible").toBool());
    QVERIFY(!w.backend.pdf()->active());
    // Cancel keeps the draft; Discard opens the PDF.
    QVERIFY(QMetaObject::invokeMethod(prompt, "cancelRequested"));
    QVERIFY(!w.backend.pdf()->active());
    QVERIFY(w.backend.modified());
    QVERIFY(QMetaObject::invokeMethod(w.window.data(), "requestOpen", Q_ARG(QVariant, f.url("geometry.pdf"))));
    QTRY_VERIFY(prompt->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(prompt, "discardRequested"));
    QTRY_VERIFY(w.backend.pdf()->active());
    QCOMPARE(w.backend.pdf()->url(), f.url("geometry.pdf"));
    QCOMPARE(cycle160Read(markdown), QByteArray("# Draft\n"));
    QVERIFY(QMetaObject::invokeMethod(prompt, "close"));

    // Unsaved notes do not ask again for the PDF already on screen.
    QVERIFY(QMetaObject::invokeMethod(w.editor, "insert", Q_ARG(int, 0), Q_ARG(QString, "Note.")));
    QVERIFY(w.backend.modified());
    QVERIFY(QMetaObject::invokeMethod(w.window.data(), "requestOpen", Q_ARG(QVariant, QUrl(f.url("geometry.pdf").toString() + "#page=2"))));
    QCOMPARE(w.window->property("pendingAction").toString(), QString());
    QCOMPARE(w.backend.pdf()->requestedPage(), 1);
    QVERIFY(w.backend.modified());
    w.backend.discardRecovery();
}

void FomawriteTest::pdfExplorerListsPdfsAndFoldsNotesIntoABadge() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto path = [&](const QString &name) { return directory.filePath(name); };
    QVERIFY(PdfFixtures::write(path("Report.pdf"), PdfFixtures::document({PdfFixtures::Page{
        PdfFixtures::wordsContent() + "% #pdfonlytag\n", 0, {}, {}}})));
    QVERIFY(PdfFixtures::write(path("Report.pdf.md"), "Notes #notestag\n"));
    QVERIFY(PdfFixtures::write(path("Scan.PDF"), PdfFixtures::document({PdfFixtures::geometryPages().first()})));
    QVERIFY(PdfFixtures::write(path("Lonely.pdf.md"), "No PDF here\n"));
    QVERIFY(PdfFixtures::write(path("Plain.md"), "# Plain\n"));
    FileLibrary library;
    library.setRootFolder(QUrl::fromLocalFile(directory.path()));
    QMap<QString, QVariantMap> rows;
    for (const QVariant &entry : library.entries()) rows.insert(entry.toMap().value("name").toString(), entry.toMap());
    QCOMPARE(QStringList(rows.keys()), (QStringList{"Lonely.pdf.md", "Plain.md", "Report.pdf", "Scan.PDF"}));
    QVERIFY(rows["Report.pdf"].value("pdf").toBool());
    QVERIFY(rows["Report.pdf"].value("hasNotes").toBool());
    QVERIFY(rows["Scan.PDF"].value("pdf").toBool());
    QVERIFY(!rows["Scan.PDF"].value("hasNotes").toBool());
    QVERIFY(!rows["Plain.md"].value("pdf").toBool());
    QVERIFY(!rows["Lonely.pdf.md"].value("pdf").toBool());

    // A PDF is never read as text: no excerpt, card, tags or content search.
    const QUrl report = QUrl::fromLocalFile(path("Report.pdf"));
    QVERIFY(library.excerpt(report).isEmpty());
    QVERIFY(library.documentSummary(report).isEmpty());
    library.refreshTags();
    QTRY_VERIFY(!library.tagStatus().contains("Scanning", Qt::CaseInsensitive) && !library.tagIndex().isEmpty());
    QStringList tags;
    for (const QVariant &tag : library.tagIndex()) tags << tag.toMap().value("tag").toString() << tag.toMap().value("name").toString();
    QVERIFY2(tags.join(' ').contains("notestag"), qPrintable(tags.join(' ')));
    QVERIFY2(!tags.join(' ').contains("pdfonlytag"), qPrintable(tags.join(' ')));
    library.quickSearch(QStringLiteral("pdfonlytag"), true);
    QTRY_VERIFY(library.quickStatus().contains("matching"));
    QVERIFY(library.quickResults().isEmpty());

    // Rows that are about to be opened, and refused text actions.
    Backend backend;
    QCOMPARE(library.showFile(report) >= 0, true);
    const QVariantMap info = backend.libraryItemInfo(report);
    QVERIFY(info.value("pdf").toBool());
    QVERIFY(info.value("hasNotes").toBool());
    // Rename and Trash move the notes with the PDF since Cycle 162 (tests/cycle162-pdf-integration.inc).
    for (const char *action : {"copyMarkdown", "duplicate", "exportHtml"})
        QVERIFY2(!backend.libraryItemAction(report, QString::fromLatin1(action), "Other.pdf"), action);
    QVERIFY(QFileInfo::exists(path("Report.pdf")) && !QFileInfo::exists(path("Other.pdf")));

    // Without its PDF the notes file is listed again.
    QVERIFY(QFile::remove(path("Report.pdf")));
    library.refresh();
    QStringList names;
    for (const QVariant &entry : library.entries()) names << entry.toMap().value("name").toString();
    QVERIFY(names.contains("Report.pdf.md"));
    backend.discardRecovery();
}

void FomawriteTest::pdfExplorerRowOpensThePdfAndTheWindowShowsIt() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    const QString markdown = f.path("Draft.md");
    QVERIFY(PdfFixtures::write(markdown, "# Draft\n"));
    QVERIFY(PdfFixtures::write(f.path("navigation.pdf.md"), "Navigation notes\n"));
    Cycle117Window w; QVERIFY2(w.start(), qPrintable(w.error));
    QVERIFY(w.backend.open(QUrl::fromLocalFile(markdown)));
    w.layout->setProperty("liveEditEnabled", false);
    w.layout->setProperty("layoutMode", 0);
    QTRY_COMPARE(w.layout->property("effectiveLayoutMode").toInt(), 0);
    w.layout->setProperty("filesVisible", true);
    auto *library = qobject_cast<FileLibrary *>(w.backend.library()); QVERIFY(library);
    library->setRootFolder(QUrl::fromLocalFile(f.directory.path()));
    auto *files = w.window->findChild<QQuickItem *>("libraryFiles"); QVERIFY(files);
    QTRY_VERIFY(files->isVisible());

    // The PDF row carries its kind label and no badge without notes.
    QQuickItem *row = nullptr;
    QTRY_VERIFY((row = cycle117Entry(files, "geometry.pdf")) != nullptr);
    auto *kind = row->findChild<QQuickItem *>("libraryEntryKind"); QVERIFY(kind);
    QVERIFY(kind->isVisible());
    QCOMPARE(kind->property("text").toString(), QStringLiteral("PDF"));
    auto *badge = row->findChild<QQuickItem *>("libraryEntryNotesBadge"); QVERIFY(badge);
    QVERIFY(!badge->isVisible());
    QQuickItem *navigation = nullptr;
    QTRY_VERIFY((navigation = cycle117Entry(files, "navigation.pdf")) != nullptr);
    QVERIFY(navigation->findChild<QQuickItem *>("libraryEntryNotesBadge")->isVisible());
    QVERIFY(!cycle117Entry(files, "navigation.pdf.md"));
    auto *markdownRow = cycle117Entry(files, "Draft.md"); QVERIFY(markdownRow);
    QVERIFY(!markdownRow->findChild<QQuickItem *>("libraryEntryKind")->isVisible());

    // Clicking the row opens the PDF: PDF only, the reader in the preview slot.
    w.quick->requestActivate();
    QVERIFY(cycle102Click(w.quick, row));
    QTRY_VERIFY(w.backend.pdf()->active());
    const auto canonical = [&](const QString &name) { return QFileInfo(f.path(name)).canonicalFilePath(); };
    QCOMPARE(QFileInfo(w.backend.pdf()->url().toLocalFile()).canonicalFilePath(), canonical("geometry.pdf"));
    QTRY_COMPARE(w.layout->property("layoutMode").toInt(), 2);
    auto *reader = w.window->findChild<QQuickItem *>("pdfReader");
    QTRY_VERIFY((reader = w.window->findChild<QQuickItem *>("pdfReader")) != nullptr);
    QTRY_VERIFY(reader->isVisible());
    QVERIFY(!w.preview->isVisible());
    auto *pageList = reader->findChild<QQuickItem *>("pdfPageList"); QVERIFY(pageList);
    QTRY_COMPARE(pageList->property("count").toInt(), 5);
    QTRY_VERIFY([&] { for (auto *page : cycle160Pages(pageList)) if (page->rendered()) return true; return false; }());
    QCOMPARE(QFileInfo(w.window->findChild<QQuickItem *>("libraryPane")->property("currentFile").toUrl().toLocalFile()).canonicalFilePath(), canonical("geometry.pdf"));

    // A PDF with notes opens split; the notes are in the editor.
    QVERIFY(QMetaObject::invokeMethod(w.window.data(), "requestOpen", Q_ARG(QVariant, f.url("navigation.pdf"))));
    QTRY_COMPARE(QFileInfo(w.backend.pdf()->url().toLocalFile()).canonicalFilePath(), canonical("navigation.pdf"));
    QTRY_COMPARE(w.layout->property("layoutMode").toInt(), 1);
    QTRY_COMPARE(w.layout->property("effectiveLayoutMode").toInt(), 1);
    QCOMPARE(w.editor->property("text").toString(), QStringLiteral("Navigation notes\n"));
    QTRY_VERIFY(reader->isVisible() && w.editor->isVisible());
    QVERIFY(!w.preview->isVisible());

    // #page=2 scrolls the list to the second page.
    QVERIFY(QMetaObject::invokeMethod(w.window.data(), "requestOpen", Q_ARG(QVariant, QUrl(f.url("navigation.pdf").toString() + "#page=2"))));
    QCOMPARE(w.backend.pdf()->requestedPage(), 1);
    QTRY_VERIFY(pageList->property("contentY").toReal() > 100);

    // Leaving PDF mode restores the layout chosen before it and the Markdown interface.
    QVERIFY(QMetaObject::invokeMethod(w.window.data(), "requestOpen", Q_ARG(QVariant, QUrl::fromLocalFile(markdown))));
    QTRY_VERIFY(!w.backend.pdf()->active());
    QTRY_COMPARE(w.layout->property("layoutMode").toInt(), 0);
    QTRY_VERIFY(!w.window->findChild<QQuickItem *>("pdfReader"));
    QCOMPARE(w.editor->property("text").toString(), QStringLiteral("# Draft\n"));
    w.layout->setProperty("layoutMode", 1);
    QTRY_VERIFY(w.preview->isVisible());
}
