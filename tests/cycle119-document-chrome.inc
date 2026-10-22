namespace {
QMap<QString, QRectF> cycle119DocumentGeometry(const Cycle117Window &f) {
    QMap<QString, QRectF> result;
    for (const char *name : {"editorPane", "editorScroll", "documentHeader", "documentFooter"})
        if (auto *item = f.item(name)) result.insert(name, cycle103SceneBounds(item));
    return result;
}
QPoint cycle119TopRevealPoint(QQuickItem *header) {
    const QRectF rect = cycle103SceneBounds(header);
    return QPointF(rect.right() - 4, rect.top() + 4).toPoint();
}
void cycle119Wheel(QQuickWindow *window, const QPointF &position) {
    QWheelEvent event(position, window->mapToGlobal(position.toPoint()), QPoint(), QPoint(0, -120),
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(window, &event);
    QCoreApplication::processEvents();
}
}

void FomawriteTest::documentChromeHidesForUserInputAndRevealsEdgesWithoutReflow() {
    Cycle103SettingsSnapshot isolatedSettings;
    // Verify the shipped default independently of unrelated always-visible fixtures.
    { QSettings settings; settings.remove("workspace/autoHideChrome"); settings.sync(); }
    Cycle117Window f; QVERIFY2(f.start(1280), qPrintable(f.error));
    auto *settings = f.window->findChild<QObject *>("workspaceSettings");
    auto *header = f.item("topChrome"), *scroll = f.item("editorScroll");
    auto *source = f.item("sourceModeButton");
    QVERIFY(settings && header && scroll && source);
    QVERIFY(settings->property("autoHideChrome").toBool());
    QVERIFY(f.mode(0));
    QString text = "# Calm document bars\n\n";
    for (int i = 0; i < 70; ++i) text += QString("Paragraph %1 keeps enough source to scroll.\n\n").arg(i);
    f.editor->setProperty("text", text);
    QTRY_VERIFY(!f.window->property("changingDocumentView").toBool());
    QVERIFY(cycle118FooterRenderBarrier(f.quick));
    const auto geometry = cycle119DocumentGeometry(f);
    QCOMPARE(geometry.size(), 4);
    const QPoint writingPoint = cycle103SceneBounds(scroll).center().toPoint();
    QTest::mouseMove(f.quick, writingPoint);
    f.editor->forceActiveFocus(Qt::MouseFocusReason);
    f.editor->setProperty("cursorPosition", 0);
    cycle111KeyClick(f.quick, 'x');
    QTRY_COMPARE(f.editor->property("text").toString(), "x" + text);
    QTRY_VERIFY(f.window->property("documentChromeHidden").toBool());
    QTRY_VERIFY(!header->property("documentChromeVisible").toBool());
    QTRY_VERIFY(!f.footer->property("chromeVisible").toBool());
    QVERIFY(!source->isEnabled());
    QTRY_VERIFY(f.footer->opacity() < 0.01);
    QCOMPARE(cycle119DocumentGeometry(f), geometry);

    // Each edge reveals its own bar; the other bar stays quiet.
    QTest::mouseMove(f.quick, cycle119TopRevealPoint(header));
    QTRY_VERIFY(header->property("documentChromeVisible").toBool());
    QVERIFY(!f.footer->property("chromeVisible").toBool());
    QTest::mouseMove(f.quick, writingPoint);
    QTest::qWait(200);
    QVERIFY(header->property("documentChromeVisible").toBool());
    QVERIFY(!f.footer->property("chromeVisible").toBool());
    QCOMPARE(cycle119DocumentGeometry(f), geometry);
    f.editor->forceActiveFocus(Qt::MouseFocusReason);
    f.editor->setProperty("cursorPosition", 0);
    cycle111KeyClick(f.quick, 'y');
    QTRY_COMPARE(f.editor->property("text").toString(), "yx" + text);
    QTRY_VERIFY(!header->property("documentChromeVisible").toBool());
    QTest::mouseMove(f.quick, QPoint(f.quick->width() - 5, f.quick->height() - 4));
    QTRY_VERIFY(f.footer->property("chromeVisible").toBool());
    QVERIFY(!header->property("documentChromeVisible").toBool());
    QVERIFY(source->isEnabled());
    QTest::mouseMove(f.quick, writingPoint);
    QTest::qWait(200);
    QVERIFY(f.footer->property("chromeVisible").toBool());
    QVERIFY(!header->property("documentChromeVisible").toBool());
    QCOMPARE(cycle119DocumentGeometry(f), geometry);

    // Scrolling clears a bottom-edge reveal after the pointer has left it.
    f.editor->forceActiveFocus(Qt::MouseFocusReason);
    scroll->setProperty("contentY", 0);
    cycle119Wheel(f.quick, writingPoint);
    QTRY_VERIFY(f.window->property("documentChromeHidden").toBool());
    QTRY_VERIFY(scroll->property("contentY").toReal() > 0);
    QTRY_VERIFY(!f.footer->property("chromeVisible").toBool());
    QCOMPARE(cycle119DocumentGeometry(f), geometry);
    QVERIFY(!header->property("documentChromeVisible").toBool());
    QCOMPARE(f.editor->property("text").toString(), "yx" + text);
    // Both edges can be latched independently, and activity resets them on every cycle.
    for (int cycle = 0; cycle < 2; ++cycle) {
        QTest::mouseMove(f.quick, cycle119TopRevealPoint(header));
        QTRY_VERIFY(header->property("documentChromeVisible").toBool());
        QVERIFY(!f.footer->property("chromeVisible").toBool());
        QTest::mouseMove(f.quick, QPoint(f.quick->width() - 5, f.quick->height() - 4));
        QTRY_VERIFY(f.footer->property("chromeVisible").toBool());
        QTest::mouseMove(f.quick, writingPoint);
        QTest::qWait(200);
        QVERIFY(header->property("documentChromeVisible").toBool());
        QVERIFY(f.footer->property("chromeVisible").toBool());
        QVERIFY(QMetaObject::invokeMethod(f.window.data(), "writingActivity"));
        QTRY_VERIFY(!header->property("documentChromeVisible").toBool());
        QTRY_VERIFY(!f.footer->property("chromeVisible").toBool());
        QCOMPARE(cycle119DocumentGeometry(f), geometry);
    }
    // A programmatic document update alone does not hide newly revealed chrome.
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "revealDocumentChrome"));
    f.editor->setProperty("text", text);
    QCoreApplication::processEvents();
    QVERIFY(!f.window->property("documentChromeHidden").toBool());

    // Clipboard selection/copy are harmless; actual paste and native Undo are edits.
    const QString clipboardDraft = "A short clipboard draft.\n";
    f.editor->setProperty("text", clipboardDraft);
    QTRY_VERIFY(!f.window->property("changingDocumentView").toBool());
    scroll->setProperty("contentY", 0);
    f.editor->forceActiveFocus(Qt::MouseFocusReason);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "writingActivity"));
    const auto revealTop = [&] {
        QTest::mouseMove(f.quick, cycle119TopRevealPoint(header));
        QTest::mouseMove(f.quick, writingPoint);
        QTest::qWait(200);
        return header->property("documentChromeVisible").toBool();
    };
    QVERIFY(revealTop());
    QTest::keyClick(f.quick, Qt::Key_A, Qt::ControlModifier);
    QTRY_COMPARE(f.editor->property("selectedText").toString(), clipboardDraft);
    QTest::keyClick(f.quick, Qt::Key_C, Qt::ControlModifier);
    QTRY_COMPARE(QGuiApplication::clipboard()->text(), clipboardDraft);
    QVERIFY(header->property("documentChromeVisible").toBool());
    QGuiApplication::clipboard()->setText("Pasted draft.");
    QTest::keyClick(f.quick, Qt::Key_V, Qt::ControlModifier);
    QTRY_COMPARE(f.editor->property("text").toString(), QString("Pasted draft."));
    QTRY_VERIFY(!header->property("documentChromeVisible").toBool());
    QVERIFY(revealTop());
    QVERIFY(testDesktopMenusMatchPlatform(f.window.data()));
    auto *undo = f.window->property("isMac").toBool() ? f.window->findChild<QObject *>("editUndo")
        : testWindowsMenuItem(f.window.data(), QStringLiteral("&Undo"));
    QVERIFY(undo && undo->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(undo, "triggered"));
    QTRY_COMPARE(f.editor->property("text").toString(), clipboardDraft);
    QTRY_VERIFY(!header->property("documentChromeVisible").toBool());

    QCOMPARE(cycle119DocumentGeometry(f), geometry);
}

void FomawriteTest::documentChromeKeepsMenusKeyboardAndNativeToggleAccessible() {
    Cycle103SettingsSnapshot isolatedSettings;
    Cycle117Window f; QVERIFY2(f.start(1280), qPrintable(f.error));
    auto *settings = f.window->findChild<QObject *>("workspaceSettings");
    auto *toggle = f.window->findChild<QObject *>("nativeAutoHideDocumentChrome");
    auto *fade = f.window->findChild<QObject *>("native_titleBarFade");
    auto *always = f.window->findChild<QObject *>("native_titleBarAlways");
    auto *menu = f.window->findChild<QObject *>("sourceAppearanceMenu");
    auto *source = f.item("sourceModeButton"), *appearance = f.item("sourceAppearanceButton");
    auto *header = f.item("topChrome"), *scroll = f.item("editorScroll");
    QVERIFY(testDesktopMenusMatchPlatform(f.window.data()));
    const bool mac = f.window->property("isMac").toBool();
    QVERIFY(settings && menu && source && appearance && header && scroll);
    if (mac) {
        QVERIFY(toggle && fade && always);
        QCOMPARE(fade->property("text").toString(), QString("Fade In/Out"));
        QCOMPARE(always->property("text").toString(), QString("Always Show"));
    } else {
        QVERIFY(!toggle && !fade && !always);
        fade = testDesktopCommand(f.window.data(), QStringLiteral("titleBarFade"));
        always = testDesktopCommand(f.window.data(), QStringLiteral("titleBarAlways"));
        QVERIFY(fade && always);
        QCOMPARE(fade->property("text").toString(), QStringLiteral("Fade In/Out"));
        QCOMPARE(always->property("text").toString(), QStringLiteral("Always Show"));
    }
    const auto checked = [&](QObject *nativeItem, const QString &id) {
        Q_UNUSED(id);
        return nativeItem->property("checked").toBool();
    };
    const auto choose = [&](QObject *nativeItem, const QString &id) {
        Q_UNUSED(id);
        return QMetaObject::invokeMethod(nativeItem, "triggered");
    };
    const auto toggleBars = [&] {
        if (mac) return QMetaObject::invokeMethod(toggle, "triggered");
        // Windows exposes the two explicit Title Bar choices. Exercise those
        // real menu routes instead of mutating the shared setting directly.
        return QMetaObject::invokeMethod(settings->property("autoHideChrome").toBool()
                                            ? always : fade, "triggered");
    };
    QVERIFY(f.mode(0));
    settings->setProperty("autoHideChrome", true);
    const QPoint writingPoint = cycle103SceneBounds(scroll).center().toPoint();
    QTest::mouseMove(f.quick, writingPoint);
    const int toolbarPreference = settings->property("toolbarVisibilityMode").toInt();
    settings->setProperty("titleBarMode", 0); // Legacy hover preference must not mask Always Show.
    QTRY_VERIFY(checked(fade, QStringLiteral("titleBarFade")));
    QVERIFY(!checked(always, QStringLiteral("titleBarAlways")));
    QVERIFY(choose(always, QStringLiteral("titleBarAlways")));
    QTRY_VERIFY(!settings->property("autoHideChrome").toBool());
    QTRY_VERIFY(checked(always, QStringLiteral("titleBarAlways")));
    QVERIFY(!checked(fade, QStringLiteral("titleBarFade")));
    QVERIFY(!(mac ? toggle->property("checked").toBool() : settings->property("autoHideChrome").toBool()));
    QTRY_COMPARE(header->property("titleContentOpacity").toReal(), qreal(1));
    f.editor->forceActiveFocus(Qt::MouseFocusReason);
    const QString beforeAlwaysTyping = f.editor->property("text").toString();
    cycle111KeyClick(f.quick, 'a');
    QTRY_VERIFY(f.editor->property("text").toString() != beforeAlwaysTyping);
    QVERIFY(header->property("documentChromeVisible").toBool());
    QVERIFY(f.footer->property("chromeVisible").toBool());
    QVERIFY(!f.window->property("documentChromeHidden").toBool());
    settings->setProperty("titleBarMode", 1); // Legacy Always preference must not override Fade.
    QVERIFY(choose(fade, QStringLiteral("titleBarFade")));
    QTRY_VERIFY(settings->property("autoHideChrome").toBool());
    QTRY_VERIFY(checked(fade, QStringLiteral("titleBarFade")));
    QVERIFY(!checked(always, QStringLiteral("titleBarAlways")));
    QVERIFY((mac ? toggle->property("checked").toBool() : settings->property("autoHideChrome").toBool()));
    QCOMPARE(settings->property("toolbarVisibilityMode").toInt(), toolbarPreference);
    const QString beforeFadeTyping = f.editor->property("text").toString();
    cycle111KeyClick(f.quick, 'f');
    QTRY_VERIFY(f.editor->property("text").toString() != beforeFadeTyping);
    QTRY_VERIFY(!header->property("documentChromeVisible").toBool());
    QTRY_COMPARE(header->property("titleContentOpacity").toReal(), qreal(0));
    QTRY_VERIFY(!f.footer->property("chromeVisible").toBool());
    QTest::mouseMove(f.quick, cycle119TopRevealPoint(header));
    QTRY_VERIFY(header->property("documentChromeVisible").toBool());
    QTRY_COMPARE(header->property("titleContentOpacity").toReal(), qreal(1));
    QTest::mouseMove(f.quick, writingPoint);
    QTest::qWait(200);
    QVERIFY(header->property("documentChromeVisible").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "writingActivity"));
    QTRY_VERIFY(!header->property("documentChromeVisible").toBool());
    QTRY_VERIFY((mac ? toggle->property("checked").toBool() : settings->property("autoHideChrome").toBool()));
    QVERIFY(toggleBars());
    QTRY_VERIFY(!settings->property("autoHideChrome").toBool());
    QTRY_VERIFY(checked(always, QStringLiteral("titleBarAlways")));
    QVERIFY(!checked(fade, QStringLiteral("titleBarFade")));
    QTRY_VERIFY(f.footer->property("chromeVisible").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "writingActivity"));
    QVERIFY(!f.window->property("documentChromeHidden").toBool());
    QVERIFY(toggleBars());
    QTRY_VERIFY(settings->property("autoHideChrome").toBool());
    QTRY_VERIFY(checked(fade, QStringLiteral("titleBarFade")));
    QVERIFY(!checked(always, QStringLiteral("titleBarAlways")));

    QVERIFY(cycle102Click(f.quick, appearance));
    QTRY_VERIFY(menu->property("opened").toBool());
    QTest::mouseMove(f.quick, writingPoint);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "writingActivity"));
    QVERIFY(!f.window->property("documentChromeHidden").toBool());
    QVERIFY(f.footer->property("chromeVisible").toBool());
    QVERIFY(QMetaObject::invokeMethod(menu, "close"));
    QTRY_VERIFY(!menu->property("opened").toBool());

    source->forceActiveFocus(Qt::TabFocusReason);
    QTRY_VERIFY(source->hasActiveFocus());
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "writingActivity"));
    QVERIFY(!f.window->property("documentChromeHidden").toBool());
    QVERIFY(f.footer->property("chromeVisible").toBool());
    // Move focus away before a real click; forcing a new reason on the already
    // focused control does not generate a focus transition in Qt Quick.
    f.editor->forceActiveFocus(Qt::MouseFocusReason);
    QVERIFY(cycle102Click(f.quick, source));
    QTest::mouseMove(f.quick, writingPoint);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "writingActivity"));
    QTRY_VERIFY(f.window->property("documentChromeHidden").toBool());
    QTRY_VERIFY(!f.footer->property("chromeVisible").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "focusWorkspaceRegion", Q_ARG(QVariant, "toolbar")));
    QTRY_VERIFY(header->property("documentChromeVisible").toBool());
    QVERIFY(!f.window->property("documentChromeHidden").toBool());
    // Preserve an explicit hidden-toolbar preference while recovering workspace access.
    settings->setProperty("toolbarVisibilityMode", 2);
    QVERIFY(!header->property("toolbarContentVisible").toBool());
    QVERIFY(f.item("topChromeLibraryButton")->isEnabled());
}

void FomawriteTest::publishingWebCommandSelectsWebWithoutChangingEditor() {
    Cycle103SettingsSnapshot isolatedSettings;
    Cycle117Window f; QVERIFY2(f.start(1280), qPrintable(f.error));
    auto *settings = f.window->findChild<QObject *>("workspaceSettings");
    QVERIFY(testDesktopMenusMatchPlatform(f.window.data()));
    auto *command = testDesktopCommand(f.window.data(), QStringLiteral("webPreview"));
    QVERIFY(settings && command);
    QVERIFY(f.mode(1));
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "selectWritingMode", Q_ARG(QVariant, "live")));
    f.editor->setProperty("text", "# Keep this draft\n\n**Unchanged** Markdown.\n");
    const QString source = f.editor->property("text").toString();
    settings->setProperty("publishingFormat", "pdf");
    QTRY_VERIFY(!f.window->property("changingDocumentView").toBool());
    QTRY_VERIFY(!f.preview->property("viewportRefreshPending").toBool());
    QTRY_VERIFY(!command->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(command, "triggered"));
    QTRY_COMPARE(settings->property("publishingFormat").toString(), QString("web"));
    QTRY_VERIFY(command->property("checked").toBool());
    QVERIFY(f.layout->property("liveEditEnabled").toBool());
    QCOMPARE(f.layout->property("effectiveLayoutMode").toInt(), 1);
    QCOMPARE(f.editor->property("text").toString(), source);
}

// Exercise the real in-window menu on Mac as well as Windows so its layout
// cannot cover the document bar's pointer reveal surface.
void FomawriteTest::desktopMenuLeavesDocumentChromeHoverAccessible() {
    Cycle103SettingsSnapshot isolatedSettings;
    Cycle117Window f; QVERIFY2(f.start(1280), qPrintable(f.error));
    auto *loader = f.window->findChild<QObject *>("windowsMenuLoader"); QVERIFY(loader);
    if (!f.window->property("isWindows").toBool()) {
        QVERIFY(!loader->property("active").toBool());
        QCOMPARE(f.window->property("topPadding").toReal(), qreal(0));
        loader->setProperty("active", true);
    }
    QTRY_VERIFY(loader->property("active").toBool());
    auto *desktop = f.item("windowsMenuBar"), *header = f.item("topChrome"), *scroll = f.item("editorScroll");
    auto *settings = f.window->findChild<QObject *>("workspaceSettings");
    QVERIFY(desktop && header && scroll && settings);
    QVERIFY(f.mode(0));
    settings->setProperty("autoHideChrome", true);
    QVERIFY(cycle118FooterRenderBarrier(f.quick));
    const QRectF menuBounds = cycle103SceneBounds(desktop), headerBounds = cycle103SceneBounds(header);
    qInfo() << "Desktop menu/header bounds" << menuBounds << headerBounds;
    QVERIFY(menuBounds.height() > 0);
    QVERIFY2(qAbs(headerBounds.top() - menuBounds.bottom()) <= 0.5,
        "Document header must meet the desktop menu without overlap or extra padding");
    const auto geometry = cycle119DocumentGeometry(f);
    const QPoint writingPoint = cycle103SceneBounds(scroll).center().toPoint();
    QTest::mouseMove(f.quick, writingPoint);
    f.editor->forceActiveFocus(Qt::MouseFocusReason);
    cycle111KeyClick(f.quick, 'x');
    QTRY_VERIFY(!header->property("documentChromeVisible").toBool());
    QTest::mouseMove(f.quick, cycle119TopRevealPoint(header));
    QTRY_VERIFY(header->property("documentChromeVisible").toBool());
    QVERIFY(!f.footer->property("chromeVisible").toBool());
    QTest::mouseMove(f.quick, writingPoint);
    QTest::qWait(200);
    QVERIFY(header->property("documentChromeVisible").toBool());
    QCOMPARE(cycle119DocumentGeometry(f), geometry);
}
