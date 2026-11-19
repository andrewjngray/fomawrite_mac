// Cycle 161 — the PDF pane: toolbar and folding, zoom and fit, keyboard and
// page navigation, selection and copy, links, find (incremental in
// PdfSession), thumbnails, Inspector, password and error states, remembered
// view, deferred page renders, and the window's Notes / Split / PDF footer,
// title and PDF menu. Fixtures: tests/pdffixtures.h and Cycle160Fixtures.

#include <QClipboard>
#include <QElapsedTimer>
#include <QPainter>
#include <QPdfWriter>

namespace {
// Pages of dense text, for timing search on something like a real document.
QByteArray cycle161DenseDocument(int count) {
    QList<PdfFixtures::Page> pages;
    for (int n = 0; n < count; ++n) {
        QByteArray content;
        for (int line = 0; line < 45; ++line)
            content += "BT /F1 10 Tf 50 " + QByteArray::number(750 - line * 15)
                + " Td (The quick brown fox jumps over the lazy dog and keeps running far away, line "
                + QByteArray::number(line) + ") Tj ET\n";
        pages.append(PdfFixtures::Page{content, 0, {}, {}});
    }
    return PdfFixtures::document(pages);
}
// Five pages labelled i, ii, 1, 2, 3.
QByteArray cycle161LabelledDocument() {
    QList<PdfFixtures::Page> pages;
    for (int n = 0; n < 5; ++n) pages.append(PdfFixtures::Page{PdfFixtures::wordsContent(), 0, {}, {}});
    return PdfFixtures::document(pages, [](PdfFixtures::Writer &, const QList<int> &) {
        return QByteArray("/PageLabels << /Nums [0 << /S /r >> 2 << /S /D >>] >>");
    });
}

void cycle161Type(QWindow *window, const QString &text) {
    for (const QChar c : text) QTest::keyClick(window, c.toLatin1());
}

// The PDF menu and Edit > Copy are in the native menu bar on macOS and in
// WindowsMenuBar.qml elsewhere (the native bar is not loaded there). Tests
// reach the items through this, by command id ("goToPage", "copyQuote", …).
struct Cycle161Menus {
    QObject *window = nullptr;
    QObject *nativeMenu() const { return window->findChild<QObject *>("nativePdfMenu"); }
    QObject *windowsBar() const { return window->findChild<QObject *>("windowsMenuBar"); }
    bool native() const { return nativeMenu() != nullptr; }
    QObject *item(const QString &command) const {
        if (native()) return window->findChild<QObject *>("pdfMenu" + command.left(1).toUpper() + command.mid(1));
        return windowsBar() ? windowsBar()->findChild<QObject *>("windows_pdf_" + command) : nullptr;
    }
    QObject *editCopy() const {
        if (native()) return window->findChild<QObject *>("editCopy");
        return windowsBar() ? windowsBar()->findChild<QObject *>("windows_editCopy") : nullptr;
    }
    bool pdfMenuShown() const {
        if (auto *menu = nativeMenu()) return menu->property("visible").toBool();
        QVariant index = -1;
        if (auto *bar = windowsBar()) QMetaObject::invokeMethod(bar, "pdfMenuIndex", Q_RETURN_ARG(QVariant, index));
        return index.toInt() >= 0;
    }
    // The Windows bar hands focus back to the item it last saw editing, as it
    // does when a menu opens; a test that triggers an item directly says which.
    void rememberFocus(QQuickWindow *quick) const {
        if (auto *bar = windowsBar())
            QMetaObject::invokeMethod(bar, "rememberEditingFocus", Q_ARG(QVariant, QVariant::fromValue(quick->activeFocusItem())));
    }
};

struct Cycle161Pane {
    Cycle117Window w;
    QQuickItem *reader = nullptr;
    QQuickItem *list = nullptr;
    bool start(int width = 1280) {
        if (!w.start(width)) return false;
        w.layout->setProperty("liveEditEnabled", false);
        return true;
    }
    bool open(const QUrl &url) {
        if (!QMetaObject::invokeMethod(w.window.data(), "requestOpen", Q_ARG(QVariant, url))) return false;
        const bool shown = QTest::qWaitFor([&] {
            reader = w.window->findChild<QQuickItem *>("pdfReader");
            list = reader ? reader->findChild<QQuickItem *>("pdfPageList") : nullptr;
            return reader && list && reader->property("ready").toBool()
                && list->property("count").toInt() == w.backend.pdf()->pageCount()
                && reader->property("shownUrl").toUrl() == w.backend.pdf()->url();
        }, 5000);
        QTest::qWait(60); // the requested page is shown on the next turn
        return shown;
    }
    QQuickItem *item(const char *name) const { return reader->findChild<QQuickItem *>(name); }
    QObject *object(const char *name) const { return reader->findChild<QObject *>(name); }
    QVariant call(const char *method) {
        QVariant result;
        QMetaObject::invokeMethod(reader, method, Q_RETURN_ARG(QVariant, result));
        return result;
    }
    QVariant call(const char *method, const QVariant &a) {
        QVariant result;
        QMetaObject::invokeMethod(reader, method, Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, a));
        return result;
    }
    QVariant call(const char *method, const QVariant &a, const QVariant &b) {
        QVariant result;
        QMetaObject::invokeMethod(reader, method, Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, a), Q_ARG(QVariant, b));
        return result;
    }
    qreal scale() const { return reader->property("pageScale").toReal(); }
    qreal zoom() const { return reader->property("zoom").toReal(); }
    int currentPage() const { return reader->property("currentPage").toInt(); }
    PdfPageItem *page(int index) const {
        for (auto *item : cycle160Pages(list)) if (item->objectName() == "pdfPage" && item->page() == index) return item;
        return nullptr;
    }
    // A point given in page view points, in the window's scene.
    QPointF scene(int index, QPointF points) const {
        auto *item = page(index);
        return item ? item->mapToScene(points * scale()) : QPointF(-1, -1);
    }
    QRectF viewport() const { return cycle103SceneBounds(list); }
    QMap<QString, QVariant> anchor() {
        return call("anchorAt", list->width() / 2, list->height() / 2).toMap();
    }
    // Scrolls so that a point of a page (in view points) is in the middle of the pane.
    void reveal(int index, QPointF points) {
        call("goToPage", index);
        QTest::qWait(30);
        const QPointF at = scene(index, points);
        list->setProperty("contentY", list->property("contentY").toReal() + at.y() - viewport().center().y());
        QTest::qWait(30);
    }
    void key(int key, Qt::KeyboardModifiers modifiers = Qt::NoModifier) { QTest::keyClick(w.quick, Qt::Key(key), modifiers); }
    bool settled() {
        return QTest::qWaitFor([&] { return !reader->property("restoringView").toBool(); }, 2000);
    }
};
}

void FomawriteTest::pdfSessionFindsIncrementallyAndRemembersViews() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    QVERIFY(PdfFixtures::write(f.path("pages-300.pdf"), PdfFixtures::manyPagesDocument(300)));
    QVERIFY(PdfFixtures::write(f.path("dense-300.pdf"), cycle161DenseDocument(300)));

    // How long the one-call search takes (the brief's measure, and a dense document).
    qint64 fixtureMs = 0, denseMs = 0;
    {
        PdfSession session; QVERIFY(session.load(f.url("pages-300.pdf")));
        QElapsedTimer clock; clock.start();
        QCOMPARE(session.search("PAGE").size(), 300);
        fixtureMs = clock.elapsed();
    }
    {
        PdfSession session; QVERIFY(session.load(f.url("dense-300.pdf")));
        QElapsedTimer clock; clock.start();
        QCOMPARE(session.search("lazy").size(), PdfSession::SearchLimit);
        denseMs = clock.elapsed();
    }
    qInfo("PdfSession::search, first query: 300-page fixture %lld ms, 300 dense pages %lld ms", fixtureMs, denseMs);

    // startSearch returns at once and the results arrive page by page.
    PdfSession dense; QVERIFY(dense.load(f.url("dense-300.pdf")));
    QSignalSpy changed(&dense, &PdfSession::searchResultsChanged);
    QSignalSpy searching(&dense, &PdfSession::searchingChanged);
    QElapsedTimer clock; clock.start();
    dense.startSearch("lazy");
    const qint64 returned = clock.elapsed();
    QVERIFY(dense.searching());
    QCOMPARE(dense.searchQuery(), QStringLiteral("lazy"));
    QVERIFY2(dense.searchResults().size() < PdfSession::SearchLimit, qPrintable(QString::number(dense.searchResults().size())));
    QVERIFY2(returned < 150, qPrintable(QStringLiteral("startSearch took %1 ms").arg(returned)));
    QTRY_VERIFY_WITH_TIMEOUT(!dense.searching(), 15000);
    qInfo("PdfSession::startSearch on 300 dense pages: returned after %lld ms, %lld result updates, finished after %lld ms",
          returned, qint64(changed.size()), clock.elapsed());
    QVERIFY(changed.size() > 2);
    QCOMPARE(searching.size(), 2);
    const QVariantList all = dense.search("lazy");
    QCOMPARE(dense.searchResults().size(), all.size());
    for (int i = 0; i < all.size(); i += 97) {
        QCOMPARE(dense.searchResults().at(i).toMap().value("page"), all.at(i).toMap().value("page"));
        QCOMPARE(dense.searchResults().at(i).toMap().value("start"), all.at(i).toMap().value("start"));
        QCOMPARE(dense.searchResults().at(i).toMap().value("rects"), all.at(i).toMap().value("rects"));
    }
    dense.clearSearch();
    QVERIFY(dense.searchResults().isEmpty() && dense.searchQuery().isEmpty() && !dense.searching());

    // In page order, case-insensitive; closing or another document clears them.
    PdfSession session; QVERIFY(session.load(f.url("geometry.pdf")));
    session.startSearch("omega");
    QTRY_VERIFY(!session.searching());
    QCOMPARE(session.searchResults().size(), 5);
    for (int i = 0; i < 5; ++i) QCOMPARE(session.searchResults().at(i).toMap().value("page").toInt(), i);
    QVERIFY(session.load(f.url("plain.pdf")));
    QVERIFY(session.searchResults().isEmpty());

    // Remembered views, by canonical path, newest first, at most 200.
    QVERIFY(session.rememberedView(f.url("geometry.pdf")).isEmpty());
    session.rememberView(f.url("geometry.pdf"), 3, "fixed", 1.5);
    QVariantMap view = session.rememberedView(f.url("geometry.pdf"));
    QCOMPARE(view.value("page").toInt(), 3);
    QCOMPARE(view.value("mode").toString(), QStringLiteral("fixed"));
    QCOMPARE(view.value("zoom").toReal(), 1.5);
    QCOMPARE(view.value("path").toString(), QFileInfo(f.path("geometry.pdf")).canonicalFilePath());
    // Opening a PDF asks for its remembered page; a reload keeps the page in view.
    QVERIFY(session.load(f.url("geometry.pdf")));
    QCOMPARE(session.requestedPage(), 3);
    QVERIFY(session.load(f.url("plain.pdf")));
    QCOMPARE(session.requestedPage(), 0);
    for (int i = 0; i < 205; ++i)
        session.rememberView(QUrl::fromLocalFile(f.path(QStringLiteral("missing-%1.pdf").arg(i))), i, "fit", 1);
    QCOMPARE(QSettings().value("pdf/rememberedViews").toList().size(), PdfSession::RememberedViewLimit);
    QCOMPARE(session.rememberedView(QUrl::fromLocalFile(f.path("missing-204.pdf"))).value("page").toInt(), 204);
    QVERIFY(session.rememberedView(QUrl::fromLocalFile(f.path("missing-4.pdf"))).isEmpty());
    QVERIFY(session.rememberedView(f.url("geometry.pdf")).isEmpty()); // the oldest went first
    QCOMPARE(QSettings().value("pdf/rememberedViews").toList().first().toMap().value("page").toInt(), 204);

    // A PDF that places each glyph on its own (as QPdfWriter does) still gives
    // one rectangle per line for a match and a selection.
    {
        QPdfWriter writer(f.path("glyphs.pdf"));
        writer.setResolution(72);
        QPainter painter(&writer);
        painter.setFont(QFont("Helvetica", 20));
        painter.drawText(72, 100, "Glyph by glyph marigold text");
        painter.drawText(72, 140, "and a second line");
    }
    PdfSession glyphs; QVERIFY(glyphs.load(f.url("glyphs.pdf")));
    const QVariantList marigold = glyphs.search("marigold");
    QCOMPARE(marigold.size(), 1);
    QCOMPARE(marigold.first().toMap().value("rects").toList().size(), 1);
    const QRectF band = marigold.first().toMap().value("rects").toList().first().toRectF();
    QVERIFY2(band.width() > 3 * band.height(), qPrintable(QStringLiteral("%1 x %2").arg(band.width()).arg(band.height())));
    const int lineStart = int(glyphs.pageText(0).indexOf("text"));
    QVERIFY(lineStart > 0);
    const QVariantMap twoLines = glyphs.selectionAt(0, lineStart, int(glyphs.pageText(0).indexOf("second")) - lineStart + 6);
    QCOMPARE(twoLines.value("rects").toList().size(), 2);

    // The thumbnails setting is the app's, and copyText reaches the clipboard.
    QVERIFY(!session.thumbnailsVisible());
    session.setThumbnailsVisible(true);
    QVERIFY(PdfSession().thumbnailsVisible());
    session.copyText("Copied from the PDF");
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("Copied from the PDF"));
}

void FomawriteTest::pdfPaneToolbarFoldsWithoutOverlapAtEveryWidth() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(1700), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    const QStringList names{"pdfThumbnailsToggle", "pdfZoomOut", "pdfActualSize", "pdfZoomIn", "pdfFitWidth",
                            "pdfPageField", "pdfPageCount", "pdfInspectorButton", "pdfFindButton", "pdfToolbarOverflow"};
    const QStringList foldOrder{"pdfActualSize", "pdfFitWidth", "pdfThumbnailsToggle", "pdfInspectorButton"};
    const QStringList overflowItems{"pdfOverflowActualSize", "pdfOverflowFitWidth", "pdfOverflowThumbnails", "pdfOverflowInspector"};
    auto *toolbar = p.item("pdfToolbar"); QVERIFY(toolbar);
    for (const int width : {320, 420, 600, 900}) {
        p.w.layout->setProperty("previewWidth", width);
        QTRY_VERIFY2(qAbs(p.reader->width() - width) < 1.5, qPrintable(QStringLiteral("%1 for %2").arg(p.reader->width()).arg(width)));
        QTest::qWait(30);
        const QRectF bar = cycle103SceneBounds(toolbar);
        QList<QQuickItem *> shown;
        for (const QString &name : names) {
            auto *control = p.item(name.toUtf8().constData());
            QVERIFY2(control, qPrintable(name));
            if (!control->isVisible()) continue;
            const QRectF bounds = cycle103SceneBounds(control);
            QVERIFY2(bar.adjusted(-0.5, -0.5, 0.5, 0.5).contains(bounds),
                     qPrintable(QStringLiteral("%1 clipped at %2").arg(name).arg(width)));
            for (auto *other : shown) {
                const QRectF overlap = bounds.intersected(cycle103SceneBounds(other));
                QVERIFY2(overlap.width() <= 0.5 || overlap.height() <= 0.5,
                         qPrintable(QStringLiteral("%1 overlaps %2 at %3").arg(name, other->objectName()).arg(width)));
            }
            shown << control;
        }
        // Zoom, page and find never fold; the rest fold in order into the menu.
        for (const char *name : {"pdfZoomOut", "pdfZoomIn", "pdfPageField", "pdfFindButton"})
            QVERIFY2(p.item(name)->isVisible(), name);
        bool folding = true;
        int folded = 0;
        QList<bool> hiddenControls;
        for (int i = 0; i < foldOrder.size(); ++i) {
            const bool hidden = !p.item(foldOrder.at(i).toUtf8().constData())->isVisible();
            if (!hidden) folding = false;
            QVERIFY2(!hidden || folding, qPrintable(QStringLiteral("fold order broken at %1").arg(width)));
            folded += hidden;
            hiddenControls << hidden;
        }
        QCOMPARE(p.item("pdfToolbarOverflow")->isVisible(), folded > 0);
        // The "more" menu offers exactly the folded controls.
        if (folded > 0) {
            auto *menu = toolbar->findChild<QObject *>("pdfToolbarOverflowMenu"); QVERIFY(menu);
            QVERIFY(cycle102Click(p.w.quick, p.item("pdfToolbarOverflow")));
            QTRY_VERIFY(menu->property("opened").toBool());
            for (int i = 0; i < overflowItems.size(); ++i) {
                auto *entry = toolbar->findChild<QQuickItem *>(overflowItems.at(i));
                QVERIFY(entry);
                QCOMPARE(entry->isVisible(), hiddenControls.at(i));
            }
            QMetaObject::invokeMethod(menu, "close");
            QTRY_VERIFY(!menu->property("opened").toBool());
        }
        if (width == 900) QCOMPARE(folded, 0);
        if (width == 320) QVERIFY(folded >= 2);
        qInfo("PDF toolbar at %d px: %d control(s) folded", width, folded);
    }
}

void FomawriteTest::pdfPaneZoomsAndKeepsThePlace() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    QVERIFY(PdfFixtures::write(f.path("pages-20.pdf"), PdfFixtures::manyPagesDocument(20)));
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("pages-20.pdf")));
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);

    // Fit width by default: the page fills the pane less its margins.
    QCOMPARE(p.reader->property("zoomMode").toString(), QStringLiteral("fit"));
    QTRY_VERIFY(p.page(0));
    QTRY_VERIFY(qAbs(p.page(0)->width() - (p.list->width() - 32)) < 1);
    QVERIFY(p.item("pdfFitWidth")->property("checked").toBool());

    // Actual size: one point is 4/3 pixels.
    QVERIFY(cycle102Click(p.w.quick, p.item("pdfActualSize")));
    QTRY_COMPARE(p.zoom(), 1.0);
    QTRY_VERIFY(qAbs(p.page(0)->width() - 816) < 0.5);
    QCOMPARE(p.item("pdfActualSize")->property("text").toString(), QStringLiteral("100%"));
    QVERIFY(!p.item("pdfFitWidth")->property("checked").toBool());
    QVERIFY(cycle102Click(p.w.quick, p.item("pdfZoomIn")));
    QTRY_COMPARE(p.zoom(), 1.1);
    QVERIFY(cycle102Click(p.w.quick, p.item("pdfZoomOut")));
    QTRY_COMPARE(p.zoom(), 1.0);
    QVERIFY(cycle102Click(p.w.quick, p.item("pdfZoomOut")));
    QTRY_COMPARE(p.zoom(), 0.9);
    // The stops run from 25% to 500% and the buttons stop at the ends.
    for (int i = 0; i < 20; ++i) p.call("zoomOut");
    QTRY_COMPARE(p.zoom(), 0.25);
    QTRY_VERIFY(!p.item("pdfZoomOut")->isEnabled());
    for (int i = 0; i < 30; ++i) p.call("zoomIn");
    QTRY_COMPARE(p.zoom(), 5.0);
    QTRY_VERIFY(!p.item("pdfZoomIn")->isEnabled());
    QTRY_VERIFY(qAbs(p.page(p.currentPage())->width() - 816 * 5) < 1);
    // Wider than the pane: it scrolls sideways.
    QVERIFY(p.list->property("contentWidth").toReal() > p.list->width() + 100);
    QVERIFY(p.settled());

    // The keys zoom while the pane has focus.
    p.call("actualSize");
    QTRY_COMPARE(p.zoom(), 1.0);
    p.call("focusPages");
    QTRY_VERIFY(p.list->hasActiveFocus());
    p.key(Qt::Key_Equal, Qt::ControlModifier);
    QTRY_COMPARE(p.zoom(), 1.1);
    p.key(Qt::Key_Minus, Qt::ControlModifier);
    QTRY_COMPARE(p.zoom(), 1.0);
    p.key(Qt::Key_Minus, Qt::ControlModifier);
    QTRY_COMPARE(p.zoom(), 0.9);
    p.key(Qt::Key_0, Qt::ControlModifier);
    QTRY_COMPARE(p.zoom(), 1.0);
    QVERIFY(p.settled());

    // Zooming keeps the same place of the document in the middle of the pane.
    p.call("goToPage", 7);
    QVERIFY(p.settled());
    p.list->setProperty("contentY", p.list->property("contentY").toReal() + 300);
    QTest::qWait(30);
    const QVariantMap before = p.anchor();
    QCOMPARE(before.value("page").toInt(), 7);
    QVERIFY(cycle102Click(p.w.quick, p.item("pdfZoomIn")));
    QTRY_COMPARE(p.zoom(), 1.1);
    QVERIFY(p.settled());
    QTest::qWait(30);
    QVariantMap after = p.anchor();
    QCOMPARE(after.value("page").toInt(), 7);
    QVERIFY2(qAbs(after.value("fy").toReal() - before.value("fy").toReal()) < 0.01,
             qPrintable(QStringLiteral("%1 then %2").arg(before.value("fy").toReal()).arg(after.value("fy").toReal())));
    QVERIFY(qAbs(after.value("fx").toReal() - before.value("fx").toReal()) < 0.01);
    QVERIFY(cycle102Click(p.w.quick, p.item("pdfFitWidth")));
    QTRY_COMPARE(p.reader->property("zoomMode").toString(), QStringLiteral("fit"));
    QVERIFY(p.settled());
    QTest::qWait(30);
    after = p.anchor();
    QCOMPARE(after.value("page").toInt(), 7);
    QVERIFY(qAbs(after.value("fy").toReal() - before.value("fy").toReal()) < 0.01);

    // Cmd/Ctrl + wheel zooms around the pointer.
    p.call("actualSize");
    QVERIFY(p.settled());
    QTest::qWait(30);
    const QPointF pointer = p.viewport().topLeft() + QPointF(p.list->width() * 0.3, p.list->height() * 0.25);
    const QPointF local = pointer - p.viewport().topLeft();
    const QVariantMap under = p.call("anchorAt", local.x(), local.y()).toMap();
    QWheelEvent wheel(pointer, p.w.quick->mapToGlobal(pointer.toPoint()), QPoint(), QPoint(0, 120),
                      Qt::NoButton, Qt::ControlModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(p.w.quick, &wheel);
    QTRY_VERIFY(p.zoom() > 1.1);
    QVERIFY(p.settled());
    QTest::qWait(30);
    const QVariantMap still = p.call("anchorAt", local.x(), local.y()).toMap();
    QCOMPARE(still.value("page").toInt(), under.value("page").toInt());
    QVERIFY(qAbs(still.value("fy").toReal() - under.value("fy").toReal()) < 0.01);
    // (Across, a page narrower than the pane stays centred.)
    // A plain wheel scrolls instead.
    const qreal zoomed = p.zoom(), y = p.list->property("contentY").toReal();
    QWheelEvent scroll(pointer, p.w.quick->mapToGlobal(pointer.toPoint()), QPoint(), QPoint(0, -120),
                       Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(p.w.quick, &scroll);
    QTRY_VERIFY(p.list->property("contentY").toReal() > y);
    QCOMPARE(p.zoom(), zoomed);
}

void FomawriteTest::pdfPaneNavigatesByKeyboardAndPageNumbers() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    QVERIFY(PdfFixtures::write(f.path("pages-20.pdf"), PdfFixtures::manyPagesDocument(20)));
    QVERIFY(PdfFixtures::write(f.path("labelled.pdf"), cycle161LabelledDocument()));
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("pages-20.pdf")));
    auto *field = p.item("pdfPageField"); auto *count = p.item("pdfPageCount");
    QVERIFY(field && count);
    QCOMPARE(p.currentPage(), 0);
    QCOMPARE(field->property("text").toString(), QStringLiteral("1"));
    QCOMPARE(count->property("text").toString(), QStringLiteral("of 20"));

    // Clicking a page gives the pane the keyboard.
    QTest::mouseClick(p.w.quick, Qt::LeftButton, Qt::NoModifier, p.scene(0, QPointF(20, 20)).toPoint());
    QTRY_VERIFY(p.list->hasActiveFocus());
    const auto y = [&] { return p.list->property("contentY").toReal(); };
    const qreal top = y(), step = p.list->height() - 40;
    p.key(Qt::Key_PageDown);
    QTRY_VERIFY(qAbs(y() - (top + step)) < 1);
    p.key(Qt::Key_Space);
    QTRY_VERIFY(qAbs(y() - (top + 2 * step)) < 1);
    p.key(Qt::Key_Space, Qt::ShiftModifier);
    QTRY_VERIFY(qAbs(y() - (top + step)) < 1);
    p.key(Qt::Key_PageUp);
    QTRY_VERIFY(qAbs(y() - top) < 1);
    p.key(Qt::Key_Down);
    QTRY_VERIFY(qAbs(y() - (top + 40)) < 1);
    p.key(Qt::Key_Up);
    QTRY_VERIFY(qAbs(y() - top) < 1);
    p.key(Qt::Key_End);
    QTRY_COMPARE(p.currentPage(), 19);
    QCOMPARE(field->property("text").toString(), QStringLiteral("20"));
    p.key(Qt::Key_Home);
    QTRY_COMPARE(p.currentPage(), 0);
    QVERIFY(qAbs(y() - p.list->property("originY").toReal()) < 1);

    // Go to a page by typing its number.
    p.call("focusPageField");
    QTRY_VERIFY(field->hasActiveFocus());
    cycle161Type(p.w.quick, "7");
    p.key(Qt::Key_Return);
    QTRY_COMPARE(p.currentPage(), 6);
    QCOMPARE(field->property("text").toString(), QStringLiteral("7"));
    QTRY_VERIFY(p.list->hasActiveFocus());
    // The page with most area in view is current, and requestedPage scrolls there.
    p.w.backend.pdf()->setRequestedPage(12);
    QTRY_COMPARE(p.currentPage(), 12);
    QVERIFY(qAbs(p.page(12)->mapToScene(QPointF()).y() - p.viewport().top() - 12) < 2);

    // Labels: typed labels win over numbers; the count shows the number.
    QVERIFY(p.open(f.url("labelled.pdf")));
    QCOMPARE(field->property("text").toString(), QStringLiteral("i"));
    QCOMPARE(count->property("text").toString(), QStringLiteral("(1 of 5)"));
    for (const auto &[typed, expected] : {std::pair{"ii", 1}, std::pair{"3", 4}, std::pair{"1", 2}, std::pair{"99", 2}, std::pair{"IV", 2}}) {
        p.call("focusPageField");
        QTRY_VERIFY(field->hasActiveFocus());
        cycle161Type(p.w.quick, typed);
        p.key(Qt::Key_Return);
        QTRY_COMPARE(p.currentPage(), expected);
    }
    QCOMPARE(field->property("text").toString(), QStringLiteral("1"));
    QCOMPARE(count->property("text").toString(), QStringLiteral("(3 of 5)"));

    // F6 reaches the PDF pane as the window's preview region.
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    p.w.editor->forceActiveFocus();
    QTRY_VERIFY(p.w.editor->hasActiveFocus());
    p.key(Qt::Key_F6);
    QTRY_VERIFY(p.list->hasActiveFocus());
    p.key(Qt::Key_F6, Qt::ShiftModifier);
    QTRY_VERIFY(p.w.editor->hasActiveFocus());
}

void FomawriteTest::pdfPaneSelectsCopiesAndFollowsLinks() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    QTRY_VERIFY(p.page(0));
    const QString text = p.w.backend.pdf()->pageText(0);
    const QRectF alpha = PdfFixtures::userToView(PdfFixtures::userBox(PdfFixtures::Alpha), 0, PdfFixtures::geometryCrop(0));
    const QRectF omega = PdfFixtures::userToView(PdfFixtures::userBox(PdfFixtures::Omega), 0, PdfFixtures::geometryCrop(0));

    // Drag across ALPHA.
    const QPoint from = p.scene(0, QPointF(alpha.left() + 0.5, alpha.center().y())).toPoint();
    const QPoint to = p.scene(0, QPointF(alpha.right() + 1, alpha.center().y())).toPoint();
    QTest::mousePress(p.w.quick, Qt::LeftButton, Qt::NoModifier, from);
    QTest::mouseMove(p.w.quick, (from + to) / 2);
    QTest::mouseMove(p.w.quick, to);
    QTest::mouseRelease(p.w.quick, Qt::LeftButton, Qt::NoModifier, to);
    QTRY_COMPARE(p.reader->property("selectionText").toString(), QStringLiteral("ALPHA"));
    QCOMPARE(p.reader->property("selectionPage").toInt(), 0);
    QCOMPARE(p.reader->property("selectionStart").toInt(), int(text.indexOf("ALPHA")));
    QCOMPARE(p.reader->property("selectionLength").toInt(), 5);
    QTRY_COMPARE(cycle103VisualItems(p.list, "pdfSelectionRect").size(), 1);
    const QRectF drawn = cycle103SceneBounds(cycle103VisualItems(p.list, "pdfSelectionRect").first());
    // Drawn over the session's rectangle for the word (with 2 px above and below), inside the word's box.
    const QRectF word = p.w.backend.pdf()->selectionAt(0, int(text.indexOf("ALPHA")), 5).value("rects").toList().first().toRectF();
    const QRectF expected(p.scene(0, word.topLeft()), p.scene(0, word.bottomRight()));
    const QRectF box(p.scene(0, alpha.topLeft()), p.scene(0, alpha.bottomRight()));
    QVERIFY2(qAbs(drawn.left() - expected.left()) < 1 && qAbs(drawn.right() - expected.right()) < 1
             && qAbs(drawn.top() - (expected.top() - 2)) < 1 && qAbs(drawn.bottom() - (expected.bottom() + 2)) < 1
             && box.adjusted(-3, -3, 3, 3).contains(drawn),
             qPrintable(QStringLiteral("drawn %1,%2 %3x%4").arg(drawn.x()).arg(drawn.y()).arg(drawn.width()).arg(drawn.height())));
    QVERIFY(p.list->hasActiveFocus());

    // Cmd+C copies it; so does Edit → Copy while the pane has focus.
    QGuiApplication::clipboard()->setText("before");
    p.key(Qt::Key_C, Qt::ControlModifier);
    QTRY_COMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("ALPHA"));
    const Cycle161Menus menus{p.w.window.data()};
    auto *menuCopy = menus.editCopy();
    QVERIFY(menuCopy);
    QVERIFY(menuCopy->property("enabled").toBool());
    QGuiApplication::clipboard()->setText("before");
    menus.rememberFocus(p.w.quick);
    QVERIFY(QMetaObject::invokeMethod(menuCopy, "triggered"));
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("ALPHA"));

    // Escape clears; a double-click selects a word; a click elsewhere clears.
    p.key(Qt::Key_Escape);
    QTRY_COMPARE(p.reader->property("selectionText").toString(), QString());
    QCOMPARE(p.reader->property("selectionPage").toInt(), -1);
    QTRY_COMPARE(cycle103VisualItems(p.list, "pdfSelectionRect").size(), 0);
    p.reveal(0, omega.center());
    QTest::mouseDClick(p.w.quick, Qt::LeftButton, Qt::NoModifier, p.scene(0, omega.center()).toPoint());
    QTRY_COMPARE(p.reader->property("selectionText").toString(), QStringLiteral("OMEGA"));
    QCOMPARE(p.reader->property("selectionStart").toInt(), int(text.indexOf("OMEGA")));
    QTest::qWait(400);
    QCOMPARE(p.reader->property("selectionText").toString(), QStringLiteral("OMEGA"));
    QTest::mouseClick(p.w.quick, Qt::LeftButton, Qt::NoModifier, p.scene(0, omega.center() + QPointF(150, 0)).toPoint());
    QTRY_COMPARE(p.reader->property("selectionText").toString(), QString());

    // Links: a pointing hand over one; an internal link goes to its page.
    QVERIFY(p.open(f.url("navigation.pdf")));
    QTRY_VERIFY(p.page(0));
    QCOMPARE(p.currentPage(), 0);
    QQuickItem *mouse = nullptr;
    for (auto *candidate : cycle103VisualItems(p.list, "pdfPageMouse"))
        if (candidate->parentItem() == p.page(0)->parentItem()) mouse = candidate;
    QVERIFY(mouse);
    QTest::mouseMove(p.w.quick, p.scene(0, alpha.center()).toPoint());
    QTRY_COMPARE(mouse->property("cursorShape").toInt(), int(Qt::PointingHandCursor));
    QTest::mouseMove(p.w.quick, p.scene(0, QPointF(40, 40)).toPoint());
    QTRY_COMPARE(mouse->property("cursorShape").toInt(), int(Qt::IBeamCursor));
    QTest::mouseClick(p.w.quick, Qt::LeftButton, Qt::NoModifier, p.scene(0, alpha.center()).toPoint());
    QTRY_COMPARE(p.currentPage(), 1);

    // A web link asks first and opens nothing until the reader agrees.
    p.reveal(0, omega.center());
    QTRY_COMPARE(p.currentPage(), 0);
    QSignalSpy opened(p.reader, SIGNAL(openUrlRequested(QUrl)));
    auto *confirm = p.object("pdfLinkConfirm"); QVERIFY(confirm);
    QVERIFY(!confirm->property("opened").toBool());
    QTest::mouseClick(p.w.quick, Qt::LeftButton, Qt::NoModifier, p.scene(0, omega.center()).toPoint());
    QTRY_VERIFY(confirm->property("opened").toBool());
    auto *url = confirm->findChild<QQuickItem *>("pdfLinkUrl"); QVERIFY(url);
    QCOMPARE(url->property("text").toString(), QStringLiteral("https://example.org/omega"));
    QVERIFY(confirm->findChild<QQuickItem *>("pdfLinkOpen"));
    QVERIFY(cycle102Click(p.w.quick, confirm->findChild<QQuickItem *>("pdfLinkCancel")));
    QTRY_VERIFY(!confirm->property("opened").toBool());
    QCOMPARE(opened.size(), 0);
    QCOMPARE(p.currentPage(), 0);
}

void FomawriteTest::pdfPaneFindsInThePdf() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);
    auto *bar = p.item("pdfFindBar"); auto *field = p.item("pdfFindField"); auto *status = p.item("pdfFindStatus");
    QVERIFY(bar && field && status);
    QVERIFY(!bar->isVisible());

    // Cmd+F with the PDF alone on screen opens the PDF's find, not the editor's.
    p.key(Qt::Key_F, Qt::ControlModifier);
    QTRY_VERIFY(bar->isVisible());
    QTRY_VERIFY(field->hasActiveFocus());
    QVERIFY(!p.w.window->property("searchOpen").toBool());
    QCOMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);
    cycle161Type(p.w.quick, "omega");
    QTRY_COMPARE(p.w.backend.pdf()->searchResults().size(), 5);
    QTRY_COMPARE(status->property("text").toString(), QStringLiteral("1 of 5"));
    const auto current = [&]() -> QQuickItem * {
        const auto items = cycle103VisualItems(p.list, "pdfFindCurrent");
        return items.isEmpty() ? nullptr : items.first();
    };
    const auto onScreen = [&](QQuickItem *item) {
        return item && item->isVisible() && p.viewport().contains(cycle103SceneBounds(item));
    };
    QTRY_VERIFY(onScreen(current()));
    QVERIFY(!cycle103VisualItems(p.list, "pdfFindTint").isEmpty() || cycle103VisualItems(p.list, "pdfFindCurrent").size() == 1);
    // The current match is on page 1; next and previous, Return and Shift+Return, wrap.
    const auto currentPageOfMatch = [&] {
        const int index = p.reader->property("findIndex").toInt();
        return p.w.backend.pdf()->searchResults().value(index).toMap().value("page").toInt();
    };
    QCOMPARE(currentPageOfMatch(), 0);
    p.key(Qt::Key_Return);
    QTRY_COMPARE(status->property("text").toString(), QStringLiteral("2 of 5"));
    QCOMPARE(currentPageOfMatch(), 1);
    QTRY_VERIFY(onScreen(current()));
    p.key(Qt::Key_Return, Qt::ShiftModifier);
    QTRY_COMPARE(status->property("text").toString(), QStringLiteral("1 of 5"));
    p.key(Qt::Key_Return, Qt::ShiftModifier);
    QTRY_COMPARE(status->property("text").toString(), QStringLiteral("5 of 5"));
    QCOMPARE(currentPageOfMatch(), 4);
    QTRY_VERIFY(onScreen(current()));
    // Cmd+G and Shift+Cmd+G move too.
    p.key(Qt::Key_G, Qt::ControlModifier);
    QTRY_COMPARE(status->property("text").toString(), QStringLiteral("1 of 5"));
    p.key(Qt::Key_G, Qt::ControlModifier | Qt::ShiftModifier);
    QTRY_COMPARE(status->property("text").toString(), QStringLiteral("5 of 5"));
    p.key(Qt::Key_G, Qt::ControlModifier);
    QTRY_COMPARE(status->property("text").toString(), QStringLiteral("1 of 5"));
    QTRY_VERIFY(onScreen(current()));
    // Every match on a visible page is tinted: page 1 shows one current match.
    const QRectF match = cycle103SceneBounds(current());
    const QRectF omega = PdfFixtures::userToView(PdfFixtures::userBox(PdfFixtures::Omega), 0, PdfFixtures::geometryCrop(0));
    QVERIFY(QRectF(p.scene(0, omega.topLeft()), p.scene(0, omega.bottomRight())).adjusted(-4, -6, 4, 6).contains(match));
    p.call("goToPage", 1);
    QTRY_VERIFY(!cycle103VisualItems(p.list, "pdfFindTint").isEmpty());
    // No matches.
    QTest::keyClick(p.w.quick, Qt::Key_A, Qt::ControlModifier);
    cycle161Type(p.w.quick, "absent");
    QTRY_COMPARE(status->property("text").toString(), QStringLiteral("No matches"));
    QTRY_VERIFY(current() == nullptr);
    // Escape closes and clears.
    QTest::keyClick(p.w.quick, Qt::Key_A, Qt::ControlModifier);
    cycle161Type(p.w.quick, "alpha");
    QTRY_COMPARE(p.w.backend.pdf()->searchResults().size(), 5);
    p.key(Qt::Key_Escape);
    QTRY_VERIFY(!bar->isVisible());
    QVERIFY(p.w.backend.pdf()->searchResults().isEmpty());
    QTRY_VERIFY(cycle103VisualItems(p.list, "pdfFindTint").isEmpty() && current() == nullptr);
    QTRY_VERIFY(p.list->hasActiveFocus());

    // In Split, Cmd+F in the notes is still the editor's find; in the PDF pane it is the PDF's.
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    p.w.layout->setProperty("liveEditEnabled", false);
    p.w.editor->forceActiveFocus();
    QTRY_VERIFY(p.w.editor->hasActiveFocus());
    p.key(Qt::Key_F, Qt::ControlModifier);
    QTRY_VERIFY(p.w.window->property("searchOpen").toBool());
    QVERIFY(!bar->isVisible());
    QMetaObject::invokeMethod(p.w.window.data(), "closeSearch", Q_ARG(QVariant, false));
    QTRY_VERIFY(!p.w.window->property("searchOpen").toBool());
    p.call("focusPages");
    QTRY_VERIFY(p.list->hasActiveFocus());
    p.key(Qt::Key_F, Qt::ControlModifier);
    QTRY_VERIFY(bar->isVisible());
    QVERIFY(!p.w.window->property("searchOpen").toBool());
}

void FomawriteTest::pdfPaneThumbnailsAndInspector() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    QVERIFY(PdfFixtures::write(f.path("pages-300.pdf"), PdfFixtures::manyPagesDocument(300)));
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("pages-300.pdf")));
    auto *thumbnails = p.item("pdfThumbnails"); auto *toggle = p.item("pdfThumbnailsToggle");
    QVERIFY(thumbnails && toggle);
    QVERIFY(!thumbnails->isVisible());
    const qreal listLeft = p.viewport().left();
    QVERIFY(cycle102Click(p.w.quick, toggle));
    QTRY_VERIFY(thumbnails->isVisible());
    QVERIFY(toggle->property("checked").toBool());
    QVERIFY(QSettings().value("pdf/thumbnailsVisible").toBool());
    QTRY_VERIFY(qAbs(p.viewport().left() - (listLeft + 140)) < 1);
    auto *column = thumbnails->findChild<QQuickItem *>("pdfThumbnailList"); QVERIFY(column);
    QTRY_COMPARE(column->property("count").toInt(), 300);

    // The current page is marked and kept in view as the pages move.
    const auto marked = [&]() -> QQuickItem * {
        const auto items = cycle103VisualItems(column, "pdfThumbnailCurrent");
        return items.size() == 1 ? items.first()->parentItem() : nullptr;
    };
    QTRY_VERIFY(marked() && marked()->property("index").toInt() == 0);
    p.call("goToPage", 150);
    QTRY_COMPARE(p.currentPage(), 150);
    QTRY_VERIFY(marked() && marked()->property("index").toInt() == 150);
    QTRY_VERIFY(cycle103SceneBounds(column).adjusted(-1, -1, 1, 1).contains(cycle103SceneBounds(marked())));
    // A click on a thumbnail goes to its page.
    QQuickItem *next = nullptr;
    for (auto *thumb : cycle103VisualItems(column, "pdfThumbnail"))
        if (thumb->property("index").toInt() != 150 && cycle103SceneBounds(column).contains(cycle103SceneBounds(thumb))) next = thumb;
    QVERIFY(next);
    const int target = next->property("index").toInt();
    QTest::mouseClick(p.w.quick, Qt::LeftButton, Qt::NoModifier, cycle103SceneBounds(next).center().toPoint());
    QTRY_COMPARE(p.currentPage(), target);
    QTRY_VERIFY(marked() && marked()->property("index").toInt() == target);
    // Memory: only the thumbnails near the view exist, and only those on screen render.
    for (int page = 0; page < 300; page += 25) { p.call("goToPage", page); QTest::qWait(20); }
    QTest::qWait(300);
    int thumbs = 0, rendered = 0;
    for (auto *page : cycle160Pages(column)) { ++thumbs; rendered += page->rendered(); }
    qInfo("Thumbnails over 300 pages: %d items alive, %d rendered", thumbs, rendered);
    QVERIFY2(thumbs < 20, qPrintable(QString::number(thumbs)));
    QVERIFY(rendered >= 1);
    // The setting survives the pane.
    QVERIFY(cycle102Click(p.w.quick, toggle));
    QTRY_VERIFY(!thumbnails->isVisible());
    QVERIFY(!QSettings().value("pdf/thumbnailsVisible").toBool());

    // The Inspector shows what the file says, "–" for what it does not.
    QVERIFY(p.open(f.url("navigation.pdf")));
    auto *inspector = p.object("pdfInspector"); QVERIFY(inspector);
    QVERIFY(cycle102Click(p.w.quick, p.item("pdfInspectorButton")));
    QTRY_VERIFY(inspector->property("opened").toBool());
    const auto value = [&](const QString &key) {
        const auto labels = cycle103VisualItems(qvariant_cast<QQuickItem *>(inspector->property("contentItem")), "pdfInspectorValue_" + key);
        auto *label = labels.isEmpty() ? nullptr : labels.first();
        return label ? label->property("text").toString() : QStringLiteral("<missing %1>").arg(key);
    };
    QCOMPARE(value("fileName"), QStringLiteral("navigation.pdf"));
    QCOMPARE(value("kind"), QStringLiteral("PDF document"));
    QCOMPARE(value("pdfVersion"), QStringLiteral("1.7"));
    QCOMPARE(value("pageCount"), QStringLiteral("2"));
    QCOMPARE(value("pageSize"), QStringLiteral("21.59 × 27.94 cm (8.50 × 11.00 in)"));
    QCOMPARE(value("title"), QStringLiteral("Fixture Title"));
    QCOMPARE(value("author"), QStringLiteral("Fixture Author"));
    QCOMPARE(value("subject"), QStringLiteral("Fixture Subject"));
    QCOMPARE(value("keywords"), QStringLiteral("alpha omega"));
    QCOMPARE(value("producer"), QStringLiteral("Fomawrite tests"));
    QCOMPARE(value("creator"), QStringLiteral("pdffixtures.h"));
    QVERIFY2(value("created").contains("2026") || value("created").contains("26"), qPrintable(value("created")));
    QVERIFY(!value("created").contains("T12:00")); // formatted for people, not ISO
    QCOMPARE(value("modified"), QStringLiteral("–"));
    QCOMPARE(value("encrypted"), QStringLiteral("No"));
    QCOMPARE(cycle103VisualItems(qvariant_cast<QQuickItem *>(inspector->property("contentItem")), "pdfInspectorLabel_pageSize").first()->property("text").toString(), QStringLiteral("Page size"));
    p.key(Qt::Key_Escape);
    QTRY_VERIFY(!inspector->property("opened").toBool());
}

void FomawriteTest::pdfPanePasswordAndErrorStates() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/password.pdf"), f.path("locked.pdf")));
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "requestOpen", Q_ARG(QVariant, f.url("locked.pdf"))));
    QTRY_COMPARE(p.w.backend.pdf()->state(), QStringLiteral("password"));
    QTRY_VERIFY((p.reader = p.w.window->findChild<QQuickItem *>("pdfReader")) != nullptr);
    p.list = p.item("pdfPageList");
    auto *prompt = p.item("pdfPasswordPrompt"); auto *field = p.item("pdfPasswordField");
    auto *unlock = p.item("pdfUnlockButton"); auto *error = p.item("pdfPasswordError");
    QVERIFY(prompt && field && unlock && error);
    QTRY_VERIFY(prompt->isVisible());
    QVERIFY(!p.list->isVisible());
    QVERIFY(!error->isVisible());
    for (const char *name : {"pdfZoomIn", "pdfZoomOut", "pdfActualSize", "pdfFitWidth", "pdfPageField", "pdfFindButton", "pdfInspectorButton", "pdfThumbnailsToggle"})
        QVERIFY2(!p.item(name)->isEnabled(), name);

    // A wrong password says so; the right one (Return or Unlock) shows the pages.
    field->forceActiveFocus();
    cycle161Type(p.w.quick, "wrong");
    p.key(Qt::Key_Return);
    QTRY_VERIFY(error->isVisible());
    QCOMPARE(error->property("text").toString(), QStringLiteral("The password is not correct"));
    QCOMPARE(p.w.backend.pdf()->state(), QStringLiteral("password"));
    QVERIFY(field->hasActiveFocus());
    QTest::keyClick(p.w.quick, Qt::Key_A, Qt::ControlModifier);
    cycle161Type(p.w.quick, "fomawrite-user");
    QVERIFY(cycle102Click(p.w.quick, unlock));
    QTRY_COMPARE(p.w.backend.pdf()->state(), QStringLiteral("ready"));
    QTRY_VERIFY(p.list->isVisible());
    QVERIFY(!prompt->isVisible());
    QTRY_COMPARE(p.list->property("count").toInt(), 1);
    QTRY_VERIFY(p.item("pdfZoomIn")->isEnabled() && p.item("pdfPageField")->isEnabled());

    // A file that turns unreadable shows the session's message instead of pages.
    QVERIFY(p.open(f.url("plain.pdf")));
    auto *message = p.item("pdfErrorText"); QVERIFY(message);
    QVERIFY(!message->isVisible());
    QVERIFY(PdfFixtures::write(f.path("plain.pdf"), PdfFixtures::damagedDocument()));
    QTRY_COMPARE_WITH_TIMEOUT(p.w.backend.pdf()->state(), QStringLiteral("error"), 5000);
    QTRY_VERIFY(message->isVisible());
    QCOMPARE(message->property("text").toString(), p.w.backend.pdf()->errorText());
    QVERIFY(message->property("text").toString().contains("plain.pdf"));
    QVERIFY(!p.list->isVisible());
    QVERIFY(!prompt->isVisible());
    for (const char *name : {"pdfZoomIn", "pdfPageField", "pdfFindButton", "pdfInspectorButton"})
        QVERIFY2(!p.item(name)->isEnabled(), name);
}

void FomawriteTest::pdfPaneRemembersPageAndZoomPerPdf() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    QVERIFY(PdfFixtures::write(f.path("pages-20.pdf"), PdfFixtures::manyPagesDocument(20)));
    const QString markdown = f.path("Draft.md");
    QVERIFY(PdfFixtures::write(markdown, "# Draft\n"));
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("pages-20.pdf")));
    QCOMPARE(p.currentPage(), 0);
    QCOMPARE(p.reader->property("zoomMode").toString(), QStringLiteral("fit"));
    p.call("actualSize");
    p.call("zoomIn"); p.call("zoomIn"); p.call("zoomIn"); // 110%, 125%, 150%
    QTRY_COMPARE(p.zoom(), 1.5);
    QVERIFY(p.settled());
    // A zoom alone is remembered shortly after.
    QTRY_COMPARE(p.w.backend.pdf()->rememberedView(f.url("pages-20.pdf")).value("zoom").toReal(), 1.5);
    QCOMPARE(p.w.backend.pdf()->rememberedView(f.url("pages-20.pdf")).value("mode").toString(), QStringLiteral("fixed"));
    p.call("goToPage", 12);
    QTRY_COMPARE(p.currentPage(), 12);

    // Leaving PDF mode and coming back returns to page 13 at 150%.
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "requestOpen", Q_ARG(QVariant, QUrl::fromLocalFile(markdown))));
    QTRY_VERIFY(!p.w.backend.pdf()->active());
    const QVariantMap stored = p.w.backend.pdf()->rememberedView(f.url("pages-20.pdf"));
    QCOMPARE(stored.value("page").toInt(), 12);
    QCOMPARE(stored.value("mode").toString(), QStringLiteral("fixed"));
    QCOMPARE(stored.value("zoom").toReal(), 1.5);
    QVERIFY(p.open(f.url("pages-20.pdf")));
    QTRY_COMPARE(p.currentPage(), 12);
    QCOMPARE(p.reader->property("zoomMode").toString(), QStringLiteral("fixed"));
    QCOMPARE(p.zoom(), 1.5);
    QVERIFY(qAbs(p.page(12)->mapToScene(QPointF()).y() - p.viewport().top() - 12) < 2);

    // Another PDF keeps its own view, and switching straight between PDFs saves it.
    p.call("goToPage", 5);
    QTRY_COMPARE(p.currentPage(), 5);
    QVERIFY(p.open(f.url("geometry.pdf")));
    QCOMPARE(p.currentPage(), 0);
    QCOMPARE(p.reader->property("zoomMode").toString(), QStringLiteral("fit"));
    QCOMPARE(p.w.backend.pdf()->rememberedView(f.url("pages-20.pdf")).value("page").toInt(), 5);

    // #page=N wins over the remembered page; the remembered zoom still applies.
    QVERIFY(p.open(QUrl(f.url("pages-20.pdf").toString() + "#page=3")));
    QTRY_COMPARE(p.currentPage(), 2);
    QCOMPARE(p.zoom(), 1.5);
    QTest::qWait(150);
    QCOMPARE(p.currentPage(), 2);
}

void FomawriteTest::pdfPaneDefersRendersDuringFastScroll() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    QVERIFY(PdfFixtures::write(f.path("pages-300.pdf"), PdfFixtures::manyPagesDocument(300)));
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("pages-300.pdf")));
    QTRY_VERIFY(p.page(0) && p.page(0)->rendered());
    QTest::qWait(300);
    auto *session = p.w.backend.pdf();
    const int before = session->renderRequestCount();
    // A flick through a hundred pages, one page about every 4 ms.
    const qreal pageStep = p.page(0)->height() + 12;
    for (int i = 1; i <= 100; ++i) {
        p.list->setProperty("contentY", p.list->property("originY").toReal() + i * pageStep);
        QTest::qWait(4);
    }
    const int during = session->renderRequestCount() - before;
    QTRY_VERIFY(p.page(100) && p.page(100)->rendered());
    QTest::qWait(300);
    const int after = session->renderRequestCount() - before;
    qInfo("Fast scroll through 100 pages: %d render requests while moving, %d once settled", during, after);
    QVERIFY2(during <= 10, qPrintable(QString::number(during)));
    QVERIFY2(after <= 14, qPrintable(QString::number(after)));
    QCOMPARE(p.currentPage(), 100);
}

void FomawriteTest::pdfModeFooterTitleAndMenusRestoreMarkdown() {
    Cycle103SettingsSnapshot settings;
    QSettings().setValue("workspace/formatBarVisible", true);
    Cycle160Fixtures f; QVERIFY(f.make());
    const QString markdown = f.path("Draft.md");
    QVERIFY(PdfFixtures::write(markdown, "# Draft\n"));
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.w.backend.open(QUrl::fromLocalFile(markdown)));
    p.w.layout->setProperty("layoutMode", 1);
    p.w.layout->setProperty("previewWidth", 600);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    const Cycle161Menus menus{p.w.window.data()};
    QVERIFY(menus.native() || menus.windowsBar());
    const QStringList footerNames{"singleModeButton", "previewSplitButton", "previewOnlyButton"};
    const QStringList chrome{"documentPublishingControls", "previewTemplateButton", "previewHeaderModeLabel",
                             "previewZoomControls", "previewHeaderExportButton", "formatBar", "sourceAppearanceButton"};
    // What the Markdown interface looks like, to compare after PDF mode.
    const auto snapshot = [&] {
        QStringList state;
        for (const QString &name : footerNames) {
            auto *button = p.w.item(name.toUtf8().constData());
            state << name + "=" + button->property("text").toString() + "|" + button->property("hint").toString();
        }
        for (const QString &name : chrome) {
            auto *item = p.w.item(name.toUtf8().constData());
            state << name + (item ? (item->isVisible() ? "+" : "-") + item->property("text").toString() : QStringLiteral("?"));
        }
        state << "title=" + p.w.window->property("title").toString();
        state << QStringLiteral("pdfMenu=") + (menus.pdfMenuShown() ? "true" : "false");
        return state;
    };
    QTest::qWait(50);
    const QStringList markdownState = snapshot();
    QVERIFY2(markdownState.contains("singleModeButton=Single|Single — show the chosen editor alone"), qPrintable(markdownState.join("\n")));
    QVERIFY(markdownState.contains("previewOnlyButton=Preview|Preview — read the published output on its own"));
    QVERIFY(markdownState.contains("documentPublishingControls+"));
    QVERIFY(markdownState.contains("previewHeaderModeLabel+Preview"));
    QVERIFY(markdownState.contains("title=Draft.md - Fomawrite"));
    QVERIFY2(markdownState.contains("pdfMenu=false"), qPrintable(markdownState.join("\n")));
    QVERIFY2(markdownState.contains("formatBar+"), qPrintable(markdownState.join("\n")));

    // PDF mode: Notes / Split / PDF, no publishing controls, the PDF's name, the PDF menu.
    QVERIFY(p.open(f.url("geometry.pdf")));
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    auto *single = p.w.item("singleModeButton"), *split = p.w.item("previewSplitButton"), *only = p.w.item("previewOnlyButton");
    QCOMPARE(single->property("text").toString(), QStringLiteral("Notes"));
    QCOMPARE(single->property("hint").toString(), QStringLiteral("Notes — show the PDF's notes alone"));
    QCOMPARE(split->property("text").toString(), QStringLiteral("Split"));
    QCOMPARE(split->property("hint").toString(), QStringLiteral("Split — show the notes beside the PDF"));
    QCOMPARE(only->property("text").toString(), QStringLiteral("PDF"));
    QCOMPARE(only->property("hint").toString(), QStringLiteral("PDF — read the PDF on its own"));
    QTRY_VERIFY(!p.w.item("documentPublishingControls")->isVisible());
    QVERIFY(!p.w.item("previewTemplateButton")->isVisible());
    QTRY_VERIFY(p.w.item("sourceAppearanceButton")->isVisible());
    QTRY_VERIFY(p.w.item("formatBar")->isVisible());
    QCOMPARE(p.w.item("previewHeaderModeLabel")->property("text").toString(), QStringLiteral("PDF"));
    QVERIFY(!p.w.item("previewZoomControls")->isVisible());
    QVERIFY(!p.w.item("previewHeaderExportButton")->isVisible());
    QCOMPARE(p.w.item("previewDocumentName")->property("text").toString(), QStringLiteral("geometry.pdf"));
    QCOMPARE(p.w.window->property("title").toString(), QStringLiteral("geometry.pdf - Fomawrite"));
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "insert", Q_ARG(int, 0), Q_ARG(QString, "Notes.\n")));
    QTRY_COMPARE(p.w.window->property("title").toString(), QStringLiteral("* geometry.pdf - Fomawrite"));
    p.w.backend.save();
    QTRY_COMPARE(p.w.window->property("title").toString(), QStringLiteral("geometry.pdf - Fomawrite"));
    QTRY_VERIFY(menus.pdfMenuShown());

    // The PDF menu's items drive the pane.
    const auto menuItem = [&](const char *command) { return menus.item(command); };
    for (const char *name : {"goToPage", "zoomIn", "zoomOut", "actualSize", "fitWidth", "thumbnails", "inspector", "find"}) {
        QVERIFY2(menuItem(name), name);
        QVERIFY2(menuItem(name)->property("enabled").toBool(), name);
    }
    // The Mac shortcut; the Windows menu gives the Inspector none (Ctrl+Alt is AltGr).
    if (menus.native()) QCOMPARE(menuItem("inspector")->property("shortcut").toString(), QStringLiteral("Ctrl+Alt+I"));
    QVERIFY(QMetaObject::invokeMethod(menuItem("actualSize"), "triggered"));
    QTRY_COMPARE(p.zoom(), 1.0);
    QVERIFY(QMetaObject::invokeMethod(menuItem("zoomIn"), "triggered"));
    QTRY_COMPARE(p.zoom(), 1.1);
    QVERIFY(QMetaObject::invokeMethod(menuItem("fitWidth"), "triggered"));
    QTRY_COMPARE(p.reader->property("zoomMode").toString(), QStringLiteral("fit"));
    QCOMPARE(menuItem("thumbnails")->property("text").toString(), QStringLiteral("Show Thumbnails"));
    QVERIFY(QMetaObject::invokeMethod(menuItem("thumbnails"), "triggered"));
    QTRY_VERIFY(p.item("pdfThumbnails")->isVisible());
    QCOMPARE(menuItem("thumbnails")->property("text").toString(), QStringLiteral("Hide Thumbnails"));
    QVERIFY(QMetaObject::invokeMethod(menuItem("thumbnails"), "triggered"));
    QTRY_VERIFY(!p.item("pdfThumbnails")->isVisible());
    QVERIFY(QMetaObject::invokeMethod(menuItem("goToPage"), "triggered"));
    QTRY_VERIFY(p.item("pdfPageField")->hasActiveFocus());
    QVERIFY(QMetaObject::invokeMethod(menuItem("inspector"), "triggered"));
    QTRY_VERIFY(p.object("pdfInspector")->property("opened").toBool());
    QMetaObject::invokeMethod(p.object("pdfInspector"), "close");
    QTRY_VERIFY(!p.object("pdfInspector")->property("opened").toBool());
    // From Notes only, Find in PDF brings the PDF back beside the notes.
    p.w.layout->setProperty("layoutMode", 0);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 0);
    QVERIFY(QMetaObject::invokeMethod(menuItem("find"), "triggered"));
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    QTRY_VERIFY(p.item("pdfFindBar")->isVisible());
    p.call("closeFind");

    // In PDF only, the header names the PDF and drops the publishing controls.
    p.w.layout->setProperty("layoutMode", 2);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);
    QTRY_COMPARE(p.w.item("fullPreviewHeaderModeLabel")->property("text").toString(), QStringLiteral("PDF"));
    QCOMPARE(p.w.item("fullPreviewDocumentName")->property("text").toString(), QStringLiteral("geometry.pdf"));
    QVERIFY(!p.w.item("exportHubButton")->isVisible());
    QVERIFY(!p.w.item("workspaceAppearanceButton")->isVisible());
    QVERIFY(!p.w.item("formatBar")->isVisible());

    // Leaving PDF mode restores the Markdown interface exactly.
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "requestOpen", Q_ARG(QVariant, QUrl::fromLocalFile(markdown))));
    QTRY_VERIFY(!p.w.backend.pdf()->active());
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    QTRY_VERIFY(!menus.pdfMenuShown());
    QTest::qWait(50);
    QCOMPARE(snapshot(), markdownState);
}
