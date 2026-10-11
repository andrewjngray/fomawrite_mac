// Cycle148 — document navigation: the top bar's document name shows the full path
// and opens the Library context menu, and "Explore Folder" makes a file's own
// folder the middle pane's current folder.

static QUrl cycle148Make(const QString &path, const QByteArray &text = "text") {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(text) >= 0 ? QUrl::fromLocalFile(path) : QUrl();
}

static QUrl cycle148Canonical(const QString &path) { return QUrl::fromLocalFile(QFileInfo(path).canonicalFilePath()); }

void FomawriteTest::exploreFolderMakesTheFilesFolderCurrentInTreeAndListModes() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir top, other;
    QVERIFY(top.isValid() && other.isValid());
    const QUrl deepFile = cycle148Make(top.filePath("nested/deep/Note.md"));
    cycle148Make(top.filePath("nested/deep/Sibling.md"));
    cycle148Make(top.filePath("Top.md"));
    const QUrl outsideFile = cycle148Make(other.filePath("Elsewhere/Other.md"));
    QVERIFY(!deepFile.isEmpty() && !outsideFile.isEmpty());
    for (int mode : {0, 1}) {
        FileLibrary library;
        library.setNavigationMode(mode);
        library.setRootFolder(QUrl::fromLocalFile(top.path()));
        if (mode == 0) { // Tree: expand down to the file so it is already visible deeper in the tree.
            library.revealFile(deepFile);
            bool visibleDeeper = false;
            for (const auto &entry : library.entries()) visibleDeeper = visibleDeeper || entry.toMap().value("name").toString() == QStringLiteral("Note.md");
            QVERIFY(visibleDeeper);
        }
        library.setFilter(QStringLiteral("does-not-match"));
        const int row = library.exploreFolder(deepFile);
        QVERIFY2(row >= 0, qPrintable(library.error()));
        QCOMPARE(library.rootFolder(), cycle148Canonical(top.filePath("nested/deep")));
        QVERIFY(library.filter().isEmpty());
        QCOMPARE(library.entries().size(), 2);
        QCOMPARE(library.entries().at(row).toMap().value("name").toString(), QStringLiteral("Note.md"));
        QCOMPARE(library.entries().at(row).toMap().value("url").toUrl(), cycle148Canonical(deepFile.toLocalFile()));
        // Exploring again from the folder itself is stable.
        QCOMPARE(library.exploreFolder(deepFile), row);
        // A file outside every folder the library has shown: its parent becomes current.
        const int outsideRow = library.exploreFolder(outsideFile);
        QVERIFY2(outsideRow >= 0, qPrintable(library.error()));
        QCOMPARE(library.rootFolder(), cycle148Canonical(other.filePath("Elsewhere")));
        QCOMPARE(library.entries().at(outsideRow).toMap().value("name").toString(), QStringLiteral("Other.md"));
    }
}

void FomawriteTest::exploreFolderInsideALocationAddsNoLocation() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir top;
    QVERIFY(top.isValid());
    const QUrl deepFile = cycle148Make(top.filePath("nested/deep/Note.md"));
    QVERIFY(!deepFile.isEmpty());
    FileLibrary library;
    library.setRootFolder(QUrl::fromLocalFile(top.path()));
    const int locations = library.locations().size();
    QVERIFY(locations >= 1);
    QVERIFY(library.exploreFolder(deepFile) >= 0);
    QCOMPARE(library.locations().size(), locations);
    QCOMPARE(library.rootFolder(), cycle148Canonical(top.filePath("nested/deep")));
}

void FomawriteTest::exploreFolderRefusesMissingRemoteFolderAndNonTextTargets() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    cycle148Make(directory.filePath("Real.md"));
    cycle148Make(directory.filePath("picture.png"));
    FileLibrary library;
    library.setRootFolder(QUrl::fromLocalFile(directory.path()));
    const QUrl root = library.rootFolder();
    QCOMPARE(library.exploreFolder(QUrl::fromLocalFile(directory.filePath("missing.md"))), -1);
    QVERIFY(!library.error().isEmpty());
    QCOMPARE(library.exploreFolder(QUrl(QStringLiteral("https://example.com/note.md"))), -1);
    QCOMPARE(library.exploreFolder(QUrl::fromLocalFile(directory.path())), -1);
    QCOMPARE(library.exploreFolder(QUrl::fromLocalFile(directory.filePath("picture.png"))), -1);
    QVERIFY(library.error().contains(QStringLiteral("only Markdown and text files")));
    // A hidden file fails before the folder changes, with its own reason.
    cycle148Make(directory.filePath("elsewhere/.hidden.md"));
    QCOMPARE(library.exploreFolder(QUrl::fromLocalFile(directory.filePath("elsewhere/.hidden.md"))), -1);
    QVERIFY(library.error().contains(QStringLiteral("hidden files")));
    QCOMPARE(library.rootFolder(), root);
}

void FomawriteTest::backendFilePathIsEmptyUntilADocumentIsOpened() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("Some folder/Doc.md");
    QVERIFY(!cycle148Make(path).isEmpty());
    Backend backend;
    QVERIFY(backend.filePath().isEmpty());
    QSignalSpy changed(&backend, &Backend::fileUrlChanged);
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    QVERIFY(changed.count() >= 1);
    QCOMPARE(backend.filePath(), QDir::toNativeSeparators(backend.fileUrl().toLocalFile()));
    QCOMPARE(QFileInfo(backend.filePath()).canonicalFilePath(), QFileInfo(path).canonicalFilePath());
    QVERIFY(backend.filePath().endsWith(QStringLiteral("Doc.md")));
    QCOMPARE(backend.property("filePath").toString(), backend.filePath());
    backend.discardRecovery();
}

// The top bar: the full path in the tooltip, the Library menu on a right-click,
// and Explore Folder moving the middle pane. Depends on the Cycle 148 QML.
void FomawriteTest::topBarDocumentNameShowsThePathAndOpensTheLibraryMenu() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("ops/nested/CATCH-UP.md");
    QVERIFY(!cycle148Make(path, "# Catch up\n").isEmpty());
    cycle148Make(directory.filePath("ops/nested/Aardvark.md"));
    cycle148Make(directory.filePath("ops/Other.md"));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    auto *identity = f.item("previewDocumentIdentity");
    auto *topChrome = f.window->findChild<QObject *>("topChrome");
    QVERIFY(identity && topChrome);
    // Untitled: the short name, and no menu.
    QVERIFY(f.mode(1)); QTRY_VERIFY(identity->isVisible());
    QCOMPARE(identity->property("tooltipText").toString(), QStringLiteral("Untitled.md"));
    auto *menu = f.window->findChild<QObject *>("documentContextMenu"); QVERIFY(menu);
    QVERIFY(QMetaObject::invokeMethod(topChrome, "documentMenuRequested", Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject *>(identity))), Q_ARG(QPointF, QPointF(4, 4))));
    QTest::qWait(150);
    QVERIFY(!menu->property("opened").toBool());

    // Opened from disk: the full native path, " — Edited" once modified.
    auto *library = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(library);
    library->setNavigationMode(0); library->setRootFolder(QUrl::fromLocalFile(directory.path()));
    QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    const QString native = f.backend.filePath();
    QVERIFY(native.endsWith(QDir::toNativeSeparators(QStringLiteral("ops/nested/CATCH-UP.md"))));
    QTRY_COMPARE(identity->property("tooltipText").toString(), native);
    QCOMPARE(f.window->findChild<QQuickItem *>("previewDocumentName")->property("text").toString(), QStringLiteral("CATCH-UP.md"));
    QVERIFY(QMetaObject::invokeMethod(f.editor, "insert", Q_ARG(int, 0), Q_ARG(QString, "Unsaved. ")));
    QTRY_VERIFY(f.backend.modified());
    QTRY_COMPARE(identity->property("tooltipText").toString(), native + QStringLiteral(" — Edited"));
    f.backend.save(); QTRY_VERIFY(!f.backend.modified());
    QTRY_COMPARE(identity->property("tooltipText").toString(), native);

    // The document menu is the Library file menu, with Explore Folder under the Open items.
    QVERIFY(QMetaObject::invokeMethod(topChrome, "documentMenuRequested", Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject *>(identity))), Q_ARG(QPointF, QPointF(4, 4))));
    QTRY_VERIFY(menu->property("opened").toBool());
    auto *explore = menu->findChild<QObject *>("exploreFolderItem"); QVERIFY(explore);
    QVERIFY(explore->property("visible").toBool());
    QVERIFY(explore->property("enabled").toBool());
    QCOMPARE(explore->property("text").toString(), QStringLiteral("Explore Folder"));
    QVERIFY(QMetaObject::invokeMethod(menu, "close"));
    QTRY_VERIFY(!menu->property("opened").toBool());

    // Explore Folder from the menu item: the file's folder becomes current, the Library
    // column shows, and the file's row (not the first row) is selected.
    library->setSortMode(0); library->setAscending(true);
    f.layout->setProperty("filesVisible", false);
    auto *list = f.window->findChild<QQuickItem *>("libraryFiles"); QVERIFY(list);
    QVERIFY(QMetaObject::invokeMethod(topChrome, "documentMenuRequested", Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject *>(identity))), Q_ARG(QPointF, QPointF(4, 4))));
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(explore, "click"));
    QTRY_COMPARE(library->rootFolder(), cycle148Canonical(directory.filePath("ops/nested")));
    QVERIFY(library->filter().isEmpty());
    QVERIFY(f.layout->property("filesVisible").toBool());
    int row = -1;
    for (int i = 0; i < library->entries().size(); ++i)
        if (library->entries().at(i).toMap().value("name").toString() == QStringLiteral("CATCH-UP.md")) row = i;
    QCOMPARE(row, 1); // Aardvark.md sorts first
    QTRY_COMPARE(list->property("currentIndex").toInt(), row);
    auto currentRow = [&] { return list->property("currentItem").value<QQuickItem *>(); };
    QTRY_VERIFY(currentRow() && currentRow()->property("highlighted").toBool());

    // Exploring a file that is not the open document selects that file's row instead.
    QVariant index;
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "exploreFolderOf", Q_RETURN_ARG(QVariant, index), Q_ARG(QVariant, QUrl::fromLocalFile(directory.filePath("ops/Other.md")))));
    QVERIFY(index.toInt() >= 0);
    QCOMPARE(library->rootFolder(), cycle148Canonical(directory.filePath("ops")));
    QTRY_COMPARE(list->property("currentIndex").toInt(), index.toInt());
    QTRY_VERIFY(currentRow() && currentRow()->property("highlighted").toBool());
    QCOMPARE(currentRow()->property("modelData").toMap().value("name").toString(), QStringLiteral("Other.md"));
    int highlightedRows = 0;
    auto *content = list->property("contentItem").value<QQuickItem *>(); QVERIFY(content);
    for (auto *row : content->childItems()) if (row->objectName() == QLatin1String("libraryEntry") && row->isVisible() && row->property("highlighted").toBool()) ++highlightedRows;
    QCOMPARE(highlightedRows, 1);

    // A window too narrow to dock the Library opens the Files drawer instead.
    f.layout->setProperty("filesVisible", false);
    f.quick->resize(480, 720);
    // The window geometry notifies asynchronously offscreen; wait until the layout has the new width.
    QTRY_COMPARE(f.layout->property("availableWidth").toReal(), 477.0);
    auto *drawer = f.window->findChild<QObject *>("workspaceNavigationDrawer"); QVERIFY(drawer);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "exploreFolderOf", Q_RETURN_ARG(QVariant, index), Q_ARG(QVariant, f.backend.fileUrl())));
    QVERIFY(index.toInt() >= 0);
    QTRY_VERIFY(drawer->property("opened").toBool() || drawer->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(drawer, "close"));
    QTRY_VERIFY(!drawer->property("visible").toBool());
    f.quick->resize(1400, 720);

    // A folder target never offers Explore Folder.
    const QVariantMap folderEntry{{"url", QUrl::fromLocalFile(directory.filePath("ops"))}, {"name", "ops"}, {"directory", true}};
    QVERIFY(QMetaObject::invokeMethod(menu, "showFor", Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject *>(identity))), Q_ARG(QVariant, folderEntry), Q_ARG(QVariant, false), Q_ARG(QVariant, QPointF(4, 4))));
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(!explore->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(menu, "close"));
    QTRY_VERIFY(!menu->property("opened").toBool());
    f.backend.discardRecovery();
}

// A real right-click on the document name in the top bar opens the menu (and only
// for a saved document). Depends on the Cycle 148 QML.
void FomawriteTest::rightClickingTheTopBarDocumentNameOpensTheLibraryMenu() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("Right click.md");
    QVERIFY(!cycle148Make(path, "# Right\n").isEmpty());
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    auto *identity = f.item("previewDocumentIdentity"); QVERIFY(identity);
    auto *menu = f.window->findChild<QObject *>("documentContextMenu"); QVERIFY(menu);
    QVERIFY(f.mode(1)); QTRY_VERIFY(identity->isVisible());
    auto *name = f.item("previewDocumentName"); QVERIFY(name);
    const auto rightClick = [&] {
        const QPointF centre = name->mapToScene(QPointF(name->width() / 2, name->height() / 2));
        QTest::mouseClick(f.quick, Qt::RightButton, Qt::NoModifier, centre.toPoint());
    };
    rightClick(); QTest::qWait(250);
    QVERIFY(!menu->property("opened").toBool()); // untitled
    QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    QTRY_COMPARE(identity->property("tooltipText").toString(), f.backend.filePath());
    // Empty header space to the right of the name does not open the menu.
    QTRY_VERIFY(identity->width() > name->mapToItem(identity, QPointF(name->width(), 0)).x() + 40);
    QTest::mouseClick(f.quick, Qt::RightButton, Qt::NoModifier, identity->mapToScene(QPointF(identity->width() - 10, identity->height() / 2)).toPoint());
    QTest::qWait(250);
    QVERIFY(!menu->property("opened").toBool());
    rightClick();
    QTRY_VERIFY(menu->property("opened").toBool());
    auto *explore = menu->findChild<QObject *>("exploreFolderItem"); QVERIFY(explore);
    QVERIFY(explore->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(menu, "close"));
    f.backend.discardRecovery();
}

// The folder button above the middle pane opened its chooser at the top of the
// disk (build 148), then one level up (build 149). It now opens inside the
// current folder.
void FomawriteTest::folderChooserStartsWhereTheCurrentFolderLives() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    cycle148Make(directory.filePath("repo/ops/BOARD.md"));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    auto *library = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(library);
    library->setRootFolder(QUrl::fromLocalFile(directory.filePath("repo/ops")));
    const QUrl ops = cycle148Canonical(directory.filePath("repo/ops"));
    QCOMPARE(library->rootFolder(), ops);
    auto *pane = f.window->findChild<QObject *>("libraryPane"); QVERIFY(pane);
    auto *dialog = f.window->findChild<QObject *>("libraryFolderDialog"); QVERIFY(dialog);
    QVERIFY(QMetaObject::invokeMethod(pane, "chooseFolder"));
    QCOMPARE(dialog->property("currentFolder").toUrl().toLocalFile(), ops.toLocalFile());
    QVERIFY(QMetaObject::invokeMethod(dialog, "reject"));
    QCOMPARE(library->rootFolder(), ops);
    f.backend.discardRecovery();
}
