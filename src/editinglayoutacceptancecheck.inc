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
                const bool previousEditor = layout->property("liveEditEnabled").toBool();
                action(mode == 0 ? "singleModeButton" : "previewSplitButton", mode, previousEditor);
                for (bool live : {false, true, true, false}) {
                    action(live ? "liveEditToggle" : "sourceModeButton", mode, live);
                    auto *livePane = window->findChild<QQuickItem *>("liveEditorPane");
                    check(editor->isVisible() == !live && (livePane && livePane->isVisible()) == live
                          && pane->isVisible() == (mode == 1),
                          "Editing/Layout did not present exactly one chosen editor plus optional read-only Preview");
                    for (const QString &name : stationaryNames)
                        check(bounds(buttons.value(name)) == baseline.value(name), "Editing/Layout moved footer control: " + name);
                    auto *editing = child("documentEditingControls"), *arrangement = child("documentViewControls");
                    check(editing && arrangement && bounds(editing).right() + 10 <= bounds(arrangement).left(), "Editing/Layout groups are not visually separated");
                    check(editor->property("text").toString() == text && !backend.modified(), "Choosing Editing/Layout changed the document");
                    if (pass == 0) capture(QString("editing-%1-layout-%2").arg(live ? "live" : "source").arg(mode ? "split" : "single"));
                }
            }
        }
        action("liveEditToggle", 0, true); action("previewSplitButton", 1, true);
        window->resize(720, 620); check(waitUntil(ready), "Narrow chosen-editor layout did not settle");
        auto *livePane = window->findChild<QQuickItem *>("liveEditorPane");
        check(layout->property("effectiveLayoutMode").toInt() == 0 && layout->property("liveEditEnabled").toBool()
              && livePane && livePane->isVisible() && !pane->isVisible(), "Narrow layout lost chosen Live editor");
        capture("editing-live-narrow-single"); window->resize(1280, 720); settle(100);
        action("previewOnly", 2, true);
        QMetaObject::invokeMethod(window, "selectWritingMode", Q_ARG(QVariant, "live"));
        check(waitUntil(ready), "Preview Only editor return did not settle");
        check(pane->isVisible() && livePane->isVisible(), "Preview Only return forgot chosen editor/layout");
        action("sourceModeButton", 1, false);
    });
