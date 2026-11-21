namespace {

bool cycle166Command(Cycle161Pane &pane, const QString &command) {
    return QMetaObject::invokeMethod(pane.w.window.data(), "pdfCommand", Q_ARG(QVariant, command));
}

QObject *cycle166Dialog(Cycle161Pane &pane, const char *name) {
    auto *dialog = pane.w.window->findChild<QObject *>(name);
    if (dialog && QTest::qWaitFor([&] { return dialog->property("opened").toBool(); }, 3000)) return dialog;
    return nullptr;
}

}

void FomawriteTest::pdfPageControlsEditTheSelectedPagesAndSave() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f;
    QVERIFY(f.make());
    const QByteArray original = cycle165Marked(4);
    QVERIFY(PdfFixtures::write(f.path("edit.pdf"), original));
    QVERIFY(PdfFixtures::write(f.path("insert.pdf"), cycle165Marked(1, 5)));
    Cycle161Pane p;
    QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("edit.pdf")));
    auto *pdf = p.w.backend.pdf();
    const auto recovery = qScopeGuard([&] { QDir(p.w.backend.pdfRecoveryFolder()).removeRecursively(); });
    p.call("selectThumbnail", 0, int(Qt::NoModifier));
    p.call("selectThumbnail", 1, int(Qt::ShiftModifier));
    QVERIFY(cycle166Command(p, QStringLiteral("rotateRight")));
    QVERIFY(pdf->pagesModified());
    QCOMPARE(pdf->rotationOfWorkingPage(0), 90);
    QCOMPARE(pdf->rotationOfWorkingPage(1), 90);
    QCOMPARE(pdf->rotationOfWorkingPage(2), 0);
    QVERIFY(p.w.window->property("documentModified").toBool());
    QCOMPARE(cycle165Bytes(f.path("edit.pdf")), original);
    QVERIFY(cycle166Command(p, QStringLiteral("undoPages")));
    QVERIFY(!pdf->pagesModified());
    QVERIFY(cycle166Command(p, QStringLiteral("redoPages")));
    QVERIFY(pdf->pagesModified());

    p.call("selectThumbnail", 0, int(Qt::NoModifier));
    p.call("moveSelectedPages", 4);
    QCOMPARE(pdf->pageText(3).trimmed().left(5), QStringLiteral("PAGEA"));
    QVERIFY(cycle166Command(p, QStringLiteral("deletePages")));
    QCOMPARE(pdf->pageCount(), 3);
    p.call("insertPagesAt", f.url("insert.pdf"), 1);
    QCOMPARE(pdf->pageCount(), 4);
    QCOMPARE(pdf->pageText(1).trimmed().left(5), QStringLiteral("PAGEF"));

    QVERIFY(cycle166Command(p, QStringLiteral("exportPages")));
    auto *exportDialog = p.object("exportPdfPagesDialog");
    QVERIFY(exportDialog);
    QVERIFY(QMetaObject::invokeMethod(exportDialog, "close"));

    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "saveDocument"));
    QVERIFY2(!pdf->pagesModified(), qPrintable(p.w.backend.pdfSaveError()));
    QCOMPARE(cycle165Bytes(f.path("edit.pdf")), pdf->workingBytes());
    QVERIFY(!pdf->canUndoPageEdit());
    QVERIFY(!p.w.window->property("documentModified").toBool());
}

void FomawriteTest::pdfPageSavePromptsBeforeSwitchingAndReplacing() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f;
    QVERIFY(f.make());
    const QByteArray original = cycle165Marked(3);
    QVERIFY(PdfFixtures::write(f.path("edit.pdf"), original));
    Cycle161Pane p;
    QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("edit.pdf")));
    auto *pdf = p.w.backend.pdf();
    const auto recovery = qScopeGuard([&] { QDir(p.w.backend.pdfRecoveryFolder()).removeRecursively(); });
    QVERIFY(cycle166Command(p, QStringLiteral("rotateRight")));
    const QByteArray pending = pdf->workingBytes();
    auto *window = p.w.window.data();
    QVERIFY(QMetaObject::invokeMethod(window, "requestOpen", Q_ARG(QVariant, f.url("geometry.pdf"))));
    auto *prompt = cycle166Dialog(p, "unsavedChangesPrompt");
    QVERIFY(prompt && prompt->property("pdfPagesModified").toBool());
    QVERIFY(QMetaObject::invokeMethod(prompt, "reject"));
    QVERIFY(pdf->pagesModified());
    QCOMPARE(pdf->url(), f.url("edit.pdf"));

    const QByteArray external = cycle165Marked(2, 2);
    QVERIFY(PdfFixtures::write(f.path("edit.pdf"), external));
    QVERIFY(QMetaObject::invokeMethod(window, "saveDocument"));
    auto *overwrite = cycle166Dialog(p, "pdfOverwriteDialog");
    QVERIFY(overwrite);
    QVERIFY(QMetaObject::invokeMethod(overwrite, "reject"));
    QCOMPARE(cycle165Bytes(f.path("edit.pdf")), external);
    QVERIFY(pdf->pagesModified());

    QVERIFY(QMetaObject::invokeMethod(window, "saveDocument"));
    QVERIFY(cycle166Dialog(p, "pdfOverwriteDialog"));
    QVERIFY(QMetaObject::invokeMethod(overwrite, "accept"));
    QCOMPARE(cycle165Bytes(f.path("edit.pdf")), pending);
    QVERIFY(!pdf->pagesModified());

    QVERIFY(cycle166Command(p, QStringLiteral("rotateLeft")));
    QVERIFY(QMetaObject::invokeMethod(window, "requestOpen", Q_ARG(QVariant, f.url("geometry.pdf"))));
    QVERIFY(cycle166Dialog(p, "unsavedChangesPrompt"));
    QVERIFY(QMetaObject::invokeMethod(prompt, "close"));
    QVERIFY(QMetaObject::invokeMethod(prompt, "discardRequested"));
    QCOMPARE(pdf->url(), f.url("geometry.pdf"));
    QCOMPARE(cycle165Bytes(f.path("edit.pdf")), pending);
}

void FomawriteTest::pdfPageControlsAskBeforeChangingASignedPdf() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f;
    QVERIFY(f.make());
    const QByteArray signedPdf = cycle165Signed(true, "/SigFlags 3");
    QVERIFY(PdfFixtures::write(f.path("signed.pdf"), signedPdf));
    Cycle161Pane p;
    QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("signed.pdf")));
    auto *pdf = p.w.backend.pdf();
    QVERIFY(cycle166Command(p, QStringLiteral("rotateRight")));
    auto *confirmation = cycle166Dialog(p, "signedPdfPageEditDialog");
    QVERIFY(confirmation);
    QVERIFY(!pdf->pagesModified());
    QVERIFY(QMetaObject::invokeMethod(confirmation, "reject"));
    QVERIFY(!pdf->signedEditsAllowed());
    QVERIFY(cycle166Command(p, QStringLiteral("rotateRight")));
    QVERIFY(cycle166Dialog(p, "signedPdfPageEditDialog"));
    QVERIFY(QMetaObject::invokeMethod(confirmation, "accept"));
    QVERIFY(pdf->signedEditsAllowed());
    QVERIFY(pdf->pagesModified());
    QCOMPARE(cycle165Bytes(f.path("signed.pdf")), signedPdf);
}

void FomawriteTest::pdfThumbnailPointerDragReordersPages() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f;
    QVERIFY(f.make());
    QVERIFY(PdfFixtures::write(f.path("drag.pdf"), cycle165Marked(3)));
    Cycle161Pane p;
    QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("drag.pdf")));
    auto *pdf = p.w.backend.pdf();
    pdf->setThumbnailsVisible(true);
    auto *list = p.item("pdfThumbnailList");
    QVERIFY(list);
    const auto row = [&](int index) -> QQuickItem * {
        for (auto *thumb : cycle103VisualItems(list, "pdfThumbnail"))
            if (thumb->property("index").toInt() == index) return thumb;
        return nullptr;
    };
    QTRY_VERIFY(row(0) && row(1));
    const QPoint from = row(0)->mapToScene(QPointF(row(0)->width() / 2, row(0)->height() / 2)).toPoint();
    const QPoint to = row(1)->mapToScene(QPointF(row(1)->width() / 2, row(1)->height() * 0.8)).toPoint();
    QVERIFY(cycle103SceneBounds(list).contains(from));
    QVERIFY(cycle103SceneBounds(list).contains(to));
    QTest::mousePress(p.w.quick, Qt::LeftButton, Qt::NoModifier, from);
    QTest::mouseMove(p.w.quick, (from + to) / 2);
    QTest::mouseMove(p.w.quick, to);
    QTest::mouseRelease(p.w.quick, Qt::LeftButton, Qt::NoModifier, to);
    QTRY_COMPARE(pdf->pageText(0).trimmed().left(5), QStringLiteral("PAGEB"));
    QCOMPARE(pdf->pageText(1).trimmed().left(5), QStringLiteral("PAGEA"));
}
