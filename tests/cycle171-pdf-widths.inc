#ifdef Q_OS_MACOS
void FomawriteTest::pdfSplitFitsWiderSidebarsInTheSameWindow() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(1295), qPrintable(p.w.error));
    QVERIFY(cycle164Open(p, f, QStringLiteral("Itinerary sample"), 1));
    auto *library = p.w.window->findChild<QQuickItem *>("organizerSlot");
    auto *files = p.w.window->findChild<QQuickItem *>("filesSlot");
    auto *source = p.w.window->findChild<QQuickItem *>("editorPane");
    auto *preview = p.w.window->findChild<QQuickItem *>("pdfReaderSlot");
    QVERIFY(library && files && source && preview);
    p.w.layout->setProperty("organizerVisible", true);
    p.w.layout->setProperty("filesVisible", true);
    p.w.layout->setProperty("reviewVisible", false);
    const auto reset = [&] {
        p.w.layout->setProperty("organizerWidth", 184);
        p.w.layout->setProperty("fileWidth", 232);
        p.w.layout->setProperty("previewWidth", 420);
    };
    const auto drag = [&](QQuickItem *left, int dx) {
        const QPoint point = left->mapToScene(QPointF(left->width() + 2, 250)).toPoint();
        QTest::mousePress(p.w.quick, Qt::LeftButton, Qt::NoModifier, point);
        QTest::mouseMove(p.w.quick, point + QPoint(dx / 2, 0), 30);
        QTest::mouseMove(p.w.quick, point + QPoint(dx, 0), 30);
        QTest::mouseRelease(p.w.quick, Qt::LeftButton, Qt::NoModifier, point + QPoint(dx, 0));
    };
    reset();
    QTRY_COMPARE(library->width(), 184);
    QTRY_COMPARE(files->width(), 232);
    const int windowWidth = p.w.quick->width();
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    p.w.editor->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "openSearch", Q_ARG(QVariant, false), Q_ARG(QVariant, false)));
    auto *find = p.w.window->findChild<QQuickItem *>("searchField");
    QVERIFY(find && find->isVisible());
    find->setProperty("text", QStringLiteral("Itinerary"));
    drag(library, 172);
    QTRY_VERIFY(library->width() >= 350);
    QCOMPARE(p.w.quick->width(), windowWidth);
    QVERIFY(source->width() >= 360 && preview->width() >= 320);
    QCOMPARE(p.w.layout->property("organizerWidth").toReal(), library->width());
    reset();
    QTRY_COMPARE(library->width(), 184);
    QTRY_COMPARE(files->width(), 232);
    drag(files, 192);
    QTRY_VERIFY(files->width() >= 420);
    QCOMPARE(p.w.quick->width(), windowWidth);
    QVERIFY(source->width() >= 360 && preview->width() >= 320);
    QVERIFY(find->isVisible());
    QCOMPARE(find->property("text").toString(), QStringLiteral("Itinerary"));
    QCOMPARE(p.w.editor->property("text").toString(), QStringLiteral("Itinerary sample"));
}
#endif
