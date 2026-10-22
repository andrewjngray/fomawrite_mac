// The Windows menu can be instantiated on every platform, so its routing and
// check-state regression runs in the Mac gate as well as Windows CI.
void FomawriteTest::windowsMenuRoutesPortableCommands() {
    Cycle103SettingsSnapshot isolatedSettings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *loader = f.window->findChild<QObject *>("windowsMenuLoader");
    QVERIFY2(loader, "Windows must have a desktop menu loader");
    loader->setProperty("active", true);
    QTRY_VERIFY(f.window->findChild<QObject *>("windowsMenuBar"));
    auto *menu = f.window->findChild<QObject *>("windowsMenuBar");
    QVERIFY(!menu->findChild<QObject *>("windows_engineMacos"));
    QObject *helpMenu = nullptr;
    for (QObject *candidate : menu->findChildren<QObject *>())
        if (candidate->property("title").toString() == QStringLiteral("&Help")) { helpMenu = candidate; break; }
    QVERIFY(helpMenu);
    auto *checkUpdates = helpMenu->findChild<QObject *>("windows_checkWritingCheckerUpdates");
    auto *useBundled = helpMenu->findChild<QObject *>("windows_useBuiltInWritingChecker");
    auto *automaticUpdates = helpMenu->findChild<QObject *>("windows_checkWritingCheckerAutomatically");
    QVERIFY(checkUpdates && useBundled && automaticUpdates);
    auto *updater = f.backend.harperUpdater();
    QVERIFY(updater->updatesSupported() && !updater->checking() && !updater->installing());
    QVERIFY(checkUpdates->property("enabled").toBool());
    QVERIFY(!checkUpdates->property("checkable").toBool());
    QCOMPARE(useBundled->property("enabled").toBool(), updater->hasOverride());
    QVERIFY(!useBundled->property("checkable").toBool());
    QVERIFY(automaticUpdates->property("enabled").toBool());
    QVERIFY(automaticUpdates->property("checkable").toBool());
    QVERIFY(updater->autoCheck() && automaticUpdates->property("checked").toBool());
    auto *source = menu->findChild<QObject *>("windows_sourceEditing");
    auto *live = menu->findChild<QObject *>("windows_liveEditing");
    auto *web = menu->findChild<QObject *>("windows_webPreview");
    auto *pdf = menu->findChild<QObject *>("windows_pdfPreview");
    QVERIFY(source && live && web && pdf);
    const QString original = QString::fromUtf8("# Menu fixture\n\nCafé **stays**.\n");
    f.editor->setProperty("text", original);
    // A Qt Quick popup takes focus. Formatting must keep the writing surface
    // that owned the selection when the desktop menu was opened.
    f.editor->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, original.indexOf(QString::fromUtf8("Café"))), Q_ARG(int, original.indexOf(QString::fromUtf8("Café")) + 4)));
    QObject *formatMenu = nullptr;
    for (QObject *candidate : menu->findChildren<QObject *>())
        if (candidate->property("title").toString() == QStringLiteral("F&ormat")) { formatMenu = candidate; break; }
    QVERIFY(formatMenu);
    QObject *bold = nullptr;
    for (QObject *candidate : formatMenu->findChildren<QObject *>())
        if (candidate->property("text").toString() == QStringLiteral("&Bold")) { bold = candidate; break; }
    QVERIFY(bold);
    QVERIFY(QMetaObject::invokeMethod(formatMenu, "open"));
    QTRY_VERIFY(formatMenu->property("opened").toBool());
    QTRY_VERIFY(!f.editor->hasActiveFocus());
    QVERIFY2(bold->property("enabled").toBool(), "Opening the desktop Format menu must retain Source formatting ownership");
    QVERIFY(cycle102Click(f.quick, qobject_cast<QQuickItem *>(bold)));
    QTRY_VERIFY(!formatMenu->property("visible").toBool());
    const QString formatted = QString(original).replace(QString::fromUtf8("Café"), QString::fromUtf8("**Café**"));
    QTRY_COMPARE(f.editor->property("text").toString(), formatted);
    QObject *editMenu = nullptr;
    for (QObject *candidate : menu->findChildren<QObject *>())
        if (candidate->property("title").toString() == QStringLiteral("&Edit")) { editMenu = candidate; break; }
    QVERIFY(editMenu);
    const auto editItem = [&](const QString &text) -> QQuickItem * {
        for (QObject *candidate : editMenu->findChildren<QObject *>())
            if (candidate->property("text").toString() == text) return qobject_cast<QQuickItem *>(candidate);
        return nullptr;
    };
    auto *copy = editItem(QStringLiteral("&Copy"));
    auto *cut = editItem(QStringLiteral("Cu&t"));
    auto *paste = editItem(QStringLiteral("&Paste"));
    auto *undo = editItem(QStringLiteral("&Undo"));
    QVERIFY(copy && cut && paste && undo);
    // Source copy retains the Markdown/authorship clipboard contract.
    f.editor->forceActiveFocus();
    const int cafe = formatted.indexOf(QString::fromUtf8("Café"));
    QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, cafe), Q_ARG(int, cafe + 4)));
    QVERIFY(QMetaObject::invokeMethod(editMenu, "open"));
    QTRY_VERIFY(editMenu->property("opened").toBool());
    QVERIFY(cycle102Click(f.quick, copy));
    QTRY_VERIFY(!editMenu->property("visible").toBool());
    QTRY_COMPARE(QGuiApplication::clipboard()->text(), QString::fromUtf8("Café"));
    QVERIFY(QGuiApplication::clipboard()->mimeData()->hasFormat("text/markdown"));
    QVERIFY(QGuiApplication::clipboard()->mimeData()->hasFormat("application/x-omawrite-authorship+json"));
    QVERIFY(QMetaObject::invokeMethod(editMenu, "open"));
    QTRY_VERIFY(editMenu->property("opened").toBool());
    QVERIFY(cycle102Click(f.quick, undo));
    QTRY_VERIFY(!editMenu->property("visible").toBool());
    QTRY_COMPARE(f.editor->property("text").toString(), original);
    // A focused auxiliary field keeps its own cut/paste target after menu focus.
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "openSearch", Q_ARG(QVariant, false), Q_ARG(QVariant, false)));
    auto *field = f.window->findChild<QQuickItem *>("searchField"); QVERIFY(field);
    field->setProperty("text", QStringLiteral("auxiliary selection"));
    field->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(field, "selectAll"));
    QVERIFY(QMetaObject::invokeMethod(editMenu, "open"));
    QTRY_VERIFY(editMenu->property("opened").toBool());
    QVERIFY(cycle102Click(f.quick, cut));
    QTRY_VERIFY(!editMenu->property("visible").toBool());
    QTRY_COMPARE(field->property("text").toString(), QString());
    QTRY_COMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("auxiliary selection"));
    QGuiApplication::clipboard()->setText(QStringLiteral("field replacement"));
    QVERIFY(QMetaObject::invokeMethod(editMenu, "open"));
    QTRY_VERIFY(editMenu->property("opened").toBool());
    QVERIFY(cycle102Click(f.quick, paste));
    QTRY_VERIFY(!editMenu->property("visible").toBool());
    QTRY_COMPARE(field->property("text").toString(), QStringLiteral("field replacement"));
    QCOMPARE(f.editor->property("text").toString(), original);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "closeSearch", Q_ARG(QVariant, true)));
    QVERIFY(QMetaObject::invokeMethod(live, "triggered"));
    QTRY_VERIFY(f.layout->property("liveEditEnabled").toBool());
    QTRY_VERIFY(live->property("checked").toBool());
    // The lazy qrc page must finish its own handshake and document replay.
    // Fabricated readiness can be invalidated by the real load starting later,
    // making requestUndo correctly refuse an unconnected page during the click.
    QTRY_VERIFY_WITH_TIMEOUT(f.backend.editorBridge()->isReady(), 15000);
    auto *liveView = f.window->findChild<QQuickItem *>("liveEditorView"); QVERIFY(liveView);
    const auto liveText = [&] { return cycle143PageResult(liveView, QStringLiteral("window.fomawriteEditor.getText()"), 0).toString(); };
    QTRY_COMPARE_WITH_TIMEOUT(liveText(), original, 10000);
    const bool modifiedBeforeLiveUndo = f.backend.modified();
    // Live Undo must reach its real page history while the popup owns focus.
    liveView->forceActiveFocus();
    QTRY_VERIFY(f.window->property("canFormatLive").toBool());
    QSignalSpy liveUndo(f.backend.editorBridge(), &EditorBridge::undo);
    QVERIFY(QMetaObject::invokeMethod(editMenu, "open"));
    QTRY_VERIFY(editMenu->property("opened").toBool());
    QTRY_VERIFY(!liveView->hasActiveFocus());
    QVERIFY2(f.window->property("canFormatLive").toBool(), "Desktop menu focus must preserve Live history ownership");
    QVERIFY(cycle102Click(f.quick, undo));
    QTRY_VERIFY(!editMenu->property("visible").toBool());
    QCOMPARE(liveUndo.count(), 1);
    QCOMPARE(f.editor->property("text").toString(), original);
    QTRY_COMPARE_WITH_TIMEOUT(liveText(), original, 5000);
    QCOMPARE(f.backend.modified(), modifiedBeforeLiveUndo);
    QVERIFY(QMetaObject::invokeMethod(source, "triggered"));
    QTRY_VERIFY(!f.layout->property("liveEditEnabled").toBool());
    QTRY_VERIFY(source->property("checked").toBool());
    QTRY_VERIFY(!f.window->property("lifecycleInputBlocked").toBool());
    QCOMPARE(f.editor->property("text").toString(), original);
    QVERIFY(QMetaObject::invokeMethod(pdf, "triggered"));
    auto *settings = f.window->findChild<QObject *>("workspaceSettings");
    QVERIFY(settings);
    QTRY_COMPARE(settings->property("publishingFormat").toString(), QStringLiteral("pdf"));
    QTRY_VERIFY(pdf->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(web, "triggered"));
    QTRY_COMPARE(settings->property("publishingFormat").toString(), QStringLiteral("web"));
    QTRY_VERIFY(web->property("checked").toBool());
    QCOMPARE(f.editor->property("text").toString(), original);
    QVERIFY2(f.error.isEmpty(), qPrintable(f.error));
}

// These are real Qt Quick menu clicks, including nested popup entry and
// repeated activation. The same Windows component also runs in the Mac gate;
// native Windows OS input remains a separate normal-launch acceptance check.
void FomawriteTest::windowsMenuWorkspaceActionsShareState_data() {
    QTest::addColumn<QByteArray>("command");
    QTest::addColumn<bool>("libraryOwner");
    QTest::addColumn<QByteArray>("property");
    QTest::addColumn<QVariant>("before");
    QTest::addColumn<QVariant>("after");
    QTest::addColumn<QStringList>("menuPath");
    QTest::addColumn<QStringList>("exclusive");
    const QStringList root{QStringLiteral("&View")};
    const QStringList sort = root + QStringList{QStringLiteral("Sort Files By")};
    const QStringList options = root + QStringList{QStringLiteral("View Options")};
    const QStringList date = options + QStringList{QStringLiteral("Show Date")};
    const QStringList navigation = options + QStringList{QStringLiteral("Navigation")};
    const QStringList sortChoices{"sortName", "sortModified", "sortCreated", "sortExtension"};
    const QStringList directionChoices{"ascending", "descending"};
    const QStringList dateChoices{"dateModified", "dateCreated", "dateNone"};
    const QStringList navigationChoices{"navigationTree", "navigationList"};
    const auto add = [&](const char *id, bool library, const char *name, QVariant from, QVariant to,
                         const QStringList &path, const QStringList &choices = {}) {
        QTest::newRow(id) << QByteArray(id) << library << QByteArray(name) << from << to << path << choices;
    };
    add("sortBar", false, "showSortBar", false, true, root);
    add("filterBar", false, "showFilterBar", false, true, root);
    add("sortName", true, "sortMode", 1, 0, sort, sortChoices);
    add("sortModified", true, "sortMode", 0, 1, sort, sortChoices);
    add("sortCreated", true, "sortMode", 0, 2, sort, sortChoices);
    add("sortExtension", true, "sortMode", 0, 3, sort, sortChoices);
    add("ascending", true, "ascending", false, true, sort, directionChoices);
    add("descending", true, "ascending", true, false, sort, directionChoices);
    add("foldersFirst", true, "foldersFirst", false, true, sort);
    add("dateModified", false, "dateMode", 0, 1, date, dateChoices);
    add("dateCreated", false, "dateMode", 0, 2, date, dateChoices);
    add("dateNone", false, "dateMode", 1, 0, date, dateChoices);
    add("excerpts", false, "showExcerpts", false, true, options);
    add("navigationTree", true, "navigationMode", 1, 0, navigation, navigationChoices);
    add("navigationList", true, "navigationMode", 0, 1, navigation, navigationChoices);
}

void FomawriteTest::windowsMenuWorkspaceActionsShareState() {
    QFETCH(QByteArray, command); QFETCH(bool, libraryOwner); QFETCH(QByteArray, property);
    QFETCH(QVariant, before); QFETCH(QVariant, after);
    QFETCH(QStringList, menuPath); QFETCH(QStringList, exclusive);
    Cycle103SettingsSnapshot isolatedSettings;
    Cycle117Window f; QVERIFY2(f.start(900), qPrintable(f.error));
    f.quick->resize(900, 540); // Nested menus must work in a short window too.
    auto *loader = f.window->findChild<QObject *>("windowsMenuLoader"); QVERIFY(loader);
    loader->setProperty("active", true);
    QTRY_VERIFY(f.window->findChild<QObject *>("windowsMenuBar"));
    auto *bar = f.window->findChild<QObject *>("windowsMenuBar");
    auto *item = bar->findChild<QQuickItem *>(QStringLiteral("windows_") + QString::fromLatin1(command));
    QVERIFY2(item, qPrintable(QStringLiteral("The Windows desktop menu is missing %1").arg(QString::fromLatin1(command))));
    auto *pane = f.window->findChild<QObject *>("libraryPane"); QVERIFY(pane);
    QVariantMap paneState, libraryState;
    for (const char *key : {"dateMode", "showExcerpts", "showSortBar", "showFilterBar"}) paneState.insert(key, pane->property(key));
    for (const char *key : {"navigationMode", "ascending", "sortMode", "foldersFirst"}) libraryState.insert(key, f.backend.library()->property(key));
    const auto cleanup = qScopeGuard([&] {
        for (auto it = paneState.cbegin(); it != paneState.cend(); ++it) pane->setProperty(it.key().toLatin1().constData(), it.value());
        for (auto it = libraryState.cbegin(); it != libraryState.cend(); ++it) f.backend.library()->setProperty(it.key().toLatin1().constData(), it.value());
    });
    QObject *owner = libraryOwner ? f.backend.library() : pane;
    QVERIFY(owner->setProperty(property.constData(), before));
    QTRY_VERIFY(!item->property("checked").toBool());
    QVERIFY(item->property("enabled").toBool());
    QVERIFY(item->property("checkable").toBool());
    QCOMPARE(item->property("text").toString(), testCommandLabel(f.window.data(), QString::fromLatin1(command)));
    const QString original = QString::fromUtf8("# Workspace menu\n\nA quiet café draft.\n");
    QVERIFY(f.editor->setProperty("text", original));
    QVERIFY(QMetaObject::invokeMethod(f.editor, "insert", Q_ARG(int, original.size()), Q_ARG(QString, "Unsaved ending.")));
    QVERIFY(cycle113SettleViewport(f.window.data()));
    f.editor->forceActiveFocus();
    const int selectionStart = original.indexOf(QString::fromUtf8("café"));
    QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, selectionStart), Q_ARG(int, selectionStart + 4)));
    QTRY_VERIFY(f.editor->hasActiveFocus());
    const QString draft = f.editor->property("text").toString();
    const int revision = f.backend.documentRevision();
    const QVariant undo = f.editor->property("canUndo"), redo = f.editor->property("canRedo");
    QVERIFY(f.backend.modified()); QVERIFY(undo.toBool());
    const auto menuNamed = [&](const QString &title) -> QObject * {
        for (QObject *candidate : bar->findChildren<QObject *>())
            if (candidate->property("title").toString() == title) return candidate;
        return nullptr;
    };
    const auto navigateRow = [&](QObject *popup, QQuickItem *target) {
        int targetIndex = -1;
        for (int index = 0; index < popup->property("count").toInt(); ++index) {
            QQuickItem *candidate = nullptr;
            if (!QMetaObject::invokeMethod(popup, "itemAt", Q_RETURN_ARG(QQuickItem *, candidate), Q_ARG(int, index))) return -1;
            if (candidate == target) targetIndex = index;
        }
        if (targetIndex < 0 || !popup->property("opened").toBool()) return -1;
        cycle111KeyClick(f.quick, Qt::Key_Home);
        for (int step = 0; step <= popup->property("count").toInt()
             && popup->property("currentIndex").toInt() != targetIndex; ++step)
            cycle111KeyClick(f.quick, Qt::Key_Down);
        return popup->property("currentIndex").toInt() == targetIndex ? targetIndex : -1;
    };
    const auto clickPopupItem = [&](QObject *popup, QQuickItem *target) {
        auto *viewport = qvariant_cast<QQuickItem *>(popup->property("contentItem"));
        if (!viewport) return false;
        const int index = navigateRow(popup, target);
        // A dormant non-first row reports y=0 until its layout is polished.
        // Observe its real position before choosing a wheel direction.
        if (index < 0 || !QTest::qWaitFor([&] { return index == 0 || target->y() > 0; }, 1000)) return false;
        const auto exposed = [&] {
            const QPointF centre = target->mapToScene(QPointF(target->width()/2, target->height()/2));
            if (!target->isVisible() || !target->isEnabled() || target->width() <= 0 || target->height() <= 0
                || !QRectF(QPointF(0, 0), f.quick->size()).contains(centre)
                || !viewport->mapRectToScene(viewport->boundingRect()).contains(centre)) return false;
            for (auto *parent = target->parentItem(); parent; parent = parent->parentItem())
                if (parent->clip() && !parent->mapRectToScene(parent->boundingRect()).contains(centre)) return false;
            return true;
        };
        QElapsedTimer wheelClock; wheelClock.start();
        for (int step = 0; step < 32 && !exposed(); ++step) {
            const QRectF crop = viewport->mapRectToScene(viewport->boundingRect());
            const QPointF position = crop.center();
            const QPointF centre = target->mapToScene(QPointF(target->width()/2, target->height()/2));
            const int delta = centre.y() < crop.top() ? 120 : -120;
            const qreal previous = viewport->property("contentY").toReal();
            QTest::mouseMove(f.quick, position.toPoint());
            QWheelEvent event(position, f.quick->mapToGlobal(position.toPoint()), QPoint(), QPoint(0, delta),
                              Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
            event.setTimestamp(static_cast<quint64>(wheelClock.msecsSinceReference() + wheelClock.elapsed()));
            QCoreApplication::sendEvent(f.quick, &event);
            if (!QTest::qWaitFor([&] { return qAbs(viewport->property("contentY").toReal() - previous) > .01; }, 1000)) return false;
            if (!QTest::qWaitFor([&] { return !viewport->property("moving").toBool(); }, 1000)) return false;
        }
        QSignalSpy frames(f.quick, &QQuickWindow::frameSwapped); f.quick->update();
        if (!QTest::qWaitFor([&] { return !frames.isEmpty() && exposed(); }, 1000)) return false;
        QSignalSpy clicked(target, SIGNAL(clicked()));
        const QPoint point = target->mapToScene(QPointF(target->width()/2, target->height()/2)).toPoint();
        QTest::mouseClick(f.quick, Qt::LeftButton, Qt::NoModifier, point);
        return clicked.isValid() && clicked.count() == 1;
    };
    const auto openMenus = [&]() {
        QObject *parent = nullptr;
        for (const QString &title : menuPath) {
            auto *menu = menuNamed(title);
            if (!menu) { qWarning() << "Missing desktop submenu" << title; return false; }
            QQuickItem *entry = nullptr;
            for (auto *candidate : f.window->findChildren<QQuickItem *>()) {
                const bool barItem = candidate->inherits("QQuickMenuBarItem")
                    && qvariant_cast<QObject *>(candidate->property("menu")) == menu;
                if (barItem || qvariant_cast<QObject *>(candidate->property("subMenu")) == menu) { entry = candidate; break; }
            }
            if (!entry) return false;
            if (parent) {
                // Real keyboard submenu navigation needs no dormant row bounds.
                if (navigateRow(parent, entry) < 0) return false;
                cycle111KeyClick(f.quick, Qt::Key_Right);
            } else if (!cycle102Click(f.quick, entry)) return false;
            if (!QTest::qWaitFor([&] { return menu->property("opened").toBool(); }, 3000)) return false;
            parent = menu;
        }
        return true;
    };
    auto *viewMenu = menuNamed(QStringLiteral("&View")); QVERIFY(viewMenu);
    const auto dismissMenus = [&]() {
        for (int index = menuPath.size() - 1; index >= 0; --index) {
            auto *menu = menuNamed(menuPath.at(index));
            if (!menu || !menu->property("visible").toBool()) continue;
            QTest::keyClick(f.quick, Qt::Key_Escape);
            if (!QTest::qWaitFor([&] { return !menu->property("visible").toBool(); }, 3000)) return false;
        }
        return true;
    };
    for (int activation = 0; activation < 2; ++activation) {
        QVERIFY2(openMenus(), qPrintable(QStringLiteral("Could not open %1").arg(menuPath.join(" > "))));
        QVERIFY(!item->property("checked").toBool());
        QSignalSpy triggered(item, SIGNAL(triggered())); QVERIFY(triggered.isValid());
        QVERIFY(clickPopupItem(menuNamed(menuPath.last()), item));
        QTRY_COMPARE(triggered.count(), 1);
        QTRY_COMPARE(owner->property(property.constData()), after);
        QTRY_VERIFY(item->property("checked").toBool());
        QTRY_VERIFY(!viewMenu->property("visible").toBool());
        QTRY_VERIFY(f.editor->hasActiveFocus());
        for (const auto &id : exclusive) {
            auto *choice = bar->findChild<QObject *>(QStringLiteral("windows_") + id); QVERIFY(choice);
            QCOMPARE(choice->property("checked").toBool(), id == QString::fromLatin1(command));
        }
        // Reopen after activation: externally changed owner state must still
        // drive the check, rather than a stale item-local toggle binding.
        QVERIFY(openMenus()); QVERIFY(item->property("checked").toBool());
        QVERIFY(owner->setProperty(property.constData(), before));
        QTRY_VERIFY(!item->property("checked").toBool());
        QVERIFY(dismissMenus());
        QTRY_VERIFY(!viewMenu->property("visible").toBool());
        QTRY_VERIFY(f.editor->hasActiveFocus());
    }
    QCOMPARE(f.editor->property("text").toString(), draft);
    QCOMPARE(f.backend.documentRevision(), revision);
    QCOMPARE(f.editor->property("selectionStart").toInt(), selectionStart);
    QCOMPARE(f.editor->property("selectionEnd").toInt(), selectionStart + 4);
    QCOMPARE(f.editor->property("canUndo"), undo); QCOMPARE(f.editor->property("canRedo"), redo);
    QVERIFY(f.backend.modified());
}

// Exercise the shipped Windows component on both hosts. These are actual Qt
// pointer activations; ordinary Windows OS input remains a separate gate.
void FomawriteTest::windowsClipboardSearchMenusPreserveFocusAndUndo() {
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(900), qPrintable(f.error));
    f.quick->resize(900, 540);
    auto *loader = f.window->findChild<QObject *>("windowsMenuLoader"); QVERIFY(loader);
    loader->setProperty("active", true);
    QTRY_VERIFY(f.window->findChild<QObject *>("windowsMenuBar"));
    auto *bar = f.window->findChild<QObject *>("windowsMenuBar");
    const auto item = [&](const char *id) { return bar->findChild<QQuickItem *>("windows_" + QString::fromLatin1(id)); };
    for (const char *id : {"editCopyMarkdown", "editCopyHtml", "editCopyFormatted", "editPastePlain", "editPasteMarkdown",
                          "editDelete", "editFindNext", "editFindPrevious", "editFindSelection"})
        QVERIFY2(item(id), qPrintable(QStringLiteral("Missing Windows Edit action %1").arg(id)));
    // Three Copy, two Paste As, Delete, and three Find additions are nine
    // actions; existing Find/Replace retain their desktop routes as well.
    const auto menu = [&](const QString &title) -> QObject * {
        for (auto *candidate : bar->findChildren<QObject *>())
            if (candidate->property("title").toString() == title) return candidate;
        return nullptr;
    };
    auto *editMenu = menu(QStringLiteral("&Edit")); QVERIFY(editMenu);
    const auto openMenu = [&](const QString &title) {
        auto *popup = menu(title); if (!popup) return false;
        for (auto *candidate : f.window->findChildren<QQuickItem *>())
            if ((candidate->inherits("QQuickMenuBarItem") && qvariant_cast<QObject *>(candidate->property("menu")) == popup)
                || qvariant_cast<QObject *>(candidate->property("subMenu")) == popup)
                return cycle102Click(f.quick, candidate)
                    && QTest::qWaitFor([&] { return popup->property("opened").toBool(); }, 3000);
        return false;
    };
    const auto activate = [&](const char *id) {
        auto *target = item(id);
        if (!target || !QTest::qWaitFor([&] { return target->isEnabled(); }, 1000)) {
            qWarning() << "Desktop Edit action unavailable" << id << "canPaste" << f.editor->property("canPaste")
                       << "sourceAllowed" << f.window->property("sourceClipboardAllowed") << "liveAllowed" << f.window->property("canFormatLive")
                       << "editTarget" << f.window->property("editTarget") << "focus" << f.quick->activeFocusItem();
            return false;
        }
        if (!openMenu(QStringLiteral("&Edit"))) { qWarning() << "Could not open Edit for" << id; return false; }
        if ((QByteArray(id) == "editPastePlain" || QByteArray(id) == "editPasteMarkdown") && !openMenu(QStringLiteral("Paste As"))) return false;
        QSignalSpy triggered(target, SIGNAL(triggered()));
        return cycle102Click(f.quick, target) && triggered.count() == 1
            && QTest::qWaitFor([&] { return !editMenu->property("visible").toBool(); }, 3000);
    };
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString original = QString::fromUtf8("İ 😀 **café** and café.\n");
    const QString path = directory.filePath(QStringLiteral("Clipboard café.md"));
    QVERIFY(cycle103Write(path, original.toUtf8())); QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "setSourceEditing"));
    QVERIFY(cycle113SettleViewport(f.window.data()));
    QVERIFY(QMetaObject::invokeMethod(f.editor, "insert", Q_ARG(int, original.size()), Q_ARG(QString, "Unsaved ending.\n")));
    const QString draft = f.editor->property("text").toString();
    const int start = draft.indexOf(QString::fromUtf8("**café**"));
    const auto selectSource = [&] {
        f.editor->forceActiveFocus();
        return QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, start), Q_ARG(int, start + 8));
    };
    QVERIFY(selectSource()); QTRY_VERIFY(f.window->property("canFormatSource").toBool());
    const int revision = f.backend.documentRevision();
    const QVariant undo = f.editor->property("canUndo"), redo = f.editor->property("canRedo");
    QVERIFY(activate("editCopyMarkdown"));
    QCOMPARE(QGuiApplication::clipboard()->mimeData()->data("text/markdown"), QString::fromUtf8("**café**").toUtf8());
    QVERIFY(activate("editCopyHtml")); QVERIFY(QGuiApplication::clipboard()->text().contains("<html"));
    QVERIFY(QGuiApplication::clipboard()->text().contains(QString::fromUtf8("café")));
    QVERIFY(activate("editCopyFormatted")); QVERIFY(QGuiApplication::clipboard()->mimeData()->hasHtml());
    QCOMPARE(QGuiApplication::clipboard()->text(), QString::fromUtf8("café"));
    QCOMPARE(f.editor->property("text").toString(), draft); QCOMPARE(f.backend.documentRevision(), revision);
    QCOMPARE(f.editor->property("selectionStart").toInt(), start); QCOMPARE(f.editor->property("selectionEnd").toInt(), start + 8);
    QCOMPARE(f.editor->property("canUndo"), undo); QCOMPARE(f.editor->property("canRedo"), redo);
    QTRY_VERIFY(f.editor->hasActiveFocus());
    // Both Paste As routes replace the original selection in one undo step.
    QGuiApplication::clipboard()->setText(QString::fromUtf8("茶 🙂"));
    QVERIFY(activate("editPastePlain"));
    QTRY_COMPARE(f.editor->property("text").toString(), QString(draft).replace(start, 8, QString::fromUtf8("茶 🙂")));
    auto *undoItem = [&]() -> QQuickItem * { for (auto *candidate : bar->findChildren<QQuickItem *>()) if (candidate->property("text").toString() == "&Undo") return candidate; return nullptr; }();
    QVERIFY(undoItem);
    const auto undoMenu = [&] {
        if (!openMenu(QStringLiteral("&Edit"))) return false;
        return cycle102Click(f.quick, undoItem) && QTest::qWaitFor([&] { return !editMenu->property("visible").toBool(); }, 3000);
    };
    QVERIFY(undoMenu()); QTRY_COMPARE(f.editor->property("text").toString(), draft);
    QVERIFY(f.editor->property("canRedo").toBool());
    // Ordinary rich clipboards provide plain and HTML forms. Make them
    // disagree so this action must convert HTML, rather than use plain Paste.
    auto *html = new QMimeData; html->setHtml(QStringLiteral("<p><strong>茶</strong></p>"));
    html->setText(QStringLiteral("PLAIN FALLBACK MUST NOT BE USED"));
    QGuiApplication::clipboard()->setMimeData(html);
    const QString converted = f.backend.clipboardMarkdown(); QVERIFY(converted.contains(QStringLiteral("**茶**")));
    QVERIFY(selectSource()); QVERIFY(activate("editPasteMarkdown"));
    QTRY_COMPARE(f.editor->property("text").toString(), QString(draft).replace(start, 8, converted));
    QVERIFY(undoMenu()); QTRY_COMPARE(f.editor->property("text").toString(), draft);
    QVERIFY(selectSource()); QVERIFY(activate("editDelete"));
    QTRY_COMPARE(f.editor->property("text").toString(), QString(draft).remove(start, 8));
    QVERIFY(undoMenu()); QTRY_COMPARE(f.editor->property("text").toString(), draft);
    // Find actions must use the selected Unicode text and wrap actual matches.
    f.editor->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, start + 2), Q_ARG(int, start + 6)));
    QVERIFY(activate("editFindSelection"));
    auto *query = f.window->findChild<QQuickItem *>("searchField"); QVERIFY(query);
    QTRY_COMPARE(query->property("text").toString(), QString::fromUtf8("café"));
    const auto matches = f.backend.searchPositions(QString::fromUtf8("café")); QCOMPARE(matches.size(), 2);
    QVERIFY(activate("editFindPrevious")); QCOMPARE(f.editor->property("selectionStart").toInt(), matches.last().toInt());
    QVERIFY(activate("editFindNext")); QCOMPARE(f.editor->property("selectionStart").toInt(), matches.first().toInt());
    query->setProperty("text", "absent");
    QTRY_VERIFY(!item("editFindNext")->isEnabled()); QTRY_VERIFY(!item("editFindPrevious")->isEnabled());
    // Replace has a separate auxiliary edit target: deleting its selection
    // cannot rehighlight the query or change Source's revision/history.
    QVERIFY(activate("editReplace"));
    auto *replacement = f.window->findChild<QQuickItem *>("replaceField"); QVERIFY(replacement);
    replacement->setProperty("text", QStringLiteral("auxiliary selection"));
    replacement->forceActiveFocus(); QVERIFY(QMetaObject::invokeMethod(replacement, "selectAll"));
    for (const char *id : {"editCopyMarkdown", "editCopyHtml", "editCopyFormatted", "editPastePlain", "editPasteMarkdown"})
        QTRY_VERIFY2(!item(id)->isEnabled(), id);
    const int beforeAuxiliaryDelete = f.backend.documentRevision();
    const QVariant beforeAuxiliaryUndo = f.editor->property("canUndo"), beforeAuxiliaryRedo = f.editor->property("canRedo");
    QVERIFY(activate("editDelete")); QTRY_COMPARE(replacement->property("text").toString(), QString());
    QCOMPARE(f.editor->property("text").toString(), draft); QCOMPARE(f.backend.documentRevision(), beforeAuxiliaryDelete);
    QCOMPARE(f.editor->property("canUndo"), beforeAuxiliaryUndo); QCOMPARE(f.editor->property("canRedo"), beforeAuxiliaryRedo);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "closeSearch", Q_ARG(QVariant, true)));
    f.editor->forceActiveFocus();
    QSignalSpy saved(&f.backend, &Backend::saveSucceeded); f.backend.save(); QTRY_COMPARE(saved.count(), 1);
    QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), draft.toUtf8()); file.close();
    QVERIFY(f.backend.open(QUrl::fromLocalFile(path))); QCOMPARE(f.editor->property("text").toString(), draft);
    // A real shipped Live page supplies the selection and receives both paste
    // variants. Never manufacture bridge readiness or a successful reply.
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "setLiveEditing", Q_ARG(QVariant, true)));
    QTRY_VERIFY_WITH_TIMEOUT(f.backend.editorBridge()->isReady(), 15000);
    auto *view = f.window->findChild<QQuickItem *>("liveEditorView"); QVERIFY(view);
    const auto pageText = [&] { return cycle143PageResult(view, "window.fomawriteEditor.getText()", 0).toString(); };
    QTRY_COMPARE_WITH_TIMEOUT(pageText(), draft, 10000);
    const auto selectLive = [&] {
        view->forceActiveFocus();
        return cycle143PageResult(view, QStringLiteral("(()=>{let v=window.fomawriteEditor.view;v.focus();v.dispatch({selection:{anchor:%1,head:%2}});return v.state.selection.main.to;})()").arg(start).arg(start + 8), 0).toInt() == start + 8;
    };
    QVERIFY(selectLive()); QTRY_VERIFY(f.window->property("canFormatLive").toBool());
    QVERIFY(activate("editCopyMarkdown")); QTRY_COMPARE(QGuiApplication::clipboard()->mimeData()->data("text/markdown"), QString::fromUtf8("**café**").toUtf8());
    QVERIFY(activate("editCopyHtml")); QTRY_VERIFY(QGuiApplication::clipboard()->text().contains("<html"));
    QVERIFY(activate("editCopyFormatted")); QTRY_VERIFY(QGuiApplication::clipboard()->mimeData()->hasHtml());
    QGuiApplication::clipboard()->setText(QString::fromUtf8("茶 🙂")); QVERIFY(activate("editPastePlain"));
    const QString livePlain = QString(draft).replace(start, 8, QString::fromUtf8("茶 🙂"));
    QTRY_COMPARE_WITH_TIMEOUT(pageText(), livePlain, 5000); QTRY_COMPARE(f.editor->property("text").toString(), livePlain);
    QVERIFY(undoMenu()); QTRY_COMPARE_WITH_TIMEOUT(pageText(), draft, 5000); QTRY_COMPARE(f.editor->property("text").toString(), draft);
    auto *liveHtml = new QMimeData; liveHtml->setHtml(QStringLiteral("<p><strong>茶</strong></p>"));
    liveHtml->setText(QStringLiteral("PLAIN FALLBACK MUST NOT BE USED")); QGuiApplication::clipboard()->setMimeData(liveHtml);
    QVERIFY(selectLive()); QVERIFY(activate("editPasteMarkdown"));
    QTRY_COMPARE_WITH_TIMEOUT(pageText(), QString(draft).replace(start, 8, converted), 5000);
    QVERIFY(undoMenu()); QTRY_COMPARE_WITH_TIMEOUT(pageText(), draft, 5000); QTRY_COMPARE(f.editor->property("text").toString(), draft);
    // Preview-only cannot target either the stale Source or Live selection.
    QVERIFY(cycle118ReadPreview(f.window.data())); QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 2);
    for (const char *id : {"editCopyMarkdown", "editCopyHtml", "editCopyFormatted", "editPastePlain", "editPasteMarkdown", "editDelete"})
        QTRY_VERIFY2(!item(id)->isEnabled(), id);
    QCOMPARE(f.editor->property("text").toString(), draft);
    QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), draft.toUtf8());
}

// A menu's exit transition must not swallow the next writing shortcut. The
// shipped page and real popup clicks exercise routing before the Save barrier.
void FomawriteTest::windowsMenuFormattingKeepsImmediateSaveShortcut() {
    Cycle103SettingsSnapshot settings;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString original = QStringLiteral("# Save shortcut\n\nKeep input. intact.\n");
    const QString expected = QStringLiteral("# Save shortcut\n\nKeep **input.** intact.\n");
    const int start = original.indexOf(QStringLiteral("input."));
    // Establish the settled Live control before the immediate Live witness;
    // Source then checks that the same menu fix preserves its shortcut route.
    for (const auto &[live, settled] : {std::pair{true, true}, std::pair{true, false}, std::pair{false, false}}) {
        const QString label = live ? (settled ? "live-settled" : "live-immediate") : "source-immediate";
        const QString path = directory.filePath(label + ".md");
        QVERIFY(cycle103Write(path, original.toUtf8()));
        Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
        QVERIFY(testDesktopMenusMatchPlatform(f.window.data()));
        auto *loader = f.window->findChild<QObject *>("windowsMenuLoader"); QVERIFY(loader);
        if (!f.window->property("isWindows").toBool()) loader->setProperty("active", true);
        QTRY_VERIFY(f.window->findChild<QObject *>("windowsMenuBar"));
        auto *bar = f.window->findChild<QObject *>("windowsMenuBar");
        QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
        auto *bridge = f.backend.editorBridge();
        if (live) {
            QVERIFY(QMetaObject::invokeMethod(f.window.data(), "setLiveEditing", Q_ARG(QVariant, true)));
            QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 15000);
            auto *view = f.window->findChild<QQuickItem *>("liveEditorView"); QVERIFY(view);
            QTRY_COMPARE_WITH_TIMEOUT(cycle143PageResult(view, "window.fomawriteEditor.getText()", 0).toString(), original, 10000);
            const QString selection = QStringLiteral("(()=>{const v=window.fomawriteEditor.view;v.dispatch({selection:{anchor:%1,head:%2}});v.focus();return v.state.sliceDoc(v.state.selection.main.from,v.state.selection.main.to);})()").arg(start).arg(start + 6);
            QCOMPARE(cycle143PageResult(view, selection, 0).toString(), QStringLiteral("input."));
            view->forceActiveFocus();
            QTRY_VERIFY(f.window->property("canFormatLive").toBool());
        } else {
            // Document opening queues its restored caret; complete that setup
            // before establishing the selection for the subsequent menu click.
            QTRY_COMPARE(f.editor->property("text").toString(), original);
            QCoreApplication::processEvents();
            f.editor->forceActiveFocus();
            QVERIFY(QMetaObject::invokeMethod(f.editor, "select", Q_ARG(int, start), Q_ARG(int, start + 6)));
            QCOMPARE(f.editor->property("selectedText").toString(), QStringLiteral("input."));
            QTRY_VERIFY(f.window->property("canFormatSource").toBool());
        }
        // This fixture sends intentional shortcut keys, unlike the mouse-only
        // fixtures whose Cocoa isolation rejects spontaneous QtTest key events.
        f.isolation.reset();
        f.quick->requestActivate();
        QTRY_VERIFY(f.quick->isActive());
        QObject *format = nullptr;
        for (auto *candidate : bar->findChildren<QObject *>())
            if (candidate->property("title").toString() == "F&ormat") { format = candidate; break; }
        QVERIFY(format);
        QQuickItem *formatEntry = nullptr, *bold = nullptr;
        for (auto *candidate : f.window->findChildren<QQuickItem *>())
            if (candidate->inherits("QQuickMenuBarItem") && qvariant_cast<QObject *>(candidate->property("menu")) == format) { formatEntry = candidate; break; }
        for (auto *candidate : format->findChildren<QQuickItem *>())
            if (candidate->property("text").toString() == "&Bold") { bold = candidate; break; }
        QVERIFY(formatEntry && bold);
        QObject *shortcut = nullptr;
        for (auto *candidate : f.window->findChildren<QObject *>())
            if (candidate->metaObject()->indexOfSignal("activated()") >= 0
                && candidate->property("sequence").toString() == "Ctrl+S") { shortcut = candidate; break; }
        QVERIFY(shortcut);
        QSignalSpy activated(shortcut, SIGNAL(activated()));
        QSignalSpy triggered(bold, SIGNAL(triggered()));
        QSignalSpy selections(bridge, &EditorBridge::requestSelection);
        QSignalSpy saved(&f.backend, &Backend::saveSucceeded), failed(&f.backend, &Backend::saveFailed);
        QVERIFY(cycle102Click(f.quick, formatEntry));
        QTRY_VERIFY(format->property("opened").toBool());
        QVERIFY(bold->isEnabled());
        const QPoint point = bold->mapToScene(QPointF(bold->width() / 2, bold->height() / 2)).toPoint();
        QVERIFY(QRect(QPoint(0, 0), f.quick->size()).contains(point));
        QElapsedTimer pairTime; pairTime.start();
        QTest::mouseClick(f.quick, Qt::LeftButton, Qt::NoModifier, point, 0);
        if (settled) QTRY_VERIFY(!format->property("visible").toBool());
        // No waits, focus repair, page request or observation separates these
        // inputs in the immediate cases. QtTest processes each input normally.
        QTest::keyClick(f.quick, Qt::Key_S, Qt::ControlModifier, 0);
        qInfo() << "Windows menu Bold/Save" << label << "elapsed ms" << pairTime.elapsed()
                << "menu visible/opened" << format->property("visible") << format->property("opened")
                << "focus" << f.quick->activeFocusItem() << "shortcut activations" << activated.count()
                << "selection requests" << selections.count() << "save success/failure" << saved.count() << failed.count();
        QCOMPARE(triggered.count(), 1);
        QCOMPARE(selections.count(), live ? 1 : 0);
        QCOMPARE(activated.count(), 1);
        QTRY_COMPARE_WITH_TIMEOUT(saved.count(), 1, Backend::LivePersistenceTimeout + 1000);
        QCOMPARE(failed.count(), 0);
        QTRY_COMPARE(cycle103Read(path), expected.toUtf8());
        QTRY_COMPARE(f.editor->property("text").toString(), expected);
        QVERIFY(!f.backend.modified());
        if (live) {
            auto *view = f.window->findChild<QQuickItem *>("liveEditorView"); QVERIFY(view);
            QCOMPARE(cycle143PageResult(view, "window.fomawriteEditor.getText()", 0).toString(), expected);
        }
        QVERIFY2(f.error.isEmpty(), qPrintable(f.error));
    }
}
