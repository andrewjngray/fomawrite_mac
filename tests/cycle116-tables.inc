namespace {
void cycle116TableKey(QQuickWindow *window, bool backwards = false) {
    const auto key = backwards ? Qt::Key_Backtab : Qt::Key_Tab;
    const auto modifiers = backwards ? Qt::ShiftModifier : Qt::NoModifier;
    QKeyEvent press(QEvent::KeyPress, key, modifiers);
    QKeyEvent release(QEvent::KeyRelease, key, modifiers);
    QCoreApplication::sendEvent(window, &press);
    QCoreApplication::sendEvent(window, &release);
}
}

void FomawriteTest::visualTableCellNavigationKeepsBoundaries() {
    using Status = SourceVisualMapping::TableNavigationStatus;
    const QString source = QString::fromUtf8("Before\r\n\r\n| Name | Value | Note |\r\n| :--- | ---: | :---: |\r\n| Café | **bold** | 📝 |\r\n| Empty |   | 東京 |\r\n\r\n| Other | Table |\r\n| --- | --- |\r\n| Last | Row |\r\n");
    const auto mapping = SourceVisualMapping::create(source);
    const auto cells = mapping.tableCellObjects();
    QCOMPARE(cells.size(), 13);
    QCOMPARE(mapping.roundTripSource().toUtf8(), source.toUtf8());
    for (int index = 0; index < 9; ++index) {
        const auto &cell = cells.at(index);
        QCOMPARE(cell.row, index < 3 ? 0 : index < 6 ? 2 : 3);
        QCOMPARE(cell.column, index % 3);
        QCOMPARE(cell.tableStart, source.indexOf("| Name"));
        const auto next = mapping.navigateTable({cell.visual.start, 0}, false);
        const auto previous = mapping.navigateTable({cell.visual.start, 0}, true);
        QCOMPARE(next.status, index == 8 ? Status::Boundary : Status::Moved);
        QCOMPARE(previous.status, index == 0 ? Status::Boundary : Status::Moved);
        if (index < 8) QCOMPARE(next.cursor, cells.at(index + 1).visual.start);
        if (index > 0) QCOMPARE(previous.cursor, cells.at(index - 1).visual.start);
    }
    QCOMPARE(mapping.visualText().mid(cells.at(4).visual.start, cells.at(4).visual.length), QString("bold"));
    QCOMPARE(source.mid(cells.at(4).source.start, cells.at(4).source.length), QString("**bold**"));
    QCOMPARE(cells.at(7).visual.length, 0);
    QCOMPARE(mapping.navigateTable(cells.at(4).visual, false).cursor, cells.at(5).visual.start);
    QCOMPARE(mapping.navigateTable({cells.at(5).visual.start + 1, 0}, false).status, Status::UnsupportedSelection);
    QCOMPARE(mapping.navigateTable({cells.at(0).visual.start, cells.at(1).visual.end() - cells.at(0).visual.start}, false).status,
             Status::UnsupportedSelection);
    QCOMPARE(mapping.navigateTable({0, 0}, false).status, Status::OutsideTable);
    QCOMPARE(mapping.navigateTable({mapping.blocks().at(3).visual.start, 0}, false).status, Status::UnsupportedSelection);
    // An unsupported row is a hard boundary, not something Tab skips over.
    for (const QString &badRow : {QString("| escaped\\|pipe | value |"), QString("| `code|pipe` | value |"),
                                 QString("| short |"), QString("| [reference][id] | value |")}) {
        const auto table = SourceVisualMapping::create("| A | B |\n| --- | --- |\n" + badRow + "\n| Later | Safe |\n");
        const auto safe = table.tableCellObjects(); QCOMPARE(safe.size(), 4);
        QCOMPARE(table.navigateTable(safe.at(1).visual, false).status, Status::Boundary);
        QCOMPARE(table.navigateTable(safe.at(2).visual, true).status, Status::Boundary);
    }
    const auto unframed = SourceVisualMapping::create("A | B\n--- | ---\ncafé | 東京\n");
    QCOMPARE(unframed.tableCellObjects().size(), 4);
    QCOMPARE(unframed.navigateTable(unframed.tableCellObjects().at(1).visual, false).cursor,
             unframed.tableCellObjects().at(2).visual.start);
}

void FomawriteTest::visualTableKeyboardNavigationPreservesDraftAndUndo() {
    Cycle103SettingsSnapshot isolatedSettings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString original = QString::fromUtf8("# Table navigation\n\n| Name | Value | Note |\n| :--- | ---: | :---: |\n| Café | **bold** | 📝 |\n| Empty |   | 東京 |\n\nAfter.\n");
    const QString path = directory.filePath("Tables.md"); QVERIFY(cycle103Write(path, original.toUtf8()));
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl("qrc:/Main.qml"));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    auto *visual = pane ? pane->findChild<QQuickItem *>("visualEditor") : nullptr;
    auto *scroll = pane ? pane->findChild<QObject *>("previewScroll") : nullptr;
    auto *zoom = window->findChild<QObject *>("paneZoomState");
    QVERIFY(quick && editor && pane && visual && scroll && zoom);
    cycle106KeepNativeFocus(quick); Cycle106MouseInputIsolation input(quick);
    quick->resize(1280, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    QVERIFY(backend.open(QUrl::fromLocalFile(path)));
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, "visual")));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_COMPARE(visual->property("text").toString(), SourceVisualMapping::create(original).visualText());
    QTRY_VERIFY(!pane->property("viewportRefreshPending").toBool());
    const auto cells = SourceVisualMapping::create(original).tableCellObjects();
    QCOMPARE(backend.visualProjection()["tableCells"].toList().size(), cells.size());
    // Navigation through a saved file cannot dirty it or create an undo unit.
    visual->setProperty("cursorPosition", cells.first().visual.start); visual->forceActiveFocus();
    for (int index = 1; index < cells.size(); ++index) {
        cycle116TableKey(quick);
        QCOMPARE(visual->property("cursorPosition").toInt(), cells.at(index).visual.start);
        QCOMPARE(editor->property("text").toString(), original);
        QVERIFY(!backend.modified()); QVERIFY(!editor->property("canUndo").toBool());
    }
    cycle116TableKey(quick); QCOMPARE(visual->property("cursorPosition").toInt(), cells.last().visual.start);
    for (int index = cells.size() - 2; index >= 0; --index) {
        cycle116TableKey(quick, true);
        QCOMPARE(visual->property("cursorPosition").toInt(), cells.at(index).visual.start);
    }
    cycle116TableKey(quick, true); QCOMPARE(visual->property("cursorPosition").toInt(), cells.first().visual.start);
    QCOMPARE(editor->property("text").toString(), original); QVERIFY(!backend.modified());

    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, original.size()), Q_ARG(QString, "Unsaved ending.")));
    const QString draft = editor->property("text").toString(); QVERIFY(draft.endsWith("Unsaved ending."));
    QTRY_COMPARE(visual->property("text").toString(), SourceVisualMapping::create(draft).visualText());
    QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 2), Q_ARG(int, 7)));
    // The same real keys work on a narrow dark pane at a larger reading size.
    backend.setThemePreset("dark"); quick->resize(720, 600);
    QVERIFY(QMetaObject::invokeMethod(zoom, "setZoom", Q_ARG(QVariant, "preview"), Q_ARG(QVariant, 175)));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_VERIFY(!pane->property("viewportRefreshPending").toBool());
    visual->setProperty("cursorPosition", cells.at(6).visual.start); visual->forceActiveFocus();
    cycle116TableKey(quick);
    QCOMPARE(visual->property("cursorPosition").toInt(), cells.at(7).visual.start); // Empty cell.
    QCOMPARE(editor->property("selectionStart").toInt(), 2); QCOMPARE(editor->property("selectionEnd").toInt(), 7);
    QCOMPARE(editor->property("text").toString(), draft);
    const QRectF caret = visual->property("cursorRectangle").toRectF();
    QTRY_VERIFY(visual->y() + caret.bottom() <= scroll->property("contentY").toReal() + scroll->property("height").toReal());
    for (char key : QByteArray("new")) cycle111KeyClick(quick, key);
    QString typed = draft; typed.replace("| Empty |   |", "| Empty |   new|");
    QTRY_COMPARE(editor->property("text").toString(), typed);
    QVERIFY(QMetaObject::invokeMethod(editor, "undo")); QCOMPARE(editor->property("text").toString(), draft);
    QVERIFY(QMetaObject::invokeMethod(editor, "redo")); QCOMPARE(editor->property("text").toString(), typed);
    QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
    QVERIFY(QMetaObject::invokeMethod(editor, "undo")); QCOMPARE(editor->property("text").toString(), original);
    QVERIFY(QMetaObject::invokeMethod(editor, "redo")); QCOMPARE(editor->property("text").toString(), draft);
    QVERIFY(QMetaObject::invokeMethod(editor, "redo")); QCOMPARE(editor->property("text").toString(), typed);
    QCOMPARE(cycle103Read(path), original.toUtf8());
    QTRY_COMPARE(visual->property("text").toString(), SourceVisualMapping::create(typed).visualText());
    visual->forceActiveFocus();
    QTRY_VERIFY(visual->hasActiveFocus());
    QVERIFY(window->property("workspaceOwnsKeyboard").toBool());
    // Cocoa QA deliberately uses a nonactivating window, so macOS cannot
    // dispatch WindowShortcut events to it. Exercise the registered F6
    // handler there; the offscreen run still sends the real F6 key event.
    if (QGuiApplication::platformName() == "cocoa") {
        QVERIFY(quick->flags().testFlag(Qt::WindowDoesNotAcceptFocus));
        QObject *f6 = nullptr;
        for (auto *item : window->findChildren<QObject *>()) {
            if (item->property("sequence").toString() == QStringLiteral("F6")
                && item->metaObject()->indexOfSignal("activated()") >= 0) { f6 = item; break; }
        }
        QVERIFY(f6 && f6->property("enabled").toBool());
        QCOMPARE(f6->property("context").toInt(), int(Qt::WindowShortcut));
        QVERIFY(QMetaObject::invokeMethod(f6, "activated"));
    } else {
        cycle111KeyClick(quick, Qt::Key_F6);
    }
    QTRY_VERIFY(!visual->hasActiveFocus());
    QCOMPARE(editor->property("text").toString(), typed);
}

void FomawriteTest::visualTableNavigationRejectsStaleAndCrossCellSelections() {
    Cycle103SettingsSnapshot isolatedSettings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl("qrc:/Main.qml"));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data());
    auto *editor = window->findChild<QQuickItem *>("sourceEditor");
    auto *pane = window->findChild<QQuickItem *>("previewPane");
    auto *visual = pane ? pane->findChild<QQuickItem *>("visualEditor") : nullptr;
    QVERIFY(quick && editor && pane && visual);
    cycle106KeepNativeFocus(quick); Cycle106MouseInputIsolation input(quick);
    quick->resize(1100, 720); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    const QString source = "| A | B |\n| --- | --- |\n| One | Two |\n";
    editor->setProperty("text", source);
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, "visual")));
    QTRY_VERIFY(!window->property("changingDocumentView").toBool());
    QTRY_COMPARE(visual->property("text").toString(), SourceVisualMapping::create(source).visualText());
    const auto cells = SourceVisualMapping::create(source).tableCellObjects();
    const int first = cells.first().visual.start, last = cells.at(1).visual.end();
    QVERIFY(QMetaObject::invokeMethod(visual, "select", Q_ARG(int, first), Q_ARG(int, last)));
    visual->forceActiveFocus();
    const bool undo = editor->property("canUndo").toBool();
    const bool modified = backend.modified();
    for (bool backwards : {false, true}) {
        cycle116TableKey(quick, backwards);
        QCOMPARE(visual->property("selectionStart").toInt(), first); QCOMPARE(visual->property("selectionEnd").toInt(), last);
        QCOMPARE(editor->property("text").toString(), source);
        QCOMPARE(editor->property("canUndo").toBool(), undo); QCOMPARE(backend.modified(), modified);
    }
    // A canonical edit arrives while the old visual projection is still shown.
    visual->setProperty("cursorPosition", cells.at(2).visual.start);
    QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, source.size()), Q_ARG(QString, "Changed elsewhere.")));
    const QString changed = editor->property("text").toString();
    QCOMPARE(backend.navigateVisualTable(first, first, false, source)["reason"].toString(), QString("stale"));
    const int cursor = visual->property("cursorPosition").toInt();
    cycle116TableKey(quick);
    QCOMPARE(visual->property("cursorPosition").toInt(), cursor);
    QCOMPARE(editor->property("text").toString(), changed);
    QVERIFY(pane->property("visualStatus").toString().contains("changed elsewhere"));
    QVERIFY(QMetaObject::invokeMethod(editor, "undo")); QCOMPARE(editor->property("text").toString(), source);
    QVERIFY(QMetaObject::invokeMethod(editor, "redo")); QCOMPARE(editor->property("text").toString(), changed);
}
