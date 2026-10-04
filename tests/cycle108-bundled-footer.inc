#include <tuple>

namespace {
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
    for (const auto &[name, expectedMode] : {std::pair{"native_editor", 0}, std::pair{"native_split", 1}, std::pair{"native_preview", 2}}) {
        auto *item = window->findChild<QObject *>(name);
        if (!item || item->property("checked").toBool() != (mode == expectedMode)) return false;
    }
    return true;
}
bool cycle108WorkspaceViewChecks(QObject *menu, int mode, bool visual) {
    for (const auto &[text, expected] : {
             std::pair{"Source", mode == 0}, std::pair{"Source and preview side by side", mode == 1},
             std::pair{"Preview", mode == 2 && !visual}, std::pair{"Visual Edit", mode != 0 && visual}}) {
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
        saved += QString("Paragraph %1 has **bold words**, an _italic phrase_ and enough text to wrap differently in Split, Source and Visual Edit.\n\n").arg(index);
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
    auto *full = footer->findChild<QQuickItem *>("previewFullButton");
    auto *visual = footer->findChild<QQuickItem *>("visualEditToggle");
    QVERIFY(source && split && full && visual);
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
        QCOMPARE(bounds.size(), 3);
        for (int burst = 0; burst < 4; ++burst) {
            const QList<QQuickItem *> choices{full, visual, split, full, source, visual, split, source};
            int overlappingClicks = 0;
            for (auto *choice : choices) {
                if (window->property("changingDocumentView").toBool()) ++overlappingClicks;
                QSignalSpy painted(quick, &QQuickWindow::frameSwapped);
                // Visual Edit follows its pane; click its last painted position.
                // Only Source/Split/Full retain their baseline coordinates.
                const auto target = choice == visual ? cycle103SceneBounds(choice) : bounds.value(choice->objectName());
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
            QTRY_VERIFY(!window->property("changingDocumentView").toBool());
            QTRY_VERIFY(!pane->property("viewportRefreshPending").toBool());
            QVERIFY(!layout->property("visualEditEnabled").toBool());
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
            // No late visual refresh or timer may undo the final Source choice.
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
}

void FomawriteTest::bundledFooterRoutesFollowResponsivePresentation() {
    Cycle103SettingsSnapshot isolatedSettings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *scroll = window->findChild<QQuickItem *>("editorScroll");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    auto *visualEditor = pane ? pane->findChild<QQuickItem *>("visualEditor") : nullptr;
    auto *previewScroll = pane ? pane->findChild<QQuickItem *>("previewScroll") : nullptr;
    auto *workspaceButton = window->findChild<QQuickItem *>("topChromeLibraryButton");
    auto *workspaceMenu = window->findChild<QObject *>("workspaceMenu");
    QVERIFY(quick && layout && footer && editor && scroll && pane && visualEditor && previewScroll && workspaceButton && workspaceMenu);
    auto *source = footer->findChild<QQuickItem *>("sourceModeButton");
    auto *split = footer->findChild<QQuickItem *>("previewSplitButton");
    auto *full = footer->findChild<QQuickItem *>("previewFullButton");
    auto *visual = footer->findChild<QQuickItem *>("visualEditToggle");
    QVERIFY(source && split && full && visual);
    cycle106KeepNativeFocus(quick);
    Cycle106MouseInputIsolation mouseInput(quick);
    quick->resize(1440, 800); quick->show();
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    const QString draft = QString::fromUtf8("# Responsive editing\n\nKeep the café draft — 你好 — unchanged while choosing a view.\n");
    editor->setProperty("text", draft);
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QTest::qWaitForWindowExposed(quick));
    if (QGuiApplication::platformName() != "cocoa") {
        quick->requestActivate();
        QTRY_VERIFY(quick->isActive());
    }

    // Source-focused Split collapses to Source, even if its hidden companion
    // had visual editing enabled. The first Visual Edit click must show it.
    QVERIFY(cycle102Click(quick, split));
    QVERIFY(cycle102Click(quick, visual));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier, editor->mapToScene(QPointF(20, 15)).toPoint());
    QTRY_VERIFY(editor->hasActiveFocus());
    QTRY_COMPARE(window->property("lastWritingSurface").toString(), QStringLiteral("source"));
    quick->resize(720, 800);
    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
    QVERIFY(cycle107ViewChecked(footer, layout));
    QVERIFY(cycle108NativeViewChecks(window.data(), 0));
    QVERIFY(cycle108WorkspaceViewChecks(workspaceMenu, 0, false));
    QVERIFY(cycle102Click(quick, visual));
    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 2);
    QVERIFY(visual->property("checked").toBool());
    QVERIFY(pane->isVisible() && visualEditor->isVisible());
    QVERIFY(cycle108WorkspaceViewChecks(workspaceMenu, 2, true));
    QVERIFY(cycle102Click(quick, source));
    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 0);

    // Visual-focused Split collapses to Full Visual. Turning editing off keeps
    // Full Preview visible, instead of unexpectedly jumping to Source.
    quick->resize(1440, 800);
    QTRY_VERIFY(split->isEnabled());
    QVERIFY(cycle102Click(quick, split));
    QVERIFY(cycle102Click(quick, visual));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier, visualEditor->mapToScene(QPointF(20, 15)).toPoint());
    QTRY_COMPARE(window->property("lastWritingSurface").toString(), QStringLiteral("visual"));
    quick->resize(720, 800);
    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 2);
    QVERIFY(cycle107ViewChecked(footer, layout));
    QVERIFY(cycle108NativeViewChecks(window.data(), 2));
    QVERIFY(cycle102Click(quick, visual));
    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 2);
    QVERIFY(!visual->property("checked").toBool());
    QVERIFY(pane->findChild<QQuickItem *>("renderedPreview")->isVisible());
    QVERIFY(cycle108WorkspaceViewChecks(workspaceMenu, 2, false));

    // A remembered Split intent can still be contracted just above its minimum
    // width. An enabled explicit Split command must show both panes immediately.
    quick->resize(1440, 800);
    QTRY_VERIFY(split->isEnabled());
    QVERIFY(cycle102Click(quick, source));
    QVERIFY(cycle102Click(quick, split));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    quick->resize(720, 800);
    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
    quick->resize(820, 800);
    QTRY_VERIFY(split->isEnabled());
    QCOMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
    QVERIFY(cycle102Click(quick, split));
    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 1);
    QVERIFY(cycle107ViewChecked(footer, layout));
    QVERIFY(cycle108NativeViewChecks(window.data(), 1));

    // The rendered toolbar menu uses the same choices as the footer, including
    // Source while Visual Edit is active. Menu dismissal must not consume the
    // next click or leave an old tooltip over the view controls.
    quick->resize(1440, 800);
    QTest::qWait(100);
    const auto initial = cycle107ViewBounds(footer);
    const QRectF footerBounds = cycle103SceneBounds(footer);
    for (const auto &[text, mode, editing] : {
             std::tuple{"Visual Edit", 2, true}, std::tuple{"Visual Edit", 2, true},
             std::tuple{"Source", 0, false}, std::tuple{"Source", 0, false},
             std::tuple{"Preview", 2, false}, std::tuple{"Preview", 2, false},
             std::tuple{"Source and preview side by side", 1, false}, std::tuple{"Source and preview side by side", 1, false}}) {
        QVERIFY(cycle102Click(quick, workspaceButton));
        QTRY_VERIFY(workspaceMenu->property("opened").toBool());
        QVERIFY(cycle102Click(quick, cycle108MenuItem(workspaceMenu, QString::fromLatin1(text))));
        QTRY_VERIFY(!workspaceMenu->property("visible").toBool());
        QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), mode);
        QCOMPARE(visual->property("checked").toBool(), editing);
        QVERIFY(cycle107ViewChecked(footer, layout));
        QTRY_VERIFY2(cycle108WorkspaceViewChecks(workspaceMenu, mode, editing),
                 qPrintable(QString("View menu checkmarks disagreed after selecting %1.").arg(QString::fromLatin1(text))));
        QVERIFY(cycle108NativeViewChecks(window.data(), mode));
        QTRY_VERIFY(cycle107SameBounds(cycle107ViewBounds(footer), initial));
        QTRY_COMPARE(cycle103SceneBounds(footer), footerBounds);
        QVERIFY(!workspaceButton->property("tooltipReady").toBool());
    }
    QQuickItem *hidePreview = nullptr;
    for (auto *item : window->findChildren<QQuickItem *>()) {
        if (item->property("hint").toString() == QStringLiteral("Hide preview")) { hidePreview = item; break; }
    }
    QVERIFY(hidePreview && hidePreview->isVisible());
    QVERIFY(cycle102Click(quick, hidePreview));
    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
    QVERIFY(cycle107ViewChecked(footer, layout));
    QVERIFY(cycle108NativeViewChecks(window.data(), 0));
    QVERIFY(cycle102Click(quick, full));
    QVERIFY(cycle102Click(quick, source));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 0);
    QCOMPARE(editor->property("text").toString(), draft);
    QVERIFY(cycle107SameBounds(cycle107ViewBounds(footer), initial));
}
