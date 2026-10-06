// Cycle132 — publishing robustness. Unrelated theme-folder changes must not
// cancel renders, the selected theme's CSS is computed once per folder state,
// the themes folder is never recreated by a refresh, failures keep the last
// readable output, and Web mode completes even when the URL is unchanged.

namespace {
struct Cycle132Editor {
    Backend backend;
    QQmlEngine engine;
    QScopedPointer<QObject> editor;
    bool create() {
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
        editor.reset(component.create());
        return !editor.isNull();
    }
};
QByteArray cycle132Read(const QUrl &url) {
    QFile file(url.toLocalFile());
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray("<unreadable>");
}
bool cycle132Write(const QString &path, const QByteArray &bytes, bool append = false) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly | (append ? QIODevice::Append : QIODevice::Truncate)) && file.write(bytes) == bytes.size();
}
bool cycle132Ready(const QList<QList<QVariant>> &ready, quint64 id, QVariantMap *result) {
    for (const auto &arguments : ready) if (arguments[0].toULongLong() == id) { *result = arguments[1].toMap(); return true; }
    return false;
}
}

void FomawriteTest::publishingUnrelatedThemeFolderChurnKeepsPreviewsAlive() {
    Cycle103SettingsSnapshot settings;
    const QString folder = PublishingThemes::directory(); QVERIFY(QDir().mkpath(folder));
    const QString junk = folder + "/cycle132-churn.log";
    const auto cleanup = qScopeGuard([&] { QFile::remove(junk); });
    Cycle132Editor fixture; QVERIFY(fixture.create());
    QVERIFY(fixture.backend.selectPublishingTheme("claude-like"));
    fixture.editor->setProperty("text", "# Churn\n\nThe folder changes while this renders.\n");
    QTest::qWait(400);
    QObject consumer;
    QSignalSpy ready(&fixture.backend, &Backend::publishingPreviewReady);
    QSignalSpy themes(&fixture.backend, &Backend::publishingThemesChanged);
    QSignalSpy css(&fixture.backend, &Backend::publishingCssChanged);
    const auto request = fixture.backend.requestPublishingPreview("pdf", &consumer, "churn:1");
    QVERIFY2(request["pending"].toBool(), qPrintable(request["error"].toString()));
    const quint64 id = request["requestId"].toULongLong();
    // Finder, editors and loggers write unrelated files into the themes folder
    // at a cadence that lets the 200 ms theme refresh run while Chromium is
    // still rendering. Before Cycle132 every refresh cancelled the render.
    QElapsedTimer churn; churn.start();
    QVariantMap result; bool done = false;
    while (churn.elapsed() < 25000 && !done) {
        QVERIFY(cycle132Write(junk, "tick\n", true));
        QTest::qWait(300);
        done = cycle132Ready(ready, id, &result);
    }
    QVERIFY2(done, "the render never completed while unrelated files changed in the themes folder");
    QVERIFY2(result["ok"].toBool(), qPrintable(result["error"].toString()));
    QCOMPARE(css.count(), 0);
    QCOMPARE(themes.count(), 0);
    fixture.backend.discardRecovery();
}

void FomawriteTest::publishingSelectedThemeCssIsMemoizedUntilItChanges() {
    Cycle103SettingsSnapshot settings;
    const QString folder = PublishingThemes::directory(); QVERIFY(QDir().mkpath(folder));
    const QString id = "cycle132-memo";
    const QString theme = folder + '/' + id + ".css";
    const QString assets = folder + '/' + id + "-assets";
    const QString font = assets + "/heavy.woff";
    const auto cleanup = qScopeGuard([&] { QFile::remove(theme); QDir(assets).removeRecursively(); });
    QVERIFY(QDir().mkpath(assets));
    // The sanitizer embeds fonts by suffix, so a synthetic 2 MiB file costs the
    // same base64 work as a real one.
    QVERIFY(cycle132Write(font, QByteArray(2 * 1024 * 1024, 'A')));
    QVERIFY(cycle132Write(theme, "@font-face{font-family:Heavy;src:url(cycle132-memo-assets/heavy.woff)} #write{font-family:Heavy}"));
    Cycle132Editor fixture; QVERIFY(fixture.create());
    fixture.editor->setProperty("text", "# Memo\n");
    QTest::qWait(400);
    QVERIFY2(fixture.backend.selectPublishingTheme(id), qPrintable(fixture.backend.status()));
    QObject consumer;
    const auto first = fixture.backend.requestPublishingPreview("html", &consumer, "memo:1");
    QVERIFY2(first["ok"].toBool(), qPrintable(first["error"].toString()));
    QVERIFY(cycle132Read(first["url"].toUrl()).contains("data:font/woff;base64,"));
    // Remove the font and request again before the 200 ms watcher refresh can
    // run. A memoized theme still carries the embedded font; re-reading the
    // folder on every request would silently switch to the fallback family.
    QVERIFY(QFile::remove(font));
    fixture.editor->setProperty("text", "# Memo edited\n");
    const auto second = fixture.backend.requestPublishingPreview("html", &consumer, "memo:2");
    QVERIFY2(second["ok"].toBool(), qPrintable(second["error"].toString()));
    QVERIFY2(cycle132Read(second["url"].toUrl()).contains("data:font/woff;base64,"), "theme CSS was recomputed on an ordinary request");
    // Once the watcher reports the change the memo is replaced.
    QSignalSpy css(&fixture.backend, &Backend::publishingCssChanged);
    QTRY_VERIFY_WITH_TIMEOUT(css.count() >= 1, 3000);
    fixture.editor->setProperty("text", "# Memo edited again\n");
    const auto third = fixture.backend.requestPublishingPreview("html", &consumer, "memo:3");
    QVERIFY2(third["ok"].toBool(), qPrintable(third["error"].toString()));
    const QByteArray updated = cycle132Read(third["url"].toUrl());
    QVERIFY(updated.contains("Fomawrite unavailable theme font"));
    QVERIFY(!updated.contains("data:font/woff;base64,"));
    fixture.backend.discardRecovery();
}

void FomawriteTest::publishingThemeFolderIsNotRecreatedByRefresh() {
    Cycle103SettingsSnapshot settings;
    QSettings().remove("output/publishingTheme");
    const QString folder = PublishingThemes::directory(); QVERIFY(QDir().mkpath(folder));
    Backend backend(nullptr, true);
    QTest::qWait(500);
    const QString aside = folder + ".cycle132-aside";
    QVERIFY(QDir().rename(folder, aside));
    const auto restore = qScopeGuard([&] {
        if (QDir(aside).exists()) { if (QDir(folder).exists()) QDir(folder).removeRecursively(); QDir().rename(aside, folder); }
    });
    // A refresh (watcher-driven or an explicit Reload Themes) runs while the
    // folder is aside: the user must not find an empty replacement.
    QTest::qWait(900);
    backend.reloadPublishingThemes();
    QTest::qWait(300);
    QVERIFY2(!QDir(folder).exists(), "a theme refresh recreated the publishing themes folder");
    QVERIFY(QDir().rename(aside, folder));
    // The returning folder is noticed through its parent and watched again.
    QSignalSpy changed(&backend, &Backend::publishingThemesChanged);
    const QString added = folder + "/cycle132-returned.css";
    const auto cleanupAdded = qScopeGuard([&] { QFile::remove(added); });
    QTest::qWait(500);
    QVERIFY(cycle132Write(added, "#write{color:#123456}"));
    QTRY_VERIFY_WITH_TIMEOUT(changed.count() >= 1, 4000);
}

void FomawriteTest::publishingComponentWebModeCompletesAndReloadsSameUrl() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto page = QUrl::fromLocalFile(directory.filePath("web.html"));
    QVERIFY(cycle132Write(page.toLocalFile(), "<!doctype html><html><head><meta charset=\"utf-8\"></head><body><p>Web fixture</p></body></html>"));
    Cycle128Preview fixture(QFINDTESTDATA("tst_fomawrite.cpp"), QVariantMap{{"ok", true}, {"url", page}});
    QVERIFY2(fixture.root, qPrintable(fixture.error)); QVERIFY(fixture.preview());
    fixture.preview()->setProperty("publishingMode", "web");
    fixture.document("untitled:1", "# Web fixture");
    fixture.window.show();
    const auto webReady = [&] {
        return fixture.preview()->property("displayedFormat").toString() == "web"
            && fixture.preview()->property("outputUrl").toUrl() == page
            && !fixture.preview()->property("viewportRefreshPending").toBool()
            && fixture.preview()->property("errorText").toString().isEmpty();
    };
    QTRY_VERIFY_WITH_TIMEOUT(webReady(), 10000);
    auto *web = fixture.preview()->findChild<QObject *>("renderedPreview"); QVERIFY(web);
    QCOMPARE(web->property("url").toUrl(), page);
    // The view navigated elsewhere while the pane's URL stayed the same (an
    // error page, a crash, an external navigation). A reload returning the
    // same cache URL must still load and complete instead of waiting forever.
    web->setProperty("url", QUrl("about:blank"));
    QTRY_COMPARE_WITH_TIMEOUT(web->property("url").toUrl(), QUrl("about:blank"), 5000);
    QVERIFY(QMetaObject::invokeMethod(fixture.preview(), "reload"));
    QTRY_VERIFY_WITH_TIMEOUT(webReady() && web->property("url").toUrl() == page, 10000);
}
