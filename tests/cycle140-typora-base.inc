// Cycle140 — the Typora base shim. Typora themes colour the page through
// custom properties on :root (--bg-color, --text-color, ...) and rely on
// Typora's own base stylesheet to paint it. Fomawrite now supplies a small
// shim (src/typorabase.cpp) ahead of the theme CSS in Web/PDF output and in the
// Live page, and reads the variables back for the host
// (Publisher::themeVariable, Backend::themeBackgroundColor).
//
// Fixtures: tests/fixtures/typora/lapis.css + lapis-dark.css are copies of the
// Lapis theme (YiNN, MIT); lapis-dark imports lapis and overrides its colours.

namespace {
// Evaluates a probe in the Live page and returns what it reported. The page has
// no host-visible console, so the probe writes its answer into the document as
// "@@...@@" through the page's own change path and the host reads it back.
QString cycle140Probe(Cycle134Page &page, QObject *view, const QString &script, bool *ok) {
    const auto count = [&] { return page.canonical().count(QStringLiteral("@@")); };
    const int before = count();
    *ok = QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, script));
    const auto deadline = QDeadlineTimer(10000);
    while (count() == before && !deadline.hasExpired()) QTest::qWait(25);
    const QString text = page.canonical();
    const int start = text.indexOf(QStringLiteral("@@"));
    const int end = start < 0 ? -1 : text.indexOf(QStringLiteral("@@"), start + 2);
    *ok = *ok && count() > before && end > start;
    return *ok ? text.mid(start + 2, end - start - 2) : QString();
}
const char *const cycle140ProbeScript = R"JS(
(function() {
    const cs = (e) => getComputedStyle(e);
    const w = document.getElementById('write');
    const sel = document.querySelector('.cm-selectionBackground');
    const r = [cs(document.documentElement).backgroundColor, cs(document.body).backgroundColor, cs(document.body).color,
               cs(w).color, w.classList.contains('fw-live-fill-bg') ? 1 : 0, w.classList.contains('fw-live-fill-fg') ? 1 : 0,
               sel ? cs(sel).backgroundColor : 'none', (document.querySelector('.cm-cursor') ? cs(document.querySelector('.cm-cursor')).borderLeftColor : 'none')].join('|');
    window.fomawriteEditor.simulateUserChanges(JSON.stringify([{from: 0, to: 0, insert: '@@' + r + '@@'}]));
})()
)JS";
}

// The parser behind Publisher::themeVariable / Backend::themeBackgroundColor.
void FomawriteTest::typoraBaseReadsThemeVariables() {
    // Pure parser: last declaration wins, var() resolves (with fallbacks and through chains), @media blocks are
    // ignored, cycles and unresolvable references are dropped, comments and !important are tolerated.
    const QString css = QStringLiteral(
        "/* :root { --bg-color: red } */\n"
        "@import \"other.css\";\n"
        ":root { --bg-color: #fff; --text-color: #111 !important; --alias: var(--bg-color); --chain: var(--alias);"
        " --fallback: var(--missing, #abc); --nested: rgb(var(--n, 1), 2, 3); --loop-a: var(--loop-b); --loop-b: var(--loop-a);"
        " --broken: var(--nowhere); --mixed: 1px solid var(--text-color); --name: \"a;b\"; }\n"
        "@media print { :root { --text-color: #000; --print-only: 1 } }\n"
        "html { --bg-color: #1e222a; }\n"
        "#write { --not-root: 1 }\n"
        ".dark :root, :root.other { --scoped: 1 }\n"
        "body { color: var(--text-color) }\n");
    const auto variables = TyporaBase::themeVariables(css);
    QCOMPARE(variables.value("--bg-color"), QStringLiteral("#1e222a"));
    QCOMPARE(variables.value("--text-color"), QStringLiteral("#111"));
    QCOMPARE(variables.value("--alias"), QStringLiteral("#1e222a")); // resolved against the final value
    QCOMPARE(variables.value("--chain"), QStringLiteral("#1e222a"));
    QCOMPARE(variables.value("--fallback"), QStringLiteral("#abc"));
    QCOMPARE(variables.value("--nested"), QStringLiteral("rgb(1, 2, 3)"));
    QCOMPARE(variables.value("--mixed"), QStringLiteral("1px solid #111"));
    QCOMPARE(variables.value("--name"), QStringLiteral("\"a;b\""));
    QVERIFY(!variables.contains("--loop-a")); QVERIFY(!variables.contains("--loop-b"));
    QVERIFY(!variables.contains("--broken"));
    QVERIFY(!variables.contains("--print-only")); QVERIFY(!variables.contains("--not-root")); QVERIFY(!variables.contains("--scoped"));
    QCOMPARE(TyporaBase::themeVariable(css, "bg-color"), QStringLiteral("#1e222a")); // with or without the dashes
    QCOMPARE(TyporaBase::themeVariable(css, "--bg-color"), QStringLiteral("#1e222a"));
    QCOMPARE(TyporaBase::themeVariable(css, "--absent"), QString());
    // Review fixes: the shim's body colour inherits (no final fallback) so a theme
    // that colours html alone keeps it; a fan-out of var() references resolves in
    // linear time and over-long values are dropped rather than expanded.
    QVERIFY(TyporaBase::css().contains("body { background: var(--bg-color, var(--fw-bg, transparent)); color: var(--text-color, var(--fw-fg)); }"));
    QVERIFY(!TyporaBase::css().contains("html, body"));
    QString fanOut = QStringLiteral(":root { --v17: x; ");
    for (int n = 16; n >= 1; --n) { fanOut += QStringLiteral("--v%1: ").arg(n); for (int k = 0; k < 10; ++k) fanOut += QStringLiteral("var(--v%1) ").arg(n + 1); fanOut += QLatin1String("; "); }
    fanOut += QLatin1String("}");
    QElapsedTimer fast; fast.start();
    const auto fanned = TyporaBase::themeVariables(fanOut);
    QVERIFY2(fast.elapsed() < 1000, "fan-out resolution must be linear");
    QCOMPARE(fanned.value("--v17"), QStringLiteral("x"));
    QVERIFY(!fanned.contains("--v1")); // 10^16 characters: dropped, not expanded
    QVERIFY(TyporaBase::themeVariables(QString()).isEmpty());
    QVERIFY(TyporaBase::themeVariables(QStringLiteral(":root { --a: 1")).isEmpty()); // an unterminated block is ignored

    // The shim itself: defined once, every rule commented, the editor-only rules not in the published text.
    QVERIFY(TyporaBase::css().contains("var(--bg-color"));
    QVERIFY(TyporaBase::css().contains("var(--text-color"));
    QVERIFY(!TyporaBase::css().contains("cm-selectionBackground"));
    QVERIFY(TyporaBase::liveCss().startsWith(TyporaBase::css()));
    QVERIFY(TyporaBase::liveCss().contains("cm-selectionBackground"));

    // The real thing: Andrew's Lapis Dark, whose import of lapis.css is inlined by the theme loader.
    Cycle103SettingsSnapshot settings;
    Cycle140Lapis lapis; QVERIFY(lapis.install());
    QString error;
    const auto resolved = PublishingThemes::css(QStringLiteral("lapis-dark"), &error);
    QVERIFY2(resolved, qPrintable(error));
    QVERIFY(resolved->contains("--bg-color: #ffffff")); // lapis.css's light value is in the text ...
    QCOMPARE(TyporaBase::themeVariable(*resolved, "--bg-color"), QStringLiteral("#1e222a")); // ... and lapis-dark's later one wins
    QCOMPARE(TyporaBase::themeVariable(*resolved, "--text-color"), QStringLiteral("#e4e4e4e3")); // the @media print override is ignored
    QCOMPARE(TyporaBase::themeVariable(*resolved, "--select-text-bg-color"), QStringLiteral("#dae3ea2e"));
    QCOMPARE(TyporaBase::themeVariable(*resolved, "--side-bar-bg-color"), QStringLiteral("#181c24")); // light value was var(--bg-color)
    QCOMPARE(TyporaBase::themeVariable(*resolved, "--heading-char-color"), QStringLiteral("#e4e4e4e3")); // var(--text-color)
    QCOMPARE(TyporaBase::themeVariable(*resolved, "--img-shadow-color"), QStringLiteral("#080e1d")); // var(--block-bg-color)
    QCOMPARE(TyporaBase::themeVariable(*resolved, "--window-border"), QStringLiteral("1px solid #1e222a"));
    QCOMPARE(TyporaBase::themeVariable(*resolved, "--source-color"), QStringLiteral("#a8a8a9")); // only lapis.css defines it

    // Publisher and Backend expose it, and notify when the answer changes.
    Backend backend;
    QSignalSpy cssChanged(&backend, &Backend::publishingCssChanged);
    QCOMPARE(backend.themeBackgroundColor(), QString()); // no theme: the basic preset defines no --bg-color
    QVERIFY2(backend.selectPublishingTheme("lapis-dark"), qPrintable(backend.status()));
    QCOMPARE(backend.themeBackgroundColor(), QStringLiteral("#1e222a"));
    QCOMPARE(backend.property("themeBackgroundColor").toString(), QStringLiteral("#1e222a"));
    auto *publisher = static_cast<Publisher *>(backend.publisher());
    QCOMPARE(publisher->themeVariable("--bg-color"), QStringLiteral("#1e222a"));
    QCOMPARE(publisher->themeVariable("primary-color"), QStringLiteral("#8393ad"));
    QCOMPARE(publisher->themeVariables().value("--code-color"), QStringLiteral("#abbad4"));
    QVERIFY(cssChanged.count() >= 1);
    QVERIFY2(backend.selectPublishingTheme("lapis"), qPrintable(backend.status()));
    QCOMPARE(backend.themeBackgroundColor(), QStringLiteral("#ffffff"));
    // A basic preset replaces the theme: the property changes and says so even though no theme CSS changed hands.
    QVERIFY2(backend.selectPublishingTheme("lapis-dark"), qPrintable(backend.status()));
    const int before = cssChanged.count();
    backend.setOutputStyle(0);
    QCOMPARE(backend.themeBackgroundColor(), QString());
    QVERIFY(cssChanged.count() > before);
    backend.discardRecovery();
}

// A theme that only sets variables must paint the published page. Red on the old
// code: no shim in the HTML, the page stayed white.
void FomawriteTest::publishedOutputPaintsThePageFromThemeVariables() {
    Cycle103SettingsSnapshot settings;
    const QString folder = PublishingThemes::directory(); QVERIFY(QDir().mkpath(folder));
    const QString id = "cycle140-variables-only";
    const QString theme = folder + '/' + id + ".css";
    const auto cleanup = qScopeGuard([&] { QFile::remove(theme); });
    QVERIFY(cycle132Write(theme, ":root { --bg-color: #1e222a; --text-color: #e4e4e4 }\n#write { max-width: 600px; margin: 0 auto }"));
    Cycle132Editor fixture; QVERIFY(fixture.create());
    fixture.editor->setProperty("text", "# Dark page\n\nBody text on a dark page.\n");
    QTest::qWait(300);
    QVERIFY2(fixture.backend.selectPublishingTheme(id), qPrintable(fixture.backend.status()));
    const auto web = fixture.backend.publishingPreview("html");
    QVERIFY2(web["ok"].toBool(), qPrintable(web["error"].toString()));
    const QString html = cycle140Html(web["url"].toUrl());
    const qsizetype shim = html.indexOf(TyporaBase::css());
    const qsizetype themeRoot = html.indexOf(":root { --bg-color: #1e222a");
    const qsizetype themeRule = html.indexOf("#write { max-width: 600px");
    QVERIFY2(shim >= 0, "the Typora base shim is missing from the published HTML");
    QVERIFY(themeRoot >= 0 && themeRule >= 0);
    // The page rule that reads the variables comes before every theme rule, so the theme can override it.
    const qsizetype paint = html.indexOf("html { background: var(--bg-color");
    QVERIFY(paint >= 0);
    QVERIFY(paint < themeRoot); QVERIFY(paint < themeRule);
    QCOMPARE(html.count(TyporaBase::css()), 1);
    QVERIFY(html.indexOf("body { margin: 0; }") < shim); // foundations first, shim next, theme last
    QVERIFY(html.indexOf("</style>") > themeRule);
    // The browser agrees: a real PDF of the page has a dark ground, not white paper.
    const auto pdf = fixture.backend.publishingPreview("pdf");
    QVERIFY2(pdf["ok"].toBool(), qPrintable(pdf["error"].toString()));
    QPdfDocument document; QCOMPARE(document.load(pdf["url"].toUrl().toLocalFile()), QPdfDocument::Error::None);
    QVERIFY(document.pageCount() >= 1);
    const auto size = document.pagePointSize(0);
    const QImage page = document.render(0, QSize(int(size.width()), int(size.height())));
    QVERIFY(!page.isNull());
    const QColor ground = page.pixelColor(page.width() / 2, page.height() * 9 / 10);
    QVERIFY2(cycle140Luma(ground) < 80, qPrintable("PDF page ground is " + ground.name()));
    // A theme that sets no variables and no colours renders as before: nothing in the shim paints the page.
    QVERIFY(fixture.backend.selectPublishingTheme("claude-like"));
    const auto plain = fixture.backend.publishingPreview("html");
    QVERIFY(plain["ok"].toBool());
    QVERIFY(cycle140Html(plain["url"].toUrl()).contains(TyporaBase::css()));
    fixture.backend.discardRecovery();
}

// The Live page: Lapis Dark's variables paint the page (filtered and exact), the readability fill
// stays out of the way, selection and caret follow the theme, and a theme without
// variables is untouched. Red on the old code: the body stayed the editor's own colour.
void FomawriteTest::liveEditorPaintsThePageFromThemeVariables() {
    Cycle103SettingsSnapshot settings;
    Cycle140Lapis lapis; QVERIFY(lapis.install());
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    page.window->resize(960, 700);
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    page.editor->setProperty("text", QStringLiteral("# Dark Lapis\n\nHello **world** and `code` in a long enough line to select.\n"));
    QVERIFY2(page.backend.selectPublishingTheme("lapis-dark"), qPrintable(page.backend.status()));
    page.backend.syncLiveEditor(0);
    const auto appearance = [&](bool filter, bool dark) {
        page.bridge()->applyAppearance(QStringLiteral("{\"appearance\":\"manuscript\",\"dark\":%1,\"fontSize\":17,\"liveThemeFilter\":%2,\"palette\":{\"background\":\"%3\",\"text\":\"%4\"}}")
            .arg(dark ? "true" : "false").arg(filter ? "true" : "false").arg(dark ? "#1f1f1f" : "#ffffff").arg(dark ? "#e6e6e6" : "#222222"));
    };
    appearance(true, false);
    page.bridge()->selectMode("live");
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), page.canonical(), 10000);
    auto *view = page.root->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QVERIFY(QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, QStringLiteral(
        "window.fomawriteEditor.view.dispatch({selection:{anchor:14,head:30}})"))));
    QTest::qWait(400);
    bool ok = false;
    // html | body background | body colour | #write colour | fill-bg | fill-fg | selection layer | caret
    QStringList fields = cycle140Probe(page, view, QString::fromLatin1(cycle140ProbeScript), &ok).split('|');
    QVERIFY2(ok && fields.size() == 8, "the probe did not report");
    QVERIFY2(fields[1].startsWith("rgb(30, 34, 42"), qPrintable("body background " + fields[1]));
    QVERIFY2(fields[0].startsWith("rgb(30, 34, 42"), qPrintable("html background " + fields[0]));
    QVERIFY2(fields[2].contains("228, 228, 228"), qPrintable("body colour " + fields[2]));
    QVERIFY2(fields[3].contains("228, 228, 228"), qPrintable("#write colour " + fields[3]));
    QCOMPARE(fields[4], QStringLiteral("0")); // the page measured the body as painted: no readability fill
    QCOMPARE(fields[5], QStringLiteral("0"));
    QVERIFY2(fields[6].contains("218, 227, 234"), qPrintable("selection layer " + fields[6]));
    QVERIFY2(fields[7].contains("228, 228, 228"), qPrintable("caret " + fields[7]));
    // Exact mode keeps the same page.
    appearance(false, false); QTest::qWait(300);
    fields = cycle140Probe(page, view, QString::fromLatin1(cycle140ProbeScript), &ok).split('|');
    QVERIFY(ok && fields.size() == 8);
    QVERIFY2(fields[1].startsWith("rgb(30, 34, 42"), qPrintable("exact body background " + fields[1]));
    QVERIFY2(fields[6].contains("218, 227, 234"), qPrintable("exact selection layer " + fields[6]));
    // A dark editor does not change what the theme says.
    appearance(true, true); QTest::qWait(300);
    fields = cycle140Probe(page, view, QString::fromLatin1(cycle140ProbeScript), &ok).split('|');
    QVERIFY(ok && fields.size() == 8);
    QVERIFY2(fields[1].startsWith("rgb(30, 34, 42"), qPrintable("dark-editor body background " + fields[1]));
    // A basic preset defines no variables: the page keeps the editor's own colours (the fallbacks are no-ops).
    page.backend.setOutputStyle(0);
    page.backend.pushLiveTheme();
    appearance(true, false); QTest::qWait(300);
    fields = cycle140Probe(page, view, QString::fromLatin1(cycle140ProbeScript), &ok).split('|');
    QVERIFY(ok && fields.size() == 8);
    QCOMPARE(fields[1], QStringLiteral("rgb(255, 255, 255)")); // --fw-bg
    QCOMPARE(fields[2], QStringLiteral("rgb(36, 41, 47)"));    // --fw-fg
    QVERIFY2(fields[6].contains("9, 105, 218"), qPrintable("default selection layer " + fields[6])); // --fw-selection
    appearance(true, true); QTest::qWait(300);
    fields = cycle140Probe(page, view, QString::fromLatin1(cycle140ProbeScript), &ok).split('|');
    QVERIFY(ok && fields.size() == 8);
    QCOMPARE(fields[1], QStringLiteral("rgb(30, 31, 34)"));
    QCOMPARE(fields[2], QStringLiteral("rgb(230, 237, 243)"));
    page.backend.discardRecovery();
}

// Andrew, 9 Oct: Typora's dark themes darken the whole app. With Appearance
// Follows Output Style (default on) the window takes the theme's page colours
// and its luminance decides dark mode; the Theme preset returns when the theme
// defines no page colour or the setting is off.
void FomawriteTest::appearanceFollowsTheOutputStylesPageColours() {
    Cycle103SettingsSnapshot settings;
    Cycle140Lapis lapis; QVERIFY(lapis.install());
    Backend backend;
    backend.setThemePreset("studio");
    QVERIFY(backend.appearanceFollowsOutputStyle());
    QVERIFY(!backend.darkMode());
    QSignalSpy colours(&backend, &Backend::themeColorsChanged);
    QVERIFY2(backend.selectPublishingTheme("lapis-dark"), qPrintable(backend.status()));
    QTRY_VERIFY(backend.darkMode());
    QCOMPARE(backend.palette().value("page").toString().toLower(), QStringLiteral("#1e222a"));
    QCOMPARE(backend.palette().value("text").toString().toLower(), QStringLiteral("#e4e4e4")); // alpha dropped
    QVERIFY(colours.count() >= 1);
    // A light theme from the same family: light chrome, white page.
    QVERIFY2(backend.selectPublishingTheme("lapis"), qPrintable(backend.status()));
    QTRY_VERIFY(!backend.darkMode());
    QCOMPARE(backend.palette().value("page").toString().toLower(), QStringLiteral("#ffffff"));
    // Switching the setting off restores the preset while a dark theme is selected.
    QVERIFY2(backend.selectPublishingTheme("lapis-dark"), qPrintable(backend.status()));
    QTRY_VERIFY(backend.darkMode());
    backend.setAppearanceFollowsOutputStyle(false);
    QVERIFY(!backend.darkMode());
    QCOMPARE(backend.palette().value("page").toString().toUpper(), QStringLiteral("#F6F3EE"));
    QCOMPARE(QSettings().value("appearance/followOutputStyle").toBool(), false);
    backend.setAppearanceFollowsOutputStyle(true);
    QVERIFY(backend.darkMode());
    // A basic preset defines no --bg-color: the Theme preset decides again.
    backend.setOutputStyle(0);
    QTRY_VERIFY(!backend.darkMode());
    QCOMPARE(backend.palette().value("page").toString().toUpper(), QStringLiteral("#F6F3EE"));
    QVERIFY(!backend.appearanceOverridden());
    // Colour forms: rgb() counts; transparent does not drive the chrome.
    const auto writeProbe = [&](const QString &id, const QByteArray &css) {
        QSaveFile file(lapis.folder + '/' + id + ".css"); lapis.files.append(lapis.folder + '/' + id + ".css");
        return file.open(QIODevice::WriteOnly) && file.write(css) == css.size() && file.commit();
    };
    QVERIFY(writeProbe("probe-rgb", ":root { --bg-color: rgb(30, 34, 42); --text-color: hsl(0, 0%, 90%); }\n"));
    QVERIFY(writeProbe("probe-clear", ":root { --bg-color: transparent; --text-color: #e4e4e4; }\n"));
    Backend probe;
    probe.setThemePreset("studio");
    QVERIFY2(probe.selectPublishingTheme("probe-rgb"), qPrintable(probe.status()));
    QTRY_VERIFY(probe.darkMode());
    QCOMPARE(probe.palette().value("page").toString().toLower(), QStringLiteral("#1e222a"));
    QVERIFY(probe.appearanceOverridden());
    QVERIFY2(probe.selectPublishingTheme("probe-clear"), qPrintable(probe.status()));
    QTRY_VERIFY(!probe.darkMode());
    QCOMPARE(probe.palette().value("page").toString().toUpper(), QStringLiteral("#F6F3EE"));
    QVERIFY(!probe.appearanceOverridden());
    // The palette is announced only when a colour moved: a CSS change that
    // leaves the colours alone does not re-announce.
    QSignalSpy announced(&probe, &Backend::themeColorsChanged);
    emit probe.publishingCssChanged();
    QCOMPARE(announced.count(), 0);
}
