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
    // The arrows need 238 px of header content (66 arrows + 80 folder name + two 34 px buttons + 3 gaps of 8).
    // On macOS with the organizer hidden the window buttons take an 84 px inset instead of 12.
    for (bool organizer : {true, false}) {
        f.layout->setProperty("organizerVisible", organizer); f.layout->setProperty("filesVisible", true);
        for (int width : {420, 360, 334, 333, 288, 232}) {
            f.layout->setProperty("fileWidth", width);
            QTRY_COMPARE(f.layout->property("effectiveFileWidth").toReal(), double(width));
            auto *header = f.window->findChild<QQuickItem *>("filesHeader");
            QTRY_COMPARE(header->width(), double(width));
            const int content = width - ((mac && !organizer) ? 84 : 12) - 12;
            const bool expected = content >= 238;
            QTRY_COMPARE_WITH_TIMEOUT(arrows->isVisible(), expected, 3000);
            QVERIFY(folder->width() >= (expected ? 80 : 32));
            QVERIFY2(cycle151FilesHeaderFits(f.window.data()), qPrintable(QStringLiteral("organizer %1 width %2").arg(organizer).arg(width)));
        }
    }
    // The minimum column behind the macOS window buttons: 136 px of content holds the
    // folder button (32 min), New and Hide (116 px) but not the arrows as well.
    QVERIFY(232 - 84 - 12 >= 32 + 34 + 34 + 16);
    f.backend.discardRecovery();
}
