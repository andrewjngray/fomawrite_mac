#include <tuple>

namespace {
// Real page replies establish text and selection before testing a rapid choice.
QString cycle108PageText(EditorBridge *bridge) {
    QSignalSpy received(bridge, &EditorBridge::textReceived);
    const int token = bridge->fetchText();
    QString text;
    const bool answered = QTest::qWaitFor([&] {
        for (const auto &args : received) if (args.at(0).toInt() == token) {
            text = args.at(1).toString(); return true;
        }
        return false;
    }, 5000);
    return answered ? text : QStringLiteral("<timeout>");
}
QPair<int, int> cycle108PageSelection(EditorBridge *bridge) {
    QSignalSpy received(bridge, &EditorBridge::selectionReceived);
    const int token = bridge->fetchSelection();
    QPair<int, int> result{-1, -1};
    QTest::qWaitFor([&] {
        for (const auto &args : received) if (args.at(0).toInt() == token) {
            result = {args.at(1).toInt(), args.at(2).toInt()}; return true;
        }
        return false;
    }, 5000);
    return result;
}
// Click where the previous frame painted the persistent action, rather than
// an intermediate SplitView position that the user has never seen.
bool cycle108PaintedClick(QQuickWindow *window, QQuickItem *item, const QPoint &point) {
    if (!item || !item->isVisible() || !item->isEnabled()) return false;
    if (!QRect(0, 0, window->width(), window->height()).contains(point)) return false;
    QSignalSpy clicked(item, SIGNAL(clicked()));
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point, 0);
    return clicked.isValid() && clicked.count() == 1;
}
QQuickItem *cycle108MenuItem(QObject *menu, const QString &text) {
    for (auto *item : menu->findChildren<QQuickItem *>()) {
        if (item->property("text").toString() == text
                && item->metaObject()->indexOfSignal("triggered()") >= 0) return item;
    }
    return nullptr;
}
bool cycle108NativeViewChecks(QObject *window, int mode) {
    if (!testDesktopMenusMatchPlatform(window)) return false;
    for (const auto &[name, expectedMode] : {std::pair{"editor", 0}, std::pair{"split", 1}, std::pair{"preview", 2}}) {
        auto *item = testDesktopCommand(window, QString::fromLatin1(name));
        if (!item || item->property("checked").toBool() != (mode == expectedMode)) return false;
    }
    return true;
}
bool cycle108WorkspaceViewChecks(QObject *menu, int mode, bool live) {
    for (const auto &[text, expected] : {
             std::pair{"Source", mode != 2 && !live}, std::pair{"Live", mode != 2 && live},
             std::pair{"Single", mode == 0}, std::pair{"Split", mode == 1}, std::pair{"Preview Only", mode == 2}}) {
        auto *item = cycle108MenuItem(menu, QString::fromLatin1(text));
        if (!item || item->property("checked").toBool() != expected) return false;
    }
    return true;
}
}

void FomawriteTest::bundledFooterRapidModesPreserveDraftAndViewport() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString saved = QString::fromUtf8("# A long café draft — 你好\n\n");
    for (int index = 0; index < 90; ++index)
        saved += QString("Paragraph %1 has **bold words**, an _italic phrase_ and enough text to wrap differently in Split, Source and Live.\n\n").arg(index);
    const QString path = directory.filePath("Rapid mode draft.md");
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
    auto *scroll = window->findChild<QQuickItem *>("editorScroll");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    QVERIFY(quick && layout && settings && footer && editor && scroll && pane);
    auto *source = footer->findChild<QQuickItem *>("sourceModeButton");
    auto *split = footer->findChild<QQuickItem *>("previewSplitButton");
    auto *full = footer->findChild<QQuickItem *>("singleModeButton");
    auto *live = footer->findChild<QQuickItem *>("liveEditToggle");
    QVERIFY(source && split && full && live);
    cycle106KeepNativeFocus(quick);
    Cycle106MouseInputIsolation mouseInput(quick);
    quick->resize(1440, 800); quick->show();
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    layout->setProperty("organizerVisible", false);
    layout->setProperty("filesVisible", false);
    QVERIFY(QTest::qWaitForWindowExposed(quick));
    if (QGuiApplication::platformName() != "cocoa") {
        quick->requestActivate();
        QTRY_VERIFY(quick->isActive());
    }
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, saved.size()), Q_ARG(QString, QStringLiteral("Unsaved ending.\n"))));
    const QString draft = editor->property("text").toString();
    const int start = draft.indexOf(QStringLiteral("Paragraph 40"));
    const int end = start + QStringLiteral("Paragraph 40").size();
    QVERIFY(start > 0);

    for (bool synchronized : {false, true}) {
        settings->setProperty("synchronizedScroll", synchronized);
        QVERIFY(cycle102Click(quick, source));
        QVERIFY(cycle102Click(quick, full));
        QTRY_VERIFY(!window->property("changingDocumentView").toBool());
        QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, start), Q_ARG(int, end)));
        QTest::qWait(100);
        const qreal maximum = scroll->property("contentHeight").toReal() - scroll->height();
        QVERIFY(maximum > 4 * scroll->height());
        scroll->setProperty("contentY", maximum * 0.43);
        QTest::qWait(60);
        const qreal sourceY = scroll->property("contentY").toReal();
        const int cursor = editor->property("cursorPosition").toInt();
        const auto bounds = cycle107ViewBounds(footer);
        const QRectF footerBounds = cycle103SceneBounds(footer);
        QCOMPARE(bounds.size(), 5);
        for (int burst = 0; burst < 4; ++burst) {
            const QList<QQuickItem *> choices{full, live, split, full, source, live, split, full, source};
            int overlappingClicks = 0;
            for (auto *choice : choices) {
                if (window->property("changingDocumentView").toBool()) ++overlappingClicks;
                QSignalSpy painted(quick, &QQuickWindow::frameSwapped);
                // The editing group may compact; use its currently painted position.
                // Single/Split retain their baseline coordinates.
                const auto target = choice == live || choice == source ? cycle103SceneBounds(choice) : bounds.value(choice->objectName());
                QVERIFY(cycle108PaintedClick(quick, choice, target.center().toPoint()));
                quick->update();
                // Wait for one presented frame, not the 48–120ms viewport
                // restoration. Check the geometry actually offered to users.
                QVERIFY(QTest::qWaitFor([&] { return !painted.isEmpty(); }, 1000));
                QVERIFY(cycle107SameBounds(cycle107ViewBounds(footer), bounds));
                QVERIFY(cycle117PaneFooterAnchors(window.data()));
                QVERIFY(cycle107ViewChecked(footer, layout));
            }
            QVERIFY2(overlappingClicks > 0, "Rapid clicks must overlap an unfinished viewport restoration.");
            QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
            // Source is an acknowledged Live-text transition. Keep the burst
            // above frame-paced; only its final semantic state must settle.
            QTRY_VERIFY(!layout->property("liveEditEnabled").toBool());
            QTRY_VERIFY(!window->property("lifecycleInputBlocked").toBool());
            QTRY_VERIFY(!window->property("changingDocumentView").toBool());
            QTRY_VERIFY(!pane->property("viewportRefreshPending").toBool());
            QVERIFY(!layout->property("liveEditEnabled").toBool());
            QVERIFY(cycle107ViewChecked(footer, layout));
            QVERIFY(cycle108NativeViewChecks(window.data(), 0));
            QTRY_VERIFY(cycle107SameBounds(cycle107ViewBounds(footer), bounds));
            QCOMPARE(cycle103SceneBounds(footer), footerBounds);
            QCOMPARE(editor->property("text").toString(), draft);
            QCOMPARE(editor->property("cursorPosition").toInt(), cursor);
            QCOMPARE(editor->property("selectionStart").toInt(), start);
            QCOMPARE(editor->property("selectionEnd").toInt(), end);
            QTRY_VERIFY2(qAbs(scroll->property("contentY").toReal() - sourceY) <= 2,
                         "A rapid mode change must return to the same Source reading position.");
            // No late refresh or timer may undo the final Source choice.
            QTest::qWait(320);
            QCOMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
            QVERIFY(cycle107SameBounds(cycle107ViewBounds(footer), bounds));
            QVERIFY(qAbs(scroll->property("contentY").toReal() - sourceY) <= 2);
            QCOMPARE(editor->property("text").toString(), draft);
        }
    }
    QVERIFY(backend.modified());
    QCOMPARE(cycle103Read(path), saved.toUtf8());
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QCOMPARE(editor->property("text").toString(), saved);
    QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
    QCOMPARE(editor->property("text").toString(), draft);
    // A newer Live choice cancels only the older pending Source transition;
    // its page/history/caret must survive rather than being reloaded later.
    QVERIFY(cycle102Click(quick, live));
    auto *bridge = backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 15000);
    QCOMPARE(cycle108PageText(bridge), draft);
    const QString marker = QStringLiteral("Newest Live choice stays.\n"), newest = draft + marker;
    const QJsonArray change{QJsonObject{{"from", draft.size()}, {"to", draft.size()}, {"insert", marker}}};
    bridge->injectUserChanges(QString::fromUtf8(QJsonDocument(change).toJson(QJsonDocument::Compact)));
    QTRY_COMPARE(editor->property("text").toString(), newest);
    const auto caretBefore = cycle108PageSelection(bridge); QVERIFY(caretBefore.first >= 0);
    QVERIFY(cycle108PaintedClick(quick, source, cycle103SceneBounds(source).center().toPoint()));
    QVERIFY(window->property("documentBarrierRequest").toInt() > 0);
    QVERIFY(cycle108PaintedClick(quick, live, cycle103SceneBounds(live).center().toPoint()));
    QTRY_COMPARE(window->property("documentBarrierRequest").toInt(), 0);
    QVERIFY2(layout->property("liveEditEnabled").toBool(), "A late Source acknowledgement must not replace the newer Live choice.");
    QCOMPARE(cycle108PageText(bridge), newest);
    QCOMPARE(cycle108PageSelection(bridge), caretBefore);
    QCOMPARE(editor->property("text").toString(), newest);
    bridge->requestUndo(); QTRY_COMPARE(editor->property("text").toString(), draft);
    QCOMPARE(cycle108PageText(bridge), draft);
    bridge->requestRedo(); QTRY_COMPARE(editor->property("text").toString(), newest);
    QCOMPARE(cycle108PageText(bridge), newest);
    QCOMPARE(cycle103Read(path), saved.toUtf8());
}

void FomawriteTest::bundledFooterRoutesFollowResponsivePresentation() {
    Cycle103SettingsSnapshot isolatedSettings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl("qrc:/Main.qml"));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    auto *workspaceButton = window->findChild<QQuickItem *>("topChromeLibraryButton");
    auto *workspaceMenu = window->findChild<QObject *>("workspaceMenu");
    QVERIFY(quick && layout && footer && editor && pane && workspaceButton && workspaceMenu);
    auto *source = footer->findChild<QQuickItem *>("sourceModeButton");
    auto *split = footer->findChild<QQuickItem *>("previewSplitButton");
    auto *single = footer->findChild<QQuickItem *>("singleModeButton");
    auto *live = footer->findChild<QQuickItem *>("liveEditToggle");
    QVERIFY(source && split && single && live);
    const auto liveVisible = [&] { auto *p = window->findChild<QQuickItem *>("liveEditorPane"); return p && p->isVisible(); };
    cycle106KeepNativeFocus(quick); Cycle106MouseInputIsolation mouseInput(quick);
    quick->resize(1440, 800); quick->show();
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    const QString draft = QString::fromUtf8("# Responsive editing\n\nKeep the café draft — 你好 — unchanged while choosing a view.\n");
    editor->setProperty("text", draft);
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QTest::qWaitForWindowExposed(quick));
    // Width pressure contracts the layout, never changes the chosen editor.
    for (auto *editing : {source, live}) {
        quick->resize(1440, 800); QTRY_VERIFY(split->isEnabled());
        QVERIFY(cycle102Click(quick, editing)); QVERIFY(cycle102Click(quick, split));
        QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 1);
        quick->resize(720, 800);
        QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
        QTRY_COMPARE(layout->property("liveEditEnabled").toBool(), editing == live);
        QTRY_COMPARE(liveVisible(), editing == live);
        QVERIFY(cycle107ViewChecked(footer, layout));
        QVERIFY(cycle108NativeViewChecks(window.data(), 0));
        QVERIFY(cycle108WorkspaceViewChecks(workspaceMenu, 0, editing == live));
        quick->resize(820, 800); QTRY_VERIFY(split->isEnabled());
        QCOMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
        QVERIFY(cycle102Click(quick, split));
        QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 1);
        QTRY_COMPARE(layout->property("liveEditEnabled").toBool(), editing == live);
        QVERIFY(cycle107ViewChecked(footer, layout));
    }
    quick->resize(1440, 800); QVERIFY(cycle102Click(quick, single)); QVERIFY(cycle102Click(quick, source));
    QTest::qWait(100); const auto initial = cycle107ViewBounds(footer);
    for (const auto &[text, mode, editing] : {
             std::tuple{"Live", 0, true}, std::tuple{"Live", 0, true},
             std::tuple{"Split", 1, true}, std::tuple{"Source", 1, false},
             std::tuple{"Single", 0, false}, std::tuple{"Live", 0, true},
             std::tuple{"Preview Only", 2, true}, std::tuple{"Live", 0, true},
             std::tuple{"Source", 0, false}, std::tuple{"Split", 1, false}}) {
        QVERIFY(cycle102Click(quick, workspaceButton)); QTRY_VERIFY(workspaceMenu->property("opened").toBool());
        QVERIFY(cycle102Click(quick, cycle108MenuItem(workspaceMenu, QString::fromLatin1(text))));
        QTRY_VERIFY(!workspaceMenu->property("visible").toBool());
        QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), mode);
        QTRY_COMPARE(layout->property("liveEditEnabled").toBool(), editing);
        QTRY_VERIFY(!window->property("lifecycleInputBlocked").toBool());
        QVERIFY(cycle107ViewChecked(footer, layout));
        QTRY_VERIFY(cycle108WorkspaceViewChecks(workspaceMenu, mode, editing));
        QVERIFY(cycle108NativeViewChecks(window.data(), mode));
        QTRY_VERIFY(cycle107SameBounds(cycle107ViewBounds(footer), initial));
        QCOMPARE(editor->property("text").toString(), draft);
        QVERIFY(!workspaceButton->property("tooltipReady").toBool());
    }
    QQuickItem *hidePreview = nullptr;
    for (auto *item : window->findChildren<QQuickItem *>())
        if (item->property("hint").toString() == "Hide preview") { hidePreview = item; break; }
    QVERIFY(hidePreview && hidePreview->isVisible());
    QVERIFY(cycle102Click(quick, live));
    QVERIFY(cycle102Click(quick, hidePreview));
    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
    QVERIFY(layout->property("liveEditEnabled").toBool());
    QVERIFY(cycle107ViewChecked(footer, layout));
    QCOMPARE(editor->property("text").toString(), draft);
    // Preview Only changes arrangement, preserving the pending Source choice
    // as the editor used when writing resumes. It must remain Preview Only.
    QTRY_VERIFY_WITH_TIMEOUT(backend.editorBridge()->isReady(), 15000);
    QCOMPARE(cycle108PageText(backend.editorBridge()), draft);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, "source")));
    QVERIFY(window->property("documentBarrierRequest").toInt() > 0);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "setDocumentView", Q_ARG(QVariant, 2)));
    QTRY_COMPARE(window->property("documentBarrierRequest").toInt(), 0);
    QCOMPARE(layout->property("effectiveLayoutMode").toInt(), 2);
    QVERIFY(!layout->property("liveEditEnabled").toBool());
    QCOMPARE(cycle108PageText(backend.editorBridge()), draft);
    QCOMPARE(editor->property("text").toString(), draft);
}
