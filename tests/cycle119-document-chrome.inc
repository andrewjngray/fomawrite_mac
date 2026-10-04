namespace {
QMap<QString, QRectF> cycle119DocumentGeometry(const Cycle117Window &f) {
    QMap<QString, QRectF> result;
    for (const char *name : {"editorPane", "editorScroll", "documentHeader", "documentFooter"})
        if (auto *item = f.item(name)) result.insert(name, cycle103SceneBounds(item));
    return result;
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
    QTest::mouseMove(f.quick, QPoint(f.quick->width() - 5, 4));
    QTRY_VERIFY(header->property("documentChromeVisible").toBool());
    QVERIFY(!f.footer->property("chromeVisible").toBool());
    QTest::mouseMove(f.quick, writingPoint);
    QTRY_VERIFY(!header->property("documentChromeVisible").toBool());
    QTest::mouseMove(f.quick, QPoint(f.quick->width() - 5, f.quick->height() - 4));
    QTRY_VERIFY(f.footer->property("chromeVisible").toBool());
    QVERIFY(!header->property("documentChromeVisible").toBool());
    QVERIFY(source->isEnabled());
    QCOMPARE(cycle119DocumentGeometry(f), geometry);

    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "revealDocumentChrome"));
    QTest::mouseMove(f.quick, writingPoint);
    f.editor->forceActiveFocus(Qt::MouseFocusReason);
    scroll->setProperty("contentY", 0);
    cycle119Wheel(f.quick, writingPoint);
    QTRY_VERIFY(f.window->property("documentChromeHidden").toBool());
    QTRY_VERIFY(scroll->property("contentY").toReal() > 0);
    QTRY_VERIFY(!f.footer->property("chromeVisible").toBool());
    QCOMPARE(cycle119DocumentGeometry(f), geometry);
    QCOMPARE(f.editor->property("text").toString(), "x" + text);
    // A programmatic document update alone does not hide newly revealed chrome.
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "revealDocumentChrome"));
    f.editor->setProperty("text", text);
    QCoreApplication::processEvents();
    QVERIFY(!f.window->property("documentChromeHidden").toBool());
}

void FomawriteTest::documentChromeKeepsMenusKeyboardAndNativeToggleAccessible() {
    Cycle103SettingsSnapshot isolatedSettings;
    Cycle117Window f; QVERIFY2(f.start(1280), qPrintable(f.error));
    auto *settings = f.window->findChild<QObject *>("workspaceSettings");
    auto *toggle = f.window->findChild<QObject *>("nativeAutoHideDocumentChrome");
    auto *menu = f.window->findChild<QObject *>("sourceAppearanceMenu");
    auto *source = f.item("sourceModeButton"), *appearance = f.item("sourceAppearanceButton");
    auto *header = f.item("topChrome"), *scroll = f.item("editorScroll");
    QVERIFY(settings && toggle && menu && source && appearance && header && scroll);
    QVERIFY(f.mode(0));
    settings->setProperty("autoHideChrome", true);
    const QPoint writingPoint = cycle103SceneBounds(scroll).center().toPoint();
    QTest::mouseMove(f.quick, writingPoint);
    f.editor->forceActiveFocus(Qt::MouseFocusReason);
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "writingActivity"));
    QTRY_VERIFY(!f.footer->property("chromeVisible").toBool());
    QTRY_VERIFY(toggle->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(toggle, "triggered"));
    QTRY_VERIFY(!settings->property("autoHideChrome").toBool());
    QTRY_VERIFY(f.footer->property("chromeVisible").toBool());
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "writingActivity"));
    QVERIFY(!f.window->property("documentChromeHidden").toBool());
    QVERIFY(QMetaObject::invokeMethod(toggle, "triggered"));
    QTRY_VERIFY(settings->property("autoHideChrome").toBool());

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
    auto *command = f.window->findChild<QObject *>("native_webPreview");
    QVERIFY(settings && command);
    QVERIFY(f.mode(1));
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "selectWritingMode", Q_ARG(QVariant, "visual")));
    f.editor->setProperty("text", "# Keep this draft\n\n**Unchanged** Markdown.\n");
    const QString source = f.editor->property("text").toString();
    settings->setProperty("publishingFormat", "pdf");
    QTRY_VERIFY(!f.window->property("changingDocumentView").toBool());
    QTRY_VERIFY(!f.preview->property("viewportRefreshPending").toBool());
    QTRY_VERIFY(!command->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(command, "triggered"));
    QTRY_COMPARE(settings->property("publishingFormat").toString(), QString("web"));
    QTRY_VERIFY(command->property("checked").toBool());
    QVERIFY(f.layout->property("visualEditEnabled").toBool());
    QCOMPARE(f.layout->property("effectiveLayoutMode").toInt(), 1);
    QCOMPARE(f.editor->property("text").toString(), source);
}
