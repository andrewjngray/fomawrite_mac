namespace {
bool cycle112ZoomMenuClick(QQuickWindow *window, const QString &pane, const QString &entry) {
    auto *button = window->findChild<QQuickItem *>(pane + "ZoomMenuButton");
    if (!cycle102Click(window, button)) return false;
    auto *menu = window->findChild<QObject *>(pane + "ZoomMenu");
    if (!menu || !menu->property("opened").toBool()) return false;
    auto *item = window->findChild<QQuickItem *>(pane + "Zoom" + entry);
    return cycle102Click(window, item);
}
QVariantMap cycle112Anchor(QObject *owner, const char *method) {
    QVariant result;
    if (!QMetaObject::invokeMethod(owner, method, Q_RETURN_ARG(QVariant, result))) return {};
    if (result.canConvert<QJSValue>()) return result.value<QJSValue>().toVariant().toMap();
    return result.toMap();
}
qreal cycle112AnchorOffset(QQuickItem *field, QObject *scroll, int position) {
    QRectF rect;
    if (!QMetaObject::invokeMethod(field, "positionToRectangle", Q_RETURN_ARG(QRectF, rect), Q_ARG(int, position))) return -999999;
    return field->y() + rect.y() - scroll->property("contentY").toReal();
}
}

void FomawriteTest::paneZoomPresetsLinkAndPersistWithoutEditing() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QByteArray saved = QString::fromUtf8("# Independent magnification\n\nA café draft with **Markdown** and 你好.\n").toUtf8();
    const QString path = directory.filePath("Zoom.md"); QVERIFY(cycle103Write(path, saved));
    {
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *quick = qobject_cast<QQuickWindow *>(window.data());
        auto *zoom = window->findChild<QObject *>("paneZoomState");
        auto *layout = window->findChild<QObject *>("workspaceLayout");
        auto *editor = window->findChild<QQuickItem *>("sourceEditor");
        auto *preview = window->findChild<QQuickItem *>("previewPane");
        QVERIFY(quick && zoom && layout && editor && preview);
        cycle106KeepNativeFocus(quick); Cycle106MouseInputIsolation input(quick);
        quick->resize(1440, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
        QVERIFY(backend.open(QUrl::fromLocalFile(path)));
        const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
        QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
        layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
        QVERIFY(QMetaObject::invokeMethod(window.data(), "setDocumentView", Q_ARG(QVariant, 1)));
        QTRY_VERIFY(!window->property("changingDocumentView").toBool());
        QVERIFY(editor->isVisible() && preview->isVisible());
        QVERIFY(!zoom->property("linked").toBool());
        QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, editor->property("text").toString().size()), Q_ARG(QString, QStringLiteral("Unsaved ending."))));
        const QString draft = editor->property("text").toString();
        QVERIFY(draft.endsWith("Unsaved ending.")); QVERIFY(backend.modified());
        QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 4), Q_ARG(int, 15)));
        editor->forceActiveFocus();
        const QString before = directory.filePath("before.html"), after = directory.filePath("after.html");
        QVERIFY(backend.exportDocument(QUrl::fromLocalFile(before), "html"));

        QVERIFY(cycle112ZoomMenuClick(quick, "source", "Preset125"));
        QTRY_COMPARE(zoom->property("sourceZoom").toInt(), 125);
        QCOMPARE(zoom->property("previewZoom").toInt(), 100);
        QVERIFY(editor->hasActiveFocus());
        // Reselecting the active preset must leave its checkmark bound to state.
        QVERIFY(cycle112ZoomMenuClick(quick, "source", "Preset125"));
        QVERIFY(window->findChild<QObject *>("sourceZoomPreset125")->property("checked").toBool());
        QVERIFY(cycle112ZoomMenuClick(quick, "preview", "Preset175"));
        QCOMPARE(zoom->property("sourceZoom").toInt(), 125); QCOMPARE(zoom->property("previewZoom").toInt(), 175);
        QVERIFY(cycle112ZoomMenuClick(quick, "preview", "Link"));
        QVERIFY(zoom->property("linked").toBool());
        QCOMPARE(zoom->property("sourceZoom").toInt(), 175); QCOMPARE(zoom->property("previewZoom").toInt(), 175);
        QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>("sourceZoomOutButton")));
        QCOMPARE(zoom->property("sourceZoom").toInt(), 165); QCOMPARE(zoom->property("previewZoom").toInt(), 165);
        QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>("previewZoomInButton")));
        QCOMPARE(zoom->property("sourceZoom").toInt(), 175); QCOMPARE(zoom->property("previewZoom").toInt(), 175);
        QVERIFY(cycle112ZoomMenuClick(quick, "source", "Reset"));
        QCOMPARE(zoom->property("sourceZoom").toInt(), 100); QCOMPARE(zoom->property("previewZoom").toInt(), 100);
        QVERIFY(cycle112ZoomMenuClick(quick, "source", "Link"));
        QVERIFY(!zoom->property("linked").toBool());
        QCOMPARE(zoom->property("sourceZoom").toInt(), 100); QCOMPARE(zoom->property("previewZoom").toInt(), 100);
        QVERIFY(cycle112ZoomMenuClick(quick, "source", "Preset200"));
        QVERIFY(!window->findChild<QQuickItem *>("sourceZoomInButton")->isEnabled());
        QVERIFY(window->findChild<QQuickItem *>("sourceZoomOutButton")->isEnabled());
        QCOMPARE(zoom->property("previewZoom").toInt(), 100);
        QVERIFY(cycle112ZoomMenuClick(quick, "preview", "Preset75"));
        QVERIFY(!window->findChild<QQuickItem *>("previewZoomOutButton")->isEnabled());
        QVERIFY(window->findChild<QQuickItem *>("previewZoomInButton")->isEnabled());
        QCOMPARE(zoom->property("sourceZoom").toInt(), 200);
        QVERIFY(cycle112ZoomMenuClick(quick, "source", "Preset125"));
        QVERIFY(cycle112ZoomMenuClick(quick, "preview", "Preset150"));
        QTRY_VERIFY(!window->property("changingDocumentView").toBool());
        // The View menu's shared text-size actions target the focused pane.
        auto *commands = window->findChild<QObject *>("workspaceCommands");
        auto *rendered = preview->findChild<QQuickItem *>("renderedPreview"); QVERIFY(commands && rendered);
        rendered->forceActiveFocus();
        QTRY_COMPARE(zoom->property("effectivePane").toString(), QString("preview"));
        QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, "larger")));
        QCOMPARE(zoom->property("previewZoom").toInt(), 160); QCOMPARE(zoom->property("sourceZoom").toInt(), 125);
        QVERIFY(cycle112ZoomMenuClick(quick, "preview", "Preset150"));
        editor->forceActiveFocus();
        QTRY_COMPARE(zoom->property("effectivePane").toString(), QString("source"));
        QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, "smaller")));
        QCOMPARE(zoom->property("sourceZoom").toInt(), 115); QCOMPARE(zoom->property("previewZoom").toInt(), 150);
        QVERIFY(cycle112ZoomMenuClick(quick, "source", "Preset125"));
        QTRY_VERIFY(!window->property("changingDocumentView").toBool());
        QCOMPARE(editor->property("selectionStart").toInt(), 4); QCOMPARE(editor->property("selectionEnd").toInt(), 15);
        QCOMPARE(editor->property("text").toString(), draft);
        QVERIFY(backend.exportDocument(QUrl::fromLocalFile(after), "html"));
        QCOMPARE(cycle103Read(after), cycle103Read(before));
        QVERIFY(QMetaObject::invokeMethod(editor, "undo")); QCOMPARE(editor->property("text").toString(), QString::fromUtf8(saved));
        QVERIFY(QMetaObject::invokeMethod(editor, "redo")); QCOMPARE(editor->property("text").toString(), draft);
        QCOMPARE(cycle103Read(path), saved);
    }
    QSettings().sync();
    // A new window inherits both independent values without another migration.
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *zoom = window->findChild<QObject *>("paneZoomState"); QVERIFY(zoom);
    QCOMPARE(zoom->property("sourceZoom").toInt(), 125); QCOMPARE(zoom->property("previewZoom").toInt(), 150);
    QVERIFY(!zoom->property("linked").toBool());
    QVERIFY(QMetaObject::invokeMethod(zoom, "setLinked", Q_ARG(QVariant, true), Q_ARG(QVariant, "preview")));
    window.reset(); QSettings().sync();
    window.reset(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    zoom = window->findChild<QObject *>("paneZoomState"); QVERIFY(zoom);
    QVERIFY(zoom->property("linked").toBool());
    QCOMPARE(zoom->property("sourceZoom").toInt(), 150); QCOMPARE(zoom->property("previewZoom").toInt(), 150);
}

void FomawriteTest::paneZoomMigrationKeepsLegacySourceAppearance() {
    Cycle103SettingsSnapshot isolatedSettings;
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"QML(import QtQuick
import "qrc:/"
Item {
    property alias preferences: preferences
    QtObject {
        id: preferences
        property int sourceZoom: 100; property int previewZoom: 100
        property bool linkedZoom: false; property int zoomRevision: 0
        property int writingSize: 22
    }
    PaneZoomState { settings: preferences; legacyAppearanceOffset: 3 }
})QML", QUrl());
    QScopedPointer<QObject> root(component.create()); QVERIFY2(root, qPrintable(component.errorString()));
    auto *zoom = root->findChild<QObject *>("paneZoomState"); QVERIFY(zoom);
    auto *settings = qvariant_cast<QObject *>(root->property("preferences")); QVERIFY(settings);
    QVERIFY(QMetaObject::invokeMethod(zoom, "initialize"));
    QCOMPARE(zoom->property("sourceZoom").toInt(), 132); // 25px old Editorial / 19px new baseline.
    QCOMPARE(qRound(19 * zoom->property("sourceZoom").toInt() / 100.0), 25);
    QCOMPARE(zoom->property("previewZoom").toInt(), 100);
    QVERIFY(!zoom->property("linked").toBool()); QCOMPARE(settings->property("zoomRevision").toInt(), 1);
    QVERIFY(QMetaObject::invokeMethod(zoom, "setZoom", Q_ARG(QVariant, "source"), Q_ARG(QVariant, 125)));
    QVERIFY(QMetaObject::invokeMethod(zoom, "setZoom", Q_ARG(QVariant, "preview"), Q_ARG(QVariant, 150)));
    settings->setProperty("writingSize", 32);
    QVERIFY(QMetaObject::invokeMethod(zoom, "initialize"));
    QCOMPARE(zoom->property("sourceZoom").toInt(), 125); QCOMPARE(zoom->property("previewZoom").toInt(), 150);
    // Invalid settings are bounded rather than producing an unusable surface.
    settings->setProperty("sourceZoom", -10); settings->setProperty("previewZoom", 900);
    QVERIFY(QMetaObject::invokeMethod(zoom, "initialize"));
    QCOMPARE(zoom->property("sourceZoom").toInt(), 75); QCOMPARE(zoom->property("previewZoom").toInt(), 200);
    QVERIFY(QMetaObject::invokeMethod(zoom, "setLinked", Q_ARG(QVariant, true), Q_ARG(QVariant, "preview")));
    QCOMPARE(zoom->property("sourceZoom").toInt(), 200); QCOMPARE(zoom->property("previewZoom").toInt(), 200);
    QVERIFY(QMetaObject::invokeMethod(zoom, "adjustZoom", Q_ARG(QVariant, "source"), Q_ARG(QVariant, 10)));
    QCOMPARE(zoom->property("sourceZoom").toInt(), 200); QCOMPARE(zoom->property("previewZoom").toInt(), 200);
    QVERIFY(QMetaObject::invokeMethod(zoom, "resetZoom", Q_ARG(QVariant, "preview")));
    QCOMPARE(zoom->property("sourceZoom").toInt(), 100); QCOMPARE(zoom->property("previewZoom").toInt(), 100);
    // Verify Main actually performs the migration on an existing preference set.
    QSettings persisted;
    persisted.setValue("workspace/appearanceRevision", 1);
    persisted.setValue("workspace/writingAppearance", "editorial");
    persisted.setValue("workspace/writingSize", 22);
    persisted.setValue("workspace/zoomRevision", 0);
    persisted.sync();
    Backend backend;
    QQmlEngine mainEngine; mainEngine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent main(&mainEngine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(main.create()); QVERIFY2(window, qPrintable(main.errorString()));
    auto *migrated = window->findChild<QObject *>("paneZoomState");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor"); QVERIFY(migrated && editor);
    QCOMPARE(migrated->property("sourceZoom").toInt(), 132);
    QCOMPARE(migrated->property("previewZoom").toInt(), 100);
    QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 25);
}

void FomawriteTest::documentDividerDoubleClickEqualizesWithoutChangingZoom() {
    Cycle103SettingsSnapshot isolatedSettings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *zoom = window->findChild<QObject *>("paneZoomState");
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *editorPane = window->findChild<QQuickItem *>("editorPane");
    auto *sourceScroll = window->findChild<QObject *>("editorScroll");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    auto *preview = pane ? pane->findChild<QQuickItem *>("renderedPreview") : nullptr;
    auto *previewScroll = pane ? pane->findChild<QObject *>("previewScroll") : nullptr;
    QVERIFY(quick && zoom && layout && editor && editorPane && pane && preview && sourceScroll && previewScroll);
    cycle106KeepNativeFocus(quick); Cycle106MouseInputIsolation input(quick);
    quick->resize(1400, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    QString source;
    for (int index = 0; index < 140; ++index)
        source += QStringLiteral("Paragraph %1 has a café sentence long enough to rewrap when each independent pane changes magnification.\n\n").arg(index, 3, 10, QLatin1Char('0'));
    editor->setProperty("text", source);
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "setDocumentView", Q_ARG(QVariant, 1)));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_VERIFY(!pane->property("viewportRefreshPending").toBool());
    sourceScroll->setProperty("contentY", 1200.0); previewScroll->setProperty("contentY", 1800.0);
    QCoreApplication::processEvents();
    const auto sourceAnchor = cycle112Anchor(window.data(), "captureSourceReadingAnchor");
    const auto previewAnchor = cycle112Anchor(pane, "captureReadingAnchor");
    QVERIFY(!sourceAnchor.isEmpty() && !previewAnchor.isEmpty());
    QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>("sourceZoomInButton")));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QVERIFY(qAbs(cycle112AnchorOffset(editor, sourceScroll, sourceAnchor["position"].toInt()) - sourceAnchor["offset"].toReal()) <= 2);
    QVERIFY(qAbs(cycle112AnchorOffset(preview, previewScroll, previewAnchor["position"].toInt()) - previewAnchor["offset"].toReal()) <= 2);
    QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>("previewZoomInButton")));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QVERIFY(qAbs(cycle112AnchorOffset(preview, previewScroll, previewAnchor["position"].toInt()) - previewAnchor["offset"].toReal()) <= 2);
    QVERIFY(QMetaObject::invokeMethod(layout, "updateWidth", Q_ARG(QVariant, "preview"), Q_ARG(QVariant, 400)));
    QTRY_COMPARE(qRound(pane->width()), 400);
    QVERIFY(qAbs(editorPane->width() - pane->width()) > 100);
    QTest::qWait(30); // Wait for the resized divider position to be painted.
    auto *divider = window->findChild<QQuickItem *>("documentSplitDivider"); QVERIFY(divider && divider->isVisible());
    const auto point = divider->mapToScene(QPointF(divider->width() / 2, divider->height() / 2)).toPoint();
    QTest::mouseDClick(quick, Qt::LeftButton, Qt::NoModifier, point);
    QTRY_VERIFY_WITH_TIMEOUT(qAbs(editorPane->width() - pane->width()) <= 2, 1500);
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QCOMPARE(zoom->property("sourceZoom").toInt(), 110); QCOMPARE(zoom->property("previewZoom").toInt(), 110);
    // The double-click target must not steal the ordinary divider drag.
    QTest::qWait(30);
    const qreal beforeDrag = pane->width();
    const auto dragStart = divider->mapToScene(QPointF(divider->width() / 2, divider->height() / 2)).toPoint();
    const auto dragEnd = dragStart + QPoint(80, 0);
    QTest::mousePress(quick, Qt::LeftButton, Qt::NoModifier, dragStart);
    QTest::mouseMove(quick, dragEnd, 30);
    QTest::mouseRelease(quick, Qt::LeftButton, Qt::NoModifier, dragEnd, 30);
    QTRY_VERIFY_WITH_TIMEOUT(pane->width() < beforeDrag - 40, 1500);
    QTRY_VERIFY(qAbs(layout->property("previewWidth").toReal() - pane->width()) <= 2);
    const qreal dragged = pane->width();
    QVERIFY(QMetaObject::invokeMethod(window.data(), "setDocumentView", Q_ARG(QVariant, 0)));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QVERIFY(QMetaObject::invokeMethod(window.data(), "setDocumentView", Q_ARG(QVariant, 1)));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_VERIFY(qAbs(pane->width() - dragged) <= 2);
    QCOMPARE(zoom->property("sourceZoom").toInt(), 110); QCOMPARE(zoom->property("previewZoom").toInt(), 110);
    QCOMPARE(editor->property("text").toString(), source);
}

void FomawriteTest::paneZoomPresetsPreserveReadingAnchors() {
    Cycle103SettingsSnapshot isolatedSettings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *sourceScroll = window->findChild<QObject *>("editorScroll");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    auto *preview = pane ? pane->findChild<QQuickItem *>("renderedPreview") : nullptr;
    auto *previewScroll = pane ? pane->findChild<QObject *>("previewScroll") : nullptr;
    QVERIFY(quick && layout && editor && sourceScroll && pane && preview && previewScroll);
    cycle106KeepNativeFocus(quick); Cycle106MouseInputIsolation input(quick);
    quick->resize(1400, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "applyReferenceWorkspace"));
    QString source;
    for (int i = 0; i < 100; ++i)
        source += QStringLiteral("Paragraph %1 has a café sentence with **bold text** and enough words to wrap to several lines at independent reading sizes.\n\n").arg(i, 3, 10, QLatin1Char('0'));
    editor->setProperty("text", source);
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
    QVERIFY(QMetaObject::invokeMethod(window.data(), "setDocumentView", Q_ARG(QVariant, 1)));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_VERIFY(!pane->property("viewportRefreshPending").toBool());
    sourceScroll->setProperty("contentY", 1250.0); previewScroll->setProperty("contentY", 1840.0);
    QCoreApplication::processEvents();
    auto sourceAnchor = cycle112Anchor(window.data(), "captureSourceReadingAnchor");
    auto previewAnchor = cycle112Anchor(pane, "captureReadingAnchor");
    QVERIFY(!sourceAnchor.isEmpty() && !previewAnchor.isEmpty());
    for (int cycle = 0; cycle < 3; ++cycle) {
        for (const auto &[target, action] : {std::pair{"source", "Preset125"}, std::pair{"source", "Reset"},
                                           std::pair{"preview", "Preset125"}, std::pair{"preview", "Reset"}}) {
            QVERIFY(cycle112ZoomMenuClick(quick, target, action));
            QTRY_VERIFY(!window->property("changingDocumentView").toBool());
            QTRY_VERIFY(!pane->property("viewportRefreshPending").toBool());
            const qreal sourceDelta = cycle112AnchorOffset(editor, sourceScroll, sourceAnchor["position"].toInt()) - sourceAnchor["offset"].toReal();
            const qreal previewDelta = cycle112AnchorOffset(preview, previewScroll, previewAnchor["position"].toInt()) - previewAnchor["offset"].toReal();
            QVERIFY2(qAbs(sourceDelta) <= 2, qPrintable(QString("Source anchor drifted %1px after %2 %3, cycle %4").arg(sourceDelta).arg(target, action).arg(cycle)));
            QVERIFY2(qAbs(previewDelta) <= 2, qPrintable(QString("Preview anchor drifted %1px after %2 %3, cycle %4").arg(previewDelta).arg(target, action).arg(cycle)));
            QCOMPARE(editor->property("text").toString(), source);
        }
        if (cycle == 1) {
            // A real scroll gesture must supersede the prior zoom checkpoint.
            // Both panes receive wheel events through the window, not a
            // direct scroll-property update or a call to the invalidation helper.
            for (auto *scroll : {sourceScroll, previewScroll}) {
                const qreal oldY = scroll->property("contentY").toReal();
                auto *item = qobject_cast<QQuickItem *>(scroll); QVERIFY(item);
                const QPointF local = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
                QWheelEvent wheel(local, quick->mapToGlobal(local.toPoint()), QPoint(), QPoint(0, -360),
                                  Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
                QCoreApplication::sendEvent(quick, &wheel);
                QTRY_VERIFY2(qAbs(scroll->property("contentY").toReal() - oldY) > 80, qPrintable(scroll->objectName()));
            }
            QTRY_VERIFY(!sourceScroll->property("moving").toBool() && !previewScroll->property("moving").toBool());
            // The source's wheel easing is separate from Flickable.moving.
            // Capture only after both real scrolling animations stop.
            qreal lastSourceY = -1, lastPreviewY = -1;
            int stableSamples = 0;
            QElapsedTimer settled; settled.start();
            while (stableSamples < 3 && settled.elapsed() < 1500) {
                QTest::qWait(40);
                const qreal sy = sourceScroll->property("contentY").toReal();
                const qreal py = previewScroll->property("contentY").toReal();
                stableSamples = qAbs(sy - lastSourceY) < 0.01 && qAbs(py - lastPreviewY) < 0.01 ? stableSamples + 1 : 0;
                lastSourceY = sy; lastPreviewY = py;
            }
            QCOMPARE(stableSamples, 3);
            sourceAnchor = cycle112Anchor(window.data(), "captureSourceReadingAnchor");
            previewAnchor = cycle112Anchor(pane, "captureReadingAnchor");
        }
    }
}
