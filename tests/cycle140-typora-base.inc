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

// Highlighted text (`==text==`) must stay readable under any theme. Typora's
// own base paints <mark> from --highlight-color; Fomawrite's shim does the
// same and picks the text colour (--fw-highlight-fg) for contrast. Before the
// fix Web/PDF kept the page's light text on the pale yellow of a dark theme,
// and Live did not render `==text==` at all.
namespace {
// "rgb(r, g, b)" / "rgba(r, g, b, a)" as computed by the page; transparent / unparseable -> alpha 0.
QColor cycle140Computed(const QString &css) {
    static const QRegularExpression pattern(QStringLiteral("^rgba?\\(\\s*([0-9.]+)[,\\s]+([0-9.]+)[,\\s]+([0-9.]+)(?:[,\\s/]+([0-9.]+%?))?\\s*\\)$"));
    const auto match = pattern.match(css.trimmed());
    if (!match.hasMatch()) return QColor(0, 0, 0, 0);
    QString alphaText = match.captured(4);
    double alpha = 1.0;
    if (!alphaText.isEmpty()) { const bool percent = alphaText.endsWith('%'); if (percent) alphaText.chop(1); alpha = alphaText.toDouble() / (percent ? 100.0 : 1.0); }
    QColor colour(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());
    colour.setAlphaF(qBound(0.0, alpha, 1.0));
    return colour;
}
QColor cycle140Over(const QColor &top, const QColor &under) {
    const double a = top.alphaF();
    return QColor::fromRgbF(top.redF() * a + under.redF() * (1 - a), top.greenF() * a + under.greenF() * (1 - a), top.blueF() * a + under.blueF() * (1 - a));
}
double cycle140Luminance(const QColor &c) {
    const auto channel = [](double v) { return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
    return 0.2126 * channel(c.redF()) + 0.7152 * channel(c.greenF()) + 0.0722 * channel(c.blueF());
}
// WCAG contrast of `text` on `ground`, where the ground (possibly translucent) sits on `page` (transparent = white paper).
double cycle140PairContrast(const QColor &text, const QColor &ground, const QColor &page) {
    const QColor paper = page.alpha() == 0 ? QColor(Qt::white) : cycle140Over(page, Qt::white);
    const QColor seen = cycle140Over(ground, paper);
    const double a = cycle140Luminance(text), b = cycle140Luminance(seen);
    return (qMax(a, b) + 0.05) / (qMin(a, b) + 0.05);
}
// color | background-color | body background | highlight count | text of the first highlight
const char *const cycle140HighlightScript = R"JS((function() {
    const all = document.querySelectorAll('.fw-highlight, .highlight, mark');
    const e = all[0];
    const cs = e ? getComputedStyle(e) : null;
    return [cs ? cs.color : '', cs ? cs.backgroundColor : '', getComputedStyle(document.body).backgroundColor, all.length, e ? e.textContent : ''].join('|');
})())JS";

QStringList cycle140HighlightInPage(QWebEnginePage &page) {
    QEventLoop loop; QString answer; bool answered = false;
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    page.runJavaScript(QString::fromLatin1(cycle140HighlightScript), [&](const QVariant &value) { answer = value.toString(); answered = true; loop.quit(); });
    if (!answered) loop.exec();
    return answer.split('|');
}

// What the page painted for the first highlight must read at 4.5:1 (WCAG AA).
void cycle140CheckHighlightFields(const QStringList &fields, const QString &where, double minimum = 4.5, double *out = nullptr) {
    QVERIFY2(fields.size() == 5, qPrintable(where + ": the probe did not report"));
    QVERIFY2(fields[3].toInt() >= 1, qPrintable(where + ": the page has no highlight element"));
    const QColor text = cycle140Computed(fields[0]), ground = cycle140Computed(fields[1]), page = cycle140Computed(fields[2]);
    QVERIFY2(text.alpha() > 0, qPrintable(where + ": text colour " + fields[0]));
    QVERIFY2(ground.alpha() > 0, qPrintable(where + ": the highlight has no background " + fields[1]));
    const double ratio = cycle140PairContrast(text, ground, page);
    if (out) *out = ratio;
    QVERIFY2(ratio >= minimum, qPrintable(QStringLiteral("%1: text %2 on %3 over %4 is %5:1").arg(where, fields[0], fields[1], fields[2]).arg(ratio, 0, 'f', 2)));
}
}

// The text-colour rule on its own, and the text of the shim that carries it.
void FomawriteTest::typoraBaseChoosesReadableHighlightText() {
    Cycle103SettingsSnapshot settings;
    Cycle140Lapis lapis; QVERIFY(lapis.install());
    // No highlight colour: the editor's pale yellow, dark text.
    QCOMPARE(TyporaBase::highlightText(QString()), QStringLiteral("#1a1a1a"));
    QCOMPARE(TyporaBase::highlightText(":root { --bg-color: #1e222a; --text-color: #e4e4e4 }"), QStringLiteral("#1a1a1a"));
    // Lapis Dark: #ffffb5c2 (76% pale yellow) over #1e222a is still light, so dark text.
    QString error;
    const auto resolved = PublishingThemes::css(QStringLiteral("lapis-dark"), &error);
    QVERIFY2(resolved, qPrintable(error));
    QCOMPARE(TyporaBase::themeVariable(*resolved, "--highlight-color"), QStringLiteral("#ffffb5c2"));
    QCOMPARE(TyporaBase::highlightText(*resolved), QStringLiteral("#1a1a1a"));
    // A dark highlight needs light text: opaque, translucent over a dark page, and in the other colour forms.
    QCOMPARE(TyporaBase::highlightText(":root { --highlight-color: #2a4d8f }"), QStringLiteral("#ffffff"));
    QCOMPARE(TyporaBase::highlightText(":root { --bg-color: #101418; --highlight-color: #2a4d8fcc }"), QStringLiteral("#ffffff"));
    QCOMPARE(TyporaBase::highlightText(":root { --highlight-color: rgb(40, 60, 90) }"), QStringLiteral("#ffffff"));
    QCOMPARE(TyporaBase::highlightText(":root { --highlight-color: hsl(220, 50%, 25%) }"), QStringLiteral("#ffffff"));
    QCOMPARE(TyporaBase::highlightText(":root { --highlight-color: #04f }"), QStringLiteral("#ffffff"));
    // The page colour is part of the rule: a translucent blue is light over white paper, dark over a dark page.
    QCOMPARE(TyporaBase::highlightText(":root { --highlight-color: #2a4d8f33 }"), QStringLiteral("#1a1a1a"));
    QCOMPARE(TyporaBase::highlightText(":root { --bg-color: #101418; --highlight-color: #2a4d8f66 }"), QStringLiteral("#ffffff"));
    // Light highlights, an unusable value and a transparent one all keep the dark text.
    QCOMPARE(TyporaBase::highlightText(":root { --highlight-color: yellow }"), QStringLiteral("#1a1a1a"));
    QCOMPARE(TyporaBase::highlightText(":root { --highlight-color: oklch(0.2 0.1 250) }"), QStringLiteral("#1a1a1a"));
    QCOMPARE(TyporaBase::highlightText(":root { --highlight-color: transparent }"), QStringLiteral("#1a1a1a"));
    // The shim: <mark> and the published span read the theme's colour, the text colour is never inherited, the
    // editor-only rule is only in the Live text, and the per-theme variable follows the shim.
    const QString shim = TyporaBase::css();
    QVERIFY(shim.contains("mark, .highlight { background: var(--highlight-color, var(--fw-highlight-bg, #fff2a8)); color: var(--fw-highlight-fg, #1a1a1a); }"));
    QVERIFY(!shim.contains(".fw-highlight"));
    QVERIFY(TyporaBase::liveCss().contains("#write.fw-mode-live .fw-highlight { background-color: var(--highlight-color, var(--fw-highlight-bg)); color: var(--fw-highlight-fg); }"));
    QCOMPARE(TyporaBase::highlightCss(":root { --highlight-color: #2a4d8f }"),
        QStringLiteral("/* Typora base shim: text colour for ==highlight==, chosen from the theme's --highlight-color. */\n:root { --fw-highlight-fg: #ffffff; }\n"));
    Backend backend;
    QVERIFY2(backend.selectPublishingTheme("lapis-dark"), qPrintable(backend.status()));
    const QString live = backend.liveThemeCss();
    QVERIFY(live.indexOf(TyporaBase::liveCss()) == 0);
    const qsizetype variable = live.indexOf(":root { --fw-highlight-fg: #1a1a1a; }");
    QVERIFY(variable > live.indexOf(".fw-highlight"));
    QVERIFY(variable < live.indexOf("--highlight-color: #ffffb5c2")); // before the theme text, so the theme can still override it
    backend.discardRecovery();
}

// Web (and PDF, which prints the same page): the computed colours of the highlight in a real Chromium page.
// Red on the old code: the span kept the theme's light text on the pale yellow (about 1.2:1).
void FomawriteTest::publishedHighlightStaysReadableOnDarkThemes() {
    Cycle103SettingsSnapshot settings;
    Cycle140Lapis lapis; QVERIFY(lapis.install());
    const QString folder = PublishingThemes::directory(); QVERIFY(QDir().mkpath(folder));
    const QStringList custom{"cycle140-hl-vars", "cycle140-hl-blue", "cycle140-hl-own"};
    const auto cleanup = qScopeGuard([&] { for (const QString &id : custom) QFile::remove(folder + '/' + id + ".css"); });
    // Dark page with no highlight colour; dark page with a dark-blue one; a theme that styles mark itself, pair and all.
    QVERIFY(cycle132Write(folder + "/cycle140-hl-vars.css", ":root { --bg-color: #1e222a; --text-color: #e4e4e4 }"));
    QVERIFY(cycle132Write(folder + "/cycle140-hl-blue.css", ":root { --bg-color: #1e222a; --text-color: #e4e4e4; --highlight-color: #2a4d8f }"));
    QVERIFY(cycle132Write(folder + "/cycle140-hl-own.css", ":root { --bg-color: #1e222a; --text-color: #e4e4e4; --highlight-color: #2a4d8f }\nmark, .highlight { background: #ffd54a; color: #000 }"));
    Cycle132Editor fixture; QVERIFY(fixture.create());
    fixture.editor->setProperty("text", "# Highlight\n\nSome ==marked words== in a dark page.\n");
    QTest::qWait(300);
    QWebEngineProfile profile;
    QWebEnginePage page(&profile);
    const QStringList themes{"lapis-dark", "cycle140-hl-vars", "cycle140-hl-blue", "cycle140-hl-own", "lapis"};
    for (const QString &id : themes) {
        QVERIFY2(fixture.backend.selectPublishingTheme(id), qPrintable(id + ": " + fixture.backend.status()));
        const auto web = fixture.backend.publishingPreview("html");
        QVERIFY2(web["ok"].toBool(), qPrintable(web["error"].toString()));
        const QString html = cycle140Html(web["url"].toUrl());
        QVERIFY2(html.contains("<span class=\"highlight\">marked words</span>"), qPrintable(id + ": the span is missing"));
        QVERIFY2(html.contains("mark, .highlight { background: var(--highlight-color"), qPrintable(id + ": the shim rule is missing"));
        QVERIFY2(html.indexOf(":root { --fw-highlight-fg:") > html.indexOf("mark, .highlight {"), "the colour variable follows the shim");
        bool loaded = false; perfLoadInPage(page, web["url"].toUrl(), &loaded); QVERIFY(loaded);
        const QStringList fields = cycle140HighlightInPage(page);
        double ratio = 0;
        cycle140CheckHighlightFields(fields, "Web " + id, 4.5, &ratio);
        if (QTest::currentTestFailed()) return;
        qInfo("highlight contrast Web %s: %s on %s over %s = %.2f:1", qPrintable(id), qPrintable(fields[0]), qPrintable(fields[1]), qPrintable(fields[2]), ratio);
        if (id == "cycle140-hl-blue") QVERIFY2(cycle140Computed(fields[0]).red() > 200, "a dark highlight takes light text"); // the rule, not luck
        if (id == "lapis-dark") QVERIFY2(cycle140Computed(fields[0]).red() < 60, "a pale highlight keeps dark text on a dark page");
    }
    // A basic preset (no variables, light text colours of its own): the editor's pale yellow with dark text.
    fixture.backend.setOutputStyle(0);
    const auto basic = fixture.backend.publishingPreview("html");
    QVERIFY2(basic["ok"].toBool(), qPrintable(basic["error"].toString()));
    bool loaded = false; perfLoadInPage(page, basic["url"].toUrl(), &loaded); QVERIFY(loaded);
    cycle140CheckHighlightFields(cycle140HighlightInPage(page), "Web basic preset");
    fixture.backend.discardRecovery();
}

// Live: `==text==` renders as a highlight and stays readable (filtered, exact, dark editor, no variable, basic preset).
// Red on the old code: the page had no highlight at all, only the raw `==marked words==` text.
void FomawriteTest::liveHighlightStaysReadableOnDarkThemes() {
    Cycle103SettingsSnapshot settings;
    Cycle140Lapis lapis; QVERIFY(lapis.install());
    const QString folder = PublishingThemes::directory(); QVERIFY(QDir().mkpath(folder));
    const QStringList custom{"cycle140-live-vars", "cycle140-live-blue", "cycle140-live-inherit"};
    const auto cleanup = qScopeGuard([&] { for (const QString &id : custom) QFile::remove(folder + '/' + id + ".css"); });
    QVERIFY(cycle132Write(folder + "/cycle140-live-vars.css", ":root { --bg-color: #1e222a; --text-color: #e4e4e4 }"));
    QVERIFY(cycle132Write(folder + "/cycle140-live-blue.css", ":root { --bg-color: #1e222a; --text-color: #e4e4e4; --highlight-color: #2a4d8f }"));
    // Lapis (light) writes `mark { color: inherit }`: on a dark page that would put the page's light text on the pale yellow.
    QVERIFY(cycle132Write(folder + "/cycle140-live-inherit.css", ":root { --bg-color: #1e222a; --text-color: #e4e4e4; --highlight-color: #ffffb5c2 }\nmark { background: var(--highlight-color); color: inherit }"));
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    page.window->resize(960, 700);
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    page.editor->setProperty("text", QStringLiteral("# Dark Lapis\n\nSome ==marked words== and `code` here.\n"));
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
    // Parks the caret at the end of the document (no highlight is revealed), reads the highlight, checks its contrast.
    const auto probe = [&](const QString &where, double *ratio = nullptr) -> QStringList {
        bool ok = false;
        const QString script = QStringLiteral(
            "(function(){ const v = window.fomawriteEditor.view; v.dispatch({selection: {anchor: v.state.doc.length}});"
            " const r = %1; window.fomawriteEditor.simulateUserChanges(JSON.stringify([{from: 0, to: 0, insert: '@@' + r + '@@'}])); })()")
            .arg(QString::fromLatin1(cycle140HighlightScript));
        QTest::qWait(300);
        const QStringList fields = cycle140Probe(page, view, script, &ok).split('|');
        if (!ok) { QTest::qFail(qPrintable(where + ": the probe did not report"), __FILE__, __LINE__); return {}; }
        cycle140CheckHighlightFields(fields, where, 4.5, ratio);
        return fields;
    };
    double ratio = 0;
    QStringList fields = probe("Live lapis-dark, filtered", &ratio);
    if (QTest::currentTestFailed()) return;
    qInfo("highlight contrast Live lapis-dark filtered: %s on %s over %s = %.2f:1", qPrintable(fields[0]), qPrintable(fields[1]), qPrintable(fields[2]), ratio);
    QVERIFY2(cycle140Computed(fields[0]).red() < 60, qPrintable("dark text on the pale highlight, got " + fields[0]));
    QCOMPARE(fields[4], QStringLiteral("marked words")); // the == markers are hidden: only the words are in the element
    appearance(false, false);
    probe("Live lapis-dark, exact");
    if (QTest::currentTestFailed()) return;
    appearance(true, true);
    probe("Live lapis-dark, dark editor");
    if (QTest::currentTestFailed()) return;
    // A dark highlight colour takes light text (the ground is the theme's; the text follows the rule); no colour: pale yellow, dark text.
    for (const QString &id : {QStringLiteral("cycle140-live-blue"), QStringLiteral("cycle140-live-vars"), QStringLiteral("cycle140-live-inherit")}) {
        QVERIFY2(page.backend.selectPublishingTheme(id), qPrintable(page.backend.status()));
        page.backend.pushLiveTheme();
        appearance(true, false);
        fields = probe("Live " + id, &ratio);
        if (QTest::currentTestFailed()) return;
        qInfo("highlight contrast Live %s: %s on %s over %s = %.2f:1", qPrintable(id), qPrintable(fields[0]), qPrintable(fields[1]), qPrintable(fields[2]), ratio);
        if (id == "cycle140-live-blue") QVERIFY2(cycle140Computed(fields[0]).red() > 200, qPrintable("light text on the dark highlight, got " + fields[0]));
        else QVERIFY2(cycle140Computed(fields[0]).red() < 60, qPrintable("no highlight colour, or a theme `mark` that inherits the text colour: dark text on pale yellow, got " + fields[0]));
        appearance(true, true);
        probe("Live " + id + ", dark editor");
        if (QTest::currentTestFailed()) return;
    }
    // A basic preset (no variables at all), light and dark editor.
    page.backend.setOutputStyle(0);
    page.backend.pushLiveTheme();
    for (const bool dark : {false, true}) {
        appearance(true, dark);
        probe(dark ? "Live basic preset, dark editor" : "Live basic preset");
        if (QTest::currentTestFailed()) return;
    }
    page.backend.discardRecovery();
}
