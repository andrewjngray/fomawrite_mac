// Cycle155 review fixes (UI): Source formatting marks are an overlay, the Format Bar works without
// keyboard focus in the editor, the Style label is whole, the bar fades with the header, F6 reaches
// the bar, the narrow header's Formatting button has its own glyph, and Live's page fills dark.

namespace {
void cycle155CollectMarks(QQuickItem *root, QList<QQuickItem *> &found) {
    for (QQuickItem *child : root->childItems()) {
        if (child->objectName() == QLatin1String("sourceFormattingMark") && child->isVisible()) found << child;
        cycle155CollectMarks(child, found);
    }
}
QList<QQuickItem *> cycle155Marks(Cycle155Window &w) {
    QList<QQuickItem *> found;
    cycle155CollectMarks(w.quick->contentItem(), found);
    return found;
}
QStringList cycle155MarkTexts(const QList<QQuickItem *> &marks) {
    QStringList texts;
    for (auto *mark : marks) texts << mark->property("text").toString();
    texts.sort();
    return texts;
}
}

// Red before: Qt Quick's TextEdit ignores QTextOption's show-spaces flags, so nothing was drawn.
void FomawriteTest::sourceFormattingMarksAreAnOverlayOfDotsTabsAndParagraphMarks() {
    Cycle155Window w;
    const QString text = QStringLiteral("a b\tc\n\nlast line  \n");
    QVERIFY2(w.create(text, false, true), qPrintable(w.error));
    auto *scroll = w.item("editorScroll"); QVERIFY(scroll);
    QCOMPARE(cycle155Marks(w).size(), 0);
    const QRect area = cycle103SceneBounds(scroll).toAlignedRect();
    QTest::qWait(200);
    const QImage plain = w.quick->grabWindow().copy(area);
    auto *quickDocument = qvariant_cast<QQuickTextDocument *>(w.editor->property("textDocument")); QVERIFY(quickDocument);
    const int revision = quickDocument->textDocument()->revision();
    const bool canUndo = w.editor->property("canUndo").toBool();
    w.backend.discardRecovery();
    const bool modified = w.backend.modified();
    QVERIFY(w.click("formatMarks"));
    QTRY_VERIFY(w.backend.showFormattingMarks());
    // Four spaces, one tab, and a paragraph mark after each of the four lines (the empty ones too).
    QTRY_COMPARE_WITH_TIMEOUT(cycle155Marks(w).size(), 9, 5000);
    QCOMPARE(cycle155MarkTexts(cycle155Marks(w)),
             (QStringList{QString::fromUtf8("¶"), QString::fromUtf8("¶"), QString::fromUtf8("¶"), QString::fromUtf8("¶"),
                          QString::fromUtf8("·"), QString::fromUtf8("·"), QString::fromUtf8("·"), QString::fromUtf8("·"),
                          QString::fromUtf8("→")}));
    // The first space sits on the text line: inside the editor, to the right of the "a".
    const QRectF editorBounds = cycle103SceneBounds(w.editor);
    for (auto *mark : cycle155Marks(w)) QVERIFY(editorBounds.adjusted(-1, -1, 1, 1).intersects(cycle103SceneBounds(mark)));
    // They are pixels in the text area (the old text-option flags drew nothing).
    QTest::qWait(250);
    QVERIFY2(w.quick->grabWindow().copy(area) != plain, "the text area shows the marks");
    // Presentation only: text, revision, modified flag and undo stack are untouched.
    QCOMPARE(w.canonical(), text);
    QCOMPARE(quickDocument->textDocument()->revision(), revision);
    QCOMPARE(w.backend.modified(), modified);
    QCOMPARE(w.editor->property("canUndo").toBool(), canUndo);
    // The marks are not selectable text and take no input.
    for (auto *mark : cycle155Marks(w)) QVERIFY(!mark->acceptedMouseButtons());
    QVERIFY(w.click("formatMarks"));
    QTRY_VERIFY(!w.backend.showFormattingMarks());
    QTRY_COMPARE_WITH_TIMEOUT(cycle155Marks(w).size(), 0, 5000);
    // Editing refreshes them.
    QVERIFY(w.click("formatMarks"));
    QTRY_COMPARE_WITH_TIMEOUT(cycle155Marks(w).size(), 9, 5000);
    w.editor->forceActiveFocus();
    QMetaObject::invokeMethod(w.editor, "insert", Q_ARG(int, 0), Q_ARG(QString, QStringLiteral("x y ")));
    QTRY_COMPARE_WITH_TIMEOUT(cycle155Marks(w).size(), 11, 5000);
    w.backend.discardRecovery();
}

void FomawriteTest::sourceFormattingMarksCoverOnlyTheVisibleRangeOfALongDocument() {
    Cycle155Window w;
    QString text;
    for (int i = 0; i < 6000; ++i) text += QStringLiteral("some words on line %1\n").arg(i);
    QVERIFY2(w.create(text, false, true), qPrintable(w.error));
    auto *scroll = w.item("editorScroll"); QVERIFY(scroll);
    QVERIFY(w.click("formatMarks"));
    QTRY_VERIFY_WITH_TIMEOUT(!cycle155Marks(w).isEmpty(), 8000);
    const auto inViewport = [&] {
        const QRectF view = cycle103SceneBounds(scroll).adjusted(0, -60, 0, 60);
        const auto marks = cycle155Marks(w);
        if (marks.size() > 1500) return false;
        for (auto *mark : marks) if (!view.intersects(cycle103SceneBounds(mark))) return false;
        return true;
    };
    QVERIFY(inViewport());
    const qreal before = cycle155Marks(w).first()->y();
    scroll->setProperty("contentY", 40000.0);
    QTRY_VERIFY_WITH_TIMEOUT(cycle155Marks(w).first()->y() != before && inViewport(), 8000);
    w.backend.discardRecovery();
}

// Red before: the bar was enabled only while the editor itself had keyboard focus.
void FomawriteTest::formatBarWorksWhileFocusIsOutsideTheEditorInSourceAndLive() {
    for (const bool live : {false, true}) {
        Cycle155Window w;
        QVERIFY2(w.create(QStringLiteral("alpha beta gamma\n"), live, true), qPrintable(w.error));
        QVERIFY(live ? w.selectInPage(6, 10) : w.selectInSource(6, 10));
        // Opening a document from the Library leaves focus on its row (here: another chrome control).
        w.item("topChromeLibraryButton")->forceActiveFocus();
        QTRY_VERIFY(!w.window->property(live ? "canFormatLive" : "canFormatSource").toBool());
        auto *bold = w.item("formatBold"); QVERIFY(bold);
        QVERIFY2(bold->isEnabled(), live ? "live" : "source");
        QVERIFY(w.item("formatStyleButton")->isEnabled());
        QVERIFY(w.click("formatBold"));
        QTRY_COMPARE_WITH_TIMEOUT(w.canonical(), QStringLiteral("alpha **beta** gamma\n"), 8000);
        // The writer is typing in the surface again.
        QTRY_VERIFY_WITH_TIMEOUT(live ? w.pageHasFocus() : w.editor->hasActiveFocus(), 5000);
        // Once more from outside, with a block command.
        w.item("topChromeLibraryButton")->forceActiveFocus();
        QTRY_VERIFY(!w.window->property(live ? "canFormatLive" : "canFormatSource").toBool());
        QVERIFY(w.click("formatBullet"));
        QTRY_COMPARE_WITH_TIMEOUT(w.canonical(), QStringLiteral("- alpha **beta** gamma\n"), 8000);
        w.backend.discardRecovery();
    }
}

// Red before: Material's 24 px side padding clipped "Code Bl…".
void FomawriteTest::formatBarStyleButtonShowsItsWholeLabel() {
    Cycle155Window w;
    const QString text = QStringLiteral("```\ncode\n```\n\n###### six\n\nbody\n");
    QVERIFY2(w.create(text, false, true), qPrintable(w.error));
    auto *button = w.item("formatStyleButton"); QVERIFY(button);
    const auto labelItem = [&]() -> QQuickItem * {
        auto *content = qvariant_cast<QQuickItem *>(button->property("contentItem"));
        if (!content) return nullptr;
        for (auto *child : content->childItems()) if (child->property("elide").isValid()) return child;
        return nullptr;
    };
    for (const auto &[word, label] : {std::pair{QStringLiteral("code"), QStringLiteral("Code Block")},
                                      std::pair{QStringLiteral("six"), QStringLiteral("Heading 6")},
                                      std::pair{QStringLiteral("body"), QStringLiteral("Body")}}) {
        const int at = w.canonical().indexOf(word);
        QVERIFY(w.selectInSource(at, at));
        QTRY_COMPARE_WITH_TIMEOUT(button->property("text").toString(), label, 3000);
        auto *text = labelItem(); QVERIFY(text);
        QVERIFY2(!text->property("truncated").toBool(), qPrintable(label));
        QVERIFY2(text->width() + 0.5 >= text->implicitWidth(), qPrintable(label));
        QVERIFY(text->width() > 40);
    }
    // The button is as wide as the longest label plus the chevron, inside the bar.
    QVERIFY(button->width() >= 100);
    w.backend.discardRecovery();
}

// Red before: the header faded while typing but the bar stayed under an empty strip.
void FomawriteTest::formatBarFadesWithTheHeaderWithoutReflow() {
    Cycle103SettingsSnapshot isolatedSettings;
    { QSettings settings; settings.remove("workspace/autoHideChrome"); settings.sync(); }
    Cycle155Window w;
    QVERIFY2(w.create(QStringLiteral("some text\n"), false, true), qPrintable(w.error));
    auto *bar = w.item("formatBar"); QVERIFY(bar);
    auto *trailing = w.item("topChromeToolbarTrailing"); QVERIFY(trailing);
    w.window->findChild<QObject *>("workspaceSettings")->setProperty("autoHideChrome", true);
    QVERIFY(w.selectInSource(2, 2));
    QTest::mouseMove(w.quick, cycle103SceneBounds(w.item("editorScroll")).center().toPoint());
    QTRY_COMPARE(bar->opacity(), 1.0);
    const QRectF barBounds = cycle103SceneBounds(bar);
    const qreal editorY = cycle103SceneBounds(w.editor).top();
    QVERIFY(QMetaObject::invokeMethod(w.window.data(), "writingActivity"));
    QTRY_COMPARE_WITH_TIMEOUT(trailing->opacity(), 0.0, 3000);
    QCOMPARE(bar->opacity(), 0.0);
    QVERIFY(!bar->isEnabled());
    QCOMPARE(cycle103SceneBounds(bar), barBounds);
    QCOMPARE(cycle103SceneBounds(w.editor).top(), editorY);
    // The same reveal triggers bring it back.
    QVERIFY(QMetaObject::invokeMethod(w.window.data(), "revealDocumentChrome"));
    QTRY_COMPARE(bar->opacity(), 1.0);
    QCOMPARE(trailing->opacity(), 1.0);
    QVERIFY(bar->isEnabled());
    w.backend.discardRecovery();
}

// Red before: F6 skipped formatting whenever the bar was shown.
void FomawriteTest::f6FromSourceLandsOnTheFormatBarStyleButton() {
    Cycle103SettingsSnapshot isolatedSettings;
    { QSettings settings; settings.remove("workspace/autoHideChrome"); settings.sync(); }
    Cycle155Window w;
    QVERIFY2(w.create(QStringLiteral("alpha beta\n"), false, true), qPrintable(w.error));
    w.window->findChild<QObject *>("workspaceSettings")->setProperty("autoHideChrome", true);
    QVERIFY(w.selectInSource(2, 4));
    auto *header = w.item("topChrome"); QVERIFY(header);
    QTest::mouseMove(w.quick, cycle103SceneBounds(w.item("editorScroll")).center().toPoint());
    QVERIFY(QMetaObject::invokeMethod(header, "focusWorkspaceControl"));
    QTRY_VERIFY(w.quick->activeFocusItem() && w.quick->activeFocusItem()->objectName() == QLatin1String("formatStyleButton"));
    // It stays revealed while focused (the bar and header do not fade under the keyboard).
    QVERIFY(QMetaObject::invokeMethod(w.window.data(), "writingActivity"));
    QCOMPARE(w.item("formatBar")->opacity(), 1.0);
    w.backend.discardRecovery();
}

// Red before: both carried the pilcrow glyph, but one opens the Formatting menu.
void FomawriteTest::narrowHeaderFormattingButtonHasItsOwnGlyph() {
    Cycle155Window w;
    QVERIFY2(w.create(QStringLiteral("text\n"), false, false, 700), qPrintable(w.error));
    auto *layout = w.window->findChild<QObject *>("workspaceLayout"); QVERIFY(layout);
    layout->setProperty("organizerVisible", false); layout->setProperty("filesVisible", false);
    w.quick->resize(500, 800);
    auto *format = w.item("compactFormatButton"); QVERIFY(format);
    QTRY_VERIFY(format->isVisible());
    QVERIFY(!w.item("formatBar")->isVisible());
    QVERIFY(format->property("iconName").toString() != QLatin1String("paragraph"));
    QCOMPARE(format->property("iconName").toString(), QStringLiteral("format"));
    w.backend.discardRecovery();
}

// Red before: with a theme that paints only <html>, dark Live showed a white band below short text.
void FomawriteTest::darkLivePageFillsBelowAShortDocument() {
    Cycle155Window w;
    QVERIFY2(w.create(QStringLiteral("short\n"), true, true), qPrintable(w.error));
    auto *bridge = w.backend.editorBridge();
    bridge->applyTheme(QStringLiteral("html { background: white; color: #222; }"));
    bridge->applyAppearance(QStringLiteral("{\"appearance\":\"manuscript\",\"dark\":true,\"liveThemeFilter\":true,\"palette\":{\"background\":\"#1e1f22\",\"text\":\"#e6edf3\"}}"));
    const QString probe = QStringLiteral(
        "(function(){ var b = document.body.getBoundingClientRect(); var c = getComputedStyle(document.body).backgroundColor;"
        " return JSON.stringify({h: b.height, vh: window.innerHeight, bg: c, dark: document.documentElement.classList.contains('dark')}); })()");
    QTRY_VERIFY_WITH_TIMEOUT(QJsonDocument::fromJson(cycle143PageResult(w.liveView(), probe).toString().toUtf8()).object().value("dark").toBool(), 8000);
    const auto state = QJsonDocument::fromJson(cycle143PageResult(w.liveView(), probe).toString().toUtf8()).object();
    QVERIFY2(state.value("h").toDouble() + 1 >= state.value("vh").toDouble(), qPrintable(QString::fromUtf8(QJsonDocument(state).toJson())));
    QVERIFY2(state.value("bg").toString() != QLatin1String("rgba(0, 0, 0, 0)") && state.value("bg").toString() != QLatin1String("rgb(255, 255, 255)"),
             qPrintable(state.value("bg").toString()));
    w.backend.discardRecovery();
}
