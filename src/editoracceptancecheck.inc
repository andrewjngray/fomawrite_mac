    // Integrated acceptance runs in every shipped executable with disposable
    // documents. Input goes only to this diagnostic window, never other apps.
    const auto acceptance = [&](const QString &name, const std::function<void()> &exercise) {
        const qsizetype before = failures.size();
        exercise();
        editorAcceptanceChecks.append(QJsonObject{{"name", name}, {"passed", failures.size() == before},
            {"newFailures", int(failures.size() - before)}, {"windowWidth", window->width()},
            {"windowHeight", window->height()}, {"documentRevision", backend.documentRevision()}});
    };
    window->resize(1440, 800);
    backend.setThemePreset("studio");
    QMetaObject::invokeMethod(window, "applyReferenceWorkspace");
    acceptance("formatting-follows-writing-focus", [&] {
        const QString text = "# Focus safety\n\nA quiet passage.\n";
        if (!loadSample("Formatting focus", text) || !findAvailable) return;
        editor->forceActiveFocus();
        QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 18), Q_ARG(int, 23));
        check(click(window, searchButton), "Cannot open Find for formatting-focus check");
        check(enterText(window, findField, "quiet"), "Cannot enter focus-check query");
        const int start = editor->property("selectionStart").toInt();
        const int end = editor->property("selectionEnd").toInt();
        check(!window->property("canFormatSource").toBool(), "Find retained Source formatting ownership");
        check(QMetaObject::invokeMethod(window, "tryWrapSelection", Q_ARG(QVariant, "**"), Q_ARG(QVariant, "**")), "Cannot invoke guarded formatting route");
        check(editor->property("text").toString() == text && findField->hasActiveFocus()
              && editor->property("selectionStart").toInt() == start && editor->property("selectionEnd").toInt() == end,
              "Formatting changed Source or stole focus from Find");
        check(click(window, closeFind), "Cannot close focus-check Find");
        editor->forceActiveFocus();
        QMetaObject::invokeMethod(editor, "select", Q_ARG(int, text.indexOf("quiet")), Q_ARG(int, text.indexOf("quiet") + 5));
        check(click(window, child("compactBoldButton")), "Cannot click enabled Bold");
        check(editor->property("text").toString() == QString(text).replace("quiet", "**quiet**"), "Toolbar Bold lost its Source selection");
        QMetaObject::invokeMethod(editor, "undo");
        check(editor->property("text").toString() == text && !editor->property("canUndo").toBool(), "Bold was not one exact Undo");
    });
    auto *linkEditor = window->findChild<QObject *>("linkEditor");
    auto *linkLabel = child("linkEditorLabel");
    auto *linkUrl = child("linkEditorUrl");
    auto *linkTitle = child("linkEditorTitle");
    auto *linkApply = child("linkEditorApplyButton");
    auto *linkCancel = child("linkEditorCancelButton");
    const bool linksAvailable = check(linkEditor && linkLabel && linkUrl && linkTitle && linkApply && linkCancel,
                                      "Link editor is missing named controls");
    acceptance("link-insert-edit-cancel-and-undo", [&] {
        const QString text = QString::fromUtf8("# Link editing\n\nRead café notes.\n");
        if (!linksAvailable || !loadSample("Link editing", text)) return;
        const int start = text.indexOf(QString::fromUtf8("café"));
        editor->forceActiveFocus();
        QMetaObject::invokeMethod(editor, "select", Q_ARG(int, start), Q_ARG(int, start + 4));
        check(QMetaObject::invokeMethod(window, "tryInsertLink"), "Cannot open link editor through shared route");
        check(waitUntil([&] { return linkEditor->property("visible").toBool(); }), "Link editor did not open");
        check(enterText(window, linkUrl, "./Notes with spaces.md"), "Cannot enter local destination");
        check(enterText(window, linkTitle, QString::fromUtf8("A café title")), "Cannot enter Unicode link title");
        capture("acceptance-link-editor");
        check(click(window, linkApply), "Cannot click Insert link");
        check(waitUntil([&] { return !linkEditor->property("visible").toBool(); }), "Insert did not close link editor");
        const QString linked = editor->property("text").toString();
        check(linked.contains("[café](<./Notes with spaces.md> \"A café title\")"), "Link did not preserve local spaces/title");
        editor->setProperty("cursorPosition", start + 2); editor->forceActiveFocus();
        QMetaObject::invokeMethod(window, "tryInsertLink");
        check(waitUntil([&] { return linkEditor->property("visible").toBool(); }), "Cannot reopen existing link");
        check(linkEditor->property("editingExisting").toBool() && linkApply->property("text").toString() == "Save", "Existing link does not show Edit/Save mode");
        check(linkUrl->property("text").toString() == "./Notes with spaces.md", "Existing destination did not round-trip");
        check(click(window, linkCancel), "Cannot cancel link editor");
        check(waitUntil([&] { return !linkEditor->property("visible").toBool() && editor->hasActiveFocus(); }), "Link Cancel did not restore writing focus after closing");
        check(editor->property("text").toString() == linked && editor->hasActiveFocus(), "Cancel changed Markdown or lost Source focus");
        QMetaObject::invokeMethod(editor, "undo");
        check(editor->property("text").toString() == text && !editor->property("canUndo").toBool(), "Link insertion was not one exact Undo");
        QMetaObject::invokeMethod(editor, "redo");
        check(editor->property("text").toString() == linked, "Link Redo changed Markdown");
    });
    acceptance("link-stale-target-refused-and-entries-retained", [&] {
        if (!linksAvailable || !loadSample("Stale link", "Read these notes.\n")) return;
        window->resize(720, 520); backend.setThemePreset("dark"); settle(100);
        editor->forceActiveFocus();
        QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 5), Q_ARG(int, 10));
        QMetaObject::invokeMethod(window, "tryInsertLink");
        check(waitUntil([&] { return linkEditor->property("visible").toBool(); }), "Cannot open narrow link editor");
        check(enterText(window, linkUrl, "https://example.com/kept"), "Cannot enter retained destination");
        QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, 0), Q_ARG(QString, "Changed. "));
        const QString changed = editor->property("text").toString();
        check(click(window, linkApply), "Cannot click stale Apply");
        check(linkEditor->property("visible").toBool() && !linkEditor->property("error").toString().isEmpty()
              && editor->property("text").toString() == changed
              && linkUrl->property("text").toString() == "https://example.com/kept", "Stale link applied or lost entered fields");
        renderBarrier(window);
        for (auto *field : {linkLabel, linkUrl, linkTitle, linkApply, linkCancel, child("linkEditorError")})
            check(QRectF(QPointF(), window->size()).contains(bounds(field)), "Narrow link control is outside window: " + field->objectName());
        capture("acceptance-link-stale-narrow-dark");
        check(click(window, linkCancel), "Cannot cancel stale editor");
        check(editor->property("text").toString() == changed, "Stale Cancel changed document");
    });
    acceptance("visual-table-real-tab-navigation", [&] {
        window->resize(1100, 720); backend.setThemePreset("studio");
        const QString text = QString::fromUtf8("# Table keyboard navigation\n\n| Name | Note |\n| --- | --- |\n| café | **bold** |\n| Empty |   |\n\nAfter.\n");
        if (!loadSample("Table navigation", text)) return;
        action("visualEditToggle", 2, true);
        auto *visual = pane->findChild<QQuickItem *>("visualEditor");
        const QVariantList cells = backend.visualProjection().value("tableCells").toList();
        if (!check(visual && cells.size() == 6, "Visual table projection is missing six cells")) return;
        visual->setProperty("cursorPosition", cells.first().toMap().value("visualStart")); visual->forceActiveFocus();
        for (int index = 1; index < cells.size(); ++index) {
            key(window, Qt::Key_Tab);
            check(visual->property("cursorPosition").toInt() == cells[index].toMap().value("visualStart").toInt(), "Tab did not move to next table cell");
        }
        key(window, Qt::Key_Tab);
        check(visual->property("cursorPosition").toInt() == cells.last().toMap().value("visualStart").toInt(), "Last-cell Tab escaped table boundary");
        key(window, Qt::Key_Backtab, {}, Qt::ShiftModifier);
        check(visual->property("cursorPosition").toInt() == cells[4].toMap().value("visualStart").toInt(), "Shift Tab did not move back");
        check(editor->property("text").toString() == text && !backend.modified() && !editor->property("canUndo").toBool(), "Table navigation changed source/Undo");
        capture("acceptance-visual-table-navigation");
    });
    acceptance("continuous-resize-and-workspace-restoration", [&] {
        window->resize(1440, 800); QMetaObject::invokeMethod(window, "applyReferenceWorkspace");
        action("previewSplitButton", 1, false);
        const QString unchanged = editor->property("text").toString();
        const QVariant savedState = window->property("workspacePaneState");
        const QVariantMap savedMap = savedState.value<QJSValue>().toVariant().toMap();
        for (int pass = 0; pass < 2; ++pass) {
            backend.setThemePreset(pass ? "dark" : "studio");
            for (int step = 0; step <= 24; ++step) {
                const int width = step <= 12 ? 1440 - step * 60 : 720 + (step - 12) * 60;
                window->resize(width, 520 + (width - 720) * 280 / 720);
                settle(8); check(renderBarrier(window), "Cannot render continuous resize");
                QRectF previous;
                for (const QString &name : viewNames) {
                    auto *control = buttons.value(name); const QRectF rect = bounds(control);
                    check(control->isVisible() && bounds(footer).adjusted(-1, -1, 1, 1).contains(rect), "Resize clipped view control: " + name);
                    if (!previous.isNull()) check(previous.right() <= rect.left() + 0.5, "Resize overlapped view controls");
                    previous = rect;
                }
                check(editor->property("text").toString() == unchanged, "Resize changed Markdown");
            }
        }
        layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
        layout->setProperty("organizerWidth", 240); layout->setProperty("fileWidth", 360);
        check(QMetaObject::invokeMethod(window, "restoreWorkspacePaneState", Q_ARG(QVariant, savedState)), "Cannot restore workspace checkpoint");
        check(waitUntil(ready), "Restored workspace did not settle");
        check(layout->property("organizerVisible").toBool() && layout->property("filesVisible").toBool(), "Workspace checkpoint lost navigation visibility");
        for (const char *name : {"organizerWidth", "fileWidth", "previewWidth", "layoutMode", "visualEditEnabled"})
            check(layout->property(name) == savedMap.value(QString::fromLatin1(name)), "Workspace checkpoint changed preference: " + QString::fromLatin1(name));
        check(editor->property("text").toString() == unchanged, "Workspace restoration changed Markdown");
        capture("acceptance-restored-workspace-dark");
    });
    acceptance("export-narrow-horizontal-access-and-cancel", [&] {
        backend.setThemePreset("studio"); window->resize(1100, 720); settle(100);
        const QString unchanged = editor->property("text").toString();
        auto *hub = window->findChild<QObject *>("exportHub");
        if (!check(hub, "Export hub missing")) return;
        QMetaObject::invokeMethod(window, "openExportHub", Q_ARG(QVariant, "pdf"));
        check(waitUntil([&] { return hub->property("visible").toBool(); }), "Export hub did not open");
        hub->setProperty("requestedWidth", 380); hub->setProperty("requestedHeight", 460);
        hub->setProperty("requestedX", 16); hub->setProperty("requestedY", 16);
        settle(100); renderBarrier(window);
        auto *viewport = child("exportHorizontalViewport");
        auto *bar = child("exportHorizontalScrollBar");
        auto *cancel = child("exportCancelButton");
        auto *saveButton = child("exportDestinationButton");
        auto *previewBand = child("exportPreviewBand");
        if (check(viewport && bar && cancel && saveButton && previewBand, "Export controls missing")) {
            check(bar->isVisible() && viewport->property("contentWidth").toReal() > viewport->width(), "Narrow export does not expose horizontal scrolling");
            // Use the real scrollbar's position route to reach the third band.
            bar->setProperty("position", 1.0 - bar->property("size").toReal());
            settle(80); renderBarrier(window);
            check(viewport->property("contentX").toReal() > 300, "Horizontal scrollbar did not reach other export bands");
            check(bounds(viewport).intersects(bounds(previewBand)), "Preview band is unreachable in narrow export");
            check(qAbs(cancel->height() - saveButton->height()) < 0.5 && cancel->height() >= 34, "Export footer button heights differ");
            capture("acceptance-export-narrow-right-pane");
            check(click(window, cancel), "Cannot cancel narrow export");
        } else QMetaObject::invokeMethod(hub, "close");
        check(waitUntil([&] { return !hub->property("visible").toBool(); }), "Export Cancel did not close dialog");
        check(editor->property("text").toString() == unchanged, "Export inspection changed Markdown");
    });
