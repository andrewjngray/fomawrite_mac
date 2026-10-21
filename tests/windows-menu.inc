// The Windows menu can be instantiated on every platform, so its routing and
// check-state regression runs in the Mac gate as well as Windows CI.
void FomawriteTest::windowsMenuRoutesPortableCommands() {
    Cycle103SettingsSnapshot isolatedSettings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *loader = f.window->findChild<QObject *>("windowsMenuLoader");
    QVERIFY2(loader, "Windows must have a desktop menu loader");
    loader->setProperty("active", true);
    QTRY_VERIFY(f.window->findChild<QObject *>("windowsMenuBar"));
    auto *menu = f.window->findChild<QObject *>("windowsMenuBar");
    QVERIFY(!menu->findChild<QObject *>("windows_engineMacos"));
    QVERIFY(!menu->findChild<QObject *>("windows_checkWritingCheckerUpdates"));
    auto *source = menu->findChild<QObject *>("windows_sourceEditing");
    auto *live = menu->findChild<QObject *>("windows_liveEditing");
    auto *web = menu->findChild<QObject *>("windows_webPreview");
    auto *pdf = menu->findChild<QObject *>("windows_pdfPreview");
    QVERIFY(source && live && web && pdf);
    const QString original = QString::fromUtf8("# Menu fixture\n\nCafé **stays**.\n");
    f.editor->setProperty("text", original);
    // A Qt Quick popup takes focus. Formatting must keep the writing surface
    // that owned the selection when the desktop menu was opened.
    f.editor->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, original.indexOf(QString::fromUtf8("Café"))), Q_ARG(int, original.indexOf(QString::fromUtf8("Café")) + 4)));
    QObject *formatMenu = nullptr;
    for (QObject *candidate : menu->findChildren<QObject *>())
        if (candidate->property("title").toString() == QStringLiteral("F&ormat")) { formatMenu = candidate; break; }
    QVERIFY(formatMenu);
    QObject *bold = nullptr;
    for (QObject *candidate : formatMenu->findChildren<QObject *>())
        if (candidate->property("text").toString() == QStringLiteral("&Bold")) { bold = candidate; break; }
    QVERIFY(bold);
    QVERIFY(QMetaObject::invokeMethod(formatMenu, "open"));
    QTRY_VERIFY(formatMenu->property("opened").toBool());
    QTRY_VERIFY(!f.editor->hasActiveFocus());
    QVERIFY2(bold->property("enabled").toBool(), "Opening the desktop Format menu must retain Source formatting ownership");
    QVERIFY(cycle102Click(f.quick, qobject_cast<QQuickItem *>(bold)));
    QTRY_VERIFY(!formatMenu->property("visible").toBool());
    const QString formatted = QString(original).replace(QString::fromUtf8("Café"), QString::fromUtf8("**Café**"));
    QTRY_COMPARE(f.editor->property("text").toString(), formatted);
    QObject *editMenu = nullptr;
    for (QObject *candidate : menu->findChildren<QObject *>())
        if (candidate->property("title").toString() == QStringLiteral("&Edit")) { editMenu = candidate; break; }
    QVERIFY(editMenu);
    const auto editItem = [&](const QString &text) -> QQuickItem * {
        for (QObject *candidate : editMenu->findChildren<QObject *>())
            if (candidate->property("text").toString() == text) return qobject_cast<QQuickItem *>(candidate);
        return nullptr;
    };
    auto *copy = editItem(QStringLiteral("&Copy"));
    auto *cut = editItem(QStringLiteral("Cu&t"));
    auto *paste = editItem(QStringLiteral("&Paste"));
    auto *undo = editItem(QStringLiteral("&Undo"));
    QVERIFY(copy && cut && paste && undo);
    // Source copy retains the Markdown/authorship clipboard contract.
    f.editor->forceActiveFocus();
    const int cafe = formatted.indexOf(QString::fromUtf8("Café"));
    QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, cafe), Q_ARG(int, cafe + 4)));
    QVERIFY(QMetaObject::invokeMethod(editMenu, "open"));
    QTRY_VERIFY(editMenu->property("opened").toBool());
    QVERIFY(cycle102Click(f.quick, copy));
    QTRY_VERIFY(!editMenu->property("visible").toBool());
    QTRY_COMPARE(QGuiApplication::clipboard()->text(), QString::fromUtf8("Café"));
    QVERIFY(QGuiApplication::clipboard()->mimeData()->hasFormat("text/markdown"));
    QVERIFY(QGuiApplication::clipboard()->mimeData()->hasFormat("application/x-omawrite-authorship+json"));
    QVERIFY(QMetaObject::invokeMethod(editMenu, "open"));
    QTRY_VERIFY(editMenu->property("opened").toBool());
    QVERIFY(cycle102Click(f.quick, undo));
    QTRY_VERIFY(!editMenu->property("visible").toBool());
    QTRY_COMPARE(f.editor->property("text").toString(), original);
    // A focused auxiliary field keeps its own cut/paste target after menu focus.
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "openSearch", Q_ARG(QVariant, false), Q_ARG(QVariant, false)));
    auto *field = f.window->findChild<QQuickItem *>("searchField"); QVERIFY(field);
    field->setProperty("text", QStringLiteral("auxiliary selection"));
    field->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(field, "selectAll"));
    QVERIFY(QMetaObject::invokeMethod(editMenu, "open"));
    QTRY_VERIFY(editMenu->property("opened").toBool());
    QVERIFY(cycle102Click(f.quick, cut));
    QTRY_VERIFY(!editMenu->property("visible").toBool());
    QTRY_COMPARE(field->property("text").toString(), QString());
    QTRY_COMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("auxiliary selection"));
    QGuiApplication::clipboard()->setText(QStringLiteral("field replacement"));
    QVERIFY(QMetaObject::invokeMethod(editMenu, "open"));
    QTRY_VERIFY(editMenu->property("opened").toBool());
    QVERIFY(cycle102Click(f.quick, paste));
    QTRY_VERIFY(!editMenu->property("visible").toBool());
    QTRY_COMPARE(field->property("text").toString(), QStringLiteral("field replacement"));
    QCOMPARE(f.editor->property("text").toString(), original);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "closeSearch", Q_ARG(QVariant, true)));
    QVERIFY(QMetaObject::invokeMethod(live, "triggered"));
    QTRY_VERIFY(f.layout->property("liveEditEnabled").toBool());
    QTRY_VERIFY(live->property("checked").toBool());
    // Live Undo must reach its page history while the popup owns actual focus.
    f.backend.editorBridge()->ready();
    auto *liveView = f.window->findChild<QQuickItem *>("liveEditorView"); QVERIFY(liveView);
    liveView->forceActiveFocus();
    QTRY_VERIFY(f.window->property("canFormatLive").toBool());
    QSignalSpy liveUndo(f.backend.editorBridge(), &EditorBridge::undo);
    QVERIFY(QMetaObject::invokeMethod(editMenu, "open"));
    QTRY_VERIFY(editMenu->property("opened").toBool());
    QTRY_VERIFY(!liveView->hasActiveFocus());
    QVERIFY2(f.window->property("canFormatLive").toBool(), "Desktop menu focus must preserve Live history ownership");
    QVERIFY(cycle102Click(f.quick, undo));
    QTRY_VERIFY(!editMenu->property("visible").toBool());
    QCOMPARE(liveUndo.count(), 1);
    QCOMPARE(f.editor->property("text").toString(), original);
    QVERIFY(QMetaObject::invokeMethod(source, "triggered"));
    QTRY_VERIFY(!f.layout->property("liveEditEnabled").toBool());
    QTRY_VERIFY(source->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(pdf, "triggered"));
    auto *settings = f.window->findChild<QObject *>("workspaceSettings");
    QVERIFY(settings);
    QTRY_COMPARE(settings->property("publishingFormat").toString(), QStringLiteral("pdf"));
    QTRY_VERIFY(pdf->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(web, "triggered"));
    QTRY_COMPARE(settings->property("publishingFormat").toString(), QStringLiteral("web"));
    QTRY_VERIFY(web->property("checked").toBool());
    QCOMPARE(f.editor->property("text").toString(), original);
    QVERIFY2(f.error.isEmpty(), qPrintable(f.error));
}
