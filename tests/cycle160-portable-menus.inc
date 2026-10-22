// Host component coverage of the remaining Windows routes. Native Mac semantic
// cases remain unchanged; this fixture first checks the real platform boundary.
void FomawriteTest::windowsRemainingPortableMenuRoutes_data() {
    QTest::addColumn<QByteArray>("id");
    QTest::addColumn<QString>("label");
    for (const auto &[id, label] : {
             std::pair{"fileNewLibraryWindow", "New in Library in Window…"},
             std::pair{"fileNewLibrary", "New in Library…"},
             std::pair{"fileNewFolder", "New Folder…"},
             std::pair{"reveal", "Show in Library"},
             std::pair{"fileAutosave", "Autosave Saved Files Every Minute"},
             std::pair{"filePrintSource", "Print Markdown Source…"},
             std::pair{"editAuthorship", "Authorship Annotations…"},
             std::pair{"editExportAuthorship", "Export Authorship Metadata…"},
             std::pair{"synchronizedScroll", "Synchronized Scrolling"}})
        QTest::newRow(id) << QByteArray(id) << QString::fromUtf8(label);
}

void FomawriteTest::windowsRemainingPortableMenuRoutes() {
    QFETCH(QByteArray, id); QFETCH(QString, label);
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QDir root(QFileInfo(directory.path()).canonicalFilePath());
    const QString original = QString::fromUtf8("# Menu routes\n\nKeep café 😀 plain Markdown.\n");
    const QString path = root.filePath("original.md");
    QVERIFY(cycle103Write(path, original.toUtf8()));
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    QVERIFY(testDesktopMenusMatchPlatform(f.window.data()));
    auto *loader = f.window->findChild<QObject *>("windowsMenuLoader"); QVERIFY(loader);
    if (!f.window->property("isWindows").toBool()) loader->setProperty("active", true);
    QTRY_VERIFY(f.window->findChild<QObject *>("windowsMenuBar"));
    auto *bar = f.window->findChild<QObject *>("windowsMenuBar");
    auto *action = bar->findChild<QObject *>(QStringLiteral("windows_") + QString::fromLatin1(id));
    QVERIFY2(action, qPrintable(QStringLiteral("Windows menu is missing %1").arg(QString::fromLatin1(id))));
    QCOMPARE(action->property("text").toString(), label);
    auto *settings = f.window->findChild<QObject *>("workspaceSettings"); QVERIFY(settings);
    auto *library = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(library);
    const bool creation = id.startsWith("fileNew");
    if (creation || id == "reveal") QVERIFY(!action->property("enabled").toBool());
    QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    QTRY_COMPARE(f.editor->property("text").toString(), original);
    QCoreApplication::processEvents();
    library->setRootFolder(QUrl::fromLocalFile(root.path()));
    QTRY_VERIFY(action->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.editor, "insert", Q_ARG(int, original.size()), Q_ARG(QString, QStringLiteral("Unsaved ending."))));
    const QString draft = f.editor->property("text").toString();
    f.editor->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, 1), Q_ARG(int, 5)));
    QTRY_VERIFY(f.window->property("canFormatSource").toBool());
    QVERIFY(f.backend.modified()); QVERIFY(f.editor->property("canUndo").toBool());
    const auto dialogNamed = [&](const QString &title) -> QObject * {
        for (auto *candidate : f.window->findChildren<QObject *>())
            if (candidate->property("title").toString() == title
                && candidate->metaObject()->indexOfSignal("accepted()") >= 0) return candidate;
        return nullptr;
    };
    const auto fieldNamed = [&](QObject *dialog, const QString &placeholder) -> QQuickItem * {
        for (auto *candidate : dialog->findChildren<QQuickItem *>())
            if (candidate->property("placeholderText").toString() == placeholder) return candidate;
        return nullptr;
    };
    if (creation) {
        const bool folder = id == "fileNewFolder";
        const bool newWindow = id == "fileNewLibraryWindow";
        auto *dialog = folder ? dialogNamed(QStringLiteral("New folder"))
                              : f.window->findChild<QObject *>("newLibraryFileDialog");
        QVERIFY(dialog);
        auto *field = fieldNamed(dialog, folder ? QStringLiteral("Folder name") : QStringLiteral("Document.md")); QVERIFY(field);
        const QString name = folder ? QStringLiteral("Created folder") : QStringLiteral("Created.md");
        const QString created = root.filePath(name);
        QVERIFY(QMetaObject::invokeMethod(action, "triggered"));
        QTRY_VERIFY(dialog->property("opened").toBool());
        if (!folder) QCOMPARE(dialog->property("inNewWindow").toBool(), newWindow);
        QVERIFY(field->setProperty("text", name));
        QVERIFY(QMetaObject::invokeMethod(dialog, "reject"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QVERIFY(!QFileInfo::exists(created));
        QCOMPARE(f.editor->property("text").toString(), draft);
        QSignalSpy windows(&f.backend, &Backend::newWindowRequested);
        QVERIFY(QMetaObject::invokeMethod(action, "triggered"));
        QTRY_VERIFY(dialog->property("opened").toBool());
        QVERIFY(field->setProperty("text", name));
        QVERIFY(QMetaObject::invokeMethod(dialog, "accept"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
        if (!folder && !newWindow) {
            auto *prompt = f.window->findChild<QObject *>("unsavedChangesPrompt"); QVERIFY(prompt);
            QTRY_VERIFY(prompt->property("opened").toBool());
            QVERIFY(!QFileInfo::exists(created));
            QVERIFY(QMetaObject::invokeMethod(prompt, "reject"));
            QTRY_VERIFY(!prompt->property("visible").toBool());
            QCOMPARE(f.editor->property("text").toString(), draft);
            QVERIFY(f.backend.modified());
            QVERIFY(!QFileInfo::exists(created));
            // A second explicit creation may discard this temporary draft only
            // after the real unsaved prompt receives Discard.
            QVERIFY(QMetaObject::invokeMethod(action, "triggered"));
            QTRY_VERIFY(dialog->property("opened").toBool());
            QVERIFY(field->setProperty("text", name));
            QVERIFY(QMetaObject::invokeMethod(dialog, "accept"));
            QTRY_VERIFY(prompt->property("opened").toBool());
            QVERIFY(QMetaObject::invokeMethod(prompt, "close"));
            QVERIFY(QMetaObject::invokeMethod(prompt, "discardRequested"));
            QTRY_VERIFY(QFileInfo::exists(created));
            QCOMPARE(cycle103Read(created), QByteArray());
            QTRY_COMPARE(f.backend.fileUrl().toLocalFile(), created);
            QTRY_COMPARE(f.editor->property("text").toString(), QString());
            QVERIFY(!f.backend.modified());
            QCOMPARE(cycle103Read(path), original.toUtf8());
            return;
        }
        QTRY_VERIFY(QFileInfo::exists(created));
        if (folder) QVERIFY(QFileInfo(created).isDir());
        else {
            QCOMPARE(cycle103Read(created), QByteArray());
            QTRY_COMPARE(windows.count(), 1);
            QCOMPARE(windows.first().first().toUrl().toLocalFile(), created);
        }
    } else if (id == "reveal") {
        QVERIFY(QDir(directory.path()).mkdir("Elsewhere"));
        library->setRootFolder(QUrl::fromLocalFile(root.filePath("Elsewhere")));
        library->setFilter(QStringLiteral("does-not-match"));
        QVERIFY(QMetaObject::invokeMethod(action, "triggered"));
        QTRY_VERIFY(library->filter().isEmpty());
        QTRY_COMPARE(library->rootFolder().toLocalFile(), QFileInfo(directory.path()).canonicalFilePath());
        bool found = false;
        for (const auto &entry : library->entries()) found |= entry.toMap().value("url").toUrl() == f.backend.fileUrl();
        QVERIFY(found);
        QVERIFY(f.layout->property("filesVisible").toBool());
    } else if (id == "fileAutosave" || id == "synchronizedScroll") {
        const char *key = id == "fileAutosave" ? "autosaveEnabled" : "synchronizedScroll";
        QVERIFY(action->property("checkable").toBool());
        settings->setProperty(key, false);
        QTRY_VERIFY(!action->property("checked").toBool());
        QVERIFY(QMetaObject::invokeMethod(action, "triggered"));
        QVERIFY(settings->property(key).toBool()); QVERIFY(action->property("checked").toBool());
        settings->setProperty(key, false);
        QTRY_VERIFY(!action->property("checked").toBool());
        QVERIFY(QMetaObject::invokeMethod(action, "triggered"));
        QVERIFY(action->property("checked").toBool());
        QVERIFY(QMetaObject::invokeMethod(action, "triggered"));
        QVERIFY(!settings->property(key).toBool()); QVERIFY(!action->property("checked").toBool());
    } else if (id == "editAuthorship" || id == "editExportAuthorship") {
        f.backend.markAuthorship(1, 5, QStringLiteral("Human"), QStringLiteral("Public test writer"));
        const auto annotations = f.backend.authorshipRanges();
        QVERIFY(!annotations.isEmpty());
        auto *dialog = dialogNamed(id == "editAuthorship" ? QStringLiteral("Authorship annotations") : QStringLiteral("Export Authorship Metadata"));
        QVERIFY(dialog);
        const QStringList beforeFiles = QDir(directory.path()).entryList(QDir::Files | QDir::Hidden);
        QVERIFY(QMetaObject::invokeMethod(action, "triggered"));
        QTRY_VERIFY(dialog->property("visible").toBool());
        if (id == "editAuthorship") {
            QVariant ranges = dialog->property("ranges");
            if (ranges.metaType() == QMetaType::fromType<QJSValue>()) ranges = ranges.value<QJSValue>().toVariant();
            QCOMPARE(ranges.toList(), annotations);
        }
        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QCOMPARE(f.backend.authorshipRanges(), annotations);
        QCOMPARE(QDir(directory.path()).entryList(QDir::Files | QDir::Hidden), beforeFiles);
    } else {
        QCOMPARE(id, QByteArray("filePrintSource"));
        // Do not enter blocking print/spool code in this portable fixture.
        // Existing window-routing-smoke.inc covers actual source-print cancel
        // separately; native Windows cancellation remains explicit qualification.
        QVERIFY(!action->property("checkable").toBool());
    }
    QCOMPARE(f.editor->property("text").toString(), draft);
    QCOMPARE(f.editor->property("selectionStart").toInt(), 1);
    QCOMPARE(f.editor->property("selectionEnd").toInt(), 5);
    QCOMPARE(f.backend.fileUrl().toLocalFile(), path);
    QVERIFY(f.backend.modified()); QVERIFY(f.editor->property("canUndo").toBool());
    QCOMPARE(cycle103Read(path), original.toUtf8());
    QVERIFY2(f.error.isEmpty(), qPrintable(f.error));
}
