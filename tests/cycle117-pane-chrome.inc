namespace {
struct Cycle117Window {
    Backend backend;
    QQmlEngine engine;
    QScopedPointer<QObject> window;
    QQuickWindow *quick = nullptr;
    QQuickItem *editor = nullptr;
    QQuickItem *footer = nullptr;
    QQuickItem *preview = nullptr;
    QObject *layout = nullptr;
    std::unique_ptr<Cycle106MouseInputIsolation> isolation;
    QString error;
    bool start(int width = 1280) {
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl("qrc:/Main.qml"));
        window.reset(component.create()); error = component.errorString();
        if (!window) return false;
        quick = qobject_cast<QQuickWindow *>(window.data());
        editor = window->findChild<QQuickItem *>("sourceEditor");
        footer = window->findChild<QQuickItem *>("documentFooter");
        preview = window->findChild<QQuickItem *>("previewPane");
        layout = window->findChild<QObject *>("workspaceLayout");
        if (!quick || !editor || !footer || !preview || !layout) return false;
        cycle106KeepNativeFocus(quick); isolation.reset(new Cycle106MouseInputIsolation(quick));
        quick->resize(width, 720); quick->show();
        if (!QTest::qWaitForWindowExposed(quick)) return false;
        QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace");
        layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
        return true;
    }
    QQuickItem *item(const char *name) const {
        // ExportHub owns a separate hidden Preview footer with some shared names.
        if (footer) if (auto *control = footer->findChild<QQuickItem *>(name)) return control;
        return window->findChild<QQuickItem *>(name);
    }
    bool mode(int value) {
        if (value == 2) { if (!cycle118ReadPreview(window.data())) return false; }
        else if (!cycle102Click(quick, footer->findChild<QQuickItem *>(value == 0 ? "singleModeButton" : "previewSplitButton"))) return false;
        QSignalSpy painted(quick, &QQuickWindow::frameSwapped);
        quick->update();
        return QTest::qWaitFor([&] { return !painted.isEmpty(); }, 1000);
    }
    ~Cycle117Window() { backend.discardRecovery(); }
};
QQuickItem *cycle117Entry(QQuickItem *library, const QString &filename) {
    for (auto *row : cycle103VisualItems(library, "libraryEntry")) {
        auto *label = row->findChild<QQuickItem *>("libraryEntryTitle");
        if (label && label->property("text").toString() == filename) return row;
    }
    return nullptr;
}
}

void FomawriteTest::paneAlignedFooterMenusFollowDividerWithoutEditing() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    const QString saved = QString::fromUtf8("# Pane-owned tools\n\nA café draft — 你好.\n");
    const QString path = directory.filePath("Pane-owned tools.md"); QVERIFY(cycle103Write(path, saved.toUtf8()));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error)); QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    QVERIFY(QMetaObject::invokeMethod(f.editor, "insert", Q_ARG(int, saved.size()), Q_ARG(QString, "Unsaved ending.")));
    const QString draft = f.editor->property("text").toString();
    auto *appearance = f.item("sourceAppearanceButton"), *output = f.item("previewTemplateButton");
    auto *visual = f.item("visualEditToggle");
    auto *rule = f.item("documentFooterPaneDivider"); QVERIFY(appearance && output && visual && rule);
    QVERIFY(f.mode(1)); QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 1);
    QTRY_VERIFY(cycle117PaneFooterAnchors(f.window.data()));
    QTRY_VERIFY(!f.window->property("changingDocumentView").toBool());
    QTRY_VERIFY(!f.preview->property("viewportRefreshPending").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, 3), Q_ARG(int, 8)));
    auto *divider = f.item("documentDividerGestures");
    QVERIFY(divider && divider->isVisible());
    QTRY_VERIFY(QRectF(QPointF(), f.quick->size()).contains(cycle103SceneBounds(divider)));
    const auto fixed = cycle107ViewBounds(f.footer);
    const auto appearanceBefore = cycle103SceneBounds(appearance);
    const qreal templateBefore = cycle103SceneBounds(output).left(), previewBefore = f.preview->width();
    const auto dragStart = cycle103SceneBounds(divider).center().toPoint();
    QTest::mousePress(f.quick, Qt::LeftButton, Qt::NoModifier, dragStart);
    QTest::mouseMove(f.quick, dragStart + QPoint(70, 0), 40);
    QTest::mouseRelease(f.quick, Qt::LeftButton, Qt::NoModifier, dragStart + QPoint(70, 0));
    QTRY_VERIFY(f.preview->width() <= previewBefore - 50);
    QTRY_VERIFY(cycle117PaneFooterAnchors(f.window.data()));
    QVERIFY(cycle103SceneBounds(output).left() >= templateBefore + 50);
    QCOMPARE(cycle103SceneBounds(appearance), appearanceBefore);
    QVERIFY(cycle107SameBounds(cycle107ViewBounds(f.footer), fixed));
    QTRY_VERIFY(qAbs(cycle103SceneBounds(rule).left() - cycle103SceneBounds(f.preview).left()) <= 0.5);
    for (bool dark : {false, true}) {
        f.backend.setThemePreset(dark ? "dark" : "studio");
        for (int width : {1280, 960, 803}) {
            f.quick->resize(width, 620);
            QVERIFY(QMetaObject::invokeMethod(f.layout, "updateWidth", Q_ARG(QVariant, "preview"), Q_ARG(QVariant, 320)));
            QVERIFY(f.mode(1)); QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 1);
            QTRY_VERIFY(cycle117PaneFooterAnchors(f.window.data()));
            QTRY_VERIFY(cycle107ViewLabelsReadable(f.footer));
            auto *appearanceMenu = f.window->findChild<QObject *>("sourceAppearanceMenu");
            auto *templateMenu = f.window->findChild<QObject *>("previewTemplateMenu");
            QVERIFY(cycle102Click(f.quick, appearance)); QTRY_VERIFY(appearanceMenu->property("opened").toBool());
            QVERIFY(cycle106MenuFits(f.quick, appearanceMenu, appearance));
            QVERIFY(cycle102Click(f.quick, f.item("sourceAppearanceBook")));
            QVERIFY(cycle102Click(f.quick, output)); QTRY_VERIFY(templateMenu->property("opened").toBool());
            QVERIFY(cycle106MenuFits(f.quick, templateMenu, output));
            QVERIFY(cycle102Click(f.quick, f.item("previewTemplate6"))); QTRY_COMPARE(f.backend.outputStyle(), 6);
            QCOMPARE(f.editor->property("text").toString(), draft);
            QCOMPARE(f.editor->property("selectionStart").toInt(), 3); QCOMPARE(f.editor->property("selectionEnd").toInt(), 8);
        }
    }
    QVERIFY(f.mode(2)); QTRY_VERIFY(!appearance->isVisible() && output->isVisible());
    QVERIFY(f.mode(0)); QTRY_VERIFY(appearance->isVisible() && !output->isVisible());
    QVERIFY(cycle102Click(f.quick, visual)); QTRY_VERIFY(f.layout->property("visualEditEnabled").toBool());
    QVERIFY(!f.preview->property("visualEditEnabled").toBool());
    QVERIFY(f.mode(0));
    QCOMPARE(f.editor->property("text").toString(), draft);
    QVERIFY(QMetaObject::invokeMethod(f.editor, "undo")); QCOMPARE(f.editor->property("text").toString(), saved);
    QVERIFY(QMetaObject::invokeMethod(f.editor, "redo")); QCOMPARE(f.editor->property("text").toString(), draft);
    QCOMPARE(cycle103Read(path), saved.toUtf8());
}

void FomawriteTest::previewHeaderTracksFileAndEditedState() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    const QString saved = QString::fromUtf8("# A different document heading\n\nA café sentence.\n");
    const QString filename = QString::fromUtf8("Very long actual café filename for preview identity.md");
    const QString path = directory.filePath(filename); QVERIFY(cycle103Write(path, saved.toUtf8()));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error)); QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    QVERIFY(f.mode(1)); QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 1);
    auto *name = f.item("previewDocumentName"), *edited = f.item("previewDocumentEdited"), *icon = f.item("previewDocumentIcon");
    QVERIFY(name && edited && icon);
    QTRY_VERIFY(name->isVisible() && icon->isVisible());
    QCOMPARE(name->property("text").toString(), filename); QVERIFY(name->property("font").value<QFont>().bold());
    QVERIFY(!edited->isVisible());
    QVERIFY(QMetaObject::invokeMethod(f.editor, "insert", Q_ARG(int, saved.size()), Q_ARG(QString, "Unsaved ending.")));
    const QString draft = f.editor->property("text").toString();
    QTRY_VERIFY(edited->isVisible()); QVERIFY(edited->property("text").toString().contains("Edited"));
    for (const char *theme : {"studio", "dark"}) {
        f.backend.setThemePreset(theme);
        for (int width : {1280, 960, 803}) {
            f.quick->resize(width, 620);
            QVERIFY(QMetaObject::invokeMethod(f.layout, "updateWidth", Q_ARG(QVariant, "preview"), Q_ARG(QVariant, 320)));
            QVERIFY(f.mode(1)); QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 1);
            QTRY_VERIFY(name->isVisible() && edited->isVisible());
            auto *header = f.item("previewHeader"); QVERIFY(header);
            QTRY_VERIFY(cycle103SceneBounds(header).contains(cycle103SceneBounds(name)));
            QTRY_VERIFY(cycle103SceneBounds(header).contains(cycle103SceneBounds(edited)));
            QCOMPARE(name->property("text").toString(), filename);
            QVERIFY(cycle103SceneBounds(name).right() <= cycle103SceneBounds(edited).left());
            QCOMPARE(f.editor->property("text").toString(), draft);
        }
    }
    QVERIFY(f.mode(2));
    auto *fullName = f.item("fullPreviewDocumentName"), *fullEdited = f.item("fullPreviewDocumentEdited");
    QVERIFY(fullName && fullEdited);
    QTRY_VERIFY(fullName->isVisible() && fullEdited->isVisible());
    QCOMPARE(fullName->property("text").toString(), filename);
    QVERIFY(f.mode(0)); QTRY_VERIFY(!name->isVisible() && !fullName->isVisible());
    QVERIFY(f.mode(1));
    f.backend.save(); QTRY_VERIFY(!f.backend.modified()); QTRY_VERIFY(!edited->isVisible());
    QCOMPARE(cycle103Read(path), draft.toUtf8());
    const QString renamed = directory.filePath("Saved under another name.md"); f.backend.saveAs(QUrl::fromLocalFile(renamed));
    QTRY_COMPARE(name->property("text").toString(), QString("Saved under another name.md"));
    QCOMPARE(cycle103Read(renamed), draft.toUtf8());
}

void FomawriteTest::compactLibraryRowsOpenActualFilesAndIndentChildren() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory; QVERIFY(QDir(directory.path()).mkpath("Research"));
    const QByteArray alpha = "# Heading deliberately differs\n\nA longer public excerpt with enough words to fill two lines in a compact library row and remain readable.\n";
    const QByteArray beta = "# Real title\n\n> - ## Quoted heading\n> - [x] Finished\n> - [ ] Next step\n\n---\n* * *\n___\nBody text\n";
    const QByteArray childText = "# Child heading\n\nNested sample content.\n";
    const QString alphaPath = directory.filePath("Alpha actual file.md"), betaPath = directory.filePath("Beta actual file.md");
    const QString childPath = directory.filePath("Research/Child actual file.md");
    QVERIFY(cycle103Write(alphaPath, alpha)); QVERIFY(cycle103Write(betaPath, beta, 1)); QVERIFY(cycle103Write(childPath, childText, 2));
    Cycle117Window f; QVERIFY2(f.start(), qPrintable(f.error));
    auto *model = qobject_cast<FileLibrary *>(f.backend.library()); QVERIFY(model);
    model->setNavigationMode(0); model->setRootFolder(QUrl::fromLocalFile(directory.path()));
    QCOMPARE(model->documentSummary(QUrl::fromLocalFile(betaPath)).value("excerpt").toString(),
             QStringLiteral("Quoted heading Finished Next step Body text"));
    QCOMPARE(cycle103Read(betaPath), beta);
    f.layout->setProperty("filesVisible", true); f.layout->setProperty("fileWidth", 340);
    auto *library = f.item("libraryPane"); QVERIFY(library);
    library->setProperty("showExcerpts", true); library->setProperty("dateMode", 1);
    QTRY_VERIFY(cycle117Entry(library, "Alpha actual file.md"));
    auto *folder = cycle117Entry(library, "Research"); QVERIFY(folder);
    QVERIFY(cycle102Click(f.quick, folder));
    QTRY_VERIFY(cycle117Entry(library, "Child actual file.md"));
    auto *alphaRow = cycle117Entry(library, "Alpha actual file.md");
    folder = cycle117Entry(library, "Research");
    auto *child = cycle117Entry(library, "Child actual file.md");
    QVERIFY(alphaRow && folder && child);
    const qreal parentPadding = folder->property("leftPadding").toReal();
    QCOMPARE(child->property("leftPadding").toReal(), parentPadding + 18);
    QCOMPARE(alphaRow->property("leftPadding").toReal(), parentPadding);
    for (bool dark : {false, true}) {
        f.backend.setThemePreset(dark ? "dark" : "studio");
        for (auto *row : {alphaRow, child}) {
            auto *title = row->findChild<QQuickItem *>("libraryEntryTitle"), *date = row->findChild<QQuickItem *>("libraryEntryDate");
            auto *excerpt = row->findChild<QQuickItem *>("libraryEntryExcerpt"), *icon = row->findChild<QQuickItem *>("libraryEntryIcon");
            QVERIFY(title && date && excerpt && icon);
            QTRY_VERIFY(icon->isVisible() && date->isVisible() && excerpt->isVisible());
            QVERIFY(title->property("font").value<QFont>().weight() >= QFont::DemiBold);
            QCOMPARE(excerpt->property("maximumLineCount").toInt(), 2);
            QCOMPARE(excerpt->property("color"), date->property("color"));
            QTRY_VERIFY(cycle103SceneBounds(title).right() < cycle103SceneBounds(date).left());
            QTRY_VERIFY(qAbs(cycle103SceneBounds(title).center().y() - cycle103SceneBounds(date).center().y()) < 2);
            QVERIFY(cycle103SceneBounds(excerpt).top() > cycle103SceneBounds(title).bottom());
            QVERIFY(row->height() <= 66);
        }
    }
    QVERIFY(cycle102Click(f.quick, alphaRow)); QTRY_COMPARE(f.backend.fileUrl(), QUrl::fromLocalFile(QFileInfo(alphaPath).canonicalFilePath()));
    QCOMPARE(f.editor->property("text").toString().toUtf8(), alpha);
    QTRY_VERIFY(cycle117Entry(library, "Child actual file.md"));
    QVERIFY(cycle102Click(f.quick, cycle117Entry(library, "Child actual file.md"))); QTRY_COMPARE(f.backend.fileUrl(), QUrl::fromLocalFile(QFileInfo(childPath).canonicalFilePath()));
    QCOMPARE(f.editor->property("text").toString().toUtf8(), childText);
    QVERIFY(!f.backend.modified()); QVERIFY(!f.editor->property("canUndo").toBool());
    QCOMPARE(cycle103Read(alphaPath), alpha); QCOMPARE(cycle103Read(betaPath), beta); QCOMPARE(cycle103Read(childPath), childText);
}
