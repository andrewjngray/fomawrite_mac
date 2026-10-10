// Cycle142 — probe: what does Qt's accessibility layer expose for the two
// writing surfaces? Grammarly Desktop (and any macOS writing assistant) can
// only see what reaches the Cocoa bridge, and the bridge only maps what
// QAccessible offers. This test never fails on the findings; it reports them.

static QString cycle142StateText(const QAccessible::State &s) {
    QStringList on;
    if (s.focusable) on << "focusable"; if (s.focused) on << "focused"; if (s.editable) on << "editable";
    if (s.multiLine) on << "multiLine"; if (s.readOnly) on << "readOnly"; if (s.invisible) on << "invisible";
    if (s.offscreen) on << "offscreen"; if (s.disabled) on << "disabled"; if (s.selectable) on << "selectable";
    return on.join(',');
}

static void cycle142Report(const QString &label, QAccessibleInterface *a) {
    if (!a) { qInfo().noquote() << label << ": no accessible interface"; return; }
    QString line = QStringLiteral("%1: role=%2 name=\"%3\" state=[%4] rect=%5x%6 children=%7")
        .arg(label).arg(QStringLiteral("0x%1").arg(int(a->role()), 0, 16))
        .arg(a->text(QAccessible::Name)).arg(cycle142StateText(a->state()))
        .arg(a->rect().width()).arg(a->rect().height()).arg(a->childCount());
    if (auto *t = a->textInterface()) {
        int start = -1, end = -1; t->selection(0, &start, &end);
        QStringList rects; for (int o : {0, 5, 12, 40}) if (o < t->characterCount()) { const QRect r = t->characterRect(o); rects << QStringLiteral("%1@%2,%3 %4x%5").arg(o).arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height()); }
        line += QStringLiteral(" text{count=%1 cursor=%2 sel=%3..%4 rects[%5] firstLine=\"%6\" offsetAt(10,10)=%7}")
            .arg(t->characterCount()).arg(t->cursorPosition()).arg(start).arg(end).arg(rects.join("; "))
            .arg(t->text(0, qMin(20, t->characterCount()))).arg(t->offsetAtPoint(a->rect().topLeft() + QPoint(10, 10)));
    } else line += " text{none}";
    line += a->editableTextInterface() ? " editable{yes}" : " editable{none}";
    line += a->valueInterface() ? " value{yes}" : "";
    line += a->actionInterface() ? QStringLiteral(" actions{%1}").arg(a->actionInterface()->actionNames().join('/')) : QString();
    qInfo().noquote() << line;
}

static void cycle142Walk(QAccessibleInterface *a, int depth, int &budget, const QString &path) {
    if (!a || depth > 12 || budget <= 0) return;
    const bool text = a->textInterface() || a->role() == QAccessible::EditableText || a->role() == QAccessible::Document
        || a->role() == QAccessible::Paragraph || a->state().focused;
    if (text) { --budget; cycle142Report(path + QString::number(depth), a); }
    for (int i = 0; i < a->childCount() && budget > 0; ++i) cycle142Walk(a->child(i), depth + 1, budget, path);
}

void FomawriteTest::accessibilityProbeForWritingAssistants() {
    Cycle103SettingsSnapshot settings;
    // Activating accessibility is process-wide and makes WebEngine build its
    // tree for every later test (slow on the CI runner): restore it on exit.
    const bool wasActive = QAccessible::isActive();
    const auto restore = qScopeGuard([wasActive] { QAccessible::setActive(wasActive); });
    QAccessible::setActive(true);
    qInfo() << "QAccessible::isActive =" << QAccessible::isActive() << "platform =" << QGuiApplication::platformName();
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    auto *editor = window->findChild<QQuickItem *>("sourceEditor"); QVERIFY(editor);
    quick->resize(1200, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    editor->setProperty("text", QStringLiteral("Their going too the shop tomorow.\n\nSecond paragraph.\n"));
    editor->forceActiveFocus(); QTest::qWait(200);
    qInfo() << "--- Source surface";
    cycle142Report("window", QAccessible::queryAccessibleInterface(quick));
    cycle142Report("sourceEditor", QAccessible::queryAccessibleInterface(editor));
    if (auto *a = QAccessible::queryAccessibleInterface(editor)) {
        cycle142Report("sourceEditor.parent", a->parent());
        QAccessibleInterface *focus = QAccessible::queryAccessibleInterface(quick) ? QAccessible::queryAccessibleInterface(quick)->focusChild() : nullptr;
        cycle142Report("window.focusChild", focus);
    }
    // With the Source factory installed, character bounds are real and round-trip.
    if (auto *a = QAccessible::queryAccessibleInterface(editor)) {
        auto *t = a->textInterface(); QVERIFY(t);
        const QRect fifth = t->characterRect(5), twelfth = t->characterRect(12);
        QVERIFY2(fifth.width() > 0 && fifth.height() > 0, "characterRect(5) is empty");
        QVERIFY(twelfth.x() > fifth.x() && twelfth.y() == fifth.y());
        QCOMPARE(t->offsetAtPoint(fifth.center()), 5);
        QCOMPARE(t->offsetAtPoint(twelfth.center()), 12);
        QVERIFY(t->characterRect(t->characterCount()).height() > 0);
        QVERIFY(t->characterRect(t->characterCount() + 1).isEmpty());
        QCOMPARE(t->offsetAtPoint(QPoint(-5000, -5000)), -1);
        const QRect secondParagraph = t->characterRect(36);
        QVERIFY2(secondParagraph.y() > fifth.y(), "second paragraph is below the first");
        auto *e = a->editableTextInterface(); QVERIFY2(e, "editable-text interface kept");
        e->replaceText(0, 5, QStringLiteral("They're"));
        QVERIFY(editor->property("text").toString().startsWith(QStringLiteral("They're going too")));
        e->deleteText(0, 8); e->insertText(0, QStringLiteral("Their "));
        QVERIFY(editor->property("text").toString().startsWith(QStringLiteral("Their going too")));
        e->replaceText(0, 10000, QStringLiteral("x")); // out of range: ignored
        QVERIFY(editor->property("text").toString().startsWith(QStringLiteral("Their going too")));
    }
    qInfo() << "--- Live surface";
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    auto *bridge = backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 20000);
    auto *view = window->findChild<QQuickItem *>("liveEditorView"); QVERIFY(view);
    QTRY_VERIFY_WITH_TIMEOUT(!view->property("loading").toBool(), 15000);
    view->forceActiveFocus();
    QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, QStringLiteral("document.querySelector('.cm-content').focus()")));
    QTest::qWait(1500);
    auto *va = QAccessible::queryAccessibleInterface(view);
    cycle142Report("liveEditorView", va);
    int budget = 25;
    cycle142Walk(va, 0, budget, "live/");
    if (auto *wa = QAccessible::queryAccessibleInterface(quick)) cycle142Report("window.focusChild(live)", wa->focusChild());
    backend.discardRecovery();
}
