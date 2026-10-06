void FomawriteTest::findReplaceContinuesFromCaretAndPreservesUndo() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory;
    const QString source = QString::fromUtf8("# A café draft\n\nOne café. Two café. Three café.\n");
    const QString path = directory.filePath("Find draft.md");
    QVERIFY(cycle103Write(path, source.toUtf8()));
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *query = window->findChild<QQuickItem *>("searchField");
    auto *replacement = window->findChild<QQuickItem *>("replaceField");
    auto *layout = window->findChild<QObject *>("workspaceLayout");
    auto *footer = window->findChild<QQuickItem *>("documentFooter");
    QVERIFY(quick && editor && query && replacement && layout && footer);
    cycle106KeepNativeFocus(quick);
    Cycle106MouseInputIsolation mouseInput(quick);
    quick->resize(1120, 740); quick->show();
    QVERIFY(QTest::qWaitForWindowExposed(quick));
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_VERIFY(footer->findChild<QQuickItem *>("sourceModeButton")->isVisible());
    QVERIFY(cycle102Click(quick, footer->findChild<QQuickItem *>("sourceModeButton")));
    const int second = source.indexOf(QString::fromUtf8("café"), source.indexOf("Two"));
    QVERIFY(editor->setProperty("cursorPosition", second));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "openSearch", Q_ARG(QVariant, false), Q_ARG(QVariant, false)));
    query->setProperty("text", QString::fromUtf8("café"));
    QTRY_COMPARE(editor->property("selectionStart").toInt(), second);
    QCOMPARE(editor->property("text").toString(), source);
    QVERIFY(!backend.modified());
    QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>("findReplaceToggle")));
    QTRY_VERIFY(replacement->isVisible());
    replacement->setProperty("text", QString::fromUtf8("café noir"));
    QVERIFY(cycle102Click(quick, window->findChild<QQuickItem *>("replaceCurrentButton")));
    QString changed = source; changed.replace(second, 4, QString::fromUtf8("café noir"));
    QCOMPARE(editor->property("text").toString(), changed);
    QCOMPARE(editor->property("selectionStart").toInt(), changed.lastIndexOf(QString::fromUtf8("café")));
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QCOMPARE(editor->property("text").toString(), source);
    // Keeping Find open must not make ordinary source typing or Undo jump to
    // a match. The next Find action can still use the refreshed match count.
    const int typingPosition = source.indexOf("Three") + 2;
    QVERIFY(editor->setProperty("cursorPosition", typingPosition));
    editor->forceActiveFocus();
    cycle111KeyClick(quick, 'x');
    QString editedSource = source; editedSource.insert(typingPosition, QLatin1Char('x'));
    QCOMPARE(editor->property("text").toString(), editedSource);
    QCOMPARE(editor->property("selectionStart").toInt(), typingPosition + 1);
    QCOMPARE(editor->property("selectionEnd").toInt(), typingPosition + 1);
    QVERIFY(editor->hasActiveFocus());
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QCOMPARE(editor->property("text").toString(), source);
    QCOMPARE(editor->property("selectionStart").toInt(), typingPosition);
    QCOMPARE(editor->property("selectionEnd").toInt(), typingPosition);
    QVERIFY(editor->hasActiveFocus());
    QVERIFY(window->property("searchOpen").toBool());
    query->setProperty("text", "missing phrase");
    QCOMPARE(window->findChild<QObject *>("searchMatchStatus")->property("text").toString(), "No matches");
    QVERIFY(!window->findChild<QQuickItem *>("findNextButton")->isEnabled());
    QVERIFY(!window->findChild<QQuickItem *>("replaceAllButton")->isEnabled());
    replacement->forceActiveFocus();
    cycle111KeyClick(quick, Qt::Key_Escape);
    QTRY_VERIFY(!window->property("searchOpen").toBool());
    QTRY_VERIFY(editor->hasActiveFocus());
    QCOMPARE(editor->property("text").toString(), source);
    QFile saved(path); QVERIFY(saved.open(QIODevice::ReadOnly)); QCOMPARE(saved.readAll(), source.toUtf8());
    // Preview Only must not leave an invisible Find field consuming shortcuts.
    QVERIFY(QMetaObject::invokeMethod(window.data(), "openSearch", Q_ARG(QVariant, false), Q_ARG(QVariant, false)));
    QVERIFY(cycle118ReadPreview(window.data()));
    QTRY_VERIFY(!window->property("searchOpen").toBool());
    QTRY_COMPARE(layout->property("effectiveLayoutMode").toInt(), 2);
    // Find opened during a pending Preview transition takes keyboard
    // ownership immediately and keeps it after the old transition would finish.
    for (const QString &mode : {QStringLiteral("preview")}) {
        QVERIFY(QMetaObject::invokeMethod(window.data(), "closeSearch", Q_ARG(QVariant, false)));
        QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, "source")));
        QTRY_VERIFY(!window->property("changingDocumentView").toBool());
        QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, mode)));
        QVERIFY(window->property("changingDocumentView").toBool());
        QVERIFY(QMetaObject::invokeMethod(window.data(), "openSearch", Q_ARG(QVariant, false), Q_ARG(QVariant, false)));
        QTRY_VERIFY(query->hasActiveFocus());
        QTest::qWait(180);
        QVERIFY(query->hasActiveFocus());
        QVERIFY(!window->property("changingDocumentView").toBool());
        QVERIFY(editor->isVisible());
        QCOMPARE(editor->property("text").toString(), source);
    }
}

void FomawriteTest::findControlsFitNarrowPanelsAndStayStationary() {
    Cycle103SettingsSnapshot isolatedSettings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine);
    component.setData(R"QML(import QtQuick
import QtQuick.Window
import "qrc:/"
Window { width: 600; height: 240; DocumentFindBar { objectName: "bar"; x: 20; y: 20; width: parent.width - 40 } }
)QML", QUrl());
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *bar = window->findChild<QQuickItem *>("bar"); QVERIFY(quick && bar);
    cycle106KeepNativeFocus(quick); quick->show();
    for (bool dark : {false, true}) {
        backend.setThemePreset(dark ? "dark" : "studio");
        for (int width : {320, 480, 720}) {
            quick->resize(width, 240);
            bar->setProperty("replaceVisible", true);
            QTest::qWait(40);
            QHash<QString, QRectF> actions;
            for (const QString &name : {"findPreviousButton", "findNextButton", "findCloseButton", "findReplaceToggle", "replaceCurrentButton", "replaceAllButton"}) {
                auto *item = bar->findChild<QQuickItem *>(name); QVERIFY(item && item->isVisible());
                const QRectF bounds(item->mapToItem(bar, QPointF()), item->size());
                QVERIFY2(QRectF(QPointF(), bar->size()).contains(bounds), qPrintable(name));
                actions.insert(name, bounds);
            }
            for (const QString &name : {"searchField", "replaceField"}) {
                auto *field = bar->findChild<QQuickItem *>(name); QVERIFY(field && field->width() >= 80);
                QVERIFY(QRectF(QPointF(), bar->size()).contains(QRectF(field->mapToItem(bar, QPointF()), field->size())));
            }
            bar->setProperty("query", QString::fromUtf8("very long search café 你好 😀"));
            bar->setProperty("matchCount", 125); bar->setProperty("matchIndex", 124);
            QTest::qWait(20);
            for (auto it = actions.cbegin(); it != actions.cend(); ++it) {
                auto *item = bar->findChild<QQuickItem *>(it.key());
                QCOMPARE(QRectF(item->mapToItem(bar, QPointF()), item->size()), it.value());
            }
        }
    }
}
