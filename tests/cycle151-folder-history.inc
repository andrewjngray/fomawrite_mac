// Cycle151 — folder history in the middle (Library) pane's header: Back / Forward
// arrows before the folder name, and Cmd+Up for the enclosing folder while focus is
// in the middle pane.

namespace {
// Every control in the middle pane's header sits inside it and none overlap.
bool cycle151FilesHeaderFits(QObject *window) {
    auto *header = window->findChild<QQuickItem *>("filesHeader");
    if (!header || !header->isVisible()) return false;
    const QRectF box = cycle103SceneBounds(header).adjusted(-0.1, -0.1, 0.1, 0.1);
    QList<QRectF> seen;
    for (const char *name : {"libraryHistoryControls", "workspaceFolderButton", "newDocumentButton", "collapseFilesButton"}) {
        auto *item = window->findChild<QQuickItem *>(name);
        if (!item || !item->isVisible()) continue;
        const QRectF bounds = cycle103SceneBounds(item);
        if (bounds.width() < 1 || !box.contains(bounds)) { qWarning() << name << "outside the header" << bounds << box; return false; }
        for (const auto &other : seen)
            if (bounds.intersected(other).width() > 0.1) { qWarning() << name << "overlaps" << bounds << other; return false; }
        seen.append(bounds);
    }
    return true;
}
}

void FomawriteTest::libraryHeaderHasFolderBackAndForwardArrows() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    for (const char *name : {"one", "two", "three"}) cycle148Make(directory.filePath(QStringLiteral("%1/Note.md").arg(QLatin1String(name))));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    auto *library = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(library);
    f.layout->setProperty("organizerVisible", true); f.layout->setProperty("filesVisible", true);
    f.layout->setProperty("fileWidth", 288); // the reference layout uses 232, too narrow for the arrows
    QTRY_COMPARE(f.layout->property("effectiveFileWidth").toReal(), 288.0);
    auto *back = f.window->findChild<QQuickItem *>("libraryBackButton");
    auto *forward = f.window->findChild<QQuickItem *>("libraryForwardButton");
    auto *folder = f.window->findChild<QQuickItem *>("workspaceFolderButton");
    QVERIFY(back && forward && folder);
    QTRY_VERIFY(back->isVisible() && forward->isVisible() && folder->isVisible());
    // Before the folder button, in the same header.
    QVERIFY(cycle103SceneBounds(back).right() <= cycle103SceneBounds(forward).left() + 0.5);
    QVERIFY(cycle103SceneBounds(forward).right() <= cycle103SceneBounds(folder).left() + 0.5);
    QCOMPARE(back->property("hint").toString(), QStringLiteral("Previous folder"));
    QCOMPARE(forward->property("hint").toString(), QStringLiteral("Next folder"));

    // No history yet: both arrows are disabled, whatever the starting folder.
    QVERIFY(!library->canGoBack() && !library->canGoForward());
    QVERIFY(!back->isEnabled() && !forward->isEnabled());

    const QUrl two = cycle148Canonical(directory.filePath("two")), three = cycle148Canonical(directory.filePath("three"));
    library->setRootFolder(QUrl::fromLocalFile(directory.filePath("one")));
    library->setRootFolder(QUrl::fromLocalFile(directory.filePath("two")));
    library->setRootFolder(QUrl::fromLocalFile(directory.filePath("three")));
    QCOMPARE(library->rootFolder(), three);
    QTRY_VERIFY(back->isEnabled());
    QVERIFY(!forward->isEnabled());

    // A real click on Back returns to the previous folder; Forward comes back.
    QVERIFY(cycle102Click(f.quick, back));
    QTRY_COMPARE(library->rootFolder(), two);
    QTRY_VERIFY(forward->isEnabled());
    QVERIFY(back->isEnabled());
    QVERIFY(cycle102Click(f.quick, forward));
    QTRY_COMPARE(library->rootFolder(), three);
    QTRY_VERIFY(!forward->isEnabled());
    f.backend.discardRecovery();
}

void FomawriteTest::exploreFolderCanBeUndoneWithTheBackArrow() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    cycle148Make(directory.filePath("top/Top.md"));
    const QUrl deepFile = cycle148Make(directory.filePath("top/nested/deep/Note.md"));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    auto *library = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(library);
    f.layout->setProperty("organizerVisible", true); f.layout->setProperty("filesVisible", true);
    f.layout->setProperty("fileWidth", 288);
    QTRY_COMPARE(f.layout->property("effectiveFileWidth").toReal(), 288.0);
    library->setRootFolder(QUrl::fromLocalFile(directory.filePath("top")));
    const QUrl top = cycle148Canonical(directory.filePath("top"));
    QCOMPARE(library->rootFolder(), top);
    QVariant index;
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "exploreFolderOf", Q_RETURN_ARG(QVariant, index), Q_ARG(QVariant, deepFile)));
    QVERIFY(index.toInt() >= 0);
    QCOMPARE(library->rootFolder(), cycle148Canonical(directory.filePath("top/nested/deep")));
    auto *back = f.window->findChild<QQuickItem *>("libraryBackButton"); QVERIFY(back);
    QTRY_VERIFY(back->isVisible() && back->isEnabled());
    QVERIFY(cycle102Click(f.quick, back));
    QTRY_COMPARE(library->rootFolder(), top);
    f.backend.discardRecovery();
}

void FomawriteTest::cmdUpGoesToTheEnclosingFolderOnlyFromTheMiddlePane() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    cycle148Make(directory.filePath("top/nested/Inner.md"));
    cycle148Make(directory.filePath("top/Outer.md"));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    auto *library = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(library);
    library->setNavigationMode(1);
    f.layout->setProperty("organizerVisible", false); f.layout->setProperty("filesVisible", true);
    const QUrl top = cycle148Canonical(directory.filePath("top")), nested = cycle148Canonical(directory.filePath("top/nested"));
    library->setRootFolder(QUrl::fromLocalFile(directory.filePath("top/nested")));
    QCOMPARE(library->rootFolder(), nested);
    auto *pane = f.window->findChild<QQuickItem *>("libraryPane"); QVERIFY(pane);
    QQuickItem *row = nullptr;
    QTRY_VERIFY((row = cycle117Entry(pane, QStringLiteral("Inner.md"))) != nullptr);
    f.quick->requestActivate();
    QTRY_VERIFY(f.quick->isActive());

    // Focus in Source: Cmd+Up is the editor's "start of document" and the Library stays put.
    QVERIFY(QMetaObject::invokeMethod(f.editor, "insert", Q_ARG(int, 0), Q_ARG(QString, "one\ntwo\nthree\n")));
    f.editor->forceActiveFocus(); QTRY_VERIFY(f.editor->hasActiveFocus());
    f.editor->setProperty("cursorPosition", 9);
    QTest::keyClick(f.quick, Qt::Key_Up, Qt::ControlModifier);
    QTest::qWait(150);
    QCOMPARE(library->rootFolder(), nested);
    if (QKeySequence::keyBindings(QKeySequence::MoveToStartOfDocument).contains(QKeySequence(Qt::CTRL | Qt::Key_Up)))
        QCOMPARE(f.editor->property("cursorPosition").toInt(), 0);
    // The shortcut is not even enabled outside the Library.
    QVERIFY(!f.window->property("libraryOwnsKeyboard").toBool());

    // Focus on a Library row: Cmd+Up goes to the parent folder.
    row->forceActiveFocus(); QTRY_VERIFY(row->hasActiveFocus());
    QTRY_VERIFY(f.window->property("libraryOwnsKeyboard").toBool());
    QTest::keyClick(f.quick, Qt::Key_Up, Qt::ControlModifier);
    QTRY_COMPARE(library->rootFolder(), top);
    // ...and it is a recorded step, so Back returns.
    QVERIFY(library->canGoBack());
    QVERIFY(library->navigateHistory(-1));
    QCOMPARE(library->rootFolder(), nested);

    // The header's folder button counts as the middle pane.
    auto *folder = f.window->findChild<QQuickItem *>("workspaceFolderButton"); QVERIFY(folder);
    folder->forceActiveFocus(); QTRY_VERIFY(folder->hasActiveFocus());
    QTRY_VERIFY(f.window->property("libraryOwnsKeyboard").toBool());
    QTest::keyClick(f.quick, Qt::Key_Up, Qt::ControlModifier);
    QTRY_COMPARE(library->rootFolder(), top);

    // A text field there (the filter) keeps Cmd+Up for itself.
    library->setRootFolder(QUrl::fromLocalFile(directory.filePath("top/nested")));
    pane->setProperty("showFilterBar", true);
    auto *filter = f.window->findChild<QQuickItem *>("libraryFilter"); QVERIFY(filter);
    QTRY_VERIFY(filter->isVisible());
    filter->forceActiveFocus(); QTRY_VERIFY(filter->hasActiveFocus());
    QVERIFY(!f.window->property("libraryOwnsKeyboard").toBool());
    QTest::keyClick(f.quick, Qt::Key_Up, Qt::ControlModifier);
    QTest::qWait(150);
    QCOMPARE(library->rootFolder(), nested);
    f.backend.discardRecovery();
}

void FomawriteTest::libraryHeaderArrowsStepAsideBeforeTheyCanOverlap() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    const bool mac = f.window->property("isMac").toBool();
    auto *arrows = f.window->findChild<QQuickItem *>("libraryHistoryControls"); QVERIFY(arrows);
    auto *folder = f.window->findChild<QQuickItem *>("workspaceFolderButton"); QVERIFY(folder);
    auto *newDocument = f.window->findChild<QQuickItem *>("newDocumentButton"); QVERIFY(newDocument);
    // Narrow headers give way in order: New Document first (below 238 px of content),
    // then the arrows (below 192), so the folder name keeps room beside them.
    // On macOS with the organizer hidden the window buttons take an 84 px inset instead of 12.
    for (bool organizer : {true, false}) {
        f.layout->setProperty("organizerVisible", organizer); f.layout->setProperty("filesVisible", true);
        for (int width : {420, 360, 334, 333, 288, 287, 232}) {
            f.layout->setProperty("fileWidth", width);
            QTRY_COMPARE(f.layout->property("effectiveFileWidth").toReal(), double(width));
            auto *header = f.window->findChild<QQuickItem *>("filesHeader");
            QTRY_COMPARE(header->width(), double(width));
            const int content = width - ((mac && !organizer) ? 84 : 12) - 12;
            const bool arrowsExpected = content >= 192;
            const bool newExpected = content >= 238 || !arrowsExpected;
            QTRY_COMPARE_WITH_TIMEOUT(arrows->isVisible(), arrowsExpected, 3000);
            QTRY_COMPARE_WITH_TIMEOUT(newDocument->isVisible(), newExpected, 3000);
            QVERIFY2(folder->width() >= (arrowsExpected ? 76 : 32), qPrintable(QStringLiteral("organizer %1 width %2 folder %3").arg(organizer).arg(width).arg(folder->width())));
            QVERIFY2(cycle151FilesHeaderFits(f.window.data()), qPrintable(QStringLiteral("organizer %1 width %2").arg(organizer).arg(width)));
        }
    }
    // The default column behind the macOS window buttons keeps the arrows.
    QVERIFY(288 - 84 - 12 >= 192);
    f.backend.discardRecovery();
}

// Review fixes: an arrow that greys out keeps focus in the middle pane; a disabled arrow
// or a double-click never reaches the header's window drag/zoom; deleted folders drop
// out of the history; Cmd+Up works inside the Files drawer.
void FomawriteTest::folderArrowsKeepFocusAndClicksInsideTheGroup() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    cycle148Make(directory.filePath("top/one/Note.md"));
    cycle148Make(directory.filePath("top/two/Note.md"));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    auto *library = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(library);
    f.layout->setProperty("organizerVisible", true); f.layout->setProperty("filesVisible", true);
    f.layout->setProperty("fileWidth", 288);
    QTRY_COMPARE(f.layout->property("effectiveFileWidth").toReal(), 288.0);
    auto *back = f.window->findChild<QQuickItem *>("libraryBackButton"); QVERIFY(back);
    auto *pane = f.window->findChild<QQuickItem *>("libraryPane"); QVERIFY(pane);
    const QUrl one = cycle148Canonical(directory.filePath("top/one")), two = cycle148Canonical(directory.filePath("top/two"));
    library->setRootFolder(QUrl::fromLocalFile(directory.filePath("top/one")));
    // Start from a fresh history: two steps, so one Back reaches the start.
    while (library->navigateHistory(-1)) {}
    library->setRootFolder(QUrl::fromLocalFile(directory.filePath("top/two")));
    QTRY_VERIFY(back->isVisible() && back->isEnabled());
    f.quick->requestActivate(); QTRY_VERIFY(f.quick->isActive());
    const auto visibility = f.quick->visibility();
    // Double-click Back: the first click goes back and greys the arrow; the second
    // must not zoom the window.
    const QPoint centre = back->mapToScene(QPointF(back->width() / 2, back->height() / 2)).toPoint();
    QTest::mouseDClick(f.quick, Qt::LeftButton, Qt::NoModifier, centre);
    QTRY_COMPARE(library->rootFolder(), one);
    QTRY_VERIFY(!back->isEnabled());
    QCOMPARE(f.quick->visibility(), visibility);
    // Focus is back in the middle pane, so Cmd+Up still goes up.
    QTRY_VERIFY(f.window->property("libraryOwnsKeyboard").toBool());
    QTest::keyClick(f.quick, Qt::Key_Up, Qt::ControlModifier);
    QTRY_COMPARE(library->rootFolder(), cycle148Canonical(directory.filePath("top")));
    Q_UNUSED(two); Q_UNUSED(pane);
    f.backend.discardRecovery();
}

void FomawriteTest::deletedFoldersDropOutOfTheFolderHistory() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    for (const char *name : {"one", "gone", "three"}) cycle148Make(directory.filePath(QStringLiteral("%1/Note.md").arg(QLatin1String(name))));
    FileLibrary library;
    library.setRootFolder(QUrl::fromLocalFile(directory.filePath("one")));
    library.setRootFolder(QUrl::fromLocalFile(directory.filePath("gone")));
    library.setRootFolder(QUrl::fromLocalFile(directory.filePath("three")));
    QVERIFY(QDir(directory.filePath("gone")).removeRecursively());
    // Back skips the deleted folder and lands on the one before it.
    QVERIFY(library.navigateHistory(-1));
    QCOMPARE(library.rootFolder(), cycle148Canonical(directory.filePath("one")));
    // Forward goes straight to the last folder; the deleted one is gone from the history.
    QVERIFY(library.navigateHistory(1));
    QCOMPARE(library.rootFolder(), cycle148Canonical(directory.filePath("three")));
    QVERIFY(library.navigateHistory(-1));
    QCOMPARE(library.rootFolder(), cycle148Canonical(directory.filePath("one")));
    // With only deleted folders ahead, Forward reports why and the arrow greys out.
    QVERIFY(QDir(directory.filePath("three")).removeRecursively());
    QVERIFY(library.canGoForward());
    QVERIFY(!library.navigateHistory(1));
    QVERIFY(!library.canGoForward());
    QVERIFY(library.error().contains(QStringLiteral("no longer available")));
    QCOMPARE(library.rootFolder(), cycle148Canonical(directory.filePath("one")));
}

void FomawriteTest::cmdUpWorksInsideTheFilesDrawer() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    cycle148Make(directory.filePath("top/nested/Inner.md"));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    auto *library = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(library);
    library->setNavigationMode(1);
    library->setRootFolder(QUrl::fromLocalFile(directory.filePath("top/nested")));
    f.layout->setProperty("filesVisible", false);
    f.quick->resize(480, 720);
    QTRY_COMPARE(f.layout->property("availableWidth").toReal(), 477.0);
    QVariant index;
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "exploreFolderOf", Q_RETURN_ARG(QVariant, index), Q_ARG(QVariant, QUrl::fromLocalFile(directory.filePath("top/nested/Inner.md")))));
    auto *drawer = f.window->findChild<QObject *>("workspaceNavigationDrawer"); QVERIFY(drawer);
    QTRY_VERIFY(drawer->property("opened").toBool());
    auto *pane = f.window->findChild<QQuickItem *>("libraryPane"); QVERIFY(pane);
    QTRY_VERIFY(pane->property("inDrawer").toBool());
    QQuickItem *row = nullptr;
    QTRY_VERIFY((row = cycle117Entry(pane, QStringLiteral("Inner.md"))) != nullptr);
    f.quick->requestActivate(); QTRY_VERIFY(f.quick->isActive());
    row->forceActiveFocus(); QTRY_VERIFY(row->hasActiveFocus());
    QTRY_VERIFY(pane->property("ownsKeyboard").toBool());
    QTest::keyClick(f.quick, Qt::Key_Up, Qt::ControlModifier);
    QTRY_COMPARE(library->rootFolder(), cycle148Canonical(directory.filePath("top")));
    QVERIFY(QMetaObject::invokeMethod(drawer, "close"));
    QTRY_VERIFY(!drawer->property("visible").toBool());
    QVERIFY(!pane->property("inDrawer").toBool());
    f.backend.discardRecovery();
}
