namespace {
QMap<QString, QRectF> cycle107ViewBounds(QQuickItem *footer) {
    QMap<QString, QRectF> result;
    for (const char *name : {"visualEditToggle", "sourceModeButton", "previewSplitButton", "previewFullButton"}) {
        auto *button = footer->findChild<QQuickItem *>(name);
        if (!button || !button->isVisible()) return {};
        result.insert(QString::fromLatin1(name), cycle103SceneBounds(button));
    }
    return result;
}
bool cycle107SameBounds(const QMap<QString, QRectF> &actual, const QMap<QString, QRectF> &expected) {
    if (actual.size() != 4 || expected.size() != 4) return false;
    for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
        const auto found = actual.value(it.key());
        if (qAbs(found.left() - it.value().left()) > 0.1 || qAbs(found.top() - it.value().top()) > 0.1
                || qAbs(found.width() - it.value().width()) > 0.1 || qAbs(found.height() - it.value().height()) > 0.1) {
            return false;
        }
    }
    return true;
}
bool cycle107ViewLabelsReadable(QQuickItem *footer) {
    for (const char *name : {"visualEditToggle", "sourceModeButton", "previewSplitButton", "previewFullButton", "footerStylesButton"}) {
        auto *button = footer->findChild<QQuickItem *>(name);
        if (!button || !button->isVisible()) continue;
        auto *label = button->findChild<QQuickItem *>("footerButtonLabel");
        if (!label || label->property("truncated").toBool()) return false;
    }
    return true;
}
bool cycle107ViewChecked(QQuickItem *footer, QObject *layout) {
    const int mode = layout->property("effectiveLayoutMode").toInt();
    for (const auto &[name, expectedMode] : {std::pair{"sourceModeButton", 0}, std::pair{"previewSplitButton", 1}, std::pair{"previewFullButton", 2}}) {
        auto *button = footer->findChild<QQuickItem *>(name);
        if (!button || button->property("checked").toBool() != (mode == expectedMode)) return false;
    }
    auto *visual = footer->findChild<QQuickItem *>("visualEditToggle");
    return visual && visual->property("checked").toBool() == layout->property("visualEditEnabled").toBool();
}
}

void FomawriteTest::footerViewControlsStayStationaryAcrossModes() {
    Cycle103SettingsSnapshot isolatedSettings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    QVERIFY(quick && layout && footer && editor && pane);
    auto *source = footer->findChild<QQuickItem *>("sourceModeButton");
    auto *split = footer->findChild<QQuickItem *>("previewSplitButton");
    auto *full = footer->findChild<QQuickItem *>("previewFullButton");
    auto *visual = footer->findChild<QQuickItem *>("visualEditToggle");
    auto *styles = footer->findChild<QQuickItem *>("footerStylesButton");
    auto *appearance = footer->findChild<QQuickItem *>("sourceAppearanceButton");
    auto *output = footer->findChild<QQuickItem *>("previewTemplateButton");
    QVERIFY(source && split && full && visual && styles && appearance && output);
    cycle106KeepNativeFocus(quick);
    Cycle106MouseInputIsolation mouseInput(quick);
    quick->show();
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    editor->setProperty("text", QStringLiteral("# Steady controls\n\nThe view changes; its controls stay within reach.\n"));
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QTest::qWaitForWindowExposed(quick));

    for (bool dark : {false, true}) {
        backend.setThemePreset(dark ? "dark" : "studio");
        for (int width : {1440, 1280, 960, 739, 720}) {
            quick->resize(width, 700);
            QTest::qWait(60);
            QTRY_COMPARE(quick->size(), QSize(width, 700));
            QVERIFY(cycle102Click(quick, source));
            QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
            QTRY_VERIFY(cycle106FooterControlsFit(window.data()));
            const auto initial = cycle107ViewBounds(footer);
            QCOMPARE(initial.size(), 4);
            const QRectF initialFooter = cycle103SceneBounds(footer);
            const QRectF initialAppearance = cycle103SceneBounds(appearance);
            const QRectF initialOutput = cycle103SceneBounds(output);
            auto *viewGroup = footer->findChild<QQuickItem *>("documentViewControls"); QVERIFY(viewGroup);
            QVERIFY(qAbs(cycle103SceneBounds(viewGroup).right() - (width - 12)) < 0.1);
            QVERIFY(qAbs(initial.value("previewFullButton").right() - (width - 13)) < 0.1);
            QCOMPARE(split->isEnabled(), width >= 803);
            // Direct Source remains available in every mode; there is no second
            // source-only menu or swapped return button to search for.
            QCOMPARE(source->property("text").toString(), QStringLiteral("Source"));
            QVERIFY(!window->findChild<QObject *>("sourceModeMenu"));
            QVERIFY(!pane->property("showFooter").toBool());
            for (int pass = 0; pass < 3; ++pass) {
                QList<QPair<QQuickItem *, int>> steps{{full, 2}, {source, 0}, {visual, 2}, {visual, 2}};
                if (split->isEnabled()) steps.append({split, 1});
                steps.append({visual, split->isEnabled() ? 1 : 2});
                steps.append({full, 2});
                if (split->isEnabled()) steps.append({split, 1});
                steps.append({source, 0});
                for (int step = 0; step < steps.size(); ++step) {
                    QVERIFY(cycle102Click(quick, steps[step].first));
                    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), steps[step].second);
                    QTRY_VERIFY(cycle107SameBounds(cycle107ViewBounds(footer), initial));
                    QTRY_VERIFY(cycle107ViewChecked(footer, layout));
                    QTRY_VERIFY2(cycle107ViewLabelsReadable(footer), "Footer view labels must not be truncated.");
                    QTRY_VERIFY(cycle106FootersAlign(window.data()));
                    QTRY_VERIFY(cycle106FooterControlsFit(window.data()));
                    // Style selection is independent of the visible writing mode.
                    QCOMPARE(appearance->isVisible(), footer->width() >= 660);
                    QCOMPARE(output->isVisible(), footer->width() >= 660);
                    QCOMPARE(styles->isVisible(), footer->width() < 660);
                    if (width == 1440) {
                        QTRY_COMPARE(cycle103SceneBounds(footer), initialFooter);
                        QTRY_COMPARE(cycle103SceneBounds(output), initialOutput);
                        QTRY_COMPARE(cycle103SceneBounds(appearance), initialAppearance);
                    }
                    auto *leftControl = styles->isVisible() ? styles : appearance;
                    QVERIFY(qAbs(cycle103SceneBounds(leftControl).left()
                                 - cycle103SceneBounds(footer).left() - 12) < 0.1);
                    if (!dark && pass == 0 && (step == 0 || step == 1 || step == 2 || step == 4))
                        QVERIFY(cycle102Capture(quick, QString("cycle107-view-%1-%2-%3").arg(width).arg(step).arg(steps[step].second)));
                }
                if (!split->isEnabled()) {
                    // A disabled Split must not flash a different single pane.
                    const auto before = cycle107ViewBounds(footer);
                    QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier, cycle103SceneBounds(split).center().toPoint());
                    QCOMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
                    QVERIFY(cycle107SameBounds(cycle107ViewBounds(footer), before));
                }
            }
            if (dark && width == 1440) QVERIFY(cycle102Capture(quick, "cycle107-view-dark"));
        }
    }
    // Exercise real shortcut dispatch offscreen. Native screenshot runs use a
    // deliberately non-activating window so they never take the user's keyboard.
    if (QGuiApplication::platformName() != "cocoa") {
        layout->setProperty("organizerVisible", false);
        layout->setProperty("filesVisible", false);
        quick->resize(960, 700);
        quick->requestActivate();
        QVERIFY(cycle102Click(quick, source));
        QTest::qWait(80);
        for (Qt::KeyboardModifiers modifiers : {Qt::KeyboardModifiers(Qt::NoModifier), Qt::KeyboardModifiers(Qt::ShiftModifier)}) {
            QVERIFY(QMetaObject::invokeMethod(source, "forceActiveFocus"));
            QTRY_VERIFY(source->hasActiveFocus());
            QVERIFY(window->property("workspaceOwnsKeyboard").toBool());
            QTest::keyClick(quick, Qt::Key_F6, modifiers);
            QTRY_VERIFY(editor->hasActiveFocus());
        }
        // Shrinking a focused style trigger must not leave focus on the hidden
        // wide button after the compact Aa replacement appears.
        layout->setProperty("organizerVisible", true);
        layout->setProperty("filesVisible", true);
        quick->resize(1440, 700);
        QTRY_VERIFY(appearance->isVisible());
        QVERIFY(QMetaObject::invokeMethod(appearance, "forceActiveFocus"));
        QTRY_VERIFY(appearance->hasActiveFocus());
        quick->resize(720, 700);
        QTRY_VERIFY(styles->isVisible());
        QTRY_VERIFY(!appearance->isVisible());
        QTRY_VERIFY(!appearance->hasActiveFocus());
        QTRY_VERIFY(editor->hasActiveFocus());
    }
}

void FomawriteTest::footerViewCyclesPreserveDraftSelectionAndWidths() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    QString saved = QString::fromUtf8("# A document with a place to return to\n\nA café draft — 你好.\n\n");
    for (int line = 0; line < 70; ++line)
        saved += QString("Paragraph %1 keeps enough words on the page to wrap at different widths. **Keep this text** and its Markdown unchanged.\n\n").arg(line);
    const QString path = directory.filePath("Steady draft.md");
    QVERIFY(cycle103Write(path, saved.toUtf8()));
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *settings = window->findChild<QObject *>("workspaceSettings");
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *editorPane = window->findChild<QQuickItem *>("editorPane");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    auto *scroll = window->findChild<QQuickItem *>("editorScroll");
    QVERIFY(quick && layout && settings && footer && editor && editorPane && pane && scroll);
    auto *source = footer->findChild<QQuickItem *>("sourceModeButton");
    auto *split = footer->findChild<QQuickItem *>("previewSplitButton");
    auto *full = footer->findChild<QQuickItem *>("previewFullButton");
    auto *visual = footer->findChild<QQuickItem *>("visualEditToggle");
    QVERIFY(source && split && full && visual);
    cycle106KeepNativeFocus(quick);
    Cycle106MouseInputIsolation mouseInput(quick);
    quick->resize(1440, 800); quick->show();
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QTest::qWaitForWindowExposed(quick));
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, saved.size()), Q_ARG(QString, QStringLiteral("Unsaved addition.\n"))));
    const QString draft = editor->property("text").toString();
    const int selectionStart = draft.indexOf(QStringLiteral("Paragraph 30"));
    const int selectionEnd = selectionStart + QStringLiteral("Paragraph 30").size();
    QVERIFY(selectionStart > 0);
    // Preferred widths must survive repeated hide/reveal and the whole cycle
    // must also work when both navigation panes were explicitly collapsed.
    layout->setProperty("organizerWidth", 204);
    layout->setProperty("fileWidth", 268);
    layout->setProperty("previewWidth", 382);
    auto *previewScroll = pane->findChild<QQuickItem *>("previewScroll"); QVERIFY(previewScroll);
    QVERIFY(cycle102Click(quick, full));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_VERIFY(!pane->property("viewportRefreshPending").toBool());
    const qreal originalPreviewHeight = previewScroll->property("contentHeight").toReal();
    QVERIFY(originalPreviewHeight > previewScroll->height() * 2);
    for (bool navigation : {true, false}) {
        layout->setProperty("organizerVisible", navigation);
        layout->setProperty("filesVisible", navigation);
        for (bool synchronized : {false, true}) {
            settings->setProperty("synchronizedScroll", synchronized);
            QVERIFY(cycle102Click(quick, split));
            QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 1);
            QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, selectionStart), Q_ARG(int, selectionEnd)));
            QTest::qWait(80);
            const QRectF caret = editor->property("cursorRectangle").toRectF();
            scroll->setProperty("contentY", editor->y() + caret.y() - scroll->height() / 2);
            QTest::qWait(100);
            const qreal savedY = scroll->property("contentY").toReal();
            const int savedCursor = editor->property("cursorPosition").toInt();
            const QRectF splitSourceBounds = cycle103SceneBounds(editorPane);
            const QRectF splitPreviewBounds = cycle103SceneBounds(pane);
            QVERIFY(savedY > 0);
            for (int pass = 0; pass < 4; ++pass) {
                // Check each return to the same split geometry, not absolute
                // scroll offsets while the document is laid out at another width.
                for (auto *destination : {full, source}) {
                    QVERIFY(cycle102Click(quick, destination));
                    QVERIFY(cycle102Click(quick, split));
                    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 1);
                    QTRY_COMPARE(cycle103SceneBounds(editorPane), splitSourceBounds);
                    QTRY_COMPARE(cycle103SceneBounds(pane), splitPreviewBounds);
                    QCOMPARE(layout->property("organizerWidth").toReal(), 204.0);
                    QCOMPARE(layout->property("fileWidth").toReal(), 268.0);
                    QCOMPARE(layout->property("previewWidth").toReal(), 382.0);
                    QCOMPARE(layout->property("organizerVisible").toBool(), navigation);
                    QCOMPARE(layout->property("filesVisible").toBool(), navigation);
                    QCOMPARE(editor->property("text").toString(), draft);
                    QCOMPARE(editor->property("cursorPosition").toInt(), savedCursor);
                    QCOMPARE(editor->property("selectionStart").toInt(), selectionStart);
                    QCOMPARE(editor->property("selectionEnd").toInt(), selectionEnd);
                    QTRY_VERIFY2(qAbs(scroll->property("contentY").toReal() - savedY) <= 2,
                                 qPrintable(QString("Source scroll moved: before %1 after %2, synchronized=%3")
                                     .arg(savedY).arg(scroll->property("contentY").toReal()).arg(synchronized)));
                }
                QVERIFY(cycle102Click(quick, visual));
                QTRY_VERIFY(layout->property("visualEditEnabled").toBool());
                QVERIFY(cycle102Click(quick, full));
                QVERIFY(cycle102Click(quick, split));
                QVERIFY(cycle102Click(quick, visual));
                QTRY_VERIFY(!layout->property("visualEditEnabled").toBool());
                QCOMPARE(editor->property("text").toString(), draft);
                QCOMPARE(editor->property("cursorPosition").toInt(), savedCursor);
                QCOMPARE(editor->property("selectionStart").toInt(), selectionStart);
                QCOMPARE(editor->property("selectionEnd").toInt(), selectionEnd);
                QTRY_VERIFY(qAbs(scroll->property("contentY").toReal() - savedY) <= 2);
                QVERIFY(backend.modified());
                QCOMPARE(cycle103Read(path), saved.toUtf8());
            }
        }
    }
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QCOMPARE(editor->property("text").toString(), saved);
    QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
    QCOMPARE(editor->property("text").toString(), draft);
    QCOMPARE(cycle103Read(path), saved.toUtf8());

    // Switching through Visual Edit must not leave its hidden text contributing
    // a second, longer scrollbar range to the rendered Preview.
    layout->setProperty("organizerVisible", true);
    layout->setProperty("filesVisible", true);
    QVERIFY(cycle102Click(quick, full));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_VERIFY(!pane->property("viewportRefreshPending").toBool());
    QTRY_VERIFY(qAbs(previewScroll->property("contentHeight").toReal() - originalPreviewHeight) <= 2);

    // A new document supersedes an in-flight view restoration. Loading a short
    // note after long visual content must leave no inherited blank scroll tail.
    QVERIFY(cycle102Click(quick, visual));
    QTRY_VERIFY(layout->property("visualEditEnabled").toBool());
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    const QByteArray shortSource("# Short note\n\nA fresh document.\n");
    const QString shortPath = directory.filePath("Short note.md");
    QVERIFY(cycle103Write(shortPath, shortSource));
    QVERIFY(cycle102Click(quick, visual));
    QVERIFY(window->property("changingDocumentView").toBool());
    QVERIFY(backend.open(QUrl::fromLocalFile(shortPath)));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_COMPARE(editor->property("text").toString(), QString::fromUtf8(shortSource));
    QTRY_VERIFY(!pane->property("viewportRefreshPending").toBool());
    QTRY_COMPARE(previewScroll->property("contentHeight").toReal(), previewScroll->height());
    QTRY_VERIFY(qAbs(previewScroll->property("contentY").toReal()) <= 1);
    QVERIFY(!backend.modified());
    QCOMPARE(cycle103Read(path), saved.toUtf8());
    QCOMPARE(cycle103Read(shortPath), shortSource);
}

void FomawriteTest::footerCompactStylesRemainReachableAcrossModes() {
    Cycle103SettingsSnapshot isolatedSettings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *settings = window->findChild<QObject *>("workspaceSettings");
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    QVERIFY(quick && layout && settings && footer && editor);
    auto *styles = footer->findChild<QQuickItem *>("footerStylesButton");
    auto *stylesMenu = window->findChild<QObject *>("footerStylesMenu");
    auto *appearanceMenu = window->findChild<QObject *>("sourceAppearanceMenu");
    auto *templateMenu = window->findChild<QObject *>("previewTemplateMenu");
    auto *appearanceEntry = window->findChild<QQuickItem *>("footerWritingStylesEntry");
    auto *templateEntry = window->findChild<QQuickItem *>("footerPreviewTemplatesEntry");
    QVERIFY(styles && stylesMenu && appearanceMenu && templateMenu && appearanceEntry && templateEntry);
    cycle106KeepNativeFocus(quick);
    Cycle106MouseInputIsolation mouseInput(quick);
    quick->resize(720, 520); quick->show();
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    const QString draft = QString::fromUtf8("# Compact styles\n\nThe café draft keeps its text — 你好.\n");
    editor->setProperty("text", draft);
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QTest::qWaitForWindowExposed(quick));
    int compactChecks = 0;
    for (int width : {720, 960}) {
        quick->resize(width, 520);
        QTest::qWait(60);
        QTRY_COMPARE(quick->size(), QSize(width, 520));
        for (const char *name : {"sourceModeButton", "previewFullButton", "visualEditToggle"}) {
            QVERIFY(cycle102Click(quick, footer->findChild<QQuickItem *>(name)));
            QTRY_VERIFY(cycle106FooterControlsFit(window.data()));
            if (!styles->isVisible()) continue;
            ++compactChecks;
            QVERIFY(cycle102Click(quick, styles));
            QTRY_VERIFY(stylesMenu->property("opened").toBool());
            QVERIFY(cycle106MenuFits(quick, stylesMenu, styles));
            QVERIFY(cycle102Click(quick, appearanceEntry));
            QTRY_VERIFY(appearanceMenu->property("opened").toBool());
            QVERIFY(cycle106MenuFits(quick, appearanceMenu, styles));
            const int outputBefore = backend.outputStyle();
            QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>("sourceAppearanceBook")));
            QTRY_COMPARE(settings->property("writingAppearance").toString(), QStringLiteral("book"));
            QCOMPARE(backend.outputStyle(), outputBefore);
            QVERIFY(cycle102Click(quick, styles));
            QTRY_VERIFY(stylesMenu->property("opened").toBool());
            QVERIFY(cycle102Click(quick, templateEntry));
            QTRY_VERIFY(templateMenu->property("opened").toBool());
            QVERIFY(cycle106MenuFits(quick, templateMenu, styles));
            if (width == 720 && QString::fromLatin1(name) == "previewFullButton")
                QVERIFY(cycle102Capture(quick, "cycle107-compact-template-menu"));
            QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>("previewTemplate6")));
            QTRY_COMPARE(backend.outputStyle(), 6);
            QCOMPARE(settings->property("writingAppearance").toString(), QStringLiteral("book"));
            QCOMPARE(editor->property("text").toString(), draft);
            QVERIFY(cycle107ViewChecked(footer, layout));
        }
    }
    QVERIFY2(compactChecks >= 3, "Compact style menus must be exercised in Source, Preview and Visual Edit.");
}
