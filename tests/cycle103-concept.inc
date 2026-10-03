namespace {
// Every fixture uses a private QSettings path established by initTestCase().
// Restore its previous contents after QML Settings and Backend have been destroyed.
struct Cycle103SettingsSnapshot {
    QVariantMap values;
    QFont originalFont = QGuiApplication::font();
    Cycle103SettingsSnapshot() {
#ifdef Q_OS_MACOS
        QGuiApplication::setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
#endif
        QSettings settings;
        for (const auto &key : settings.allKeys()) values.insert(key, settings.value(key));
        settings.clear(); settings.sync();
    }
    ~Cycle103SettingsSnapshot() {
        QGuiApplication::setFont(originalFont);
        QSettings settings; settings.clear();
        for (auto it = values.cbegin(); it != values.cend(); ++it) settings.setValue(it.key(), it.value());
        settings.sync();
    }
};
bool cycle103Write(const QString &path, const QByteArray &contents, int daysAgo = 0) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size()) return false;
    file.flush();
    return file.setFileTime(QDateTime(QDate::currentDate().addDays(-daysAgo), QTime(12, 0)), QFileDevice::FileModificationTime);
}
QByteArray cycle103Read(const QString &path) {
    QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
}
QList<QQuickItem *> cycle103VisualItems(QQuickItem *root, const QString &name) {
    QList<QQuickItem *> matches;
    for (auto *child : root->childItems()) {
        if (child->objectName() == name) matches << child;
        matches << cycle103VisualItems(child, name);
    }
    return matches;
}
QRectF cycle103SceneBounds(QQuickItem *item) { return item->mapRectToScene(item->boundingRect()); }
bool cycle103HeadersAndRulesAlign(QObject *window) {
    for (const auto &[headerName, paneName] : {
             std::pair{"organizerHeader", "organizerSlot"}, std::pair{"filesHeader", "filesSlot"},
             std::pair{"documentHeader", "editorPane"}}) {
        auto *header = window->findChild<QQuickItem *>(headerName);
        auto *pane = window->findChild<QQuickItem *>(paneName);
        if (!header || !pane) return false;
        if (QString::fromLatin1(paneName) == "editorPane" && !pane->isVisible())
            pane = window->findChild<QQuickItem *>("previewPane");
        if (!pane || pane->isVisible() != header->isVisible()) return false;
        if (!pane->isVisible()) continue;
        const auto a = cycle103SceneBounds(header), b = cycle103SceneBounds(pane);
        if (qAbs(a.left() - b.left()) > 0.01 || qAbs(a.width() - b.width()) > 0.01
            || qAbs(a.bottom() - b.top()) > 0.01) return false;
    }
    const auto rules = window->findChildren<QQuickItem *>("workspaceDivider");
    for (const char *name : {"organizerHeaderDivider", "filesHeaderDivider"}) {
        auto *headerRule = window->findChild<QQuickItem *>(name);
        if (!headerRule) return false;
        if (!headerRule->isVisible()) continue;
        const auto a = cycle103SceneBounds(headerRule);
        bool matched = false;
        for (auto *rule : rules) {
            if (!rule->isVisible()) continue;
            const auto b = cycle103SceneBounds(rule);
            if (qAbs(a.x() - b.x()) < 0.01 && qAbs(a.width() - b.width()) < 0.01
                && qAbs(a.bottom() - b.top()) < 0.01) matched = true;
        }
        if (!matched) { qWarning() << "Header rule is disconnected from body divider:" << name << a; return false; }
    }
    return true;
}
}

void FomawriteTest::referenceWorkspacePresetPreservesDraftAndOutput() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    const QString source = QString::fromUtf8("# A quieter place to write\n\nA café draft — 你好.\n");
    const QString path = directory.filePath("A quieter place to write.md");
    QVERIFY(cycle103Write(path, source.toUtf8()));
    QVariantMap savedPaneState;
    {
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *quick = qobject_cast<QQuickWindow *>(window.data());
        auto *settings = window->findChild<QObject *>("workspaceSettings");
        auto *layout = window->findChild<QObject *>("workspaceLayout");
        auto *library = window->findChild<QObject *>("libraryPane");
        auto *editor = window->findChild<QObject *>("sourceEditor");
        QVERIFY(quick && settings && layout && library && editor);
        quick->resize(1280, 900);
        QVERIFY(backend.open(QUrl::fromLocalFile(path)));
        QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, source.size()), Q_ARG(QString, QStringLiteral("An unsaved addition."))));
        const QString draft = editor->property("text").toString();
        QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 5), Q_ARG(int, 12)));
        settings->setProperty("writingAppearance", "book");
        settings->setProperty("writingSize", 24);
        settings->setProperty("toolbarVisibilityMode", 2);
        layout->setProperty("layoutMode", 2);
        layout->setProperty("organizerVisible", false);
        layout->setProperty("filesVisible", false);
        library->setProperty("showExcerpts", false);
        library->setProperty("dateMode", 0);
        library->setProperty("showFilterBar", true);
        backend.setOutputStyle(6);
        backend.setThemePreset("paper");
        QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
        QTRY_VERIFY(layout->property("effectiveOrganizerVisible").toBool());
        QTRY_VERIFY(layout->property("effectiveFilesVisible").toBool());
        QCOMPARE(layout->property("layoutMode").toInt(), 0);
        QVERIFY(!layout->property("visualEditEnabled").toBool());
        QCOMPARE(settings->property("writingAppearance").toString(), QStringLiteral("manuscript"));
        QVERIFY(!settings->property("showMarkup").toBool());
        QCOMPARE(backend.themePreset(), QStringLiteral("studio"));
        QVERIFY(library->property("showExcerpts").toBool());
        QCOMPARE(library->property("dateMode").toInt(), 1);
        QVERIFY(!library->property("showFilterBar").toBool());
        QCOMPARE(backend.outputStyle(), 6);
        QCOMPARE(backend.fileUrl(), QUrl::fromLocalFile(path));
        QCOMPARE(editor->property("text").toString(), draft);
        QCOMPARE(editor->property("selectionStart").toInt(), 5);
        QCOMPARE(editor->property("selectionEnd").toInt(), 12);
        QVERIFY(backend.modified());
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), source);
        QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
        QCOMPARE(editor->property("text").toString(), draft);
        QCOMPARE(cycle103Read(path), source.toUtf8());
        QVariant rawPaneState = window->property("workspacePaneState");
        if (rawPaneState.metaType() == QMetaType::fromType<QJSValue>()) rawPaneState = rawPaneState.value<QJSValue>().toVariant();
        savedPaneState = rawPaneState.toMap();
        backend.discardRecovery();
    }
    // The real QML settings store must keep the chosen workspace on reopening.
    QSettings().sync();
    Backend reopenedBackend;
    QQmlEngine reopenedEngine; reopenedEngine.rootContext()->setContextProperty("backend", &reopenedBackend);
    QQmlComponent reopenedComponent(&reopenedEngine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> reopened(reopenedComponent.create()); QVERIFY2(reopened, qPrintable(reopenedComponent.errorString()));
    QCOMPARE(reopened->findChild<QObject *>("workspaceSettings")->property("writingAppearance").toString(), QStringLiteral("manuscript"));
    // Production restores the per-window checkpoint separately from shared appearance.
    QVERIFY(QMetaObject::invokeMethod(reopened.data(), "restoreWorkspacePaneState", Q_ARG(QVariant, savedPaneState)));
    auto *reopenedLayout = reopened->findChild<QObject *>("workspaceLayout");
    QCOMPARE(reopenedLayout->property("layoutMode").toInt(), 0);
    QCOMPARE(reopenedLayout->property("organizerWidth").toDouble(), 184.0);
    QCOMPARE(reopenedLayout->property("fileWidth").toDouble(), 232.0);
    QVERIFY(reopened->findChild<QObject *>("libraryPane")->property("showExcerpts").toBool());
    QCOMPARE(reopenedBackend.outputStyle(), 6);
    QCOMPARE(cycle103Read(path), source.toUtf8());
    reopenedBackend.discardRecovery();
}

void FomawriteTest::referenceWorkspaceCompositionMatchesConcept() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    QVERIFY(QDir(directory.path()).mkpath("Documents/Writing"));
    const QString documentsRoot = directory.filePath("Documents");
    const QString writing = QDir(documentsRoot).filePath("Writing");
    const QStringList names{"A quieter place to write", "Notes for the journey", "The first draft"};
    const QList<QByteArray> documents{
        "# A quieter place to write\n\n***Begin with the page. Give the words room to breathe, and let everything around them settle into the background.***\n\n"
        "A good writing space has a clear rhythm: the folders help you find your way, the document list keeps your work close, and the page gives your ideas somewhere to take shape.\n\n"
        "The tools should be easy to find when you need them. When you do not, the writing should hold your attention.\n\n"
        "Start with one sentence. Then another. There is time to find the right shape as you go.\n",
        "# Notes for the journey\n\nA few observations from the road, and a place to keep them.\n",
        "# The first draft\n\nKeep moving. The shape of the story comes later.\n"};
    QList<QUrl> urls;
    for (int index = 0; index < names.size(); ++index) {
        const QString path = QDir(writing).filePath(names[index] + ".md");
        QVERIFY(cycle103Write(path, documents[index], index)); urls << QUrl::fromLocalFile(QFileInfo(path).canonicalFilePath());
    }
    Backend backend;
    auto *libraryModel = qobject_cast<FileLibrary *>(backend.library()); QVERIFY(libraryModel);
    libraryModel->addLocation(QUrl::fromLocalFile(documentsRoot));
    for (const QString &name : {"Writing", "Projects", "Reference", "Journal", "Ideas"}) {
        QVERIFY(QDir(documentsRoot).mkpath(name));
        QVERIFY(libraryModel->addFavorite(QUrl::fromLocalFile(QDir(documentsRoot).filePath(name))));
    }
    libraryModel->setRootFolder(QUrl::fromLocalFile(writing));
    libraryModel->setSortMode(1); libraryModel->setAscending(false);
    libraryModel->saveSearch("Drafts", false);
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    quick->resize(1280, 900); quick->show();
    QVERIFY(backend.open(urls[0]));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *library = window->findChild<QQuickItem *>("libraryPane");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *chip = window->findChild<QQuickItem *>("editorWordCountChip");
    auto *title = window->findChild<QQuickItem *>("topChromeTitle");
    auto *count = window->findChild<QQuickItem *>("libraryDocumentCount");
    QVERIFY(layout && library && editor && chip && title && count);
    QTRY_VERIFY(cycle103HeadersAndRulesAlign(window.data()));
    QTRY_VERIFY(chip->isVisible());
    QCOMPARE(title->property("text").toString(), names[0] + ".md");
    QCOMPARE(count->property("text").toString(), QStringLiteral("3 documents"));
    const auto palette = backend.palette();
    QVERIFY(palette.value("organizer") != palette.value("library"));
    QVERIFY(palette.value("library") != palette.value("page"));
    QVERIFY(palette.value("organizer") != palette.value("page"));
    QTRY_COMPARE(cycle103VisualItems(library, "libraryEntry").size(), 3);
    const auto cards = cycle103VisualItems(library, "libraryEntry");
    QQuickItem *secondCard = nullptr;
    for (auto *card : cards) {
        auto *date = card->findChild<QQuickItem *>("libraryEntryDate");
        auto *label = card->findChild<QQuickItem *>("libraryEntryTitle");
        auto *excerpt = card->findChild<QQuickItem *>("libraryEntryExcerpt");
        QVERIFY(date && label && excerpt);
        QVERIFY(date->isVisible() && label->isVisible() && excerpt->isVisible());
        QVERIFY(!date->property("text").toString().isEmpty());
        QVERIFY(!label->property("text").toString().endsWith(".md"));
        QVERIFY(!excerpt->property("text").toString().isEmpty());
        const auto a = date->mapRectToItem(card, date->boundingRect());
        const auto b = label->mapRectToItem(card, label->boundingRect());
        const auto c = excerpt->mapRectToItem(card, excerpt->boundingRect());
        QVERIFY(a.bottom() < b.top()); QVERIFY(b.bottom() < c.top());
        QVERIFY(c.bottom() < card->height());
        if (label->property("text").toString() == names[1]) secondCard = card;
    }
    for (const char *name : {"collapseOrganizerButton", "collapseFilesButton", "newDocumentButton", "documentSearchButton", "topChromeLibraryButton"}) {
        auto *button = window->findChild<QQuickItem *>(name); QVERIFY(button && button->isVisible());
        QCOMPARE(button->width(), button->height());
        QVERIFY(button->width() >= 32);
    }
    // Card previews are real local-file actions, not a separate presentation layer.
    QVERIFY(secondCard && cycle102Click(quick, secondCard));
    QTRY_COMPARE(backend.fileUrl(), urls[1]);
    QCOMPARE(editor->property("text").toString().toUtf8(), documents[1]);
    QVERIFY(!backend.modified());
    auto *back = window->findChild<QQuickItem *>("documentBackButton");
    QVERIFY(back && cycle102Click(quick, back));
    QTRY_COMPARE(backend.fileUrl(), urls[0]);
    QCOMPARE(editor->property("text").toString().toUtf8(), documents[0]);
    // Move keyboard focus off the selected card for the normal resting-state captures.
    editor->forceActiveFocus();
    editor->setProperty("cursorPosition", 0);
    QVERIFY(cycle102Capture(quick, "cycle103-source-wide"));
    // The divider is visually one pixel but must accept a drag beside that pixel.
    QVariant originalPaneState = window->property("workspacePaneState");
    if (originalPaneState.metaType() == QMetaType::fromType<QJSValue>()) originalPaneState = originalPaneState.value<QJSValue>().toVariant();
    auto *organizerHeaderRule = window->findChild<QQuickItem *>("organizerHeaderDivider");
    auto *organizerPane = window->findChild<QQuickItem *>("organizerSlot");
    QVERIFY(organizerHeaderRule && organizerPane);
    const qreal originalOrganizerWidth = organizerPane->width();
    const QPoint besideDivider(qRound(cycle103SceneBounds(organizerHeaderRule).center().x()) + 3, 300);
    QTest::mousePress(quick, Qt::LeftButton, Qt::NoModifier, besideDivider);
    QTest::mouseMove(quick, besideDivider + QPoint(36, 0), 40);
    QTest::mouseRelease(quick, Qt::LeftButton, Qt::NoModifier, besideDivider + QPoint(36, 0));
    QTRY_VERIFY(organizerPane->width() >= originalOrganizerWidth + 24);
    QCOMPARE(editor->property("text").toString().toUtf8(), documents[0]);
    QVERIFY(cycle103HeadersAndRulesAlign(window.data()));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "restoreWorkspacePaneState", Q_ARG(QVariant, originalPaneState)));
    QTRY_COMPARE(organizerPane->width(), originalOrganizerWidth);
    quick->resize(980, 760);
    QTRY_VERIFY(cycle103HeadersAndRulesAlign(window.data()));
    QVERIFY(cycle102Capture(quick, "cycle103-source-reference"));
    window->setProperty("searchOpen", true);
    QTRY_VERIFY(window->findChild<QQuickItem *>("searchField")->isVisible());
    QVERIFY(cycle102Capture(quick, "cycle103-source-find"));
    window->setProperty("searchOpen", false);
    backend.setThemePreset("dark");
    QVERIFY(cycle102Capture(quick, "cycle103-source-dark"));
    backend.setThemePreset("studio");
    quick->resize(680, 760);
    QTRY_VERIFY(!layout->property("effectiveOrganizerVisible").toBool());
    QTRY_VERIFY(window->findChild<QQuickItem *>("restoreOrganizerButton")->isVisible());
    QTRY_VERIFY(cycle103HeadersAndRulesAlign(window.data()));
    QVERIFY(cycle102Capture(quick, "cycle103-source-narrow"));
    quick->resize(1280, 900);
    backend.setOutputStyle(6);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, "preview")));
    QTRY_VERIFY(window->findChild<QQuickItem *>("previewPane")->isVisible());
    QTRY_VERIFY(cycle103HeadersAndRulesAlign(window.data()));
    QVERIFY(cycle102Capture(quick, "cycle103-preview-wide"));
    QCOMPARE(backend.outputStyle(), 6);
    QCOMPARE(editor->property("text").toString().toUtf8(), documents[0]);
    QVERIFY(!backend.modified());
    for (int index = 0; index < urls.size(); ++index) QCOMPARE(cycle103Read(urls[index].toLocalFile()), documents[index]);
    backend.discardRecovery();
}

void FomawriteTest::libraryDocumentCardsReadSourceWithoutChangingFiles() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory, outside;
    const QByteArray source = QString::fromUtf8("---\ntitle: Private metadata\n---\n\n# Café **notes** 你好 ###\n\nA [local link](other.md), `code` and *emphasis*.\n").toUtf8();
    const QString path = directory.filePath("Notes.md");
    QVERIFY(cycle103Write(path, source));
    QVERIFY(cycle103Write(directory.filePath("Plain.txt"), "No heading here.\n"));
    QVERIFY(cycle103Write(directory.filePath("Long.md"), "# Long\n\n" + QByteArray(3000, 'x') + "LATE_SENTINEL"));
    QVERIFY(cycle103Write(outside.filePath("Outside.md"), "# Outside\nPrivate material.\n"));
    QVERIFY(cycle103Write(directory.filePath("Image.png"), "Not text content"));
    QVERIFY(cycle103Write(directory.filePath("Fence.md"), "```md\n# Not a document title\n```\n"));
    const QByteArray malformed("# Raw\n\nA broken byte: \xff\n");
    QVERIFY(cycle103Write(directory.filePath("Malformed.md"), malformed));
    const QByteArray boundary = (QString("# Emoji\n\n") + QString(199, 'x') + QString::fromUtf8("😀")).toUtf8();
    QVERIFY(cycle103Write(directory.filePath("Emoji.md"), boundary));
    FileLibrary library; library.setRootFolder(QUrl::fromLocalFile(directory.path()));
    const auto summary = library.documentSummary(QUrl::fromLocalFile(path));
    QCOMPARE(summary.value("title").toString(), QString::fromUtf8("Café notes 你好"));
    QCOMPARE(summary.value("excerpt").toString(), QStringLiteral("A local link, code and emphasis."));
    QCOMPARE(library.documentSummary(QUrl::fromLocalFile(directory.filePath("Plain.txt"))).value("title").toString(), QStringLiteral("Plain"));
    const auto longExcerpt = library.documentSummary(QUrl::fromLocalFile(directory.filePath("Long.md"))).value("excerpt").toString();
    QVERIFY(!longExcerpt.isEmpty() && longExcerpt.size() <= 200);
    QVERIFY(!longExcerpt.contains("LATE_SENTINEL"));
    QVERIFY(library.documentSummary(QUrl::fromLocalFile(outside.filePath("Outside.md"))).isEmpty());
    QVERIFY(library.documentSummary(QUrl::fromLocalFile(directory.filePath("Image.png"))).isEmpty());
    QVERIFY(library.documentSummary(QUrl("https://example.com/Notes.md")).isEmpty());
    QCOMPARE(library.documentSummary(QUrl::fromLocalFile(directory.filePath("Fence.md"))).value("title").toString(), QStringLiteral("Fence"));
    QVERIFY(!library.documentSummary(QUrl::fromLocalFile(directory.filePath("Malformed.md"))).value("excerpt").toString().isEmpty());
    const QString emojiExcerpt = library.documentSummary(QUrl::fromLocalFile(directory.filePath("Emoji.md"))).value("excerpt").toString();
    QVERIFY(!emojiExcerpt.isEmpty() && !emojiExcerpt.back().isHighSurrogate());
    QCOMPARE(cycle103Read(directory.filePath("Malformed.md")), malformed);
    QCOMPARE(cycle103Read(directory.filePath("Emoji.md")), boundary);
    QCOMPARE(cycle103Read(path), source);
}
