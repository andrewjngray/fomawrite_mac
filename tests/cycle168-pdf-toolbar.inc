#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
void FomawriteTest::pdfToolbarRotatesCurrentAndSelectedPages() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    const QByteArray original = cycle165Marked(4);
    QVERIFY(PdfFixtures::write(f.path("rotate.pdf"), original));
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("rotate.pdf")));
    auto *rotate = p.item("pdfRotate");
    QVERIFY(rotate);
    p.call("goToPage", 2);
    QTRY_COMPARE(p.currentPage(), 2);
    QVERIFY(cycle102Click(p.w.quick, rotate));
    const auto turn = [&](const char *name) {
        auto *menu = p.object("pdfRotateMenu");
        return menu && QTest::qWaitFor([&] { return menu->property("opened").toBool(); }, 3000)
            && cycle102Click(p.w.quick, p.item(name));
    };
    QVERIFY(turn("pdfToolbarRotateLeft"));
    auto *pdf = p.w.backend.pdf();
    QCOMPARE(pdf->rotationOfWorkingPage(2), 270);
    QCOMPARE(pdf->rotationOfWorkingPage(0), 0);
    p.call("selectThumbnail", 0, int(Qt::NoModifier));
    p.call("selectThumbnail", 1, int(Qt::ShiftModifier));
    QVERIFY(cycle102Click(p.w.quick, rotate));
    QVERIFY(turn("pdfToolbarRotateRight"));
    QCOMPARE(pdf->rotationOfWorkingPage(0), 90);
    QCOMPARE(pdf->rotationOfWorkingPage(1), 90);
    QCOMPARE(pdf->rotationOfWorkingPage(2), 270);
    QCOMPARE(pdf->rotationOfWorkingPage(3), 0);
    QCOMPARE(cycle165Bytes(f.path("rotate.pdf")), original);
    QVERIFY(pdf->canUndoPageEdit());
    QVERIFY(cycle102Capture(p.w.quick, QStringLiteral("cycle168-pdf-toolbar")));
}

void FomawriteTest::pdfToolbarMakesTheActiveToolClear() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    auto *hint = p.item("pdfActiveToolHint");
    QVERIFY(hint);
    QVERIFY(!hint->isVisible());
    auto *pen = p.item("pdfHighlighter");
    auto *note = p.item("pdfNote");
    QVERIFY(cycle102Click(p.w.quick, pen));
    QTRY_VERIFY(hint->isVisible());
    QVERIFY(pen->property("checked").toBool());
    QVERIFY(hint->property("text").toString().contains("Highlight"));
    QVERIFY(cycle102Click(p.w.quick, note));
    QVERIFY(!pen->property("checked").toBool());
    QVERIFY(note->property("checked").toBool());
    QCOMPARE(hint->property("text").toString(), QStringLiteral("Click the page to add a note"));
    QVERIFY(cycle102Capture(p.w.quick, QStringLiteral("cycle168-pdf-note-armed")));
    cycle111KeyClick(p.w.quick, Qt::Key_Escape);
    QTRY_VERIFY(!hint->isVisible());
    QVERIFY(!note->property("checked").toBool());
    QVERIFY(!p.w.backend.modified());
}

void FomawriteTest::pdfFitPageShowsWholePageAndRemembersTheMode() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    QVERIFY(cycle102Click(p.w.quick, p.item("pdfZoomModes")));
    auto *fit = p.item("pdfFitPage");
    QVERIFY(fit);
    p.call("goToPage", 2);
    QVERIFY(cycle102Click(p.w.quick, fit));
    QTRY_COMPARE(p.reader->property("zoomMode").toString(), QStringLiteral("fitPage"));
    QTRY_VERIFY(p.page(2));
    QTRY_VERIFY(p.viewport().adjusted(-1, -1, 1, 1).contains(p.page(2)->mapRectToScene(p.page(2)->boundingRect())));
    const qreal fitted = p.zoom();
    p.w.quick->setHeight(p.w.quick->height() - 120);
    QTRY_VERIFY(p.zoom() < fitted);
    QTRY_VERIFY(p.page(2) && p.viewport().adjusted(-1, -1, 1, 1).contains(p.page(2)->mapRectToScene(p.page(2)->boundingRect())));
    p.call("rememberNow");
    QCOMPARE(p.w.backend.pdf()->rememberedView(f.url("geometry.pdf")).value("mode").toString(), QStringLiteral("fitPage"));
    QVERIFY(p.open(f.url("plain.pdf")));
    QVERIFY(p.open(f.url("geometry.pdf")));
    QTRY_COMPARE(p.reader->property("zoomMode").toString(), QStringLiteral("fitPage"));
    QTRY_COMPARE(p.currentPage(), 2);
    p.call("zoomIn");
    QTRY_COMPARE(p.reader->property("zoomMode").toString(), QStringLiteral("fixed"));
}
#endif
