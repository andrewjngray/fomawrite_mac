// Cycle152 — the Omawrite Output Style (src/themes/omawrite.css), the second built-in theme
// next to Claude Like. IBM Plex Mono at one size, 1.82 line height, a 65-character measure,
// and a light and a dark variant: light on :root (so Web, PDF and print are always light),
// dark under html.dark (the Live page adds the class when the app is dark). The font is
// embedded as data: URLs when the theme loads, so it works in Live, Preview and both exports
// without being installed.

namespace {
// Evaluates `script` in the page's own world and returns its answer (a string), or a null QVariant.
QVariant cycle152PageResult(QObject *view, const QString &script, int timeoutMs = 4000) {
    auto *engine = qmlEngine(view);
    if (!engine) return {};
    engine->globalObject().setProperty("cycle152PageResult", QJSValue(QJSValue::UndefinedValue));
    const QJSValue callback = engine->evaluate("(function(value) { cycle152PageResult = value; })");
    if (!QMetaObject::invokeMethod(view, "runJavaScript", Q_ARG(QString, script), Q_ARG(uint, 0), Q_ARG(QJSValue, callback))) return {};
    QElapsedTimer timer; timer.start();
    while (engine->globalObject().property("cycle152PageResult").isUndefined() && timer.elapsed() < timeoutMs) QTest::qWait(10);
    return engine->globalObject().property("cycle152PageResult").toVariant();
}

// A plain browser window for an exported file (what Preview shows and what a reader opens).
struct Cycle152Browser {
    QQmlEngine engine;
    QScopedPointer<QObject> root;
    QObject *view = nullptr;
    bool create(const QUrl &url, int width, int height) {
        QQmlComponent component(&engine);
        component.setData(QStringLiteral("import QtQuick\nimport QtQuick.Window\nimport QtWebEngine\n"
            "Window { width: %1; height: %2; visible: true\n"
            "  WebEngineView { objectName: \"exportView\"; anchors.fill: parent; settings.javascriptEnabled: true } }").arg(width).arg(height).toUtf8(), QUrl());
        root.reset(component.create());
        if (!root) return false;
        view = root->findChild<QObject *>("exportView");
        if (!view) return false;
        view->setProperty("url", url);
        return true;
    }
};

// One round trip: the computed facts the tests compare, as "a|b|c" (see the order at each use).
const char *const cycle152FontProbe = R"JS(
(function() {
    const faces = Array.from(document.fonts).filter(function(f) { return f.family.indexOf('IBM Plex Mono') >= 0; });
    const state = function(weight, style) {
        const f = faces.filter(function(x) { return x.weight == weight && x.style == style; })[0];
        return f ? f.status : 'none';
    };
    return JSON.stringify({ faces: faces.length, regular: state('400', 'normal'), italic: state('400', 'italic'),
                            bold: state('700', 'normal'), boldItalic: state('700', 'italic') });
})()
)JS";

QJsonObject cycle152Object(const QVariant &answer) { return QJsonDocument::fromJson(answer.toString().toUtf8()).object(); }

QString cycle152Rgb(int r, int g, int b) { return QStringLiteral("rgb(%1, %2, %3)").arg(r).arg(g).arg(b); }
}

// The catalog, the CSS and the file-name rule.
void FomawriteTest::omawriteIsASecondBuiltInOutputStyle() {
    Cycle103SettingsSnapshot settings;
    const QVariantList catalog = PublishingThemes::catalog();
    QVERIFY(catalog.size() >= 2);
    QCOMPARE(catalog.at(0).toMap()["id"].toString(), QStringLiteral("claude-like"));
    QCOMPARE(catalog.at(0).toMap()["name"].toString(), QStringLiteral("Claude Like"));
    QCOMPARE(catalog.at(1).toMap()["id"].toString(), QStringLiteral("omawrite"));
    QCOMPARE(catalog.at(1).toMap()["name"].toString(), QStringLiteral("Omawrite"));
    QVERIFY(!catalog.at(0).toMap()["imported"].toBool());
    QVERIFY(!catalog.at(1).toMap()["imported"].toBool());

    QString error, advisory;
    const auto css = PublishingThemes::css(QStringLiteral("omawrite"), &error, &advisory);
    QVERIFY2(css, qPrintable(error));
    QVERIFY2(advisory.isEmpty(), qPrintable("advisory: " + advisory)); // no missing-font notice: the fonts ship in the app
    // The font is inside the CSS (works offline, in Live, Preview, PDF and exported HTML) as four faces.
    QCOMPARE(css->count(QStringLiteral("data:font/woff2;base64,")), 4);
    QVERIFY(css->contains(QStringLiteral("font-family: \"IBM Plex Mono\"")));
    QVERIFY(!css->contains(QStringLiteral("qrc:")));
    QVERIFY(!css->contains(QStringLiteral("url(\"fonts/")));
    QVERIFY(!css->contains(QStringLiteral("@import")));
    // Light on :root, dark under html.dark.
    QVERIFY(css->contains(QStringLiteral(":root {")) && css->contains(QStringLiteral("html.dark {")));
    QVERIFY(css->indexOf(QStringLiteral("html.dark {")) > css->indexOf(QStringLiteral(":root {")));
    // The host reads the :root variables: the light page colours and the dark twin.
    const auto variables = TyporaBase::themeVariables(*css);
    QCOMPARE(variables.value("--bg-color"), QStringLiteral("#ffffff"));
    QCOMPARE(variables.value("--text-color"), QStringLiteral("#222324"));
    QCOMPARE(variables.value("--primary-color"), QStringLiteral("#2077b2"));
    QCOMPARE(variables.value("--dark-bg-color"), QStringLiteral("#101010"));
    QCOMPARE(variables.value("--dark-text-color"), QStringLiteral("#eeeeee"));
    // Claude Like is unchanged by the second theme.
    const auto claude = PublishingThemes::css(QStringLiteral("claude-like"), &error, &advisory);
    QVERIFY2(claude, qPrintable(error)); QVERIFY(advisory.isEmpty());
    QVERIFY(!claude->contains(QStringLiteral("IBM Plex Mono")));
    QVERIFY(!PublishingThemes::css(QStringLiteral("omawrite-nope"), &error)); QVERIFY(!error.isEmpty());

    // A theme file called omawrite.css never takes the built-in id, like claude-like.css.
    const QString folder = PublishingThemes::directory(); QVERIFY(QDir().mkpath(folder));
    const QString reserved = folder + "/omawrite.css";
    const bool create = !QFileInfo::exists(reserved);
    QString importedId;
    const auto cleanup = qScopeGuard([&] {
        if (create) QFile::remove(reserved);
        if (!importedId.isEmpty()) QFile::remove(folder + '/' + importedId + ".css");
    });
    if (create) QVERIFY(cycle132Write(reserved, "#write{color:#123456}"));
    bool folderEntry = false;
    int builtIns = 0;
    for (const auto &entry : PublishingThemes::catalog()) {
        const auto item = entry.toMap();
        if (item["id"].toString() == "omawrite") ++builtIns;
        if (item["id"].toString() == "file:omawrite.css") { folderEntry = true; QCOMPARE(item["name"].toString(), QStringLiteral("Omawrite (Folder)")); }
    }
    QCOMPARE(builtIns, 1); QVERIFY(folderEntry);
    const auto fromFolder = PublishingThemes::css(QStringLiteral("file:omawrite.css"), &error);
    QVERIFY2(fromFolder, qPrintable(error));
    QVERIFY(fromFolder->contains(QStringLiteral("#123456")) || !create);
    QVERIFY(!fromFolder->contains(QStringLiteral("IBM Plex Mono")) || !create);
    // Importing it files it under a hashed id, as every import is; the built-in id stays the built-in.
    QVERIFY2(PublishingThemes::importTheme(QUrl::fromLocalFile(reserved), &importedId, &error), qPrintable(error));
    QVERIFY(importedId.startsWith(QStringLiteral("omawrite-")) && importedId != QStringLiteral("omawrite"));
    const auto builtIn = PublishingThemes::css(QStringLiteral("omawrite"), &error);
    QVERIFY2(builtIn && builtIn->contains(QStringLiteral("IBM Plex Mono")), "the built-in theme must not be shadowed");
}

// Appearance Follows Output Style: Omawrite follows the app's light/dark instead of deciding it.
void FomawriteTest::omawriteWindowFollowsTheAppsLightAndDarkMode() {
    Cycle103SettingsSnapshot settings;
    Backend backend;
    QVERIFY(backend.appearanceFollowsOutputStyle());
    backend.setThemePreset("light");
    QVERIFY2(backend.selectPublishingTheme("omawrite"), qPrintable(backend.status()));
    QTRY_VERIFY(!backend.darkMode());
    QCOMPARE(backend.palette().value("page").toString().toLower(), QStringLiteral("#ffffff"));
    QCOMPARE(backend.palette().value("text").toString().toLower(), QStringLiteral("#222324"));
    backend.setThemePreset("dark");
    QTRY_VERIFY(backend.darkMode());
    QCOMPARE(backend.palette().value("page").toString().toLower(), QStringLiteral("#101010"));
    QCOMPARE(backend.palette().value("editor").toString().toLower(), QStringLiteral("#101010"));
    QCOMPARE(backend.palette().value("text").toString().toLower(), QStringLiteral("#eeeeee"));
    // Back to light.
    backend.setThemePreset("light");
    QTRY_VERIFY(!backend.darkMode());
    QCOMPARE(backend.palette().value("page").toString().toLower(), QStringLiteral("#ffffff"));
    // A light-only theme still decides by itself (unchanged): Claude Like keeps a dark preset light.
    backend.setThemePreset("dark");
    QVERIFY2(backend.selectPublishingTheme("claude-like"), qPrintable(backend.status()));
    QTRY_VERIFY(!backend.darkMode());
    // Following off: the preset rules and the theme paints nothing on the window.
    QVERIFY2(backend.selectPublishingTheme("omawrite"), qPrintable(backend.status()));
    QTRY_VERIFY(backend.darkMode());
    backend.setAppearanceFollowsOutputStyle(false);
    QVERIFY(backend.darkMode());
    QVERIFY(backend.palette().value("page").toString().toLower() != QStringLiteral("#101010"));
    backend.discardRecovery();
}

// Both Output Style menus list Omawrite and applying it from either selects it.
void FomawriteTest::omawriteShowsInBothOutputStyleMenusAndApplies() {
    Backend backend;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create());
    QVERIFY2(window, qPrintable(component.errorString()));
    const int originalStyle = backend.outputStyle();
    const QString originalTheme = backend.publishingThemeId();
    const auto cleanup = qScopeGuard([&] {
        if (originalTheme.isEmpty()) backend.setOutputStyle(originalStyle); else backend.selectPublishingTheme(originalTheme);
        backend.discardRecovery();
    });
    auto *nativeThemes = window->findChild<QObject *>("nativeCustomThemesMenu");
    auto *previewThemes = window->findChild<QObject *>("previewCustomThemesMenu");
    QVERIFY(testDesktopMenusMatchPlatform(window.data()));
    const bool mac = window->property("isMac").toBool();
    QVERIFY(previewThemes);
    if (mac) QVERIFY(nativeThemes);
    else QVERIFY(!nativeThemes); // Output Style desktop submenu is an M2 omission.
    QObject *nativeOmawrite = nullptr, *nativeClaude = nullptr;
    if (mac) {
    // The native menu.
    QQmlListReference nativeItems(nativeThemes, "items");
    QVERIFY(nativeItems.isValid());
    qsizetype claudeAt = -1, omawriteAt = -1;
    for (qsizetype i = 0; i < nativeItems.count(); ++i) {
        const QString name = nativeItems.at(i)->objectName();
        if (name == "nativePublishingTheme_claude-like") { nativeClaude = nativeItems.at(i); claudeAt = i; }
        if (name == "nativePublishingTheme_omawrite") { nativeOmawrite = nativeItems.at(i); omawriteAt = i; }
    }
    QVERIFY(nativeClaude && nativeOmawrite);
    QCOMPARE(nativeClaude->property("text").toString(), QStringLiteral("Claude Like"));
    QCOMPARE(nativeOmawrite->property("text").toString(), QStringLiteral("Omawrite"));
    QCOMPARE(omawriteAt, claudeAt + 1);
    }

    // The in-app menu.
    QQuickItem *previewOmawrite = nullptr, *previewClaude = nullptr;
    const int previewCount = previewThemes->property("count").toInt();
    QCOMPARE(previewCount, backend.publishingThemes().size());
    for (int i = 0; i < previewCount; ++i) {
        QQuickItem *item = nullptr;
        QVERIFY(QMetaObject::invokeMethod(previewThemes, "itemAt", Q_RETURN_ARG(QQuickItem *, item), Q_ARG(int, i)));
        if (!item) continue;
        if (item->objectName() == "previewPublishingTheme_claude-like") previewClaude = item;
        if (item->objectName() == "previewPublishingTheme_omawrite") previewOmawrite = item;
    }
    QVERIFY(previewClaude && previewOmawrite);
    QCOMPARE(previewClaude->property("text").toString(), QStringLiteral("Claude Like"));
    QCOMPARE(previewOmawrite->property("text").toString(), QStringLiteral("Omawrite"));

    // Selecting applies it, and each menu shows the check.
    backend.selectPublishingTheme("claude-like");
    QVERIFY(QMetaObject::invokeMethod(mac ? nativeOmawrite : previewOmawrite, "triggered"));
    QTRY_COMPARE(backend.publishingThemeId(), QStringLiteral("omawrite"));
    QCOMPARE(backend.publishingThemeName(), QStringLiteral("Omawrite"));
    if (mac) {
        QTRY_VERIFY(nativeOmawrite->property("checked").toBool());
        QVERIFY(!nativeClaude->property("checked").toBool());
    }
    QTRY_VERIFY(previewOmawrite->property("checked").toBool());
    QVERIFY(!previewClaude->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(previewClaude, "triggered"));
    QTRY_COMPARE(backend.publishingThemeId(), QStringLiteral("claude-like"));
    // Live's text size: Claude Like follows the Source preset's size; Omawrite sets 20 px (x zoom and text scale).
    QTRY_COMPARE(window->property("liveFontPixelSize").toInt(), window->property("editorFontPixelSize").toInt());
    QTRY_VERIFY(previewClaude->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(previewOmawrite, "triggered"));
    QTRY_COMPARE(backend.publishingThemeId(), QStringLiteral("omawrite"));
    QVariant twenty;
    QVERIFY(QMetaObject::invokeMethod(window.data(), "scaledSize", Q_RETURN_ARG(QVariant, twenty), Q_ARG(QVariant, 20)));
    QTRY_COMPARE(window->property("liveFontPixelSize").toInt(), twenty.toInt());
    if (mac) QTRY_VERIFY(nativeOmawrite->property("checked").toBool());
    QTRY_VERIFY(previewOmawrite->property("checked").toBool());
    // The theme error line stays empty: nothing is missing.
    QVERIFY2(backend.publishingThemeError().isEmpty(), qPrintable(backend.publishingThemeError()));
}

// The Export hub's Style list names the themes from the catalog.
void FomawriteTest::omawriteIsListedInTheExportHubStyleChoice() {
    Backend backend;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
    QScopedPointer<QObject> window(component.create());
    QVERIFY2(window, qPrintable(component.errorString()));
    const auto cleanup = qScopeGuard([&] { backend.discardRecovery(); });
    QVERIFY(QMetaObject::invokeMethod(window.data(), "openExportHub", Q_ARG(QVariant, QStringLiteral("html"))));
    QObject *choice = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((choice = window->findChild<QObject *>("exportPublishingThemeChoice")) != nullptr, 5000);
    const QVariantList model = choice->property("model").toList();
    QStringList names;
    for (const QVariant &entry : model) names << entry.toMap()["name"].toString();
    const qsizetype claude = names.indexOf(QStringLiteral("Claude Like"));
    QVERIFY2(claude >= 0, qPrintable(names.join(" | ")));
    QCOMPARE(names.value(claude + 1), QStringLiteral("Omawrite"));
    QCOMPARE(model.value(claude + 1).toMap()["id"].toString(), QStringLiteral("omawrite"));
}

// The real Live page: Plex Mono resolves and loads, the line height and colours follow the table
// in the light and the dark app.
void FomawriteTest::liveOmawriteLoadsPlexMonoAndFollowsLightAndDark() {
    Cycle103SettingsSnapshot settings;
    Cycle134Page page; QVERIFY2(page.create(), qPrintable(page.error));
    page.window->resize(1400, 800);
    QTRY_VERIFY_WITH_TIMEOUT(page.bridge()->isReady(), 15000);
    page.editor->setProperty("text", QStringLiteral("# Heading\n\nPlain words with a [link](https://example.com) in them.\n\n> Quoted words\n\nLast `code` line.\n"));
    QVERIFY2(page.backend.selectPublishingTheme("omawrite"), qPrintable(page.backend.status()));
    page.backend.syncLiveEditor(0);
    const auto appearance = [&](bool dark, int size) {
        page.bridge()->applyAppearance(QStringLiteral("{\"appearance\":\"manuscript\",\"dark\":%1,\"fontSize\":%2,\"liveThemeFilter\":true,\"palette\":{\"background\":\"%3\",\"text\":\"%4\"}}")
            .arg(dark ? "true" : "false").arg(size).arg(dark ? "#101010" : "#ffffff").arg(dark ? "#eeeeee" : "#222324"));
    };
    appearance(false, 20);
    page.bridge()->selectMode("live");
    QTRY_COMPARE_WITH_TIMEOUT(page.pageText(), page.canonical(), 10000);
    auto *view = page.root->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    const char *const probe = R"JS(
(function() {
    const cs = function(e) { return getComputedStyle(e); };
    const w = document.getElementById('write');
    const lines = Array.from(document.querySelectorAll('#editor .cm-line'));
    const find = function(text) { return lines.filter(function(l) { return l.textContent.indexOf(text) >= 0; })[0]; };
    const para = find('Plain words'), head = lines.filter(function(l) { return l.classList.contains('fw-h1'); })[0];
    const link = document.querySelector('.fw-live-link'), quote = lines.filter(function(l) { return l.classList.contains('fw-quote-line'); })[0];
    const faces = Array.from(document.fonts).filter(function(f) { return f.family.indexOf('IBM Plex Mono') >= 0; });
    const status = function(weight, style) { const f = faces.filter(function(x) { return x.weight == weight && x.style == style; })[0]; return f ? f.status : 'none'; };
    return JSON.stringify({
        writeFont: cs(w).fontFamily, writeSize: cs(w).fontSize, paraSize: para ? cs(para).fontSize : '', paraLine: para ? cs(para).lineHeight : '',
        paraFont: para ? cs(para).fontFamily : '', headSize: head ? cs(head).fontSize : '', headWeight: head ? cs(head).fontWeight : '', headColor: head ? cs(head).color : '',
        bodyBg: cs(document.body).backgroundColor, htmlBg: cs(document.documentElement).backgroundColor, paraColor: para ? cs(para).color : '',
        linkColor: link ? cs(link).color : '', linkLine: link ? cs(link).textDecorationLine : '', quoteColor: quote ? cs(quote).color : '',
        htmlDark: document.documentElement.classList.contains('dark'), fillBg: w.classList.contains('fw-live-fill-bg'), fillFg: w.classList.contains('fw-live-fill-fg'),
        selectionVar: cs(document.documentElement).getPropertyValue('--select-text-bg-color').trim(),
        faces: faces.length, regular: status('400', 'normal'), bold: status('700', 'normal'),
        loadedPlex: document.fonts.check('20px "IBM Plex Mono"'),
        paraWidth: para ? para.getBoundingClientRect().width : 0
    });
})()
)JS";
    QJsonObject facts;
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        facts = cycle152Object(cycle152PageResult(view, QString::fromLatin1(probe)));
        return facts.value("regular").toString() == "loaded" && facts.value("bold").toString() == "loaded";
    })(), 15000);
    // The face resolves to Plex Mono and the document.fonts API reports the faces loaded (from data: URLs).
    QVERIFY2(facts.value("writeFont").toString().contains("IBM Plex Mono"), qPrintable(facts.value("writeFont").toString()));
    QVERIFY2(facts.value("paraFont").toString().contains("IBM Plex Mono"), qPrintable(facts.value("paraFont").toString()));
    QVERIFY(facts.value("faces").toInt() >= 4); // the four faces (the page also keeps Live's mapped copy of the theme)
    QVERIFY(facts.value("loadedPlex").toBool());
    // Same size for headings and body, bold headings; line height 1.82 x the size.
    QCOMPARE(facts.value("paraSize").toString(), QStringLiteral("20px"));
    QCOMPARE(facts.value("headSize").toString(), QStringLiteral("20px"));
    QCOMPARE(facts.value("headWeight").toString(), QStringLiteral("700"));
    const double paraLine = facts.value("paraLine").toString().chopped(2).toDouble();
    QVERIFY2(qAbs(paraLine - 20 * 1.82) < 0.2, qPrintable(facts.value("paraLine").toString()));
    // Light colours: page, text, link (underlined), headings.
    QCOMPARE(facts.value("htmlDark").toBool(), false);
    QCOMPARE(facts.value("bodyBg").toString(), cycle152Rgb(255, 255, 255));
    QCOMPARE(facts.value("paraColor").toString(), cycle152Rgb(34, 35, 36));
    QCOMPARE(facts.value("headColor").toString(), cycle152Rgb(34, 35, 36));
    QCOMPARE(facts.value("linkColor").toString(), cycle152Rgb(32, 119, 178));
    QVERIFY(facts.value("linkLine").toString().contains("underline"));
    QCOMPARE(facts.value("selectionVar").toString(), QStringLiteral("#2077b2"));
    // The theme paints the page itself: the readability fill never repaints the text.
    QCOMPARE(facts.value("fillFg").toBool(), false);
    // Live's column is the editor's own measure (760 px, less its padding); the theme's 65 characters is the Preview/export measure.
    QVERIFY(facts.value("paraWidth").toDouble() <= 760.0);

    // The dark app: the html.dark variant.
    appearance(true, 20);
    QTRY_COMPARE_WITH_TIMEOUT(cycle152Object(cycle152PageResult(view, QString::fromLatin1(probe))).value("bodyBg").toString(), cycle152Rgb(16, 16, 16), 5000);
    facts = cycle152Object(cycle152PageResult(view, QString::fromLatin1(probe)));
    QCOMPARE(facts.value("htmlDark").toBool(), true);
    QCOMPARE(facts.value("htmlBg").toString(), cycle152Rgb(16, 16, 16));
    QCOMPARE(facts.value("paraColor").toString(), cycle152Rgb(238, 238, 238));
    QCOMPARE(facts.value("headColor").toString(), cycle152Rgb(238, 238, 238));
    QCOMPARE(facts.value("linkColor").toString(), cycle152Rgb(85, 132, 170));
    QCOMPARE(facts.value("selectionVar").toString(), QStringLiteral("#186a9a"));
    QCOMPARE(facts.value("fillBg").toBool(), false);
    QCOMPARE(facts.value("fillFg").toBool(), false);
    QVERIFY(facts.value("paraFont").toString().contains("IBM Plex Mono"));
    QCOMPARE(facts.value("paraSize").toString(), QStringLiteral("20px"));

    // The host's text size beats the theme's 20 px in Live (fw-host-size); the line height stays 1.82 x the size.
    appearance(false, 17);
    QTRY_COMPARE_WITH_TIMEOUT(cycle152Object(cycle152PageResult(view, QString::fromLatin1(probe))).value("paraSize").toString(), QStringLiteral("17px"), 5000);
    facts = cycle152Object(cycle152PageResult(view, QString::fromLatin1(probe)));
    QVERIFY2(qAbs(facts.value("paraLine").toString().chopped(2).toDouble() - 17 * 1.82) < 0.2, qPrintable(facts.value("paraLine").toString()));
    QCOMPARE(facts.value("headSize").toString(), QStringLiteral("17px"));
    QCOMPARE(facts.value("bodyBg").toString(), cycle152Rgb(255, 255, 255));
    page.backend.discardRecovery();
}

// Web export: the light variant whatever the app's mode, the font inside the file, and the file in a
// real browser page loads the font and sets the 65-character measure.
void FomawriteTest::omawriteHtmlExportIsLightEmbedsPlexMonoAndLoadsIt() {
    Cycle103SettingsSnapshot settings;
    Cycle132Editor fixture; QVERIFY(fixture.create());
    fixture.editor->setProperty("text", "# Omawrite\n\nA paragraph with a [link](https://example.com), **bold** and *italic*.\n\n> A quote\n\n```\ncode\n```\n");
    QTest::qWait(300);
    fixture.backend.setThemePreset("dark"); // a dark app must not darken the export
    QVERIFY2(fixture.backend.selectPublishingTheme("omawrite"), qPrintable(fixture.backend.status()));
    QTRY_VERIFY(fixture.backend.darkMode());
    const auto web = fixture.backend.publishingPreview("html");
    QVERIFY2(web["ok"].toBool(), qPrintable(web["error"].toString()));
    const QString html = cycle140Html(web["url"].toUrl());
    QCOMPARE(html.count(QStringLiteral("data:font/woff2;base64,")), 4);
    QVERIFY(html.contains(QStringLiteral("font-src data:")));      // the export's own policy: data: only
    QVERIFY(!html.contains(QStringLiteral("qrc:")));
    QVERIFY(html.contains(QStringLiteral("--bg-color: #ffffff")));
    QVERIFY(html.contains(QStringLiteral("--text-color: #222324")));
    QVERIFY(html.contains(QStringLiteral("--primary-color: #2077b2")));
    QVERIFY(html.contains(QStringLiteral("line-height: var(--line)")) && html.contains(QStringLiteral("--line: 1.82")));
    QVERIFY(html.contains(QStringLiteral("<html>")));              // nothing sets class "dark" on an export
    QVERIFY(!html.contains(QStringLiteral("class=\"dark\"")));
    QVERIFY(html.contains(QStringLiteral("<article id=\"write\">")));

    // The exported file in a browser: light colours, Plex Mono loaded, a 65-character measure.
    const char *const probe = R"JS(
(function() {
    const cs = function(e) { return getComputedStyle(e); };
    const w = document.getElementById('write');
    if (!w) return '{}';
    const p = w.querySelector('p'), h = w.querySelector('h1'), a = w.querySelector('a');
    const faces = Array.from(document.fonts).filter(function(f) { return f.family.indexOf('IBM Plex Mono') >= 0; });
    const status = function(weight, style) { const f = faces.filter(function(x) { return x.weight == weight && x.style == style; })[0]; return f ? f.status : 'none'; };
    const probeSpan = document.createElement('span'); probeSpan.textContent = '0'.repeat(10); probeSpan.style.cssText = 'position:absolute;visibility:hidden;white-space:pre';
    p.appendChild(probeSpan); const charWidth = probeSpan.getBoundingClientRect().width / 10; p.removeChild(probeSpan);
    return JSON.stringify({ font: cs(p).fontFamily, size: cs(p).fontSize, headSize: cs(h).fontSize, headWeight: cs(h).fontWeight, line: cs(p).lineHeight,
        bg: cs(document.body).backgroundColor, color: cs(p).color, link: cs(a).color, linkLine: cs(a).textDecorationLine,
        regular: status('400', 'normal'), bold: status('700', 'normal'), italic: status('400', 'italic'), boldItalic: status('700', 'italic'),
        loadedPlex: document.fonts.check('20px "IBM Plex Mono"'), width: w.getBoundingClientRect().width, charWidth: charWidth,
        em: p.querySelector('em') ? cs(p.querySelector('em')).fontStyle : '', strong: p.querySelector('strong') ? cs(p.querySelector('strong')).fontWeight : '',
        dark: document.documentElement.classList.contains('dark') });
})()
)JS";
    Cycle152Browser browser; QVERIFY(browser.create(web["url"].toUrl(), 1400, 900));
    QJsonObject facts;
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        facts = cycle152Object(cycle152PageResult(browser.view, QString::fromLatin1(probe)));
        return facts.value("regular").toString() == "loaded" && facts.value("bold").toString() == "loaded";
    })(), 20000);
    QVERIFY(facts.value("font").toString().contains("IBM Plex Mono"));
    QVERIFY(facts.value("loadedPlex").toBool());
    QCOMPARE(facts.value("size").toString(), QStringLiteral("20px"));
    QCOMPARE(facts.value("headSize").toString(), QStringLiteral("20px"));
    QCOMPARE(facts.value("headWeight").toString(), QStringLiteral("700"));
    QCOMPARE(facts.value("strong").toString(), QStringLiteral("700"));
    QCOMPARE(facts.value("em").toString(), QStringLiteral("italic"));
    QVERIFY2(qAbs(facts.value("line").toString().chopped(2).toDouble() - 20 * 1.82) < 0.2, qPrintable(facts.value("line").toString()));
    QCOMPARE(facts.value("bg").toString(), cycle152Rgb(255, 255, 255));
    QCOMPARE(facts.value("color").toString(), cycle152Rgb(34, 35, 36));
    QCOMPARE(facts.value("link").toString(), cycle152Rgb(32, 119, 178));
    QVERIFY(facts.value("linkLine").toString().contains("underline"));
    QCOMPARE(facts.value("dark").toBool(), false);
    // At most 65 characters: the measure is 65 x the character width; the Plex Mono advance is 0.6 em, i.e. 12 px.
    QVERIFY2(qAbs(facts.value("charWidth").toDouble() - 12.0) < 0.05, qPrintable(QString::number(facts.value("charWidth").toDouble())));
    QVERIFY2(facts.value("width").toDouble() <= 65 * facts.value("charWidth").toDouble() + 0.5, qPrintable(QString::number(facts.value("width").toDouble())));
    QVERIFY2(facts.value("width").toDouble() >= 65 * facts.value("charWidth").toDouble() - 0.5, qPrintable(QString::number(facts.value("width").toDouble())));
    // A narrow window: twenty characters narrower than the window.
    qobject_cast<QQuickWindow *>(browser.root.data())->resize(800, 900);
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        facts = cycle152Object(cycle152PageResult(browser.view, QString::fromLatin1(probe)));
        return qAbs(facts.value("width").toDouble() - (800 - 20 * 12)) < 20; // the scroll bar may take a few pixels
    })(), 10000);
    fixture.backend.discardRecovery();
}

// PDF output: light paper, and the Plex Mono faces are embedded in the file.
void FomawriteTest::omawritePdfExportIsLightAndEmbedsPlexMono() {
    Cycle103SettingsSnapshot settings;
    Cycle132Editor fixture; QVERIFY(fixture.create());
    fixture.editor->setProperty("text", "# Omawrite\n\nA paragraph with **bold** and *italic* words in Plex Mono.\n");
    QTest::qWait(300);
    fixture.backend.setThemePreset("dark");
    QVERIFY2(fixture.backend.selectPublishingTheme("omawrite"), qPrintable(fixture.backend.status()));
    const auto pdf = fixture.backend.publishingPreview("pdf");
    QVERIFY2(pdf["ok"].toBool(), qPrintable(pdf["error"].toString()));
    QPdfDocument document; QCOMPARE(document.load(pdf["url"].toUrl().toLocalFile()), QPdfDocument::Error::None);
    QVERIFY(document.pageCount() >= 1);
    const auto size = document.pagePointSize(0);
    const QImage page = document.render(0, QSize(int(size.width()), int(size.height())));
    QVERIFY(!page.isNull());
    const QColor ground = page.pixelColor(page.width() / 2, page.height() * 9 / 10);
    QVERIFY2(cycle140Luma(ground) > 240, qPrintable("PDF page ground is " + ground.name()));
    QFile file(pdf["url"].toUrl().toLocalFile()); QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY2(file.readAll().contains("IBMPlexMono"), "the PDF does not embed Plex Mono");
    fixture.backend.discardRecovery();
}
