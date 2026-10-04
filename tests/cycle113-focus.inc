namespace {
QObject *cycle113NativeFormatItem(QObject *window, const QString &text) {
    auto *menu = window->findChild<QObject *>("nativeFormatMenu");
    if (!menu) return nullptr;
    for (auto *item : menu->findChildren<QObject *>()) {
        if (item->property("text").toString() == text && item->metaObject()->indexOfSignal("triggered()") >= 0)
            return item;
    }
    return nullptr;
}
bool cycle113TryEveryFormattingRoute(QObject *window, QQuickItem *editor, Backend &backend) {
    const auto text = editor->property("text").toString();
    const auto revision = backend.documentRevision();
    const auto first = editor->property("selectionStart"), last = editor->property("selectionEnd");
    const auto undo = editor->property("canUndo"), redo = editor->property("canRedo");
    auto *quick = qobject_cast<QQuickWindow *>(window);
    const auto focus = quick->activeFocusItem();
    auto *linkDialog = window->findChild<QObject *>("linkEditor");
    const bool linkWasOpen = linkDialog->property("opened").toBool();
    auto *commands = window->findChild<QObject *>("workspaceCommands");
    if (window->property("canFormatSource").toBool()) return false;
    if (!QMetaObject::invokeMethod(window, "tryWrapSelection", Q_ARG(QVariant, "**"), Q_ARG(QVariant, "**"))
        || !QMetaObject::invokeMethod(window, "tryInsertLink")
        || !QMetaObject::invokeMethod(window, "tryInsertSourceSnippet", Q_ARG(QVariant, "Unsafe insertion"))
        || !QMetaObject::invokeMethod(window, "editMarkdown", Q_ARG(QVariant, "heading2"))
        || !QMetaObject::invokeMethod(window, "openQuickFormat", Q_ARG(QVariant, QVariant()))) return false;
    for (const auto &name : {"strike", "inlineCode", "pageBreak"}) {
        QVariant enabled;
        if (!QMetaObject::invokeMethod(commands, "isEnabled", Q_RETURN_ARG(QVariant, enabled), Q_ARG(QVariant, name))
            || enabled.toBool() || !QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, name))) return false;
    }
    for (const auto &name : {"Bold", "Italic", "Highlight", "Strikethrough", "Code", "Heading 2", "List", "Indent", "Add Link",
                              "Add Footnote", "Add Content Block", "Add Hashtag", "Add Table of Contents", "Add Page Break"}) {
        auto *item = cycle113NativeFormatItem(window, QString::fromLatin1(name));
        if (!item || item->property("enabled").toBool() || !QMetaObject::invokeMethod(item, "triggered")) return false;
    }
    // Exercise the actual registered shortcuts where an isolated test window can
    // be active. Cocoa tests deliberately remain non-activating to protect input.
    if (QGuiApplication::platformName() != "cocoa") {
        for (auto key : {Qt::Key_B, Qt::Key_I, Qt::Key_K}) QTest::keyClick(quick, key, Qt::ControlModifier);
    }
    if (linkDialog->property("opened").toBool() != linkWasOpen) return false;
    return editor->property("text").toString() == text && backend.documentRevision() == revision
        && editor->property("selectionStart") == first && editor->property("selectionEnd") == last
        && editor->property("canUndo") == undo && editor->property("canRedo") == redo
        && quick->activeFocusItem() == focus;
}
}

void FomawriteTest::formattingRejectsAuxiliaryAndPreviewFocus() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    const QString source = QString::fromUtf8("# Focus ownership\n\nA café draft — 你好.\n");
    const QString path = directory.filePath("Focus.md");
    QVERIFY(cycle103Write(path, source.toUtf8()));
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *commands = window->findChild<QObject *>("workspaceCommands");
    auto *preview = window->findChild<QObject *>("previewPane");
    QVERIFY(quick && editor && layout && commands && preview);
    cycle106KeepNativeFocus(quick); Cycle106MouseInputIsolation input(quick);
    quick->resize(1440, 820); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    if (QGuiApplication::platformName() != "cocoa") { quick->requestActivate(); QTRY_VERIFY(quick->isActive()); }
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, source.size()), Q_ARG(QString, "Unsaved ending.\n")));
    const auto draft = editor->property("text").toString();
    const int start = draft.indexOf(QString::fromUtf8("café"));
    QTest::qWait(180);
    editor->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, start), Q_ARG(int, start + 4)));
    QTRY_VERIFY(window->property("canFormatSource").toBool());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "openSearch", Q_ARG(QVariant, true), Q_ARG(QVariant, false)));
    for (const auto *name : {"searchField", "replaceField"}) {
        auto *field = window->findChild<QQuickItem *>(name); QVERIFY(field && field->isVisible());
        field->forceActiveFocus(); QTRY_COMPARE(quick->activeFocusItem(), field);
        QTRY_VERIFY(!window->property("canFormatSource").toBool());
        QVERIFY2(cycle113TryEveryFormattingRoute(window.data(), editor, backend), name);
    }
    QVERIFY(QMetaObject::invokeMethod(window.data(), "closeSearch", Q_ARG(QVariant, true)));
    editor->forceActiveFocus(); QTRY_VERIFY(window->property("canFormatSource").toBool());
    QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, "outline")));
    auto *outline = window->findChild<QObject *>("documentOutline");
    auto *outlineSearch = window->findChild<QQuickItem *>("documentOutlineSearch");
    QTRY_VERIFY(outline->property("opened").toBool());
    outlineSearch->forceActiveFocus(); QTRY_VERIFY(!window->property("canFormatSource").toBool());
    QVERIFY(cycle113TryEveryFormattingRoute(window.data(), editor, backend));
    QVERIFY(QMetaObject::invokeMethod(outline, "close")); QTRY_VERIFY(!outline->property("opened").toBool());
    layout->setProperty("organizerVisible", true); layout->setProperty("filesVisible", true);
    auto *library = window->findChild<QObject *>("libraryPane");
    library->setProperty("showFilterBar", true);
    auto *filter = window->findChild<QQuickItem *>("libraryFilter"); QVERIFY(filter);
    QTRY_VERIFY(filter->isVisible()); filter->forceActiveFocus();
    QTRY_VERIFY(!window->property("canFormatSource").toBool());
    QVERIFY(cycle113TryEveryFormattingRoute(window.data(), editor, backend));
    // A navigation control does not make a retained Source selection current.
    auto *folder = window->findChild<QQuickItem *>("workspaceFolderButton"); QVERIFY(folder);
    folder->forceActiveFocus(); QTRY_VERIFY(!window->property("canFormatSource").toBool());
    QVERIFY(cycle113TryEveryFormattingRoute(window.data(), editor, backend));
    editor->forceActiveFocus(); QTRY_VERIFY(window->property("canFormatSource").toBool());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "tryInsertLink"));
    auto *linkDialog = window->findChild<QObject *>("linkEditor");
    QTRY_VERIFY(linkDialog->property("opened").toBool());
    for (const auto *name : {"linkEditorLabel", "linkEditorUrl", "linkEditorTitle"}) {
        auto *field = window->findChild<QQuickItem *>(name); QVERIFY(field);
        field->forceActiveFocus(); QTRY_VERIFY(!window->property("canFormatSource").toBool());
        QVERIFY2(cycle113TryEveryFormattingRoute(window.data(), editor, backend), name);
    }
    QVERIFY(QMetaObject::invokeMethod(linkDialog, "close"));
    QTRY_VERIFY(!linkDialog->property("opened").toBool());
    for (const auto &mode : {"preview", "visual"}) {
        QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, mode)));
        QTRY_VERIFY(!window->property("changingDocumentView").toBool());
        QTRY_VERIFY(!preview->property("viewportRefreshPending").toBool());
        auto *field = window->findChild<QQuickItem *>(QString::fromLatin1(mode) == "visual" ? "visualEditor" : "renderedPreview");
        field->forceActiveFocus(); QTRY_VERIFY(!window->property("canFormatSource").toBool());
        QVERIFY(cycle113TryEveryFormattingRoute(window.data(), editor, backend));
    }
    QCOMPARE(editor->property("text").toString(), draft);
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QCOMPARE(editor->property("text").toString(), source);
}

void FomawriteTest::formattingToolbarAndMenusKeepSourceSelection() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    const QString source = QString::fromUtf8("# Formatting\n\nA café draft — 你好.\n");
    const QString path = directory.filePath("Formatting.md");
    QVERIFY(cycle103Write(path, source.toUtf8()));
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *bold = window->findChild<QQuickItem *>("compactBoldButton");
    QVERIFY(quick && editor && layout && bold);
    cycle106KeepNativeFocus(quick); Cycle106MouseInputIsolation input(quick);
    quick->resize(1440, 820); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    if (QGuiApplication::platformName() != "cocoa") { quick->requestActivate(); QTRY_VERIFY(quick->isActive()); }
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
    QTest::qWait(180);
    const int start = source.indexOf(QString::fromUtf8("café"));
    auto selectSource = [&] {
        editor->forceActiveFocus();
        return QMetaObject::invokeMethod(editor, "select", Q_ARG(int, start), Q_ARG(int, start + 4));
    };
    QVERIFY(selectSource()); QTRY_VERIFY(window->property("canFormatSource").toBool());
    QVERIFY(cycle102Click(quick, bold));
    QString marked = source; marked.replace(start, 4, QString::fromUtf8("**café**"));
    QCOMPARE(editor->property("text").toString(), marked);
    QVERIFY(editor->hasActiveFocus());
    QVERIFY(QMetaObject::invokeMethod(editor, "undo")); QCOMPARE(editor->property("text").toString(), source);
    QVERIFY(!editor->property("canUndo").toBool());
    QVERIFY(selectSource());
    auto *paragraph = window->findChild<QQuickItem *>("compactParagraphButton"); QVERIFY(paragraph);
    QVERIFY(cycle102Click(quick, paragraph));
    auto *menu = window->findChild<QObject *>("quickFormatMenu"); QVERIFY(menu);
    QTRY_VERIFY(menu->property("opened").toBool());
    QTRY_VERIFY(window->property("canFormatSource").toBool());
    auto *heading = cycle108MenuItem(menu, "Heading 2"); QVERIFY(heading);
    QVERIFY(cycle102Click(quick, heading));
    QVERIFY(editor->property("text").toString().contains(QString::fromUtf8("## A café draft")));
    QVERIFY(QMetaObject::invokeMethod(editor, "undo")); QCOMPARE(editor->property("text").toString(), source);
    QVERIFY(selectSource());
    auto *native = cycle113NativeFormatItem(window.data(), "Italic"); QVERIFY(native && native->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(native, "triggered"));
    marked = source; marked.replace(start, 4, QString::fromUtf8("*café*"));
    QCOMPARE(editor->property("text").toString(), marked);
    QVERIFY(QMetaObject::invokeMethod(editor, "undo")); QCOMPARE(editor->property("text").toString(), source);
    // Keyboard traversal into the toolbar preserves the same deliberate Source target.
    QVERIFY(selectSource());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "cycleWorkspaceFocus", Q_ARG(QVariant, false)));
    QTRY_COMPARE(quick->activeFocusItem(), bold);
    QVERIFY(window->property("canFormatSource").toBool());
    cycle111KeyClick(quick, Qt::Key_Space);
    marked = source; marked.replace(start, 4, QString::fromUtf8("**café**"));
    QCOMPARE(editor->property("text").toString(), marked);
    QVERIFY(QMetaObject::invokeMethod(editor, "undo")); QCOMPARE(editor->property("text").toString(), source);
    if (QGuiApplication::platformName() != "cocoa") {
        QVERIFY(selectSource()); QTest::keyClick(quick, Qt::Key_B, Qt::ControlModifier);
        QCOMPARE(editor->property("text").toString(), marked);
        QVERIFY(QMetaObject::invokeMethod(editor, "undo")); QCOMPARE(editor->property("text").toString(), source);
        QInputMethodEvent composition(QStringLiteral("x"), {}); QCoreApplication::sendEvent(quick, &composition);
        QTRY_VERIFY(editor->property("inputMethodComposing").toBool());
        QVERIFY(!window->property("canFormatSource").toBool());
        const auto composingSource = editor->property("text").toString();
        const auto composingRevision = backend.documentRevision();
        QVERIFY(QMetaObject::invokeMethod(window.data(), "tryWrapSelection", Q_ARG(QVariant, "**"), Q_ARG(QVariant, "**")));
        QCOMPARE(editor->property("text").toString(), composingSource);
        QCOMPARE(backend.documentRevision(), composingRevision);
        QInputMethodEvent cancel; QCoreApplication::sendEvent(quick, &cancel);
        QTRY_VERIFY(!editor->property("inputMethodComposing").toBool());
    }
}


void FomawriteTest::workspaceFocusCycleIncludesRenderedPreview() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    const QString source = QString::fromUtf8("# Pane focus\n\nA café draft — 你好.\n");
    const QString path = directory.filePath("Pane focus.md");
    QVERIFY(cycle103Write(path, source.toUtf8()));
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *preview = window->findChild<QQuickItem *>("previewPane");
    auto *rendered = preview ? preview->findChild<QQuickItem *>("renderedPreview") : nullptr;
    auto *visualPane = window->findChild<QQuickItem *>("visualEditorPane");
    auto *visual = visualPane ? visualPane->findChild<QQuickItem *>("visualEditor") : nullptr;
    auto *header = window->findChild<QQuickItem *>("topChrome");
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    QVERIFY(quick && editor && preview && rendered && visual && header && layout);
    cycle106KeepNativeFocus(quick); Cycle106MouseInputIsolation input(quick);
    quick->resize(1440, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    if (QGuiApplication::platformName() != "cocoa") { quick->requestActivate(); QTRY_VERIFY(quick->isActive()); }
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "setDocumentView", Q_ARG(QVariant, 1)));
    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 1);
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_VERIFY(editor->isVisible() && preview->isVisible());
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, source.size()), Q_ARG(QString, "Unsaved sentence.\n")));
    const QString draft = editor->property("text").toString();
    const int start = draft.indexOf(QString::fromUtf8("café"));
    const auto within = [](QQuickItem *item, QQuickItem *ancestor) {
        for (; item; item = item->parentItem()) if (item == ancestor) return true;
        return false;
    };
    const auto cycle = [&](bool reverse) {
        if (QGuiApplication::platformName() == "cocoa")
            return QMetaObject::invokeMethod(window.data(), "cycleWorkspaceFocus", Q_ARG(QVariant, reverse));
        // Offscreen windows can safely activate the real registered F6 shortcuts.
        QTest::keyClick(quick, Qt::Key_F6, reverse ? Qt::ShiftModifier : Qt::NoModifier);
        return true;
    };
    for (const bool visualEnabled : {false, true}) {
        QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, "source")));
        QTRY_VERIFY(!window->property("changingDocumentView").toBool());
        editor->forceActiveFocus(); QTRY_VERIFY(window->property("canFormatSource").toBool());
        QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, start), Q_ARG(int, start + 4)));
        if (visualEnabled)
            QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, "visual")));
        QTRY_VERIFY(!window->property("changingDocumentView").toBool());
        QTRY_VERIFY(!preview->property("viewportRefreshPending").toBool());
        auto *editingSurface = visualEnabled ? visual : editor;
        QTRY_COMPARE(quick->activeFocusItem(), editingSurface);
        // The right-hand output Preview stays read-only with either left editor.
        QVERIFY(preview->property("allowVisualEdit").toBool() == false);
        QVERIFY(cycle(false)); QTRY_COMPARE(quick->activeFocusItem(), rendered);
        QVERIFY(!window->property("canFormatSource").toBool());
        QVERIFY(cycle(false)); QTRY_VERIFY(within(quick->activeFocusItem(), header));
        QVERIFY(cycle(true)); QTRY_COMPARE(quick->activeFocusItem(), rendered);
        QVERIFY(cycle(true)); QTRY_COMPARE(quick->activeFocusItem(), editingSurface);
        QCOMPARE(window->property("canFormatSource").toBool(), !visualEnabled);
        // Complete both wraparound directions without skipping the right pane.
        QVERIFY(cycle(true)); QTRY_VERIFY(within(quick->activeFocusItem(), header));
        QVERIFY(cycle(false)); QTRY_COMPARE(quick->activeFocusItem(), editingSurface);
        QCOMPARE(editor->property("selectionStart").toInt(), start);
        QCOMPARE(editor->property("selectionEnd").toInt(), start + 4);
        QCOMPARE(editor->property("text").toString(), draft);
        QVERIFY(backend.modified());
    }
    QVERIFY(QMetaObject::invokeMethod(editor, "undo")); QCOMPARE(editor->property("text").toString(), source);
    QVERIFY(QMetaObject::invokeMethod(editor, "redo")); QCOMPARE(editor->property("text").toString(), draft);
    QCOMPARE(cycle103Read(path), source.toUtf8());
}
