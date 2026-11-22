void FomawriteTest::publishingPdfRightClickOffersLinkedFileActions() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("Linked report.txt"));
    const QString copy = directory.filePath(QStringLiteral("Saved copy.txt"));
    const QByteArray payload = QStringLiteral("Café\r\nlinked contents\r\n").toUtf8();
    QVERIFY(PdfFixtures::write(path, payload));
    const QString source = directory.filePath(QStringLiteral("Index.md"));
    QVERIFY(PdfFixtures::write(source, "# Index\n\n[Read the report](Linked%20report.txt)\n"));
    Cycle117Window w; QVERIFY2(w.start(), qPrintable(w.error));
    QVERIFY(w.backend.open(QUrl::fromLocalFile(source)));
    w.layout->setProperty("layoutMode", 2);
    const auto findLink = [&]() -> QQuickItem * {
        std::function<QQuickItem *(QQuickItem *)> search = [&](QQuickItem *item) -> QQuickItem * {
            if (item->isVisible() && item->property("url").toUrl().toString().contains("Linked"))
                return item;
            for (auto *child : item->childItems()) if (auto *found = search(child)) return found;
            return nullptr;
        };
        return search(w.preview);
    };
    QVERIFY(cycle102Click(w.quick, w.item("pdfPublishingButton")));
    QTRY_VERIFY_WITH_TIMEOUT(findLink(), 15000);
    auto *link = findLink();
    cycle162ContextClick(w.quick, link->mapToScene(link->boundingRect().center()).toPoint());
    QObject *menu = nullptr;
    QTRY_VERIFY((menu = w.preview->findChild<QObject *>("previewLinkMenu")));
    QTRY_VERIFY(menu->property("opened").toBool());
    auto *pathItem = menu->findChild<QObject *>("previewLinkCopyPath");
    auto *contents = menu->findChild<QObject *>("previewLinkCopyContents");
    auto *save = menu->findChild<QObject *>("previewLinkSaveAs");
    auto *open = menu->findChild<QObject *>("previewLinkOpen");
    auto *reveal = menu->findChild<QObject *>("previewLinkReveal");
    QVERIFY(pathItem && contents && save && open && reveal);
    QCOMPARE(open->property("text").toString(), QStringLiteral("Open File"));
    QVERIFY(open->property("enabled").toBool() && reveal->property("enabled").toBool());
    QVERIFY(pathItem->property("enabled").toBool() && contents->property("enabled").toBool());
    QVERIFY(cycle102Capture(w.quick, QStringLiteral("cycle172-pdf-preview-link-menu")));
    QVERIFY(QMetaObject::invokeMethod(pathItem, "click"));
    QCOMPARE(QGuiApplication::clipboard()->text(), path);
    cycle162ContextClick(w.quick, link->mapToScene(link->boundingRect().center()).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(contents, "click"));
    QCOMPARE(QGuiApplication::clipboard()->text().toUtf8(), payload);
    cycle162ContextClick(w.quick, link->mapToScene(link->boundingRect().center()).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(save, "click"));
    auto *dialog = w.window->findChild<QObject *>("previewLinkSaveDialog");
    QVERIFY(dialog);
    QTRY_VERIFY(dialog->property("visible").toBool());
    // The OS dialog owns the selection. Drive the shared copy operation with a disposable destination.
    QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
    bool result = false;
    QVERIFY(QMetaObject::invokeMethod(&w.backend, "previewLinkAction", Q_RETURN_ARG(bool, result),
        Q_ARG(QUrl, QUrl::fromLocalFile(path)), Q_ARG(QString, QStringLiteral("save")), Q_ARG(QUrl, QUrl::fromLocalFile(copy))));
    QVERIFY(result);
    QFile output(copy); QVERIFY(output.open(QIODevice::ReadOnly));
    QCOMPARE(output.readAll(), payload);
    const QByteArray binary = QByteArray::fromHex("00667479706d7034ff");
    const auto binaryUrl = QUrl::fromLocalFile(directory.filePath("sample.mp4"));
    const auto binaryCopy = QUrl::fromLocalFile(directory.filePath("copied.mp4"));
    QVERIFY(PdfFixtures::write(binaryUrl.toLocalFile(), binary));
    const auto binaryInfo = w.backend.previewLinkInfo(binaryUrl);
    QVERIFY(binaryInfo.value("file").toBool() && !binaryInfo.value("text").toBool());
    QVERIFY(w.backend.previewLinkAction(binaryUrl, "save", binaryCopy));
    QVERIFY(!w.backend.previewLinkAction(binaryUrl, "save", binaryUrl));
    QFile copiedVideo(binaryCopy.toLocalFile()); QVERIFY(copiedVideo.open(QIODevice::ReadOnly));
    QCOMPARE(copiedVideo.readAll(), binary);
    QCOMPARE(w.backend.fileUrl(), QUrl::fromLocalFile(source));
    QVERIFY(!w.backend.modified());
}

void FomawriteTest::publishingWebRightClickOffersBrowserAndCopyAddress() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString source = directory.filePath(QStringLiteral("Index.md"));
    QVERIFY(PdfFixtures::write(source, "# Index\n\n[Website](https://example.com/report?q=with%20spaces)\n"));
    Cycle117Window w; QVERIFY2(w.start(), qPrintable(w.error));
    QVERIFY(w.backend.open(QUrl::fromLocalFile(source)));
    w.layout->setProperty("layoutMode", 2);
    QVERIFY(cycle102Click(w.quick, w.item("webPublishingButton")));
    auto *web = w.preview->findChild<QQuickItem *>("renderedPreview");
    QVERIFY(web);
    QTRY_COMPARE_WITH_TIMEOUT(w.preview->property("displayedFormat").toString(), QStringLiteral("web"), 20000);
    QVariantList position;
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        position = cycle143PageResult(web, QStringLiteral(
            "(()=>{const a=document.querySelector('#write a'); if(!a)return []; "
            "const r=a.getBoundingClientRect();return [r.x+r.width/2,r.y+r.height/2]})()")).toList();
        return position.size() == 2;
    })(), 15000);
    const qreal zoom = web->property("zoomFactor").toReal();
    cycle162ContextClick(w.quick, web->mapToScene(QPointF(position[0].toReal() * zoom, position[1].toReal() * zoom)).toPoint());
    QObject *menu = nullptr;
    QTRY_VERIFY((menu = w.preview->findChild<QObject *>("previewLinkMenu")));
    QTRY_VERIFY(menu->property("opened").toBool());
    auto *open = menu->findChild<QObject *>("previewLinkOpen");
    auto *address = menu->findChild<QObject *>("previewLinkCopyPath");
    auto *save = menu->findChild<QObject *>("previewLinkSaveAs");
    QVERIFY(open && address && save);
    QCOMPARE(open->property("text").toString(), QStringLiteral("Open in Browser"));
    QCOMPARE(address->property("text").toString(), QStringLiteral("Copy Link Address"));
    QVERIFY(!save->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(address, "click"));
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("https://example.com/report?q=with%20spaces"));
    QCOMPARE(w.backend.fileUrl(), QUrl::fromLocalFile(source));
}
