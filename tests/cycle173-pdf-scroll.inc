void FomawriteTest::publishingPdfWheelMovesProportionally() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString source = directory.filePath(QStringLiteral("Wheel.md"));
    QByteArray markdown("# Publishing scroll\n\n");
    for (int i = 0; i < 110; ++i)
        markdown += "A paragraph in the paginated preview, long enough to span several pages.\n\n";
    QVERIFY(PdfFixtures::write(source, markdown));
    Cycle117Window w; QVERIFY2(w.start(), qPrintable(w.error));
    QVERIFY(w.backend.open(QUrl::fromLocalFile(source)));
    QVERIFY(w.mode(2));
    QVERIFY(cycle102Click(w.quick, w.item("pdfPublishingButton")));
    auto *scroll = w.preview->findChild<QQuickItem *>("previewScroll");
    QVERIFY(scroll && scroll->isVisible() && scroll->height() > 0);
    QTRY_COMPARE_WITH_TIMEOUT(w.preview->property("displayedFormat").toString(), QStringLiteral("pdf"), 20000);
    QTRY_VERIFY(scroll->property("contentHeight").toReal() > scroll->height() * 2);

    const auto position = scroll->mapToScene(scroll->boundingRect().center());
    const auto contentY = [&] { return scroll->property("contentY").toReal(); };
    auto wheel = [&](int angle) {
        QWheelEvent event(position, w.quick->mapToGlobal(position.toPoint()), QPoint(), QPoint(0, angle),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(w.quick, &event);
    };

    const qreal detent = 40 * QGuiApplication::styleHints()->wheelScrollLines();
    QVERIFY(detent > 0);
    const qreal initial = contentY();
    wheel(-30);
    QTest::qWait(180);
    QVERIFY2(qAbs(contentY() - initial - detent / 4) < 1.0,
             qPrintable(QString("Quarter detent moved %1 px, expected %2").arg(contentY() - initial).arg(detent / 4)));
    for (int i = 0; i < 3; ++i) wheel(-30);
    QCOMPARE(contentY(), initial + detent);
    wheel(120);
    QCOMPARE(contentY(), initial);
    wheel(120);
    QCOMPARE(contentY(), 0.0);
}
