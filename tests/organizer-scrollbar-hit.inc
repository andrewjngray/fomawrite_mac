void FomawriteTest::organizerHeaderButtonsStayClickable_data() {
    QTest::addColumn<bool>("longList");
    QTest::newRow("short-organizer") << false;
    QTest::newRow("scrolling-organizer") << true;
}

void FomawriteTest::organizerHeaderButtonsStayClickable() {
    QFETCH(bool, longList);
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    if (longList) {
        auto *library = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(library);
        for (int i = 0; i < 40; ++i) {
            const QString folder = directory.filePath(QStringLiteral("Favorite %1").arg(i));
            QVERIFY(QDir().mkdir(folder));
            QVERIFY(library->addFavorite(QUrl::fromLocalFile(folder)));
        }
    }
    f.layout->setProperty("organizerWidth", 184);
    f.layout->setProperty("organizerVisible", true);
    auto *pane = f.window->findChild<QQuickItem *>("organizerPane"); QVERIFY(pane);
    QTRY_VERIFY(pane->isVisible());
    QQuickItem *add = nullptr, *favorite = nullptr, *vertical = nullptr;
    QObject *dialog = nullptr;
    for (auto *item : pane->findChildren<QQuickItem *>()) {
        if (item->property("hint").toString() == "Add library location") add = item;
        if (item->property("hint").toString() == "Add a favorite folder or document") favorite = item;
        if (QString::fromLatin1(item->metaObject()->className()).contains("ScrollBar")
                && item->property("orientation").toInt() == Qt::Vertical) vertical = item;
    }
    for (auto *object : pane->findChildren<QObject *>())
        if (object->property("title").toString() == "Add writing location") dialog = object;
    QVERIFY(add && favorite && vertical && dialog);
    const auto rejectDialog = qScopeGuard([&] { QMetaObject::invokeMethod(dialog, "reject"); });
    QTRY_VERIFY(longList ? vertical->property("size").toDouble() < 1.0
                        : vertical->property("size").toDouble() >= 1.0);
    QTest::qWait(100);
    const QPoint center = add->mapToScene(QPointF(add->width() / 2, add->height() / 2)).toPoint();
    QTest::mouseMove(f.quick, center);
    QTest::qWait(50);
    QVERIFY(vertical->property("interactive").toBool());
    // A normal pointer click must open the actual FolderDialog. Its fallback
    // is nonblocking under the suite's existing AA_DontUseNativeDialogs policy.
    QTest::mouseClick(f.quick, Qt::LeftButton, Qt::NoModifier, center);
    QTRY_VERIFY_WITH_TIMEOUT(dialog->property("visible").toBool(), 1500);
    QVERIFY(QMetaObject::invokeMethod(dialog, "reject"));
    QTRY_VERIFY(!dialog->property("visible").toBool());
    // The same reserved gutter must also protect the next header's menu action.
    QVERIFY(cycle102Click(f.quick, favorite));
    bool menuOpened = false;
    for (QObject *popup : cycle102Popups(f.quick)) {
        if (!popup->property("opened").toBool()) continue;
        menuOpened = true;
        QVERIFY(QMetaObject::invokeMethod(popup, "close"));
    }
    QVERIFY(menuOpened);
    // Protect the whole right edge, not just an unobstructed center pixel.
    QVERIFY(cycle103SceneBounds(add).right() <= cycle103SceneBounds(vertical).left() + 0.1);
    QVERIFY(cycle103SceneBounds(favorite).right() <= cycle103SceneBounds(vertical).left() + 0.1);
}
