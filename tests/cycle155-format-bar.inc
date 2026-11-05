// Cycle155 — the top toolbar's formatting buttons work in Live (real clicks), the Word-like
// Format Bar, and Show Formatting Marks.

namespace {
struct Cycle155Window {
    Cycle103SettingsSnapshot settings;
    Backend backend;
    QQmlEngine engine;
    QScopedPointer<QObject> window;
    QQuickWindow *quick = nullptr;
    QQuickItem *editor = nullptr;
    QString error;
    // formatBar false: the header's own cluster (shown only while the Format Bar is hidden).
    bool create(const QString &text, bool live, bool formatBar, int width = 2000) {
        QSettings stored; stored.setValue("workspace/formatBarVisible", formatBar); stored.sync();
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
        window.reset(component.create());
        if (!window) { error = component.errorString(); return false; }
        quick = qobject_cast<QQuickWindow *>(window.data());
        editor = window->findChild<QQuickItem *>("sourceEditor");
        if (!quick || !editor) { error = QStringLiteral("no window"); return false; }
        cycle106KeepNativeFocus(quick);
        quick->resize(width, 800); quick->show();
        if (!QTest::qWaitForWindowExposed(quick)) { error = QStringLiteral("not exposed"); return false; }
        if (QGuiApplication::platformName() != "cocoa") { quick->requestActivate(); QTest::qWaitFor([&] { return quick->isActive(); }, 3000); }
        editor->setProperty("text", text);
        if (live) {
            QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live")));
            if (!QTest::qWaitFor([&] { return backend.editorBridge()->isReady(); }, 30000)) { error = QStringLiteral("page not ready"); return false; }
            auto *view = window->findChild<QObject *>("liveEditorView");
            if (!view) { error = QStringLiteral("no view"); return false; }
            if (!QTest::qWaitFor([&] { return !view->property("loading").toBool(); }, 20000)) { error = QStringLiteral("page loading"); return false; }
            QSignalSpy received(backend.editorBridge(), &EditorBridge::textReceived);
            const bool has = QTest::qWaitFor([&] {
                const int token = backend.editorBridge()->fetchText(); QTest::qWait(60);
                for (const auto &args : received) if (args.at(0).toInt() == token && args.at(1).toString() == text) return true;
                return false;
            }, 45000);
            if (!has) { error = QStringLiteral("page text"); return false; }
        }
        return true;
    }
    QObject *liveView() { return window->findChild<QObject *>("liveEditorView"); }
    QString pageText() {
        auto *bridge = backend.editorBridge();
        QSignalSpy received(bridge, &EditorBridge::textReceived);
        const int token = bridge->fetchText();
        QElapsedTimer timer; timer.start();
        while (timer.elapsed() < 3000) {
            QTest::qWait(10);
            for (const auto &args : received) if (args.at(0).toInt() == token) return args.at(1).toString();
        }
        return QStringLiteral("<timeout>");
    }
    QString canonical() { return editor->property("text").toString(); }
    bool selectInPage(int from, int to) {
        auto *pane = window->findChild<QObject *>("liveEditorPane");
        QMetaObject::invokeMethod(pane, "focusLive");
        if (!QTest::qWaitFor([&] { return pane->property("hasLiveFocus").toBool(); }, 5000)) return false;
        QMetaObject::invokeMethod(liveView(), "runJavaScript", Q_ARG(QString, QStringLiteral(
            "window.fomawriteEditor.view.dispatch({selection:{anchor:%1,head:%2}})").arg(from).arg(to)));
        return QTest::qWaitFor([&] { return backend.editorBridge()->lastSelectionStart() == from && backend.editorBridge()->lastSelectionEnd() == to; }, 5000);
    }
    bool selectInSource(int from, int to) {
        editor->forceActiveFocus();
        return QMetaObject::invokeMethod(editor, "select", Q_ARG(int, from), Q_ARG(int, to))
            && QTest::qWaitFor([&] { return window->property("canFormatSource").toBool(); }, 3000);
    }
    QQuickItem *item(const char *name) { return window->findChild<QQuickItem *>(name); }
    bool click(const char *name) {
        auto *button = item(name);
        return button && button->isVisible() && cycle102Click(quick, button);
    }
    bool pageHasFocus() { return window->findChild<QObject *>("liveEditorPane")->property("hasLiveFocus").toBool(); }
};
}

// Red before the fix: a toolbar button takes keyboard focus when pressed, so the page had
// already lost focus when the click was handled and canFormatLive was false.
void FomawriteTest::liveToolbarButtonsFormatBySelectionWithRealClicks() {
    Cycle155Window w;
    const QString text = QStringLiteral("alpha beta gamma\n");
    QVERIFY2(w.create(text, true, false), qPrintable(w.error));
    auto *bold = w.item("compactBoldButton"); QVERIFY(bold);
    QVERIFY(w.selectInPage(6, 10));
    QTRY_VERIFY(bold->isVisible() && bold->isEnabled());
    QVERIFY(w.click("compactBoldButton"));
    QTRY_COMPARE_WITH_TIMEOUT(w.canonical(), QStringLiteral("alpha **beta** gamma\n"), 8000);
    // The writer is typing again: focus went back to the page, which still owns formatting.
    QTRY_VERIFY_WITH_TIMEOUT(w.pageHasFocus(), 5000);
    QTRY_VERIFY(w.window->property("canFormatLive").toBool());
    const int gamma = w.canonical().indexOf(QStringLiteral("gamma"));
    QVERIFY(w.selectInPage(gamma, gamma + 5));
    QVERIFY(w.click("compactItalicButton"));
    QTRY_COMPARE_WITH_TIMEOUT(w.canonical(), QStringLiteral("alpha **beta** *gamma*\n"), 8000);
    QTRY_COMPARE_WITH_TIMEOUT(w.pageText(), w.canonical(), 8000);
    // The header link opens the Live page's own link panel, not Source's dialog.
    QTRY_VERIFY_WITH_TIMEOUT(w.pageHasFocus(), 5000);
    QVERIFY(w.selectInPage(0, 5));
    QVERIFY(w.click("compactLinkButton"));
    QTRY_VERIFY_WITH_TIMEOUT(cycle143PageResult(w.liveView(), QStringLiteral("!!document.querySelector('.fw-link-panel')")).toBool(), 8000);
    QVERIFY(!w.window->findChild<QObject *>("linkEditor") || !w.window->findChild<QObject *>("linkEditor")->property("visible").toBool());
    // Focus anywhere else ends ownership: a stale Live selection is never formatted.
    w.item("topChromeLibraryButton")->forceActiveFocus();
    QTRY_VERIFY(!w.window->property("canFormatLive").toBool());
    w.backend.discardRecovery();
}

void FomawriteTest::sourceToolbarButtonsStillFormatWithRealClicks() {
    Cycle155Window w;
    const QString text = QStringLiteral("alpha beta gamma\n");
    QVERIFY2(w.create(text, false, false), qPrintable(w.error));
    QVERIFY(w.selectInSource(6, 10));
    QVERIFY(w.click("compactBoldButton"));
    QCOMPARE(w.canonical(), QStringLiteral("alpha **beta** gamma\n"));
    QVERIFY(w.editor->hasActiveFocus());
    QVERIFY(w.selectInSource(0, 5));
    QVERIFY(w.click("compactItalicButton"));
    QCOMPARE(w.canonical(), QStringLiteral("*alpha* **beta** gamma\n"));
    // The header's pilcrow toggles Show Formatting Marks.
    QVERIFY(!w.backend.showFormattingMarks());
    QVERIFY(w.click("compactParagraphButton"));
    QTRY_VERIFY(w.backend.showFormattingMarks());
    QVERIFY(w.click("compactParagraphButton"));
    QTRY_VERIFY(!w.backend.showFormattingMarks());
    w.backend.discardRecovery();
}

void FomawriteTest::formatBarTakesTheHeaderControlsPlaceAndFollowsTheSetting() {
    Cycle155Window w;
    QVERIFY2(w.create(QStringLiteral("# Title\n\nbody\n"), false, true), qPrintable(w.error));
    auto *bar = w.item("formatBar"); QVERIFY(bar);
    QTRY_VERIFY(bar->isVisible());
    QCOMPARE(bar->height(), 40.0);
    auto *header = w.item("documentHeader"); QVERIFY(header);
    // Directly under the document header, spanning the editing pane.
    QCOMPARE(cycle103SceneBounds(bar).top(), cycle103SceneBounds(header).bottom());
    QCOMPARE(cycle103SceneBounds(bar).left(), cycle103SceneBounds(w.item("editorPane")).left());
    QCOMPARE(cycle103SceneBounds(bar).width(), cycle103SceneBounds(w.item("editorPane")).width());
    // No duplicate controls while it shows.
    QVERIFY(!w.item("compactWritingControls")->isVisible());
    QVERIFY(!w.item("compactFormatButton")->isVisible());
    for (const char *name : {"formatStyleButton", "formatBold", "formatItalic", "formatStrike", "formatCode", "formatHighlight", "formatLink",
                             "formatBullet", "formatOrdered", "formatTask", "formatOutdent", "formatIndent", "formatClear", "formatMarks"})
        QVERIFY2(w.item(name) && w.item(name)->isVisible(), name);
    // Preview Only: no editing pane, no bar.
    QVERIFY(QMetaObject::invokeMethod(w.window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("preview"))));
    QTRY_VERIFY(!bar->isVisible());
    QVERIFY(QMetaObject::invokeMethod(w.window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("source"))));
    QTRY_VERIFY(bar->isVisible());
    // The command (menu, palette) hides it; the header's cluster returns.
    auto *commands = w.window->findChild<QObject *>("workspaceCommands"); QVERIFY(commands);
    QCOMPARE(commands->property("entries").toList().size() > 0, true);
    QVariant label; QVERIFY(QMetaObject::invokeMethod(commands, "label", Q_RETURN_ARG(QVariant, label), Q_ARG(QVariant, QStringLiteral("formatBar"))));
    QCOMPARE(label.toString(), QStringLiteral("Hide Format Bar"));
    QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("formatBar"))));
    QTRY_VERIFY(!bar->isVisible());
    QTRY_VERIFY(w.item("compactWritingControls")->isVisible());
    QTRY_VERIFY(QSettings().value("workspace/formatBarVisible").toString() == QStringLiteral("false"));
    // The text starts at the pane's top again (nothing is left behind).
    QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("formatBar"))));
    QTRY_VERIFY(bar->isVisible());
    QTRY_VERIFY(!w.item("compactWritingControls")->isVisible());
    w.backend.discardRecovery();
}

void FomawriteTest::formatBarButtonsWriteMarkdownInSourceAndLiveWithRealClicks() {
    const QString text = QStringLiteral("one\n\ntwo\n\nthree\n\nfour\n\nfive\n\nsix\n");
    for (const bool live : {false, true}) {
        Cycle155Window w;
        QVERIFY2(w.create(text, live, true), qPrintable(w.error));
        const auto select = [&](const QString &word) {
            const int start = w.canonical().indexOf(word);
            return start >= 0 && (live ? w.selectInPage(start, start + word.size()) : w.selectInSource(start, start + word.size()));
        };
        const auto press = [&](const char *name, const QString &expected) {
            if (!w.click(name)) return false;
            return QTest::qWaitFor([&] { return w.canonical() == expected; }, 8000);
        };
        QString expected = text;
        const auto wrap = [&](const QString &word, const char *button, const QString &mark) {
            if (!select(word)) return false;
            expected.replace(word, mark + word + mark);
            return press(button, expected);
        };
        QVERIFY2(wrap("one", "formatBold", "**"), live ? "live bold" : "source bold");
        QVERIFY2(wrap("two", "formatItalic", "*"), "italic");
        QVERIFY2(wrap("three", "formatStrike", "~~"), "strike");
        QVERIFY2(wrap("four", "formatCode", "`"), "code");
        QVERIFY2(wrap("five", "formatHighlight", "=="), "highlight");
        QVERIFY(select("six"));
        expected.replace("six", "- six");
        QVERIFY2(press("formatBullet", expected), "bullet");
        expected.replace("- six", "1. six");
        QVERIFY2(press("formatOrdered", expected), "ordered");
        expected.replace("1. six", "- [ ] six");
        QVERIFY2(press("formatTask", expected), "task");
        expected.replace("- [ ] six", "    - [ ] six");
        QVERIFY2(press("formatIndent", expected), "indent");
        expected.replace("    - [ ] six", "- [ ] six");
        QVERIFY2(press("formatOutdent", expected), "outdent");
        // Clear formatting removes the emphasis around a selected word.
        const int bolded = w.canonical().indexOf("**one**");
        QVERIFY(live ? w.selectInPage(bolded, bolded + 7) : w.selectInSource(bolded, bolded + 7));
        expected.replace("**one**", "one");
        QVERIFY2(press("formatClear", expected), "clear");
        // The style menu: the current block's name, then Heading 2 and Quote.
        const int pos = w.canonical().indexOf("one");
        QVERIFY(live ? w.selectInPage(pos, pos) : w.selectInSource(pos, pos));
        auto *styleButton = w.item("formatStyleButton"); QVERIFY(styleButton);
        QTRY_COMPARE_WITH_TIMEOUT(styleButton->property("text").toString(), QStringLiteral("Body"), 3000);
        QVERIFY(w.click("formatStyleButton"));
        auto *menu = w.window->findChild<QObject *>("formatStyleMenu"); QVERIFY(menu);
        QTRY_VERIFY(menu->property("opened").toBool());
        QTRY_VERIFY(w.window->property("canFormatLive").toBool() || w.window->property("canFormatSource").toBool());
        auto *heading = cycle108MenuItem(menu, "Heading 2"); QVERIFY(heading);
        QVERIFY(cycle102Click(w.quick, heading));
        expected.replace("one\n", "## one\n");
        QTRY_COMPARE_WITH_TIMEOUT(w.canonical(), expected, 8000);
        QTRY_COMPARE_WITH_TIMEOUT(styleButton->property("text").toString(), QStringLiteral("Heading 2"), 3000);
        // After the menu closed, the writer is typing in the surface again.
        QTRY_VERIFY_WITH_TIMEOUT(live ? w.pageHasFocus() : w.editor->hasActiveFocus(), 5000);
        QVERIFY(live ? w.window->property("canFormatLive").toBool() : w.window->property("canFormatSource").toBool());
        if (live) QTRY_COMPARE_WITH_TIMEOUT(w.pageText(), w.canonical(), 8000);
        w.backend.discardRecovery();
    }
}

void FomawriteTest::formatBarFoldsIntoTheOverflowMenuWithoutOverlap() {
    Cycle155Window w;
    QVERIFY2(w.create(QStringLiteral("text\n"), false, true), qPrintable(w.error));
    auto *layout = w.window->findChild<QObject *>("workspaceLayout"); QVERIFY(layout);
    layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
    QVERIFY(QMetaObject::invokeMethod(w.window.data(), "setDocumentView", Q_ARG(QVariant, 0)));
    auto *bar = w.item("formatBar"); QVERIFY(bar);
    const QStringList names{"formatStyleButton", "formatBold", "formatItalic", "formatStrike", "formatCode", "formatHighlight", "formatLink",
                            "formatBullet", "formatOrdered", "formatTask", "formatOutdent", "formatIndent", "formatClear", "formatMarks",
                            "formatOverflowButton"};
    bool sawOverflow = false, sawNone = false;
    for (const int width : {2000, 900, 720, 640, 560, 480, 400, 330}) {
        w.quick->resize(width, 800);
        QTRY_VERIFY(qAbs(cycle103SceneBounds(w.item("editorPane")).width() - width) < 2);
        QTest::qWait(60);
        QList<QPair<QString, QRectF>> shown;
        const QRectF barBounds = cycle103SceneBounds(bar);
        for (const auto &name : names) {
            auto *button = w.item(name.toUtf8());
            if (!button || !button->isVisible()) continue;
            const QRectF bounds = cycle103SceneBounds(button);
            QVERIFY2(barBounds.adjusted(-0.1, -0.1, 0.1, 0.1).contains(bounds), qPrintable(QStringLiteral("%1 outside the bar at %2").arg(name).arg(width)));
            for (const auto &[other, rect] : shown)
                QVERIFY2(!(bounds.intersected(rect).width() > 0.1 && bounds.intersected(rect).height() > 0.1),
                         qPrintable(QStringLiteral("%1 overlaps %2 at %3").arg(name, other).arg(width)));
            shown.append({name, bounds});
        }
        for (const char *stays : {"formatStyleButton", "formatBold", "formatItalic", "formatMarks"})
            QVERIFY2(w.item(stays)->isVisible(), qPrintable(QStringLiteral("%1 hidden at %2").arg(QLatin1String(stays)).arg(width)));
        const bool folded = w.item("formatOverflowButton")->isVisible();
        const bool allShown = w.item("formatIndent")->isVisible() && w.item("formatLink")->isVisible() && w.item("formatStrike")->isVisible();
        QCOMPARE(folded, !allShown);
        (folded ? sawOverflow : sawNone) = true;
        // The folded controls are in the overflow menu; the narrowest keeps only Style, B, I and the marks toggle.
        if (width == 330) {
            QVERIFY(!w.item("formatLink")->isVisible());
            QVERIFY(w.click("formatOverflowButton"));
            auto *menu = w.window->findChild<QObject *>("formatOverflowMenu"); QVERIFY(menu);
            QTRY_VERIFY(menu->property("opened").toBool());
            QVERIFY(cycle108MenuItem(menu, "Link…")->isVisible());
            QVERIFY(cycle108MenuItem(menu, "Increase Indent")->isVisible());
            QMetaObject::invokeMethod(menu, "close");
        }
    }
    QVERIFY(sawOverflow && sawNone);
    w.backend.discardRecovery();
}

void FomawriteTest::showFormattingMarksChangesOnlyThePresentationInSource() {
    Cycle155Window w;
    const QString text = QStringLiteral("a b\tc\n\nlast line  \n");
    QVERIFY2(w.create(text, false, true), qPrintable(w.error));
    auto *quickDocument = qvariant_cast<QQuickTextDocument *>(w.editor->property("textDocument")); QVERIFY(quickDocument);
    QTextDocument *document = quickDocument->textDocument();
    w.backend.discardRecovery();
    const int revision = document->revision();
    const bool modified = w.backend.modified();
    const bool canUndo = w.editor->property("canUndo").toBool();
    const qreal tabStop = document->defaultTextOption().tabStopDistance();
    QCOMPARE(w.canonical(), text);
    const QRect textArea = cycle103SceneBounds(w.item("editorScroll")).toAlignedRect();
    const QImage plain = w.quick->grabWindow().copy(textArea);
    auto *marks = w.item("formatMarks"); QVERIFY(marks);
    QVERIFY(!marks->property("active").toBool());
    QVERIFY(w.click("formatMarks"));
    QTRY_VERIFY(w.backend.showFormattingMarks());
    QVERIFY(marks->property("active").toBool());
    QCOMPARE(document->defaultTextOption().tabStopDistance(), tabStop);
    QCOMPARE(w.canonical(), text);
    QCOMPARE(document->revision(), revision);
    QCOMPARE(w.backend.modified(), modified);
    QCOMPARE(w.editor->property("canUndo").toBool(), canUndo);
    QTRY_VERIFY(QSettings().value("workspace/showFormattingMarks").toString() == QStringLiteral("true"));
    // The editor shows them: the text area alone (not the Format Bar's pressed toggle) differs from the plain one.
    QTest::qWait(250);
    QVERIFY2(w.quick->grabWindow().copy(textArea) != plain, "Source renders spaces, tabs and paragraph ends as marks");
    // Off again: presentation restored, document untouched.
    QVERIFY(w.click("formatMarks"));
    QTRY_VERIFY(!w.backend.showFormattingMarks());
    QCOMPARE(document->defaultTextOption().tabStopDistance(), tabStop);
    QTest::qWait(250);
    QCOMPARE(w.quick->grabWindow().copy(textArea), plain);
    QCOMPARE(w.canonical(), text);
    QCOMPARE(document->revision(), revision);
    QVERIFY(!w.backend.modified() || modified);
    // View > Show Formatting Marks (Cmd+8) is the same command; the header's ¶ also toggles it.
    auto *commands = w.window->findChild<QObject *>("workspaceCommands"); QVERIFY(commands);
    QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("formattingMarks"))));
    QTRY_VERIFY(w.backend.showFormattingMarks());
    QVariant label; QVERIFY(QMetaObject::invokeMethod(commands, "label", Q_RETURN_ARG(QVariant, label), Q_ARG(QVariant, QStringLiteral("formattingMarks"))));
    QCOMPARE(label.toString(), QStringLiteral("Hide Formatting Marks"));
    QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("formattingMarks"))));
    QTRY_VERIFY(!w.backend.showFormattingMarks());
    w.backend.discardRecovery();
}

void FomawriteTest::showFormattingMarksReachesTheLivePageAndLeavesTheDocumentAlone() {
    Cycle155Window w;
    const QString text = QStringLiteral("a b\tc\n\nlast line\n");
    QVERIFY2(w.create(text, true, true), qPrintable(w.error));
    auto *bridge = w.backend.editorBridge();
    QSignalSpy appearance(bridge, &EditorBridge::setAppearance);
    QSignalSpy changes(bridge, &EditorBridge::changesReceived);
    const int revision = w.backend.documentRevision();
    const bool modified = w.backend.modified();
    const QString lastJson = [&] { return appearance.isEmpty() ? QString() : appearance.last().at(0).toString(); }();
    QVERIFY(!lastJson.contains(QStringLiteral("\"showMarks\":true")));
    QVERIFY(w.selectInPage(2, 2));
    QVERIFY(w.click("formatMarks"));
    QTRY_VERIFY(!appearance.isEmpty() && appearance.last().at(0).toString().contains(QStringLiteral("\"showMarks\":true")));
    QVERIFY(appearance.last().at(0).toString().contains(QStringLiteral("\"appearance\"")));
    QTRY_VERIFY_WITH_TIMEOUT(w.pageHasFocus(), 5000);
    QCOMPARE(w.canonical(), text);
    QTRY_COMPARE_WITH_TIMEOUT(w.pageText(), text, 8000);
    QCOMPARE(w.backend.documentRevision(), revision);
    QCOMPARE(w.backend.modified(), modified);
    QCOMPARE(changes.count(), 0);
    QVERIFY(w.click("formatMarks"));
    QTRY_VERIFY(appearance.last().at(0).toString().contains(QStringLiteral("\"showMarks\":false")));
    QCOMPARE(w.canonical(), text);
    QCOMPARE(w.backend.documentRevision(), revision);
    w.backend.discardRecovery();
}
