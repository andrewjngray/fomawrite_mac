    // Drive the shipped Live WebEngine surface through the isolated native
    // window. Bridge reads only observe the page; they never manufacture edits.
    auto *liveBridge = backend.editorBridge();
    const auto livePageText = [&](QString *result, int timeout = 10000) {
        int requestedToken = -1;
        bool answered = false;
        const QMetaObject::Connection reply = QObject::connect(liveBridge, &EditorBridge::textReceived,
            window, [&](int token, const QString &text) {
                if (token == requestedToken) { *result = text; answered = true; }
            });
        requestedToken = liveBridge->fetchText();
        const bool received = waitUntil([&] { return answered; }, timeout);
        QObject::disconnect(reply);
        return received;
    };
    const auto focusLiveForInput = [&] {
        auto *livePane = window->findChild<QQuickItem *>("liveEditorPane");
        auto *liveView = window->findChild<QQuickItem *>("liveEditorView");
        if (!check(livePane && liveView, "Live input: shipped WebEngine pane is missing")) return false;
        if (!check(waitUntil([&] { return liveBridge->isReady() && !liveView->property("loading").toBool(); }, 30000),
                   "Live input: editor page did not finish loading")) return false;
        if (!check(QMetaObject::invokeMethod(livePane, "focusLive"), "Live input: cannot focus WebEngine pane")) return false;
        if (!check(waitUntil([&] { return livePane->property("hasLiveFocus").toBool(); }, 5000),
                   "Live input: WebEngine pane did not receive keyboard focus")) return false;
        return check(waitUntil([&] {
            return publishingDomResult(liveView, "document.activeElement?.classList.contains('cm-content')") == QVariant(true);
        }, 5000), "Live input: CodeMirror content did not receive DOM focus");
    };
    const auto livePageCaretAt = [&](int position) {
        QElapsedTimer timer;
        timer.start();
        do {
            int requestedToken = -1, anchor = -1, head = -1;
            bool answered = false;
            const QMetaObject::Connection reply = QObject::connect(liveBridge, &EditorBridge::selectionReceived,
                window, [&](int token, int pageAnchor, int pageHead) {
                    if (token == requestedToken) {
                        anchor = pageAnchor;
                        head = pageHead;
                        answered = true;
                    }
                });
            requestedToken = liveBridge->fetchSelection();
            const bool received = waitUntil([&] { return answered; }, 1000);
            QObject::disconnect(reply);
            if (!received) liveBridge->cancelSelectionRequest(requestedToken);
            if (received && anchor == position && head == position) return true;
            settle(40);
        } while (timer.elapsed() < 5000);
        return false;
    };
    acceptance("live-window-typing-source-switch-save-and-reopen", [&] {
        const QString original = QString::fromUtf8("# Live writing\n\nCaf\xC3\xA9 \xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x92\xBB stays. **Bold** remains.\n");
        if (!loadSample("Live window typing", original)) return;
        const QString path = temporary.filePath("Live window typing.md");
        action("liveEditToggle", 0, true);
        if (!focusLiveForInput()) return;
        QString page;
        if (!check(livePageText(&page) && page == original, "Live input: initial page text differs from disposable file")) return;
        const int insertion = original.indexOf(QStringLiteral(" stays.")) + 6;
        if (!check(insertion >= 6, "Live input: fixture insertion point missing")) return;
        liveBridge->placeCursor(insertion);
        if (!check(livePageCaretAt(insertion), "Live input: page caret did not reach insertion point")) return;
        for (const char ch : QByteArray(" well")) {
            const QChar letter = QLatin1Char(ch);
            key(window, ch == ' ' ? Qt::Key_Space : int(Qt::Key_A + ch - 'a'), QString(letter));
        }
        QString edited = original;
        edited.insert(insertion, QStringLiteral(" well"));
        if (!check(waitUntil([&] { return editor->property("text").toString() == edited && backend.modified(); }, 10000),
                   "Live input: native window keystrokes did not reach the canonical Markdown")) return;
        check(livePageText(&page) && page == edited, "Live input: page and canonical Markdown differ after typing");
        action("sourceModeButton", 0, false);
        check(editor->property("text").toString() == edited,
              "Live input: Source switch changed the typed Markdown");
        action("liveEditToggle", 0, true);
        check(livePageText(&page) && page == edited,
              "Live input: returning to Live changed the typed Markdown");
        backend.save();
        if (!check(waitUntil([&] { return !backend.modified(); }, 5000), "Live input: Save did not complete")) return;
        QFile savedFile(path);
        check(savedFile.open(QIODevice::ReadOnly) && savedFile.readAll() == edited.toUtf8(),
              "Live input: saved UTF-8 bytes differ from the page text");
        action("sourceModeButton", 0, false);
        if (!check(backend.open(QUrl::fromLocalFile(path)), "Live input: cannot reopen the saved disposable file")) return;
        check(waitUntil([&] { return editor->property("text").toString() == edited; }) && !backend.modified(),
              "Live input: reopen lost the typed Markdown or marked it dirty");
        action("liveEditToggle", 0, true);
        check(livePageText(&page) && page == edited,
              "Live input: page differs after disk reopen");
    });

    acceptance("live-window-keyboard-undo-and-redo", [&] {
        const QString original = QStringLiteral("# Live history\n\nA quiet line.\n");
        if (!loadSample("Live window history", original)) return;
        action("liveEditToggle", 0, true);
        if (!focusLiveForInput()) return;
        const int insertion = original.indexOf(QStringLiteral("line.")) + 4;
        liveBridge->placeCursor(insertion);
        if (!check(livePageCaretAt(insertion), "Live input: page caret did not reach history insertion point")) return;
        key(window, Qt::Key_X, QStringLiteral("x"));
        QString edited = original;
        edited.insert(insertion, QStringLiteral("x"));
        if (!check(waitUntil([&] { return editor->property("text").toString() == edited; }, 10000),
                   "Live input: typed character did not enter page history")) return;
        QMetaObject::invokeMethod(window, "undoEditing");
        check(waitUntil([&] { return editor->property("text").toString() == original; }, 10000),
              "Live input: native Undo did not restore the original Markdown");
        QMetaObject::invokeMethod(window, "redoEditing");
        check(waitUntil([&] { return editor->property("text").toString() == edited; }, 10000),
              "Live input: native Redo did not restore the typed Markdown");
        QString page;
        check(livePageText(&page) && page == edited, "Live input: page and C++ mirror diverged after Redo");
        action("sourceModeButton", 0, false);
        check(editor->property("text").toString() == edited, "Live input: Source switch lost the Redo result");
    });
