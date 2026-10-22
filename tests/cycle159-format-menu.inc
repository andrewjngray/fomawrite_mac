namespace {
QObject *cycle159Menu(QObject *bar, const QString &title) {
    for (auto *candidate : bar->findChildren<QObject *>())
        if (candidate->property("title").toString() == title) return candidate;
    return nullptr;
}
bool cycle159Activate(Cycle117Window &f, QObject *bar, const QString &group, QQuickItem *target) {
    QStringList path{QStringLiteral("F&ormat")};
    if (!group.isEmpty()) path.append(group);
    for (const auto &title : path) {
        auto *popup = cycle159Menu(bar, title); if (!popup) return false;
        QQuickItem *entry = nullptr;
        for (auto *candidate : f.window->findChildren<QQuickItem *>())
            if ((candidate->inherits("QQuickMenuBarItem") && qvariant_cast<QObject *>(candidate->property("menu")) == popup)
                || qvariant_cast<QObject *>(candidate->property("subMenu")) == popup) { entry = candidate; break; }
        if (!entry || !cycle102Click(f.quick, entry)
            || !QTest::qWaitFor([&] { return popup->property("opened").toBool(); }, 3000)) return false;
    }
    // Long menus do not lay out every offscreen model item immediately.
    // Traverse the real popup with keys to lay out the current item, then use
    // wheel input if it is clipped. A dormant row's y=0 is not a pointer target.
    auto *leaf = cycle159Menu(bar, group.isEmpty() ? "F&ormat" : group);
    auto *viewport = qvariant_cast<QQuickItem *>(leaf->property("contentItem"));
    if (!viewport) return false;
    int targetIndex = -1;
    for (int index = 0; index < leaf->property("count").toInt(); ++index) {
        QQuickItem *item = nullptr;
        if (!QMetaObject::invokeMethod(leaf, "itemAt", Q_RETURN_ARG(QQuickItem *, item), Q_ARG(int, index))) return false;
        if (item == target) targetIndex = index;
    }
    if (targetIndex < 0) return false;
    // Explicit window key events follow the same menu handlers while allowing
    // the existing Cocoa mouse-only filter to reject unrelated native typing.
    cycle111KeyClick(f.quick, Qt::Key_Home);
    for (int step = 0; step <= leaf->property("count").toInt()
         && leaf->property("currentIndex").toInt() != targetIndex; ++step)
        cycle111KeyClick(f.quick, Qt::Key_Down);
    if (leaf->property("currentIndex").toInt() != targetIndex) {
        qWarning() << "Format keyboard target not reached" << target->objectName() << "wanted/current" << targetIndex << leaf->property("currentIndex") << "focus" << f.quick->activeFocusItem();
        return false;
    }
    const auto exposed = [&] {
        const QPointF centre = target->mapToScene(QPointF(target->width() / 2, target->height() / 2));
        if (!target->isVisible() || !target->isEnabled()
            || !QRectF(QPointF(0, 0), f.quick->size()).contains(centre)
            || !viewport->mapRectToScene(viewport->boundingRect()).contains(centre)) return false;
        for (auto *parent = target->parentItem(); parent; parent = parent->parentItem())
            if (parent->clip() && !parent->mapRectToScene(parent->boundingRect()).contains(centre)) return false;
        return true;
    };
    // Wait for a painted frame after navigation before wheel input: ListView
    // content extents can still reflect the previously instantiated rows.
    QSignalSpy frames(f.quick, &QQuickWindow::frameSwapped);
    f.quick->update();
    if (!QTest::qWaitFor([&] { return !frames.isEmpty(); }, 1000)) return false;
    QElapsedTimer wheelClock; wheelClock.start();
    for (int step = 0; step < 32 && !exposed(); ++step) {
        const QPointF position = viewport->mapRectToScene(viewport->boundingRect()).center();
        const qreal previous = viewport->property("contentY").toReal();
        QTest::mouseMove(f.quick, position.toPoint());
        QWheelEvent event(position, f.quick->mapToGlobal(position.toPoint()), QPoint(), QPoint(0, -120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        // Ordinary hardware wheel events carry increasing timestamps. Zero
        // timestamps make Flickable reject later notches as zero elapsed time.
        event.setTimestamp(static_cast<quint64>(wheelClock.msecsSinceReference() + wheelClock.elapsed()));
        QCoreApplication::sendEvent(f.quick, &event);
        if (!QTest::qWaitFor([&] { return viewport->property("contentY").toReal() > previous; }, 1000)) {
            qWarning() << "Format wheel did not advance" << target->objectName() << "accepted" << event.isAccepted()
                       << "interactive" << viewport->property("interactive") << "scroll" << viewport->property("contentY")
                       << "contentHeight" << viewport->property("contentHeight") << "originY" << viewport->property("originY")
                       << "moving" << viewport->property("moving") << "step" << step;
            break;
        }
        if (!QTest::qWaitFor([&] { return !viewport->property("moving").toBool(); }, 1000)) return false;
    }
    if (!QTest::qWaitFor(exposed, 1000)) {
        qWarning() << "Format target remains clipped after input" << target->objectName()
                   << "item" << target->mapRectToScene(target->boundingRect())
                   << "viewport" << viewport->mapRectToScene(viewport->boundingRect())
                   << "scroll" << viewport->property("contentY") << "current" << leaf->property("currentIndex")
                   << "focus" << f.quick->activeFocusItem();
        return false;
    }
    QSignalSpy triggered(target, SIGNAL(triggered()));
    auto *root = cycle159Menu(bar, QStringLiteral("F&ormat"));
    const QPoint point = target->mapToScene(QPointF(target->width() / 2, target->height() / 2)).toPoint();
    QTest::mouseClick(f.quick, Qt::LeftButton, Qt::NoModifier, point);
    return triggered.count() == 1
        && QTest::qWaitFor([&] { return !root->property("visible").toBool(); }, 3000);
}
QStringList cycle159Names() {
    return {"Heading1", "Heading2", "Heading3", "Heading4", "Heading5", "Heading6", "List", "TaskList", "OrderedList",
        "OrderedTaskList", "ToggleTask", "Blockquote", "Body", "Indent", "Outdent", "LineUp", "LineDown", "Highlight",
        "CodeBlock", "Wikilink", "Footnote", "ContentBlock", "Hashtag", "Date", "Table", "TableOfContents", "HorizontalRule", "ClearStyles"};
}
}

void FomawriteTest::windowsFormatMenuSourceEditsAreAtomic_data() {
    QTest::addColumn<QString>("name"); QTest::addColumn<QString>("label"); QTest::addColumn<QString>("group");
    QTest::addColumn<QString>("before"); QTest::addColumn<int>("first"); QTest::addColumn<int>("last"); QTest::addColumn<QString>("after");
    const auto row = [](const char *name, const QString &label, const QString &group, const QString &before, int first, int last, const QString &after) {
        QTest::newRow(name) << QString::fromLatin1(name) << label << group << before << first << last << after;
    };
    for (int level = 1; level <= 6; ++level) {
        const QByteArray name = QString("Heading%1").arg(level).toLatin1();
        row(name.constData(), QString("Heading %1").arg(level), "Headings", QString::fromUtf8("Café\n"), 0, 4,
            QString(level, '#') + QString::fromUtf8(" Café\n"));
    }
    row("List", "List", "Lists", "first\nsecond", 0, 12, "- first\n- second");
    row("TaskList", "Task List", "Lists", "first\nsecond", 0, 12, "- [ ] first\n- [ ] second");
    row("OrderedList", "Ordered List", "Lists", "first\nsecond", 0, 12, "1. first\n2. second");
    row("OrderedTaskList", "Ordered Task List", "Lists", "first\nsecond", 0, 12, "1. [ ] first\n2. [ ] second");
    row("ToggleTask", "Mark Task as Completed", "Lists", "- [ ] first\n- [x] second", 0, 0, "- [x] first\n- [x] second");
    row("Blockquote", "Blockquote", "", "first\nsecond", 0, 12, "> first\n> second");
    row("Body", "Body", "", QString::fromUtf8("## Café"), 0, 7, QString::fromUtf8("Café"));
    row("Indent", "Indent", "Structure", "first\nsecond", 0, 12, "    first\n    second");
    row("Outdent", "Outdent", "Structure", "    first\n    second", 0, 20, "first\nsecond");
    row("LineUp", "Move Line Up", "Structure", "first\nsecond\nthird", 6, 12, "second\nfirst\nthird");
    row("LineDown", "Move Line Down", "Structure", "first\nsecond\nthird", 0, 5, "second\nfirst\nthird");
    row("Highlight", "Highlight", "", QString::fromUtf8("A café note."), 2, 6, QString::fromUtf8("A ==café== note."));
    row("CodeBlock", "Code Block", "", "alpha\nbeta", 0, 10, "```\nalpha\nbeta\n```\n");
    row("Wikilink", "Add Wikilink", "", QString::fromUtf8("A café note."), 2, 6, QString::fromUtf8("A [[café]] note."));
    row("Footnote", "Add Footnote", "", "X", 0, 1, "[^note]\n\n[^note]: Note text");
    row("ContentBlock", "Add Content Block", "", "X", 0, 1, "\n/chapter.md\n");
    row("Hashtag", "Add Hashtag", "", "X", 0, 1, "#tag");
    row("Date", "Add Date", "", "X", 0, 1, QDate::currentDate().toString(Qt::ISODate));
    row("Table", "Add Table", "", "X", 0, 1, "| X | Column 2 |\n| --- | --- |\n|  |  |\n");
    row("TableOfContents", "Add Table of Contents", "", "# Title\n\n## Section\n\nTOKEN\n", 21, 26,
        "# Title\n\n## Section\n\n- [Title](#title)\n  - [Section](#section)\n\n");
    row("HorizontalRule", "Add Horizontal Rule", "", "X", 0, 1, "---\n");
    row("ClearStyles", "Clear Styles", "", QString::fromUtf8("**==café==**"), 0, 12, QString::fromUtf8("café"));
}

void FomawriteTest::windowsFormatMenuSourceEditsAreAtomic() {
    QFETCH(QString, name); QFETCH(QString, label); QFETCH(QString, group); QFETCH(QString, before);
    QFETCH(int, first); QFETCH(int, last); QFETCH(QString, after);
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    QVERIFY(testDesktopMenusMatchPlatform(f.window.data()));
    // Retain native Mac menu presence; the Windows component is an explicit host fixture.
    if (f.window->property("isMac").toBool()) QVERIFY(cycle113NativeFormatItem(f.window.data(), label));
    auto *loader = f.window->findChild<QObject *>("windowsMenuLoader"); QVERIFY(loader);
    if (!f.window->property("isWindows").toBool()) loader->setProperty("active", true);
    QTRY_VERIFY(f.window->findChild<QObject *>("windowsMenuBar"));
    auto *bar = f.window->findChild<QObject *>("windowsMenuBar");
    auto *choice = bar->findChild<QQuickItem *>("windows_format" + name);
    QVERIFY2(choice, qPrintable("Missing real Windows Format node: windows_format" + name));
    QCOMPARE(choice->property("text").toString(), label);
    auto *parentMenu = cycle159Menu(bar, group.isEmpty() ? "F&ormat" : group); QVERIFY(parentMenu);
    bool inMenu = false;
    for (int index = 0; index < parentMenu->property("count").toInt(); ++index) {
        QQuickItem *item = nullptr;
        QVERIFY(QMetaObject::invokeMethod(parentMenu, "itemAt", Q_RETURN_ARG(QQuickItem *, item), Q_ARG(int, index)));
        inMenu |= item == choice;
    }
    QVERIFY(inMenu);
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString path = directory.filePath("Format.md");
    QVERIFY(cycle103Write(path, before.toUtf8())); QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    QTRY_COMPARE(f.editor->property("text").toString(), before);
    QVERIFY(cycle113SettleViewport(f.window.data()));
    f.editor->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, first), Q_ARG(int, last)));
    QTRY_VERIFY(f.window->property("canFormatSource").toBool());
    QVERIFY(choice->isEnabled()); QVERIFY(!f.editor->property("canUndo").toBool());
    QVERIFY(cycle159Activate(f, bar, group, choice));
    QTRY_COMPARE(f.editor->property("text").toString(), after);
    QVERIFY(f.backend.modified()); QCOMPARE(cycle103Read(path), before.toUtf8());
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "undoEditing"));
    QCOMPARE(f.editor->property("text").toString(), before);
    QVERIFY(!f.editor->property("canUndo").toBool()); QVERIFY(f.editor->property("canRedo").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "redoEditing"));
    QCOMPARE(f.editor->property("text").toString(), after);
    QCOMPARE(cycle103Read(path), before.toUtf8());
    QVERIFY2(f.error.isEmpty(), qPrintable(f.error));
}

void FomawriteTest::windowsFormatMenuLiveAndGuardsPreserveSelection() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    QVERIFY(testDesktopMenusMatchPlatform(f.window.data()));
    auto *loader = f.window->findChild<QObject *>("windowsMenuLoader"); QVERIFY(loader);
    if (!f.window->property("isWindows").toBool()) loader->setProperty("active", true);
    QTRY_VERIFY(f.window->findChild<QObject *>("windowsMenuBar"));
    auto *bar = f.window->findChild<QObject *>("windowsMenuBar");
    QList<QQuickItem *> choices;
    for (const auto &name : cycle159Names()) {
        auto *choice = bar->findChild<QQuickItem *>("windows_format" + name);
        QVERIFY2(choice, qPrintable("Missing real Windows Format node: windows_format" + name));
        choices.append(choice);
    }
    const QString original = QString::fromUtf8("Title\n\nA café note.\n");
    f.editor->setProperty("text", original); QVERIFY(cycle113SettleViewport(f.window.data()));
    f.editor->forceActiveFocus(); QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, 9), Q_ARG(int, 13)));
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "openSearch", Q_ARG(QVariant, false), Q_ARG(QVariant, false)));
    auto *search = f.window->findChild<QQuickItem *>("searchField"); QVERIFY(search);
    search->forceActiveFocus(); QTRY_VERIFY(!f.window->property("canFormatSource").toBool());
    const auto revision = f.backend.documentRevision();
    const auto undo = f.editor->property("canUndo"), redo = f.editor->property("canRedo");
    const bool modified = f.backend.modified();
    const auto first = f.editor->property("selectionStart"), last = f.editor->property("selectionEnd");
    for (auto *choice : choices) {
        QVERIFY(!choice->isEnabled()); QVERIFY(QMetaObject::invokeMethod(choice, "triggered"));
        QCOMPARE(f.editor->property("text").toString(), original); QCOMPARE(f.backend.documentRevision(), revision);
        QCOMPARE(f.editor->property("canUndo"), undo); QCOMPARE(f.editor->property("canRedo"), redo);
        QCOMPARE(f.backend.modified(), modified);
        QCOMPARE(f.editor->property("selectionStart"), first); QCOMPARE(f.editor->property("selectionEnd"), last);
        QCOMPARE(f.quick->activeFocusItem(), search);
    }
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "closeSearch", Q_ARG(QVariant, true)));
    QVERIFY(cycle118ReadPreview(f.window.data()));
    QTRY_VERIFY(!f.window->property("canFormatSource").toBool() && !f.window->property("canFormatLive").toBool());
    for (auto *choice : choices) {
        QVERIFY(!choice->isEnabled()); QVERIFY(QMetaObject::invokeMethod(choice, "triggered"));
        QCOMPARE(f.editor->property("text").toString(), original); QCOMPARE(f.backend.documentRevision(), revision);
    }
    auto *liveCommand = bar->findChild<QObject *>("windows_liveEditing"); QVERIFY(liveCommand);
    QVERIFY(QMetaObject::invokeMethod(liveCommand, "triggered"));
    auto *bridge = f.backend.editorBridge(); QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 15000);
    auto *view = f.window->findChild<QQuickItem *>("liveEditorView"); QVERIFY(view);
    const auto text = [&] { return cycle143PageResult(view, "window.fomawriteEditor.getText()", 0).toString(); };
    QTRY_COMPARE_WITH_TIMEOUT(text(), original, 10000);
    view->forceActiveFocus(); QTRY_VERIFY(f.window->property("canFormatLive").toBool());
    for (auto *choice : choices)
        QCOMPARE(choice->isEnabled(), choice->objectName() != "windows_formatClearStyles");
    struct Sample { const char *name; const char *group; int first, last; QString after; };
    for (const Sample &sample : {
             Sample{"Heading2", "Headings", 0, 5, QString::fromUtf8("## Title\n\nA café note.\n")},
             Sample{"Highlight", "", 9, 13, QString::fromUtf8("Title\n\nA ==café== note.\n")},
             Sample{"Footnote", "", 9, 13, QString::fromUtf8("Title\n\nA [^note]\n\n[^note]: Note text note.\n")}}) {
        const QString select = QStringLiteral("(()=>{const v=window.fomawriteEditor.view;v.dispatch({selection:{anchor:%1,head:%2}});v.focus();return v.state.selection.main.to;})()").arg(sample.first).arg(sample.last);
        QCOMPARE(cycle143PageResult(view, select, 0).toInt(), sample.last);
        view->forceActiveFocus(); QTRY_VERIFY(f.window->property("canFormatLive").toBool());
        QSignalSpy requests(bridge, &EditorBridge::requestSelection);
        auto *choice = bar->findChild<QQuickItem *>("windows_format" + QString::fromLatin1(sample.name)); QVERIFY(choice);
        QVERIFY(cycle159Activate(f, bar, QString::fromLatin1(sample.group), choice));
        QCOMPARE(requests.count(), 1);
        QTRY_COMPARE_WITH_TIMEOUT(text(), sample.after, 10000);
        QTRY_COMPARE(f.editor->property("text").toString(), sample.after);
        QVERIFY(QMetaObject::invokeMethod(f.window.data(), "undoEditing"));
        QTRY_COMPARE_WITH_TIMEOUT(text(), original, 10000);
        QTRY_COMPARE(f.editor->property("text").toString(), original);
    }
    QVERIFY2(f.error.isEmpty(), qPrintable(f.error));
}
