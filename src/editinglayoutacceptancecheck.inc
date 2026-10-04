    const auto editingLayoutCheck = [&](const QString &name, const std::function<void()> &exercise) {
        const qsizetype before = failures.size(); exercise();
        editingLayoutChecks.append(QJsonObject{{"name", name}, {"passed", failures.size() == before},
            {"newFailures", int(failures.size() - before)}});
    };
    editingLayoutCheck("four-independent-editor-layout-combinations", [&] {
        backend.setThemePreset("studio"); window->resize(1280, 720);
        layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
        const QString text = QString::fromUtf8("# Independent controls\n\nA café draft keeps its Markdown.\n");
        if (!loadSample("Independent controls", text)) return;
        const auto baseline = currentBounds();
        for (int pass = 0; pass < 2; ++pass) {
            for (int mode : {0, 1, 1, 0}) {
                const bool previousEditor = layout->property("visualEditEnabled").toBool();
                action(mode == 0 ? "singleModeButton" : "previewSplitButton", mode, previousEditor);
                for (bool visual : {false, true, true, false}) {
                    action(visual ? "visualEditToggle" : "sourceModeButton", mode, visual);
                    check(editor->isVisible() == !visual && visualPane->isVisible() == visual
                          && pane->isVisible() == (mode == 1) && !pane->property("visualEditEnabled").toBool(),
                          "Editing/Layout did not present exactly one chosen editor plus optional read-only Preview");
                    for (const QString &name : stationaryNames)
                        check(bounds(buttons.value(name)) == baseline.value(name), "Editing/Layout moved footer control: " + name);
                    auto *editing = child("documentEditingControls"), *arrangement = child("documentViewControls");
                    check(editing && arrangement && bounds(editing).right() + 10 <= bounds(arrangement).left(), "Editing/Layout groups are not visually separated");
                    check(editor->property("text").toString() == text && !backend.modified(), "Choosing Editing/Layout changed the document");
                    if (pass == 0) capture(QString("editing-%1-layout-%2").arg(visual ? "visual" : "source").arg(mode ? "split" : "single"));
                }
            }
        }
        action("visualEditToggle", 0, true); action("previewSplitButton", 1, true);
        window->resize(720, 620); check(waitUntil(ready), "Narrow chosen-editor layout did not settle");
        check(layout->property("effectiveLayoutMode").toInt() == 0 && layout->property("visualEditEnabled").toBool()
              && visualPane->isVisible() && !pane->isVisible(), "Narrow layout lost chosen Visual editor");
        capture("editing-visual-narrow-single"); window->resize(1280, 720); settle(100);
        action("previewOnly", 2, true); action("visualEditToggle", 1, true);
        check(pane->isVisible() && visualPane->isVisible(), "Preview Only return forgot chosen editor/layout");
    });
    editingLayoutCheck("visual-split-edit-readonly-output-and-single-undo", [&] {
        window->resize(1280, 720);
        const QString text = QString::fromUtf8("# Canonical source\n\nPlain café text.\n");
        if (!loadSample("Canonical source", text)) return;
        action("visualEditToggle", 0, true); action("previewSplitButton", 1, true);
        auto *visual = visualPane->findChild<QQuickItem *>("visualEditor");
        auto *rendered = pane->findChild<QQuickItem *>("renderedPreview");
        if (!check(visual && rendered && rendered->property("readOnly").toBool(), "Output Preview is not read-only")) return;
        const int start = visual->property("text").toString().indexOf(QString::fromUtf8("café"));
        check(start >= 0, "Visual projection lost sample text"); visual->forceActiveFocus();
        QMetaObject::invokeMethod(visual, "select", Q_ARG(int, start), Q_ARG(int, start + 4));
        key(window, Qt::Key_unknown, "bistro");
        const QString changed = QString(text).replace(QString::fromUtf8("café"), "bistro");
        check(waitUntil([&] { return editor->property("text").toString() == changed; }), "Visual Split typing did not update exact canonical source");
        const int revision = backend.documentRevision();
        QMetaObject::invokeMethod(pane, "focusRenderedSurface"); key(window, Qt::Key_unknown, "NO WRITE");
        check(editor->property("text").toString() == changed && backend.documentRevision() == revision, "Read-only output accepted typing");
        action("singleModeButton", 0, true); action("previewSplitButton", 1, true);
        action("sourceModeButton", 1, false);
        QMetaObject::invokeMethod(editor, "undo"); check(editor->property("text").toString() == text, "Visual edit did not remain one canonical Undo");
        QMetaObject::invokeMethod(editor, "redo"); check(editor->property("text").toString() == changed, "Visual edit Redo changed Markdown");
        QFile original(backend.fileUrl().toLocalFile()); check(original.open(QIODevice::ReadOnly) && original.readAll() == text.toUtf8(), "View/edit test saved the fixture unexpectedly");
        capture("editing-visual-split-undo-source");
    });
