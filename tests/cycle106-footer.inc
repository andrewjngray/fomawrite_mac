namespace {
bool cycle118ReadPreview(QObject *window) {
    return QMetaObject::invokeMethod(window, "selectWritingMode", Q_ARG(QVariant, "preview"));
}
bool cycle118Single(QObject *window) {
    return QMetaObject::invokeMethod(window, "setDocumentView", Q_ARG(QVariant, 0));
}

// Native QA uses targeted Qt events and never needs to take the user's keyboard.
void cycle106KeepNativeFocus(QQuickWindow *window) {
#ifdef Q_OS_MACOS
    if (QGuiApplication::platformName() == "cocoa") window->setFlag(Qt::WindowDoesNotAcceptFocus);
#else
    Q_UNUSED(window);
#endif
}
bool cycle106MenuFits(QQuickWindow *window, QObject *menu, QQuickItem *anchor) {
    auto *parent = qvariant_cast<QQuickItem *>(menu->property("parent"));
    if (!parent || !menu->property("opened").toBool()) return false;
    const QRectF bounds = parent->mapRectToScene(QRectF(menu->property("x").toReal(), menu->property("y").toReal(),
                                                       menu->property("width").toReal(), menu->property("height").toReal()));
    const auto available = cycle103SceneBounds(window->contentItem());
    const auto trigger = cycle103SceneBounds(anchor);
    const bool fits = available.adjusted(-0.1, -0.1, 0.1, 0.1).contains(bounds)
        && bounds.bottom() <= trigger.top() - 3;
    if (!fits) qWarning() << "Footer menu outside usable area:" << menu->objectName() << bounds << available << trigger;
    return fits;
}
bool cycle106FootersAlign(QObject *window) {
    auto *document = window->findChild<QQuickItem *>("documentFooter");
    if (!document || !document->isVisible()) return false;
    const auto baseline = cycle103SceneBounds(document);
    if (qAbs(baseline.height() - 52) > 0.1) return false;
    for (const auto &[footerName, paneName] : {
             std::pair{"organizerFooter", "organizerSlot"}, std::pair{"libraryFooter", "filesSlot"}}) {
        auto *pane = window->findChild<QQuickItem *>(paneName);
        auto *footer = pane ? pane->findChild<QQuickItem *>(footerName) : nullptr;
        if (!footer || !pane) return false;
        if (!pane->isVisible()) continue;
        const auto bar = cycle103SceneBounds(footer), area = cycle103SceneBounds(pane);
        if (!footer->isVisible() || qAbs(bar.left() - area.left()) > 0.1
                || qAbs(bar.right() - area.right()) > 0.1 || qAbs(bar.bottom() - area.bottom()) > 0.1
                || qAbs(bar.height() - 52) > 0.1 || qAbs(bar.top() - baseline.top()) > 0.1) {
            qWarning() << "Footer baselines differ:" << footerName << bar << baseline;
            return false;
        }
    }
    return true;
}
bool cycle106FooterControlsFit(QObject *window) {
    QList<QPair<QString, QRectF>> visible;
    for (const char *name : {"libraryFilter", "sourceModeButton", "sourceAppearanceButton", "toolbarStatistic",
                            "previewTemplateButton", "visualEditToggle", "previewSplitButton", "singleModeButton",
                            "webPublishingButton", "pdfPublishingButton"}) {
        auto *document = window->findChild<QQuickItem *>("documentFooter");
        auto *item = document ? document->findChild<QQuickItem *>(name) : nullptr;
        if (QString::fromLatin1(name) == "libraryFilter") item = window->findChild<QQuickItem *>(name);
        if (!item || !item->isVisible()) continue;
        auto *footer = item->parentItem();
        while (footer && !QStringList{"libraryFooter", "documentFooter"}.contains(footer->objectName()))
            footer = footer->parentItem();
        if (!footer || item->width() <= 0 || item->height() < 32) return false;
        const auto bounds = cycle103SceneBounds(item);
        if (!cycle103SceneBounds(footer).adjusted(-0.1, -0.1, 0.1, 0.1).contains(bounds)) return false;
        for (const auto &[other, rectangle] : visible) {
            if (bounds.intersected(rectangle).width() > 0.1 && bounds.intersected(rectangle).height() > 0.1) {
                qWarning() << "Overlapping footer controls:" << name << bounds << other << rectangle;
                return false;
            }
        }
        visible.append({QString::fromLatin1(name), bounds});
    }
    return true;
}
}

void FomawriteTest::footerModesAndTemplatesPreserveDraftAndUndo() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    const QByteArray source = QString::fromUtf8("# Footer controls\n\nA **bold** café draft — 你好.\n\n- Keep the Markdown.\n").toUtf8();
    const QString path = directory.filePath("Footer controls.md");
    QVERIFY(cycle103Write(path, source));
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    auto *mode = footer ? footer->findChild<QQuickItem *>("sourceModeButton") : nullptr;
    auto *templateButton = footer ? footer->findChild<QQuickItem *>("previewTemplateButton") : nullptr;
    auto *templateMenu = window->findChild<QObject *>("previewTemplateMenu");
    QVERIFY(quick && layout && editor && pane && mode && templateButton && templateMenu);
    Cycle106MouseInputIsolation mouseInput(quick);
    cycle106KeepNativeFocus(quick);
    quick->resize(1440, 800); quick->show();
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QTest::qWaitForWindowExposed(quick));
    QTRY_VERIFY(cycle106FootersAlign(window.data()));
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, editor->property("text").toString().size()),
                                     Q_ARG(QString, QStringLiteral("Unsaved addition."))));
    const QString draft = editor->property("text").toString();

    auto *split = footer->findChild<QQuickItem *>("previewSplitButton");
    auto *single = footer->findChild<QQuickItem *>("singleModeButton");
    auto *visual = footer->findChild<QQuickItem *>("visualEditToggle");
    auto *visualPane = window->findChild<QQuickItem *>("visualEditorPane");
    QVERIFY(split && single && visual && visualPane);
    for (int pass = 0; pass < 2; ++pass) {
        for (auto *arrangement : {single, split}) {
            QVERIFY(cycle102Click(quick, arrangement));
            const int expected = arrangement == single ? 0 : 1;
            QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), expected);
            for (auto *editing : {visual, visual, mode, mode}) {
                QVERIFY(cycle102Click(quick, editing));
                QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), expected);
                QTRY_COMPARE(layout->property("visualEditEnabled").toBool(), editing == visual);
                QTRY_COMPARE(visualPane->isVisible(), editing == visual);
                QTRY_COMPARE(editor->isVisible(), editing == mode);
                QCOMPARE(pane->isVisible(), expected == 1);
                QVERIFY(!pane->property("visualEditEnabled").toBool());
                QVERIFY(editing->property("checked").toBool());
                QCOMPARE(editor->property("text").toString(), draft);
            }
        }
    }
    QVERIFY(cycle118ReadPreview(window.data()));
    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 2);

    // Every built-in template and Custom must be reachable from the actual footer.
    for (int style : {0, 1, 2, 4, 5, 6, 7, 3}) {
        QVERIFY(cycle102Click(quick, templateButton));
        QTRY_VERIFY(templateMenu->property("opened").toBool());
        QVERIFY(cycle106MenuFits(quick, templateMenu, templateButton));
        QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>(QString("previewTemplate%1").arg(style))));
        QTRY_COMPARE(backend.outputStyle(), style);
        QTRY_VERIFY(!templateMenu->property("visible").toBool());
        QVERIFY(templateButton->property("text").toString().contains(backend.outputTemplateName()));
        auto *selected = window->findChild<QQuickItem *>(QString("previewTemplate%1").arg(style));
        QVERIFY(selected->property("checked").toBool());
        QVERIFY(cycle102Click(quick, templateButton));
        QTRY_VERIFY(templateMenu->property("opened").toBool());
        QVERIFY(cycle102Click(quick, selected));
        QVERIFY(selected->property("checked").toBool());
        QCOMPARE(backend.outputStyle(), style);
        QCOMPARE(editor->property("text").toString(), draft);
        QVERIFY(backend.modified());
    }
    QVERIFY(cycle102Click(quick, templateButton));
    QTRY_VERIFY(templateMenu->property("opened").toBool());
    QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>("previewTemplateCustomLoad")));
    auto *custom = window->findChild<QObject *>("outputStyleDialog"); QVERIFY(custom);
    QTRY_VERIFY(custom->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(custom, "close"));
    QTRY_VERIFY(!custom->property("visible").toBool());
    QCOMPARE(editor->property("text").toString(), draft);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "undoEditing"));
    QCOMPARE(editor->property("text").toString(), QString::fromUtf8(source));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "redoEditing"));
    QCOMPARE(editor->property("text").toString(), draft);
    QCOMPARE(cycle103Read(path), source);
}

void FomawriteTest::footerAppearanceAndStatisticsWorkByClick() {
    Cycle103SettingsSnapshot isolatedSettings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *settings = window->findChild<QObject *>("workspaceSettings");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *appearance = window->findChild<QQuickItem *>("sourceAppearanceButton");
    auto *menu = window->findChild<QObject *>("sourceAppearanceMenu");
    QVERIFY(quick && settings && editor && appearance && menu);
    cycle106KeepNativeFocus(quick);
    quick->resize(960, 520); quick->show();
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    auto *layout = window->findChild<QObject *>("workspaceLayout"); QVERIFY(layout);
    layout->setProperty("organizerVisible", false);
    layout->setProperty("filesVisible", false);
    const QString source = QString::fromUtf8("# A quieter footer\n\nRead a café draft — 你好.\n");
    editor->setProperty("text", source);
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, source.size()), Q_ARG(QString, QStringLiteral("Unsaved addition."))));
    const QString draft = editor->property("text").toString();
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    backend.setOutputStyle(6);
    QVERIFY(QTest::qWaitForWindowExposed(quick));
    QTRY_VERIFY(cycle106FootersAlign(window.data()));
    const auto originalFont = editor->property("font").value<QFont>();
    QSet<QString> families;
    for (const auto &[object, value] : {
             std::pair{"sourceAppearanceEditorial", "editorial"}, std::pair{"sourceAppearanceBook", "book"},
             std::pair{"sourceAppearanceManuscript", "manuscript"}}) {
        QVERIFY(cycle102Click(quick, appearance));
        QTRY_VERIFY(menu->property("opened").toBool());
        QVERIFY(cycle106MenuFits(quick, menu, appearance));
        QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>(object)));
        QTRY_COMPARE(settings->property("writingAppearance").toString(), QString::fromLatin1(value));
        QTRY_VERIFY(!menu->property("visible").toBool());
        auto *selected = window->findChild<QQuickItem *>(object);
        QVERIFY(selected->property("checked").toBool());
        QVERIFY(cycle102Click(quick, appearance));
        QTRY_VERIFY(menu->property("opened").toBool());
        QVERIFY(cycle102Click(quick, selected));
        QVERIFY(selected->property("checked").toBool());
        QCOMPARE(settings->property("writingAppearance").toString(), QString::fromLatin1(value));
        families.insert(editor->property("font").value<QFont>().family());
        QCOMPARE(editor->property("text").toString(), draft);
        QCOMPARE(backend.outputStyle(), 6);
    }
    QCOMPARE(families.size(), 3);
    QCOMPARE(editor->property("font").value<QFont>(), originalFont);
    for (const auto &[object, size] : {
             std::pair{"sourceAppearanceLarger", 110}, std::pair{"sourceAppearanceSmaller", 100},
             std::pair{"sourceAppearanceSmaller", 90}, std::pair{"sourceAppearanceReset", 100}}) {
        QVERIFY(cycle102Click(quick, appearance));
        QTRY_VERIFY(menu->property("opened").toBool());
        QVERIFY(cycle106MenuFits(quick, menu, appearance));
        QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>(object)));
        QTRY_COMPARE(settings->property("sourceZoom").toInt(), size);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), qRound(originalFont.pixelSize() * size / 100.0));
    }
    for (const auto &[size, disabled] : {std::pair{75, "sourceAppearanceSmaller"}, std::pair{200, "sourceAppearanceLarger"}}) {
        settings->setProperty("sourceZoom", size);
        QVERIFY(cycle102Click(quick, appearance));
        QTRY_VERIFY(menu->property("opened").toBool());
        auto *item = window->findChild<QQuickItem *>(disabled); QVERIFY(item);
        QVERIFY(!item->isEnabled());
        QVERIFY(QMetaObject::invokeMethod(menu, "close"));
        QTRY_VERIFY(!menu->property("visible").toBool());
    }
    settings->setProperty("sourceZoom", 100);
    settings->setProperty("toolbarMode", 1);
    auto *statistics = window->findChild<QQuickItem *>("toolbarStatistic"); QVERIFY(statistics);
    QVERIFY(cycle102Click(quick, statistics));
    auto *dialog = window->findChild<QObject *>("statisticsDialog"); QVERIFY(dialog);
    QTRY_VERIFY(dialog->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
    QTRY_VERIFY(!dialog->property("visible").toBool());
    QCOMPARE(editor->property("text").toString(), draft);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "undoEditing"));
    QCOMPARE(editor->property("text").toString(), source);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "redoEditing"));
    QCOMPARE(editor->property("text").toString(), draft);
}

void FomawriteTest::footerControlsAndMenusRemainReachableWhenResized() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    QVERIFY(cycle103Write(directory.filePath("Notes.md"), "# Notes\n\nA calm place to write.\n"));
    QVERIFY(cycle103Write(directory.filePath("Plan.md"), "# Plan\n\nKeep the controls within reach.\n"));
    Backend backend;
    auto *libraryModel = qobject_cast<FileLibrary *>(backend.library()); QVERIFY(libraryModel);
    libraryModel->setRootFolder(QUrl::fromLocalFile(directory.path()));
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *library = window->findChild<QQuickItem *>("libraryPane");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    auto *appearance = window->findChild<QQuickItem *>("sourceAppearanceButton");
    auto *appearanceMenu = window->findChild<QObject *>("sourceAppearanceMenu");
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    auto *templateButton = footer ? footer->findChild<QQuickItem *>("previewTemplateButton") : nullptr;
    auto *templateMenu = window->findChild<QObject *>("previewTemplateMenu");
    QVERIFY(quick && layout && library && editor && pane && appearance && appearanceMenu && templateButton && templateMenu);
    cycle106KeepNativeFocus(quick);
    quick->resize(1440, 800); quick->show();
    QVERIFY(backend.open(QUrl::fromLocalFile(directory.filePath("Notes.md"))));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    library->setProperty("showFilterBar", true);
    auto *filter = window->findChild<QQuickItem *>("libraryFilter"); QVERIFY(filter);
    QTRY_VERIFY(filter->isVisible());
    QTest::qWait(30);
    QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier, cycle103SceneBounds(filter).center().toPoint());
    QTest::keyClick(quick, Qt::Key_P);
    QTRY_COMPARE(libraryModel->filter(), QStringLiteral("p"));
    QTRY_COMPARE(cycle103VisualItems(library, "libraryEntry").size(), 1);
    QTest::keyClick(quick, Qt::Key_Backspace);
    QTRY_COMPARE(libraryModel->filter(), QString());
    QTRY_COMPARE(cycle103VisualItems(library, "libraryEntry").size(), 2);
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    backend.setOutputStyle(1);
    for (bool dark : {false, true}) {
        backend.setThemePreset(dark ? "dark" : "studio");
        QCOMPARE(backend.darkMode(), dark);
        QCOMPARE(QColor(backend.palette().value("page").toString()).lightness() < 128, dark);
        for (int mode : {0, 1, 2}) {
            layout->setProperty("layoutMode", mode);
            layout->setProperty("visualEditEnabled", false);
            for (int width : {1440, 960, 720}) {
                quick->resize(width, width == 1440 ? 800 : 520);
                layout->setProperty("previewWidth", 320);
                QTest::qWait(50); // Cocoa applies the native resize asynchronously.
                QTRY_COMPARE(quick->size(), QSize(width, width == 1440 ? 800 : 520));
                QTRY_VERIFY(cycle106FootersAlign(window.data()));
                QTRY_VERIFY(cycle106FooterControlsFit(window.data()));
                if (appearance->isVisible()) {
                    QVERIFY(cycle102Click(quick, appearance));
                    QTRY_VERIFY(appearanceMenu->property("opened").toBool());
                    QVERIFY(cycle106MenuFits(quick, appearanceMenu, appearance));
                    if (!dark && mode == 0 && width == 960)
                        QVERIFY(cycle102Capture(quick, "cycle106-appearance-short"));
                    QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>("sourceAppearanceManuscript")));
                    QTRY_VERIFY(!appearanceMenu->property("visible").toBool());
                }
                if (templateButton->isVisible()) {
                    QVERIFY(cycle102Click(quick, templateButton));
                    QTRY_VERIFY(templateMenu->property("opened").toBool());
                    QVERIFY(cycle106MenuFits(quick, templateMenu, templateButton));
                    QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>("previewTemplate1")));
                    QTRY_VERIFY(!templateMenu->property("visible").toBool());
                }
                if (width == 1440 || (mode == 1 && width == 960))
                    QVERIFY(cycle102Capture(quick, QString("cycle106-footer-%1-%2-%3").arg(mode).arg(width).arg(dark ? "dark" : "light")));
            }
        }
        // Visual Edit keeps the same Source return segment and view capsule.
        quick->resize(960, 520);
        QTest::qWait(50);
        layout->setProperty("layoutMode", 1);
        layout->setProperty("visualEditEnabled", true);
        QTRY_VERIFY(cycle106FootersAlign(window.data()));
        QTRY_VERIFY(cycle106FooterControlsFit(window.data()));
        QVERIFY(cycle102Click(quick, footer->findChild<QQuickItem *>("singleModeButton")));
        QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
        QTRY_VERIFY(layout->property("visualEditEnabled").toBool());
        QTRY_VERIFY(cycle106FootersAlign(window.data()));
        QTRY_VERIFY(cycle106FooterControlsFit(window.data()));
        QVERIFY(cycle102Click(quick, footer->findChild<QQuickItem *>("previewSplitButton")));
        QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 1);
        QVERIFY(cycle102Click(quick, footer->findChild<QQuickItem *>("sourceModeButton")));
        QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 1);
        QTRY_VERIFY(!layout->property("visualEditEnabled").toBool());
        QTRY_VERIFY(cycle106FootersAlign(window.data()));
        QTRY_VERIFY(cycle106FooterControlsFit(window.data()));
    }
    // Errors remain visible after collapsing navigation in full Preview, above
    // the footer controls rather than covering their hit targets.
    layout->setProperty("organizerVisible", false);
    layout->setProperty("filesVisible", false);
    layout->setProperty("layoutMode", 2);
    QVERIFY(!backend.open(QUrl::fromLocalFile(directory.filePath("Missing.md"))));
    auto *notice = window->findChild<QQuickItem *>("navigationNotice"); QVERIFY(notice);
    QTRY_VERIFY(notice->isVisible());
    QCOMPARE(window->property("navigationNotice").toString(), backend.status());
    QVERIFY(footer);
    QVERIFY(cycle103SceneBounds(notice).bottom() + 9 <= cycle103SceneBounds(footer).top());
    QQuickItem *dismiss = nullptr;
    for (auto *child : notice->findChildren<QQuickItem *>())
        if (child->property("hint").toString() == "Dismiss message") dismiss = child;
    QVERIFY(dismiss && cycle102Click(quick, dismiss));
    QTRY_VERIFY(!notice->isVisible());
    QCOMPARE(cycle103Read(directory.filePath("Notes.md")), QByteArray("# Notes\n\nA calm place to write.\n"));
    QVERIFY(!backend.modified());
}
