// Cycle134 — Live is a third option in the footer's editing-mode capsule,
// after Source (Visual Edit was retired in Cycle 135). The capsule keeps its pane-owned position and
// the options stay in order at wide and narrow window widths.

void FomawriteTest::footerLiveOptionJoinsEditingCapsule() {
    Cycle103SettingsSnapshot isolatedSettings;
    Cycle117Window f; QVERIFY2(f.start(1100), qPrintable(f.error));
    auto *source = f.item("sourceModeButton"), *live = f.item("liveEditToggle");
    auto *single = f.item("singleModeButton"), *capsule = f.item("documentEditingControls"), *view = f.item("documentViewControls");
    QVERIFY(source && live && single && capsule && view);
    QCOMPARE(live->property("text").toString(), QStringLiteral("Live"));
    QCOMPARE(live->parentItem(), source->parentItem());
    const auto checks = [&](bool sourceOn, bool liveOn) {
        QCOMPARE(source->property("checked").toBool(), sourceOn);
        QCOMPARE(live->property("checked").toBool(), liveOn);
        QCOMPARE(f.layout->property("liveEditEnabled").toBool(), liveOn);
        QVERIFY(cycle107ViewChecked(f.footer, f.layout));
    };
    QVERIFY(f.mode(0)); QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 0);
    for (int width : {1100, 720}) {
        f.quick->resize(width, 720);
        QTRY_COMPARE(f.quick->size(), QSize(width, 720));
        QVERIFY(cycle118FooterRenderBarrier(f.quick));
        QVERIFY(cycle102Click(f.quick, source)); QTRY_VERIFY(f.editor->isVisible());
        checks(true, false);
        const auto fixed = cycle107ViewBounds(f.footer);
        for (int pass = 0; pass < 2; ++pass) {
            // Live replaces Source; Source does not stay checked.
            QVERIFY(cycle102Click(f.quick, live));
            QTRY_VERIFY(f.layout->property("liveEditEnabled").toBool());
            checks(false, true);
            QVERIFY(!f.editor->isVisible());
            // Source turns Live off again.
            QVERIFY(cycle102Click(f.quick, source));
            QTRY_VERIFY(!f.layout->property("liveEditEnabled").toBool());
            QTRY_VERIFY(f.editor->isVisible());
            checks(true, false);
            // Clicking the active Live option twice leaves it on.
            QVERIFY(cycle102Click(f.quick, live)); QVERIFY(cycle102Click(f.quick, live));
            checks(false, true);
            QVERIFY(cycle102Click(f.quick, source));
            QTRY_VERIFY(!f.layout->property("liveEditEnabled").toBool());
        }
        // Options keep their order, stay in the capsule and inside the footer,
        // and the capsule neither moves the view controls nor overlaps them.
        QVERIFY(cycle102Click(f.quick, live));
        QTRY_VERIFY(f.layout->property("liveEditEnabled").toBool());
        QVERIFY(cycle118FooterRenderBarrier(f.quick));
        const auto sourceBounds = cycle103SceneBounds(source),
            liveBounds = cycle103SceneBounds(live), capsuleBounds = cycle103SceneBounds(capsule),
            footerBounds = cycle103SceneBounds(f.footer);
        QVERIFY(sourceBounds.right() <= liveBounds.left() + 0.5);
        QVERIFY(capsuleBounds.adjusted(-0.1, -0.1, 0.1, 0.1).contains(sourceBounds)
                && capsuleBounds.adjusted(-0.1, -0.1, 0.1, 0.1).contains(liveBounds));
        QVERIFY(footerBounds.adjusted(-0.1, -0.1, 0.1, 0.1).contains(capsuleBounds));
        QVERIFY(capsuleBounds.right() + 10 <= cycle103SceneBounds(view).left());
        QVERIFY(qAbs(cycle103SceneBounds(view).right() - (width - 12)) < 0.1);
        QTRY_VERIFY(cycle107SameBounds(cycle107ViewBounds(f.footer), fixed));
        QTRY_VERIFY(cycle107ViewLabelsReadable(f.footer));
        QTRY_VERIFY(cycle106FooterControlsFit(f.window.data()));
        QTRY_VERIFY(cycle117PaneFooterAnchors(f.window.data()));
        QVERIFY(cycle102Click(f.quick, source));
        QTRY_VERIFY(!f.layout->property("liveEditEnabled").toBool());
    }
    // With both library panes open the narrowest window that keeps them (739 px)
    // leaves a 321 px editor pane.
    // The capsule then uses compact labels so Live fits beside the style control.
    f.quick->resize(1440, 720); QTRY_COMPARE(f.quick->size(), QSize(1440, 720));
    f.layout->setProperty("organizerVisible", true); f.layout->setProperty("filesVisible", true);
    QTRY_VERIFY(f.layout->property("effectiveOrganizerVisible").toBool());
    // Step down through the widths the layout contracts at, keeping both panes.
    for (int width : {960, 739}) {
        f.quick->resize(width, 720); QTRY_COMPARE(f.quick->size(), QSize(width, 720));
    }
    QTRY_VERIFY(f.layout->property("effectiveOrganizerVisible").toBool());
    QTRY_COMPARE(f.footer->width(), 739 - cycle103SceneBounds(f.footer).left());
    QVERIFY(f.footer->width() < 330);
    QVERIFY(cycle102Click(f.quick, live)); QTRY_VERIFY(f.layout->property("liveEditEnabled").toBool());
    auto *appearance = f.item("sourceAppearanceButton"); QVERIFY(appearance);
    QVERIFY(cycle118FooterRenderBarrier(f.quick));
    QVERIFY(live->isVisible() && appearance->isVisible());
    QCOMPARE(live->property("hint").toString(), QStringLiteral("Live \u2014 edit with Markdown rendered in place"));
    checks(false, true);
    QTRY_VERIFY(cycle107ViewLabelsReadable(f.footer));
    QTRY_VERIFY(cycle106FooterControlsFit(f.window.data()));
    QTRY_VERIFY(cycle117PaneFooterAnchors(f.window.data()));
    QVERIFY(cycle103SceneBounds(appearance).right() + 7.5 <= cycle103SceneBounds(source).left());
    QVERIFY(cycle102Click(f.quick, source)); QTRY_VERIFY(!f.layout->property("liveEditEnabled").toBool());
    // Read-only Preview hides the whole capsule, so no editing option is checked.
    QVERIFY(QMetaObject::invokeMethod(f.window.data(), "selectWritingMode", Q_ARG(QVariant, "live")));
    QVERIFY(cycle118ReadPreview(f.window.data()));
    QTRY_COMPARE(f.layout->property("effectiveLayoutMode").toInt(), 2);
    QVERIFY(!capsule->isVisible());
    QVERIFY(!source->property("checked").toBool() && !live->property("checked").toBool());
}
