namespace {
// Menus can receive macOS input-method commits even with a non-activating
// window. This mouse-only fixture rejects unsolicited native text events;
// keyboard editing/filter behavior is tested separately with targeted Qt keys.
class Cycle106MouseInputIsolation : public QObject {
    QQuickWindow *window;
    int excludedEvents = 0;
public:
    explicit Cycle106MouseInputIsolation(QQuickWindow *target) : window(target) { qApp->installEventFilter(this); }
    ~Cycle106MouseInputIsolation() override {
        qApp->removeEventFilter(this);
        if (excludedEvents) qInfo() << "Excluded unsolicited native text events during mouse-only QA:" << excludedEvents;
    }
    bool eventFilter(QObject *target, QEvent *event) override {
        if (QGuiApplication::platformName() != "cocoa" || target != window) return false;
        const bool nativeKey = event->spontaneous()
            && (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease);
        if (nativeKey || event->type() == QEvent::InputMethod) {
            ++excludedEvents;
            event->accept();
            return true;
        }
        return false;
    }
};
// QtTest's QWindow key helper labels its events spontaneous on Cocoa, just
// like OS input. Keyboard fixtures send explicit events through the window so
// the isolation filter can still reject unrelated native typing/IME commits.
void cycle111KeyClick(QQuickWindow *window, Qt::Key key, const QString &text = {}) {
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier, text);
    QCoreApplication::sendEvent(window, &press);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier, text);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
}
void cycle111KeyClick(QQuickWindow *window, char key) {
    const QChar character = QChar::fromLatin1(key);
    cycle111KeyClick(window, Qt::Key(character.toUpper().unicode()), QString(character));
}
bool cycle102Click(QQuickWindow *window, QQuickItem *item) {
    // Let Qt Quick polish geometry after the preceding visibility change.
    QTest::qWait(30);
    if (!item || !item->isVisible() || !item->isEnabled() || item->width() <= 0 || item->height() <= 0)
        return false;
    const QPointF point = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
    if (!QRectF(0, 0, window->width(), window->height()).contains(point))
        return false;
    QSignalSpy clicked(item, SIGNAL(clicked()));
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point.toPoint());
    if (clicked.isValid() && clicked.isEmpty()) {
        qWarning() << "Pointer click did not activate" << item->objectName() << point
                   << "item bounds" << item->mapRectToScene(item->boundingRect())
                   << "window" << window->size() << "active" << window->isActive()
                   << "mouse grabber" << window->mouseGrabberItem();
        QQuickItem *hit = window->contentItem();
        while (hit) {
            qWarning() << "Hit path:" << hit->metaObject()->className() << hit->objectName();
            const QPointF local = hit->mapFromScene(point);
            hit = hit->childAt(local.x(), local.y());
        }
        return false;
    }
    return true;
}

bool cycle102Capture(QQuickWindow *window, const QString &name) {
    const QString directory = qEnvironmentVariable("FOMAWRITE_WORKSPACE_CAPTURE_DIR");
    if (directory.isEmpty()) return true;
    if (!QDir().mkpath(directory)) return false;
    // Capture the resting toolbar, rather than the last clicked button’s hover.
    QTest::mouseMove(window, QPoint(window->width() / 2, window->height() / 2));
    QTest::qWait(200);
    return window->grabWindow().save(QDir(directory).filePath(name + ".png"));
}
}

void FomawriteTest::collapsedPaneButtonsRestoreEveryLayout() {
    Backend backend;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create());
    QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *settings = window->findChild<QObject *>("workspaceSettings");
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *editor = window->findChild<QObject *>("sourceEditor");
    auto *drawer = window->findChild<QObject *>("workspaceNavigationDrawer");
    auto *hideOrganizer = window->findChild<QQuickItem *>("collapseOrganizerButton");
    auto *hideFiles = window->findChild<QQuickItem *>("collapseFilesButton");
    auto *showOrganizer = window->findChild<QQuickItem *>("restoreOrganizerButton");
    auto *showFiles = window->findChild<QQuickItem *>("restoreFilesButton");
    auto *back = window->findChild<QQuickItem *>("documentBackButton");
    QVERIFY(quick && settings && layout && editor && drawer && hideOrganizer && hideFiles
            && showOrganizer && showFiles && back);
    const int originalToolbar = settings->property("toolbarVisibilityMode").toInt();
    const auto cleanup = qScopeGuard([&] {
        settings->setProperty("toolbarVisibilityMode", originalToolbar);
        backend.discardRecovery();
    });
    quick->resize(1440, 860);
    settings->setProperty("toolbarVisibilityMode", 1);
    layout->setProperty("layoutMode", 2);
    layout->setProperty("visualEditEnabled", false);
    layout->setProperty("organizerVisible", true);
    layout->setProperty("filesVisible", true);
    quick->show();
    QTRY_VERIFY(hideOrganizer->isVisible() && hideFiles->isVisible());
    const QString source = QStringLiteral("# Pane controls\n\nA calm workspace keeps each column within reach.\n");
    editor->setProperty("text", source);
    QCoreApplication::processEvents();
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, source.size()),
                                     Q_ARG(QString, QStringLiteral("Unsaved draft."))));
    const QString draft = editor->property("text").toString();
    QVERIFY(backend.modified());
    QVERIFY(editor->property("canUndo").toBool());

    // A hidden column's control moves into the surviving writing header.
    QVERIFY(cycle102Click(quick, hideOrganizer));
    QTRY_VERIFY(!layout->property("effectiveOrganizerVisible").toBool());
    QTRY_VERIFY(showOrganizer->isVisible());
    QVERIFY(!showFiles->isVisible());
    QVERIFY(cycle102Capture(quick, "cycle102-library-only"));
    QVERIFY(cycle102Click(quick, showOrganizer));
    QTRY_VERIFY(layout->property("effectiveOrganizerVisible").toBool());
    QVERIFY(!showOrganizer->isVisible());
    QVERIFY(cycle102Click(quick, hideFiles));
    QTRY_VERIFY(!layout->property("effectiveFilesVisible").toBool());
    QTRY_VERIFY(showFiles->isVisible());
    QVERIFY(!showOrganizer->isVisible());
    QVERIFY(cycle102Click(quick, showFiles));
    QTRY_VERIFY(layout->property("effectiveFilesVisible").toBool());

    // Both restoration orders must leave the other hidden pane accessible.
    for (bool organizerFirst : {true, false}) {
        QVERIFY(cycle102Click(quick, hideOrganizer));
        QTRY_VERIFY(showOrganizer->isVisible());
        QVERIFY(cycle102Click(quick, hideFiles));
        QTRY_VERIFY(showFiles->isVisible());
        QTRY_VERIFY(showOrganizer->mapRectToScene(showOrganizer->boundingRect()).right()
                    <= showFiles->mapRectToScene(showFiles->boundingRect()).left());
        QTRY_VERIFY(showFiles->mapRectToScene(showFiles->boundingRect()).right()
                    <= back->mapRectToScene(back->boundingRect()).left());
        QTRY_VERIFY(showOrganizer->mapRectToScene(showOrganizer->boundingRect()).left()
                    >= (window->property("isMac").toBool() ? 84 : 0));
        QVERIFY(cycle102Capture(quick, "cycle102-both-collapsed"));
        // Hiding ordinary toolbar controls must not strand collapsed panes.
        settings->setProperty("toolbarVisibilityMode", 2);
        QVERIFY(showOrganizer->isVisible() && showFiles->isVisible());
        QVERIFY(cycle102Click(quick, organizerFirst ? showOrganizer : showFiles));
        QTRY_VERIFY(layout->property(organizerFirst ? "effectiveOrganizerVisible" : "effectiveFilesVisible").toBool());
        QVERIFY((organizerFirst ? showFiles : showOrganizer)->isVisible());
        QVERIFY(cycle102Click(quick, organizerFirst ? showFiles : showOrganizer));
        QTRY_VERIFY(layout->property("effectiveOrganizerVisible").toBool());
        QTRY_VERIFY(layout->property("effectiveFilesVisible").toBool());
        settings->setProperty("toolbarVisibilityMode", 1);
    }

    // Auto-contraction opens the matching Drawer without changing desired panes.
    layout->setProperty("layoutMode", 1);
    quick->resize(720, 760);
    QTRY_VERIFY(!layout->property("effectiveOrganizerVisible").toBool());
    QTRY_VERIFY(!layout->property("effectiveFilesVisible").toBool());
    QTRY_VERIFY(showOrganizer->isVisible() && showFiles->isVisible());
    QVERIFY(cycle102Capture(quick, "cycle102-compact-controls"));
    for (const QString &pane : {QStringLiteral("organizer"), QStringLiteral("files")}) {
        QVERIFY(cycle102Click(quick, pane == "organizer" ? showOrganizer : showFiles));
        QTRY_VERIFY(drawer->property("opened").toBool());
        QCOMPARE(drawer->property("paneName").toString(), pane);
        QVERIFY(QMetaObject::invokeMethod(drawer, "close"));
        QTRY_VERIFY(!drawer->property("visible").toBool());
    }
    QVERIFY(layout->property("organizerVisible").toBool());
    QVERIFY(layout->property("filesVisible").toBool());
    QCOMPARE(editor->property("text").toString(), draft);
    QVERIFY(backend.modified());
    QVERIFY(editor->property("canUndo").toBool());
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QCOMPARE(editor->property("text").toString(), source);
    QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
    QCOMPARE(editor->property("text").toString(), draft);
}

void FomawriteTest::previewTemplateFooterAndNativeMenuShareState() {
    Backend backend;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create());
    QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *editor = window->findChild<QObject *>("sourceEditor");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    QVERIFY(pane);
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    auto *button = footer ? footer->findChild<QQuickItem *>("previewTemplateButton") : nullptr;
    auto *menu = window->findChild<QObject *>("previewTemplateMenu");
    auto *palatino = window->findChild<QQuickItem *>("previewTemplate6");
    auto *nativeView = window->findChild<QObject *>("nativeViewMenu");
    auto *nativePalatino = window->findChild<QObject *>("nativeTemplate6");
    auto *nativeHelvetica = window->findChild<QObject *>("nativeTemplate5");
    QVERIFY(quick && layout && editor && button && menu && palatino && nativeView && nativePalatino && nativeHelvetica);
    Cycle106MouseInputIsolation mouseInput(quick);
    const int originalStyle = backend.outputStyle();
    const auto cleanup = qScopeGuard([&] { backend.setOutputStyle(originalStyle); backend.discardRecovery(); });
    quick->resize(1100, 800);
    layout->setProperty("layoutMode", 2);
    layout->setProperty("visualEditEnabled", false);
    layout->setProperty("organizerVisible", false);
    layout->setProperty("filesVisible", false);
#ifdef Q_OS_MACOS
    if (QGuiApplication::platformName() == "cocoa") quick->setFlag(Qt::WindowDoesNotAcceptFocus);
#endif
    quick->show();
    const QString source = QStringLiteral("# Preview templates\n\nChoose a template without changing the Markdown.\n");
    editor->setProperty("text", source);
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, source.size()),
                                     Q_ARG(QString, QStringLiteral("Unsaved draft."))));
    const QString draft = editor->property("text").toString();
    backend.setOutputStyle(0);

    // Inspect the actual native menu model, including submenu separators.
    QQmlListReference items(nativeView, "items");
    QVERIFY(items.isValid());
    QVERIFY(items.count() > 5);
    QCOMPARE(items.at(0)->objectName(), QStringLiteral("native_library"));
    QCOMPARE(items.at(1)->objectName(), QStringLiteral("native_organizer"));
    QCOMPARE(items.at(0)->property("text").toString(), QStringLiteral("Toggle Library"));
    QCOMPARE(items.at(1)->property("text").toString(), QStringLiteral("Toggle Organizer"));
    int templateIndex = -1;
    for (qsizetype index = 0; index < items.count(); ++index)
        if (items.at(index)->objectName() == "nativeTemplateEntry") templateIndex = index;
    QVERIFY(templateIndex > 2 && templateIndex + 1 < items.count());
    QCOMPARE(items.at(templateIndex - 1)->objectName(), QStringLiteral("nativeBeforeTemplate"));
    QCOMPARE(items.at(templateIndex + 1)->objectName(), QStringLiteral("nativeAfterTemplate"));
    QVERIFY(items.at(templateIndex - 1)->property("separator").toBool());
    QVERIFY(items.at(templateIndex + 1)->property("separator").toBool());
    for (int index : {0, 1}) {
        QVERIFY(QMetaObject::invokeMethod(items.at(index), "triggered"));
        QTRY_VERIFY(items.at(index)->property("checked").toBool());
        QCOMPARE(items.at(index)->property("text").toString(), index == 0 ? QStringLiteral("Toggle Library") : QStringLiteral("Toggle Organizer"));
        QVERIFY(QMetaObject::invokeMethod(items.at(index), "triggered"));
        QTRY_VERIFY(!items.at(index)->property("checked").toBool());
    }
    // A quick show/hide pair must cancel any deferred compact-navigation open.
    QCoreApplication::processEvents();
    auto *drawer = window->findChild<QObject *>("workspaceNavigationDrawer");
    QVERIFY(drawer);
    QVERIFY(!drawer->property("visible").toBool());
    QTRY_VERIFY(button->isVisible());
    QVERIFY(cycle102Capture(quick, "cycle102-before-template-click"));
    QVERIFY(cycle102Click(quick, button));
    QTRY_VERIFY(menu->property("opened").toBool());
    auto *menuParent = qvariant_cast<QQuickItem *>(menu->property("parent"));
    QVERIFY(menuParent);
    // The footer trigger remains visible below its menu, away from menu commands.
    QTRY_VERIFY(menu->property("y").toReal() + menu->property("height").toReal()
                <= button->mapToItem(menuParent, QPointF(0, 0)).y() - 4);
    QVERIFY(cycle102Capture(quick, "cycle102-template-selector"));
    QVERIFY(cycle102Click(quick, palatino));
    QTRY_COMPARE(backend.outputStyle(), 6);
    QTRY_VERIFY(!menu->property("visible").toBool());
    QCOMPARE(button->property("text").toString(), QStringLiteral("Output · Palatino"));
    QVERIFY(nativePalatino->property("checked").toBool());
    QVERIFY(!nativeHelvetica->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(nativeHelvetica, "triggered"));
    QTRY_COMPARE(backend.outputStyle(), 5);
    QCOMPARE(button->property("text").toString(), QStringLiteral("Output · Helvetica"));
    QVERIFY(!nativePalatino->property("checked").toBool());
    QVERIFY(nativeHelvetica->property("checked").toBool());
    QCOMPARE(editor->property("text").toString(), draft);
    QVERIFY(backend.modified());
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QCOMPARE(editor->property("text").toString(), source);
    QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
    QCOMPARE(editor->property("text").toString(), draft);
}

void FomawriteTest::previewTemplateRemainsReachableInNarrowSplit() {
    Backend backend;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create());
    QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    QVERIFY(pane);
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    auto *button = footer ? footer->findChild<QQuickItem *>("previewTemplateButton") : nullptr;
    auto *menu = window->findChild<QObject *>("previewTemplateMenu");
    auto *visual = footer->findChild<QQuickItem *>("visualEditToggle");
    auto *split = footer->findChild<QQuickItem *>("previewSplitButton");
    auto *single = footer->findChild<QQuickItem *>("singleModeButton");
    auto *source = footer->findChild<QQuickItem *>("sourceModeButton");
    QVERIFY(quick && layout && pane && button && menu && visual && split && single && source);
    Cycle106MouseInputIsolation mouseInput(quick);
    const int originalStyle = backend.outputStyle();
    const auto cleanup = qScopeGuard([&] { backend.setOutputStyle(originalStyle); });
    quick->resize(960, 760);
    layout->setProperty("organizerVisible", false);
    layout->setProperty("filesVisible", false);
    layout->setProperty("layoutMode", 1);
    layout->setProperty("visualEditEnabled", false);
#ifdef Q_OS_MACOS
    if (QGuiApplication::platformName() == "cocoa") quick->setFlag(Qt::WindowDoesNotAcceptFocus);
#endif
    quick->show();
    // Long names must elide while retaining the dropdown affordance and hit area.
    for (int width : {320, 360}) {
        layout->setProperty("previewWidth", width);
        QTRY_COMPARE(qRound(pane->width()), width);
        for (int style : {0, 2, 6}) {
            backend.setOutputStyle(style);
            QTRY_VERIFY(button->isVisible());
            QCoreApplication::processEvents();
            QRectF previous;
            for (auto *control : {source, visual, button, single, split}) {
                const QRectF bounds = control->mapRectToItem(footer, control->boundingRect());
                QVERIFY2(bounds.left() >= 11 && bounds.right() <= footer->width() - 11,
                         qPrintable(QString("%1 outside %2px footer: x=%3 width=%4").arg(control->objectName()).arg(width).arg(bounds.x()).arg(bounds.width())));
                QVERIFY(bounds.height() >= 32);
                if (!previous.isNull()) {
                    // Source/Visual and Single/Split each form a separate capsule.
                    const qreal gap = control == visual || control == split ? 0 : 3;
                    QVERIFY(previous.right() + gap <= bounds.left());
                }
                previous = bounds;
            }
            QVERIFY(cycle102Click(quick, button));
            QTRY_VERIFY(menu->property("opened").toBool());
            const qreal x = menu->property("x").toReal();
            const qreal y = menu->property("y").toReal();
            QVERIFY(x >= 0 && y >= 0);
            QVERIFY(x + menu->property("width").toReal() <= quick->width());
            QVERIFY(y + menu->property("height").toReal() <= quick->contentItem()->height());
            auto *menuParent = qvariant_cast<QQuickItem *>(menu->property("parent"));
            QVERIFY(menuParent);
            QVERIFY(y + menu->property("height").toReal()
                    <= button->mapToItem(menuParent, QPointF(0, 0)).y() - 4);
            if (width == 320 && style == 6) QVERIFY(cycle102Capture(quick, "cycle102-narrow-template-selector"));
            QVERIFY(QMetaObject::invokeMethod(menu, "close"));
            QTRY_VERIFY(!menu->property("visible").toBool());
        }
    }
}
