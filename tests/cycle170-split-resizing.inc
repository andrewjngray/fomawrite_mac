#ifdef Q_OS_MACOS
void FomawriteTest::pdfSplitNavigationDividersBorrowRoomAtTheSourceMinimum() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(1295), qPrintable(p.w.error));
    QVERIFY(cycle164Open(p, f, QStringLiteral("The itinerary is ready."), 1));
    auto *organizer = p.w.window->findChild<QQuickItem *>("organizerSlot");
    auto *files = p.w.window->findChild<QQuickItem *>("filesSlot");
    auto *source = p.w.window->findChild<QQuickItem *>("editorPane");
    auto *preview = p.w.window->findChild<QQuickItem *>("pdfReaderSlot");
    QVERIFY(organizer && files && source && preview);
    p.w.layout->setProperty("organizerVisible", true);
    p.w.layout->setProperty("filesVisible", true);
    p.w.layout->setProperty("reviewVisible", false);
    const auto reset = [&] {
        p.w.layout->setProperty("organizerWidth", 184);
        p.w.layout->setProperty("fileWidth", 232);
        p.w.layout->setProperty("previewWidth", 540);
        QCoreApplication::processEvents();
    };
    reset();
    QTRY_COMPARE(source->width(), 360);
    QVERIFY(preview->width() > 360);
    p.w.editor->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "openSearch", Q_ARG(QVariant, false), Q_ARG(QVariant, false)));
    auto *search = p.w.window->findChild<QQuickItem *>("searchField");
    QVERIFY(search && search->isVisible());
    search->setProperty("text", QStringLiteral("itinerary"));
    const auto drag = [&](QQuickItem *left, int delta) {
        const QPoint from = left->mapToScene(QPointF(left->width() + 2, 260)).toPoint();
        QTest::mousePress(p.w.quick, Qt::LeftButton, Qt::NoModifier, from);
        QTest::mouseMove(p.w.quick, from + QPoint(delta / 2, 0), 30);
        QTest::mouseMove(p.w.quick, from + QPoint(delta, 0), 30);
        QTest::mouseRelease(p.w.quick, Qt::LeftButton, Qt::NoModifier, from + QPoint(delta, 0));
    };
    const qreal previewBefore = preview->width();
    drag(organizer, 44);
    QTRY_VERIFY(organizer->width() >= 226);
    QTRY_VERIFY(preview->width() <= previewBefore - 42);
    QCOMPARE(source->width(), 360);
    drag(organizer, -32);
    QTRY_VERIFY(organizer->width() <= 198);
    reset();
    QTRY_COMPARE(files->width(), 232);
    QTRY_COMPARE(source->width(), 360);
    drag(files, 40);
    QTRY_VERIFY(files->width() >= 270);
    QTRY_VERIFY(preview->width() <= previewBefore - 38);
    QCOMPARE(source->width(), 360);
    reset();
    QTRY_COMPARE(source->width(), 360);
    drag(source, 32);
    QTRY_VERIFY(source->width() >= 390);
    QCOMPARE(search->property("text").toString(), QStringLiteral("itinerary"));
    QVERIFY(search->isVisible());
    QCOMPARE(p.w.editor->property("text").toString(), QStringLiteral("The itinerary is ready."));
}
#endif
