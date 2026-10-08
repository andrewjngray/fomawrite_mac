namespace {
// Nonactivating Cocoa QA windows may be occluded, which pauses automatic
// polish/render frames. Flush two actual frames before inspecting geometry;
// the fixed count never waits for a desired result or relaxes an assertion.
bool cycle118FooterRenderBarrier(QQuickWindow *window) {
    if (QGuiApplication::platformName() != "cocoa") return true;
    if (!window) return false;
    for (int frame = 0; frame < 2; ++frame) {
        window->requestUpdate();
        if (window->grabWindow().isNull()) return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
    return true;
}
QMap<QString, QRectF> cycle107ViewBounds(QQuickItem *footer) {
    QMap<QString, QRectF> result;
    for (const char *name : {"sourceModeButton", "liveEditToggle", "singleModeButton", "previewSplitButton"}) {
        auto *button = footer->findChild<QQuickItem *>(name);
        if (!button) return {};
        result.insert(QString::fromLatin1(name), cycle103SceneBounds(button));
    }
    return result;
}
bool cycle107SameBounds(const QMap<QString, QRectF> &actual, const QMap<QString, QRectF> &expected) {
    if (actual.isEmpty() || actual.size() != expected.size()) return false;
    for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
        const auto found = actual.value(it.key());
        const bool fixedAtWindowEdge = it.key() == "singleModeButton" || it.key() == "previewSplitButton";
        if ((fixedAtWindowEdge && qAbs(found.left() - it.value().left()) > 0.1) || qAbs(found.top() - it.value().top()) > 0.1
                || qAbs(found.width() - it.value().width()) > 0.1 || qAbs(found.height() - it.value().height()) > 0.1) {
            return false;
        }
    }
    // Source/Live moves together with its pane; their internal geometry is stable.
    return qAbs((actual.value("liveEditToggle").left() - actual.value("sourceModeButton").left())
        - (expected.value("liveEditToggle").left() - expected.value("sourceModeButton").left())) <= 0.1;
}
// Editing and layout are independent pane-owned groups. Style choices belong
// to their visible pane; changing editing never changes the arrangement.
bool cycle117PaneFooterAnchors(QObject *window) {
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    auto *sourcePane = window->findChild<QQuickItem *>("editorPane");
    auto *previewPane = window->findChild<QQuickItem *>("previewPane");
    auto *appearance = footer ? footer->findChild<QQuickItem *>("sourceAppearanceButton") : nullptr;
    auto *output = footer ? footer->findChild<QQuickItem *>("previewTemplateButton") : nullptr;
    auto *editing = footer ? footer->findChild<QQuickItem *>("documentEditingControls") : nullptr;
    auto *modes = footer ? footer->findChild<QQuickItem *>("documentViewControls") : nullptr;
    if (!sourcePane || !previewPane || !appearance || !output || !editing || !modes) return false;
    if (appearance->isVisible() != sourcePane->isVisible() || output->isVisible() != previewPane->isVisible()) return false;
    for (const auto &[button, pane] : {std::pair{appearance, sourcePane}, std::pair{output, previewPane}}) {
        if (!button->isVisible()) continue;
        const auto control = cycle103SceneBounds(button), region = cycle103SceneBounds(pane);
        if (qAbs(control.left() - region.left() - 12) > 0.5 || control.right() > region.right() - 11.5
            || control.width() < 34 || control.height() < 32) return false;
    }
    auto *publishing = footer->findChild<QQuickItem *>("documentPublishingControls");
    if (!publishing || editing->isVisible() != sourcePane->isVisible()
        || publishing->isVisible() != previewPane->isVisible()) return false;
    const auto modeBounds = cycle103SceneBounds(modes);
    if (qAbs(modeBounds.right() - cycle103SceneBounds(footer).right() + 12) > 0.5) return false;
    if (editing->isVisible()) {
        const auto editingBounds = cycle103SceneBounds(editing);
        const qreal ownerEnd = previewPane->isVisible() ? cycle103SceneBounds(sourcePane).right() : modeBounds.left();
        if (qAbs(editingBounds.right() - ownerEnd + 12) > 1.5
            || editingBounds.left() < cycle103SceneBounds(appearance).right() + 7.5) return false;
    }
    if (publishing->isVisible()) {
        const auto outputBounds = cycle103SceneBounds(output), publishBounds = cycle103SceneBounds(publishing);
        if (outputBounds.right() + 7.5 > publishBounds.left()
            || publishBounds.right() + 7.5 > modeBounds.left()
            || publishBounds.left() < cycle103SceneBounds(previewPane).left() + 11.5) return false;
    }
    return cycle106FooterControlsFit(window);
}
bool cycle107ViewLabelsReadable(QQuickItem *footer) {
    for (const char *name : {"liveEditToggle", "sourceModeButton", "previewSplitButton", "singleModeButton"}) {
        auto *button = footer->findChild<QQuickItem *>(name);
        if (!button || !button->isVisible()) continue;
        auto *label = button->findChild<QQuickItem *>("footerButtonLabel");
        if (!label || label->property("truncated").toBool()) return false;
    }
    return true;
}
bool cycle107ViewChecked(QQuickItem *footer, QObject *layout) {
    const int mode = layout->property("effectiveLayoutMode").toInt();
    const bool live = layout->property("liveEditEnabled").toBool();
    for (const auto &[name, expected] : {std::pair{"sourceModeButton", mode != 2 && !live},
             std::pair{"liveEditToggle", mode != 2 && live},
             std::pair{"singleModeButton", mode == 0}, std::pair{"previewSplitButton", mode == 1}}) {
        auto *button = footer->findChild<QQuickItem *>(name);
        if (!button || button->property("checked").toBool() != expected) return false;
    }
    return true;
}
}

void FomawriteTest::footerViewControlsStayStationaryAcrossModes() {
    Cycle103SettingsSnapshot isolatedSettings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    QVERIFY(quick && layout && footer && editor && pane);
    auto *source = footer->findChild<QQuickItem *>("sourceModeButton");
    auto *split = footer->findChild<QQuickItem *>("previewSplitButton");
    auto *full = footer->findChild<QQuickItem *>("singleModeButton");
    auto *live = footer->findChild<QQuickItem *>("liveEditToggle");
    auto *appearance = footer->findChild<QQuickItem *>("sourceAppearanceButton");
    auto *output = footer->findChild<QQuickItem *>("previewTemplateButton");
    QVERIFY(source && split && full && live && appearance && output);
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
            QVERIFY(cycle102Click(quick, full));
            QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
            QVERIFY(cycle118FooterRenderBarrier(quick));
            QTRY_VERIFY(cycle106FooterControlsFit(window.data()));
            const auto initial = cycle107ViewBounds(footer);
            QCOMPARE(initial.size(), 4);
            const QRectF initialFooter = cycle103SceneBounds(footer);
            auto *viewGroup = footer->findChild<QQuickItem *>("documentViewControls"); QVERIFY(viewGroup);
            QVERIFY(qAbs(cycle103SceneBounds(viewGroup).right() - (width - 12)) < 0.1);
            QVERIFY(qAbs(initial.value("previewSplitButton").right() - (width - 13)) < 0.1);
            QCOMPARE(split->isEnabled(), width >= 803);
            // Direct Source remains available in every mode; there is no second
            // source-only menu or swapped return button to search for.
            QCOMPARE(source->property("text").toString(), QStringLiteral("Source"));
            QVERIFY(!window->findChild<QObject *>("sourceModeMenu"));
            QVERIFY(!pane->property("showFooter").toBool());
            for (int pass = 0; pass < 3; ++pass) {
                QList<QPair<QQuickItem *, int>> steps{{full, 0}, {source, 0}, {live, 0}, {live, 0}};
                if (split->isEnabled()) steps.append({split, 1});
                steps.append({source, split->isEnabled() ? 1 : 0});
                steps.append({live, split->isEnabled() ? 1 : 0});
                steps.append({full, 0});
                steps.append({source, 0});
                for (int step = 0; step < steps.size(); ++step) {
                    QVERIFY(cycle102Click(quick, steps[step].first));
                    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), steps[step].second);
                    QVERIFY(cycle118FooterRenderBarrier(quick));
                    QTRY_VERIFY(cycle107SameBounds(cycle107ViewBounds(footer), initial));
                    QTRY_VERIFY(cycle117PaneFooterAnchors(window.data()));
                    QTRY_VERIFY(cycle107ViewChecked(footer, layout));
                    QTRY_VERIFY2(cycle107ViewLabelsReadable(footer), "Footer view labels must not be truncated.");
                    QTRY_VERIFY(cycle106FootersAlign(window.data()));
                    QTRY_VERIFY(cycle106FooterControlsFit(window.data()));
                    QCOMPARE(appearance->isVisible(), steps[step].second != 2);
                    QCOMPARE(output->isVisible(), steps[step].second != 0);
                    if (width == 1440) QTRY_COMPARE(cycle103SceneBounds(footer), initialFooter);
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
        // Compact styling keeps the same pane-owned trigger and keyboard focus.
        layout->setProperty("organizerVisible", true);
        layout->setProperty("filesVisible", true);
        quick->resize(1440, 700);
        QTRY_VERIFY(appearance->isVisible());
        QVERIFY(QMetaObject::invokeMethod(appearance, "forceActiveFocus"));
        QTRY_VERIFY(appearance->hasActiveFocus());
        quick->resize(720, 700);
        QTRY_VERIFY(appearance->isVisible());
        QTRY_VERIFY(appearance->hasActiveFocus());
        QTRY_VERIFY(cycle117PaneFooterAnchors(window.data()));
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
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
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
    auto *full = footer->findChild<QQuickItem *>("singleModeButton");
    auto *live = footer->findChild<QQuickItem *>("liveEditToggle");
    QVERIFY(source && split && full && live);
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
    QVERIFY(cycle118ReadPreview(window.data()));
    QVERIFY(cycle118FooterRenderBarrier(quick));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_VERIFY(!pane->property("viewportRefreshPending").toBool());
    QVERIFY(cycle118FooterRenderBarrier(quick));
    const qreal originalPreviewHeight = previewScroll->property("contentHeight").toReal();
    QVERIFY(originalPreviewHeight > previewScroll->height() * 2);
    for (bool navigation : {true, false}) {
        layout->setProperty("organizerVisible", navigation);
        layout->setProperty("filesVisible", navigation);
        for (bool synchronized : {false, true}) {
            settings->setProperty("synchronizedScroll", synchronized);
            QVERIFY(cycle102Click(quick, split));
            QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 1);
            // The browser refresh can outlast the layout-mode change. Establish
            // the reading fixture after viewport restoration has completed.
            QTRY_VERIFY(!window->property("changingDocumentView").toBool());
            QTRY_VERIFY(!pane->property("viewportRefreshPending").toBool());
            QVERIFY(cycle118FooterRenderBarrier(quick));
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
                QVERIFY(cycle102Click(quick, live));
                QTRY_VERIFY(layout->property("liveEditEnabled").toBool());
                QVERIFY(cycle102Click(quick, full));
                QVERIFY(cycle102Click(quick, split));
                QVERIFY(cycle102Click(quick, source));
                QTRY_VERIFY(!layout->property("liveEditEnabled").toBool());
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

    // Switching through Live must not leave a hidden surface contributing
    // a second, longer scrollbar range to the rendered Preview.
    layout->setProperty("organizerVisible", true);
    layout->setProperty("filesVisible", true);
    QVERIFY(cycle118ReadPreview(window.data()));
    QVERIFY(cycle118FooterRenderBarrier(quick));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_VERIFY(!pane->property("viewportRefreshPending").toBool());
    QVERIFY(cycle118FooterRenderBarrier(quick));
    QTRY_VERIFY2(qAbs(previewScroll->property("contentHeight").toReal() - originalPreviewHeight) <= 2,
                 qPrintable(QString("Read-only Preview height changed after returning to the same layout: before %1 after %2, pane width %3")
                     .arg(originalPreviewHeight).arg(previewScroll->property("contentHeight").toReal()).arg(pane->width())));

    // A new document supersedes an in-flight view restoration. Loading a short
    // note after long content must leave no inherited blank scroll tail.
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, "live")));
    QTRY_VERIFY(layout->property("liveEditEnabled").toBool());
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    const QByteArray shortSource("# Short note\n\nA fresh document.\n");
    const QString shortPath = directory.filePath("Short note.md");
    QVERIFY(cycle103Write(shortPath, shortSource));
    QVERIFY(cycle118ReadPreview(window.data()));
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
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *settings = window->findChild<QObject *>("workspaceSettings");
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    QVERIFY(quick && layout && settings && footer && editor);
    auto *appearance = footer->findChild<QQuickItem *>("sourceAppearanceButton");
    auto *output = footer->findChild<QQuickItem *>("previewTemplateButton");
    auto *appearanceMenu = window->findChild<QObject *>("sourceAppearanceMenu");
    auto *templateMenu = window->findChild<QObject *>("previewTemplateMenu");
    QVERIFY(appearance && output && appearanceMenu && templateMenu);
    cycle106KeepNativeFocus(quick); Cycle106MouseInputIsolation mouseInput(quick);
    quick->resize(720, 520); quick->show();
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    const QString draft = QString::fromUtf8("# Compact styles\n\nThe café draft keeps its text — 你好.\n");
    editor->setProperty("text", draft);
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QTest::qWaitForWindowExposed(quick));
    int appearanceChecks = 0, templateChecks = 0, compactLabelChecks = 0;
    for (int width : {720, 960, 803}) {
        QVERIFY(QMetaObject::invokeMethod(layout, "updateWidth", Q_ARG(QVariant, "preview"), Q_ARG(QVariant, 320)));
        quick->resize(width, 520); QTest::qWait(60);
        for (const char *name : {"sourceModeButton", "singleModeButton", "liveEditToggle", "previewSplitButton", "previewOnly"}) {
            if (QString::fromLatin1(name) == "previewOnly") { QVERIFY(cycle118ReadPreview(window.data())); }
            else {
                auto *mode = footer->findChild<QQuickItem *>(name);
                if (layout->property("effectiveLayoutMode").toInt() == 2
                    && (QString::fromLatin1(name) == "sourceModeButton" || QString::fromLatin1(name) == "liveEditToggle")) {
                    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode",
                        Q_ARG(QVariant, QString::fromLatin1(name) == "sourceModeButton" ? "source" : "live")));
                } else {
                    if (!mode->isEnabled()) continue;
                    QVERIFY(cycle102Click(quick, mode));
                }
            }
            QTRY_VERIFY(cycle117PaneFooterAnchors(window.data()));
            // Compact pane selectors keep a readable Aa label instead of an
            // ellipsis; narrow padding must leave the full text inside its hit area.
            for (auto *selector : {appearance, output}) {
                if (!selector->isVisible() || selector->property("text").toString() != "Aa") continue;
                ++compactLabelChecks;
                auto *label = selector->findChild<QQuickItem *>("footerButtonLabel"); QVERIFY(label);
                QTRY_VERIFY(!label->property("truncated").toBool());
                QTRY_VERIFY(label->width() + 0.1 >= label->implicitWidth());
                QVERIFY(cycle103SceneBounds(selector).contains(cycle103SceneBounds(label)));
            }
            if (appearance->isVisible()) {
                ++appearanceChecks;
                const int outputBefore = backend.outputStyle();
                if (layout->property("liveEditEnabled").toBool()) {
                    // Cycle 136/137: in Live the control names the Output Style and
                    // opens that menu instead of the Source appearances.
                    const auto appearanceBefore = settings->property("writingAppearance");
                    QVERIFY(cycle102Click(quick, appearance)); QTRY_VERIFY(templateMenu->property("opened").toBool());
                    QVERIFY(cycle106MenuFits(quick, templateMenu, appearance));
                    QVERIFY(QMetaObject::invokeMethod(templateMenu, "close"));
                    QTRY_VERIFY(!templateMenu->property("visible").toBool());
                    QCOMPARE(settings->property("writingAppearance"), appearanceBefore);
                } else {
                    QVERIFY(cycle102Click(quick, appearance)); QTRY_VERIFY(appearanceMenu->property("opened").toBool());
                    QVERIFY(cycle106MenuFits(quick, appearanceMenu, appearance));
                    QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>("sourceAppearanceBook")));
                    QTRY_COMPARE(settings->property("writingAppearance").toString(), QStringLiteral("book"));
                }
                QCOMPARE(backend.outputStyle(), outputBefore);
            }
            if (output->isVisible()) {
                ++templateChecks;
                QVERIFY(cycle102Click(quick, output)); QTRY_VERIFY(templateMenu->property("opened").toBool());
                QVERIFY(cycle106MenuFits(quick, templateMenu, output));
                if (width == 720) QVERIFY(cycle102Capture(quick, "cycle117-compact-template-menu"));
                const auto before = settings->property("writingAppearance");
                QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>("previewTemplate6")));
                QTRY_COMPARE(backend.outputStyle(), 6);
                QCOMPARE(settings->property("writingAppearance"), before);
            }
            QCOMPARE(editor->property("text").toString(), draft);
            QVERIFY(cycle107ViewChecked(footer, layout));
        }
    }
    QVERIFY(appearanceChecks >= 4 && templateChecks >= 3);
    QVERIFY2(compactLabelChecks >= 2, "Fixture must actually exercise compact Aa selectors at two split widths.");
}
