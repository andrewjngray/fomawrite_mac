// Cycle155 — the Library header's "+ ▾" split button: "+" creates a document, the ▾ opens
// New File…, New Folder…, Sort By, View Options, Hide Sort Bar and Hide Filter Bar.

namespace {
QStringList cycle155MenuTexts(QObject *menu) {
    QStringList texts;
    const int count = menu->property("count").toInt();
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = nullptr;
        QMetaObject::invokeMethod(menu, "itemAt", Q_RETURN_ARG(QQuickItem *, item), Q_ARG(int, i));
        texts << (item ? item->property("text").toString() : QStringLiteral("<null>"));
    }
    return texts;
}
QQuickItem *cycle155MenuItem(QObject *menu, const QString &text) {
    const int count = menu->property("count").toInt();
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = nullptr;
        QMetaObject::invokeMethod(menu, "itemAt", Q_RETURN_ARG(QQuickItem *, item), Q_ARG(int, i));
        if (item && item->property("text").toString() == text) return item;
    }
    return nullptr;
}
}

void FomawriteTest::libraryNewSplitButtonFitsAtEveryWidthAndFoldsInOrder() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    const bool mac = f.window->property("isMac").toBool();
    auto *group = f.window->findChild<QQuickItem *>("libraryNewControls"); QVERIFY(group);
    auto *plus = f.window->findChild<QQuickItem *>("libraryNewButton"); QVERIFY(plus);
    auto *chevron = f.window->findChild<QQuickItem *>("libraryNewMenuButton"); QVERIFY(chevron);
    auto *arrows = f.window->findChild<QQuickItem *>("libraryHistoryControls"); QVERIFY(arrows);
    auto *folder = f.window->findChild<QQuickItem *>("workspaceFolderButton"); QVERIFY(folder);
    auto *collapse = f.window->findChild<QQuickItem *>("collapseFilesButton"); QVERIFY(collapse);
    QCOMPARE(plus->property("hint").toString(), QStringLiteral("New document"));
    QCOMPARE(plus->property("iconName").toString(), QStringLiteral("plus"));
    QCOMPARE(chevron->property("hint").toString(), QStringLiteral("New and view options"));
    QCOMPARE(chevron->property("iconName").toString(), QStringLiteral("down"));
    QVERIFY(f.window->findChild<QQuickItem *>("newDocumentButton") == nullptr);
    for (bool organizer : {true, false}) {
        f.layout->setProperty("organizerVisible", organizer); f.layout->setProperty("filesVisible", true);
        for (int width : {232, 288, 334, 420}) {
            f.layout->setProperty("fileWidth", width);
            QTRY_COMPARE(f.layout->property("effectiveFileWidth").toReal(), double(width));
            auto *header = f.window->findChild<QQuickItem *>("filesHeader");
            QTRY_COMPARE(header->width(), double(width));
            const int content = width - ((mac && !organizer) ? 84 : 12) - 12;
            const QByteArray where = QStringLiteral("organizer %1 width %2").arg(organizer).arg(width).toUtf8();
            QTRY_COMPARE_WITH_TIMEOUT(group->isVisible(), content >= 156, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(chevron->isVisible(), content >= 188, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(arrows->isVisible(), content >= 260, 3000);
            QVERIFY2(collapse->isVisible(), where);
            QVERIFY2(cycle151FilesHeaderFits(f.window.data()), where);
            if (group->isVisible()) {
                const QRectF box = cycle103SceneBounds(group).adjusted(-0.1, -0.1, 0.1, 0.1);
                QVERIFY2(box.contains(cycle103SceneBounds(plus)), where);
                if (chevron->isVisible()) {
                    QVERIFY2(box.contains(cycle103SceneBounds(chevron)), where);
                    QVERIFY2(cycle103SceneBounds(plus).right() <= cycle103SceneBounds(chevron).left() + 0.5, where);
                }
                QVERIFY2(folder->width() >= 76, where);
                // Order in the header: arrows, folder name, split button, hide-library.
                QVERIFY2(cycle103SceneBounds(folder).right() <= cycle103SceneBounds(group).left() + 0.5, where);
                QVERIFY2(cycle103SceneBounds(group).right() <= cycle103SceneBounds(collapse).left() + 0.5, where);
            }
            if (arrows->isVisible())
                QVERIFY2(cycle103SceneBounds(arrows).right() <= cycle103SceneBounds(folder).left() + 0.5, where);
        }
    }
    f.backend.discardRecovery();
}

void FomawriteTest::libraryNewMenuListsTheNewAndViewItemsInOrder() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    cycle148Make(directory.filePath("Note.md"));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    auto *library = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(library);
    library->setRootFolder(QUrl::fromLocalFile(directory.path()));
    f.layout->setProperty("organizerVisible", true); f.layout->setProperty("filesVisible", true);
    f.layout->setProperty("fileWidth", 288);
    QTRY_COMPARE(f.layout->property("effectiveFileWidth").toReal(), 288.0);
    auto *pane = f.window->findChild<QQuickItem *>("libraryPane"); QVERIFY(pane);
    pane->setProperty("showSortBar", true); pane->setProperty("showFilterBar", true);
    auto *chevron = f.window->findChild<QQuickItem *>("libraryNewMenuButton"); QVERIFY(chevron);
    QTRY_VERIFY(chevron->isVisible());
    auto *menu = f.window->findChild<QObject *>("libraryOptionsMenu"); QVERIFY(menu);
    f.quick->requestActivate(); QTRY_VERIFY(f.quick->isActive());
    QVERIFY(cycle102Click(f.quick, chevron));
    QTRY_VERIFY(menu->property("visible").toBool());
    // The menu opens under the control, inside the window.
    auto *menuItem = qobject_cast<QQuickItem *>(menu->property("contentItem").value<QObject *>()); QVERIFY(menuItem);
    const QRectF menuBox = menuItem->mapRectToScene(menuItem->boundingRect());
    QVERIFY(menuBox.left() >= -0.5 && menuBox.right() <= f.quick->width() + 0.5);
    QVERIFY(menuBox.top() >= cycle103SceneBounds(chevron).bottom() - 0.5);
    const QStringList texts = cycle155MenuTexts(menu);
    const QStringList expected{QStringLiteral("New File…"), QStringLiteral("New Folder…"), QString(),
                               QStringLiteral("Sort By"), QStringLiteral("View Options"), QString(),
                               QStringLiteral("Hide Sort Bar"), QStringLiteral("Hide Filter Bar"), QString(),
                               QStringLiteral("Choose Folder…"), QStringLiteral("Refresh")};
    QCOMPARE(texts, expected);
    QVERIFY(cycle155MenuItem(menu, "New File…")->property("enabled").toBool());
    QVERIFY(cycle155MenuItem(menu, "New Folder…")->property("enabled").toBool());
    // The submenus hold the iA Writer groups.
    auto *sort = f.window->findChild<QObject *>("libraryOptionsSortMenu"); QVERIFY(sort);
    const QStringList sortTexts = cycle155MenuTexts(sort);
    QVERIFY(sortTexts.contains("Name") && sortTexts.contains("Date Modified") && sortTexts.contains("Extension"));
    auto *view = f.window->findChild<QObject *>("libraryOptionsViewMenu"); QVERIFY(view);
    const QStringList viewTexts = cycle155MenuTexts(view);
    QVERIFY(viewTexts.contains("Show Date") && viewTexts.contains("Show Text Excerpts") && viewTexts.contains("Navigation"));
    // The two bar items flip to "Show ..." when used.
    QVERIFY(QMetaObject::invokeMethod(cycle155MenuItem(menu, "Hide Sort Bar"), "triggered"));
    QTRY_VERIFY(!pane->property("showSortBar").toBool());
    QVERIFY(cycle155MenuItem(menu, "Show Sort Bar"));
    QVERIFY(QMetaObject::invokeMethod(cycle155MenuItem(menu, "Hide Filter Bar"), "triggered"));
    QTRY_VERIFY(!pane->property("showFilterBar").toBool());
    QVERIFY(cycle155MenuItem(menu, "Show Filter Bar"));
    QMetaObject::invokeMethod(menu, "close");
    f.backend.discardRecovery();
}

void FomawriteTest::libraryPlusCreatesADocumentAndNewFolderOpensTheFolderDialog() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    cycle148Make(directory.filePath("Existing.md"));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    auto *library = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(library);
    library->setRootFolder(QUrl::fromLocalFile(directory.path()));
    f.layout->setProperty("organizerVisible", true); f.layout->setProperty("filesVisible", true);
    f.layout->setProperty("fileWidth", 288);
    QTRY_COMPARE(f.layout->property("effectiveFileWidth").toReal(), 288.0);
    auto *plus = f.window->findChild<QQuickItem *>("libraryNewButton"); QVERIFY(plus);
    auto *chevron = f.window->findChild<QQuickItem *>("libraryNewMenuButton"); QVERIFY(chevron);
    QTRY_VERIFY(plus->isVisible() && chevron->isVisible());
    f.quick->requestActivate(); QTRY_VERIFY(f.quick->isActive());

    // "+" opens the new-document dialog; accepting it creates the file in the folder.
    auto *fileDialog = f.window->findChild<QObject *>("newLibraryFileDialog"); QVERIFY(fileDialog);
    auto *fileName = f.window->findChild<QObject *>("newLibraryFileName"); QVERIFY(fileName);
    QVERIFY(cycle102Click(f.quick, plus));
    QTRY_VERIFY(fileDialog->property("visible").toBool());
    QVERIFY(fileName->setProperty("text", QStringLiteral("Fresh.md")));
    QVERIFY(QMetaObject::invokeMethod(fileDialog, "accept"));
    QTRY_VERIFY(!fileDialog->property("visible").toBool());
    QTRY_VERIFY(QFileInfo::exists(directory.filePath("Fresh.md")));
    QTRY_COMPARE(f.backend.fileName(), QStringLiteral("Fresh.md"));

    // "▾ > New Folder…" opens the folder dialog; accepting it creates the folder.
    auto *menu = f.window->findChild<QObject *>("libraryOptionsMenu"); QVERIFY(menu);
    auto *folderDialog = f.window->findChild<QObject *>("newLibraryFolderDialog"); QVERIFY(folderDialog);
    auto *folderName = f.window->findChild<QObject *>("newLibraryFolderName"); QVERIFY(folderName);
    QVERIFY(cycle102Click(f.quick, chevron));
    QTRY_VERIFY(menu->property("visible").toBool());
    auto *newFolder = cycle155MenuItem(menu, "New Folder…"); QVERIFY(newFolder);
    QVERIFY(QMetaObject::invokeMethod(newFolder, "triggered"));
    QTRY_VERIFY(folderDialog->property("visible").toBool());
    QVERIFY(folderName->setProperty("text", QStringLiteral("Chapters")));
    QVERIFY(QMetaObject::invokeMethod(folderDialog, "accept"));
    QTRY_VERIFY(!folderDialog->property("visible").toBool());
    QTRY_VERIFY(QFileInfo(directory.filePath("Chapters")).isDir());

    // "▾ > New File…" opens the same document dialog as "+".
    QVERIFY(cycle102Click(f.quick, chevron));
    QTRY_VERIFY(menu->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(cycle155MenuItem(menu, "New File…"), "triggered"));
    QTRY_VERIFY(fileDialog->property("visible").toBool());
    QMetaObject::invokeMethod(fileDialog, "reject");
    QTRY_VERIFY(!fileDialog->property("visible").toBool());
    f.backend.discardRecovery();
}

void FomawriteTest::libraryNewMenuItemsAreDisabledWithoutAFolder() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    auto *library = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(library);
    library->setRootFolder(QUrl());
    QCOMPARE(library->rootFolder().toString(), QString());
    auto *menu = f.window->findChild<QObject *>("libraryOptionsMenu"); QVERIFY(menu);
    QVERIFY(!cycle155MenuItem(menu, "New File…")->property("enabled").toBool());
    QVERIFY(!cycle155MenuItem(menu, "New Folder…")->property("enabled").toBool());
    QVERIFY(cycle155MenuItem(menu, "Choose Folder…")->property("enabled").toBool());
    f.backend.discardRecovery();
}
