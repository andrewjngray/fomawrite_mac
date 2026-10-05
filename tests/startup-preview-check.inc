// Passive bounded observer installed after the ordinary production startup path.
// No layout mutation, forced frames, screenshot polling, or hidden-root startup.
QJsonArray startupObservations;
QElapsedTimer startupElapsed;
startupElapsed.start();
int startupStage = 0;
int startupStable = 0;
qint64 startupQuietSince = -1;
QMap<Backend *, int> startupQuietCounts;
std::shared_ptr<Session> startupSelected;
std::function<void(QQuickItem *, QJsonArray &)> startupImages;
startupImages = [&](QQuickItem *item, QJsonArray &images) {
    if (!item) return;
    if (item->objectName().startsWith("publishingPdfPage_")) {
        auto *paper = item->parentItem() ? item->parentItem()->parentItem() : nullptr;
        const QSize size = item->property("sourceSize").toSize();
        auto *loader = item->parentItem();
        images.append(QJsonObject{{"name", item->objectName()}, {"visible", item->isVisible()},
            {"width", item->width()}, {"height", item->height()}, {"status", item->property("status").toInt()},
            {"sourceWidth", size.width()}, {"sourceHeight", size.height()},
            {"loaderActive", loader && loader->property("active").toBool()},
            {"loaderStatus", loader ? loader->property("status").toInt() : -1},
            {"paperWidth", paper ? paper->width() : -1}, {"paperHeight", paper ? paper->height() : -1},
            {"paperY", paper ? paper->y() : -1}, {"paperPainted", paper && paper->property("painted").toBool()}});
    }
    for (auto *child : item->childItems()) startupImages(child, images);
};
std::function<void(QQuickItem *, QJsonArray &, int)> startupScene;
startupScene = [&](QQuickItem *item, QJsonArray &scene, int depth) {
    if (!item || depth > 8 || scene.size() >= 200) return;
    scene.append(QJsonObject{{"class", item->metaObject()->className()}, {"name", item->objectName()},
        {"depth", depth}, {"visible", item->isVisible()}, {"x", item->x()}, {"y", item->y()},
        {"width", item->width()}, {"height", item->height()},
        {"active", QJsonValue::fromVariant(item->property("active"))},
        {"status", QJsonValue::fromVariant(item->property("status"))}});
    for (auto *child : item->childItems()) startupScene(child, scene, depth + 1);
};
const auto startupSnapshot = [&](const std::shared_ptr<Session> &session) {
    auto *window = qobject_cast<QQuickWindow *>(session->window.data());
    auto *pane = window ? window->findChild<QQuickItem *>("previewPane") : nullptr;
    auto *pdf = pane ? pane->findChild<QObject *>("actualPdfDocument") : nullptr;
    auto *scroll = pane ? pane->findChild<QQuickItem *>("previewScroll") : nullptr;
    QJsonObject state{{"selectedUrl", session->backend->fileUrl().toString()},
        {"selectedIdentity", session->backend->publishingDocumentIdentity()},
        {"publishingThemesChangedCount", startupThemeChanges.value(session->backend)},
        {"windowVisible", window && window->isVisible()}, {"windowExposed", window && window->isExposed()},
        {"paneVisible", pane && pane->isVisible()}, {"paneWidth", pane ? pane->width() : -1},
        {"paneHeight", pane ? pane->height() : -1}, {"pdfStatus", pdf ? pdf->property("status").toInt() : -1},
        {"pdfPageCount", pdf ? pdf->property("pageCount").toInt() : -1},
        {"pdfSource", pdf ? pdf->property("source").toUrl().toString() : QString()},
        {"scrollWidth", scroll ? scroll->width() : -1}, {"scrollHeight", scroll ? scroll->height() : -1},
        {"scrollY", scroll ? scroll->property("contentY").toDouble() : -1}};
    if (pane) for (const char *key : {"documentIdentity", "appliedDocumentIdentity", "displayedDocumentIdentity",
            "refreshRevision", "appliedRevision", "requestToken", "pendingRequest", "previewRefreshInFlight",
            "viewportRefreshPending", "outputUrl", "actualPdfSource", "pdfPagesReady", "pdfVisiblePagesPainted",
            "publishingMode", "errorText", "warningText"})
        state.insert(QString::fromLatin1(key), QJsonValue::fromVariant(pane->property(key)));
    QJsonArray images;
    startupImages(pane, images);
    state.insert("pageImages", images);
    state.insert("pageImageCount", images.size());
    QJsonArray scene;
    startupScene(pane, scene, 0);
    state.insert("scene", scene);
    if (pdf) {
        QPdfDocument extracted;
        const QString path = pdf->property("source").toUrl().toLocalFile();
        if (!path.isEmpty() && extracted.load(path) == QPdfDocument::Error::None && extracted.pageCount() > 0)
            state.insert("pdfFirstPageText", extracted.getAllText(0).text());
    }
    return state;
};
const auto startupReady = [&](const std::shared_ptr<Session> &session) {
    const auto state = startupSnapshot(session);
    const QUrl actual(state["pdfSource"].toString());
    QFile source(actual.toLocalFile());
    bool painted = false;
    for (const auto value : state["pageImages"].toArray()) {
        const auto image = value.toObject();
        if (!image["visible"].toBool()) continue;
        if (image["status"].toInt() != 1 || image["width"].toDouble() <= 0 || image["height"].toDouble() <= 0) return false;
        painted = true;
    }
    auto *sourceEditor = session->window->findChild<QObject *>("sourceEditor");
    if (!sourceEditor) return false;
    QTextDocument plainSource;
    plainSource.setMarkdown(sourceEditor->property("text").toString());
    const QString expectedHeading = plainSource.toPlainText().section('\n', 0, 0).trimmed();
    return startupQmlWarnings.isEmpty() && !expectedHeading.isEmpty()
        && state["pdfFirstPageText"].toString().simplified().contains(expectedHeading.simplified())
        && state["paneVisible"].toBool() && state["pdfPagesReady"].toBool()
        && state["pdfVisiblePagesPainted"].toBool() && painted && state["errorText"].toString().isEmpty()
        && !state["viewportRefreshPending"].toBool() && actual.isLocalFile()
        && actual == QUrl(state["outputUrl"].toString()) && source.open(QIODevice::ReadOnly)
        && source.read(5) == "%PDF-"
        && state["displayedDocumentIdentity"] == state["selectedIdentity"];
};
const auto startupWriteReport = [&](bool passed, const QString &failure) {
    QJsonArray states;
    for (const auto &session : sessions) if (session->backend && session->window) states.append(startupSnapshot(session));
    QJsonObject report{{"schema", 1}, {"phase", startupPhase}, {"passed", passed}, {"failure", failure},
        {"elapsedMs", startupElapsed.elapsed()}, {"stage", startupStage}, {"qmlDiskCacheEnabled", true},
        {"settingsFile", QSettings().fileName()}, {"workspacePath", workspacePath},
        {"copiedExistingQmlCache", qEnvironmentVariable("FOMAWRITE_STARTUP_PREVIEW_COPY_CACHE") == "1"},
        {"seededRestoredGeometry", startupPhase == "restored" && qEnvironmentVariable("FOMAWRITE_STARTUP_PREVIEW_REAL_GEOMETRY") == "1"},
        {"qmlCachePath", QStandardPaths::writableLocation(QStandardPaths::CacheLocation)},
        {"copiedManagedThemes", !qEnvironmentVariable("FOMAWRITE_STARTUP_PREVIEW_THEMES").isEmpty()},
        {"restoredWorkspaceEntries", savedWorkspace.size()}, {"observations", startupObservations}, {"states", states}, {"qmlWarnings", startupQmlWarnings}};
    QSaveFile file(QDir(startupRoot).filePath(startupPhase + "-report.json"));
    if (file.open(QIODevice::WriteOnly)) { file.write(QJsonDocument(report).toJson(QJsonDocument::Indented)); file.commit(); }
    qInfo().noquote() << "Ordinary startup preview" << startupPhase << (passed ? "PASS" : "FAIL") << failure;
};
QTimer startupPoll;
startupPoll.setInterval(100);
QObject::connect(&startupPoll, &QTimer::timeout, &app, [&] {
    if (startupElapsed.elapsed() > 15000) {
        startupPoll.stop(); startupWriteReport(false, "Preview did not become visibly ready within 15 seconds.");
        app.allowExit = true; app.exit(1); return;
    }
    if (startupStage == 0) {
        if (sessions.size() != 2) return;
        if (startupPhase == "restored" && savedWorkspace.size() != 2) {
            startupPoll.stop(); startupWriteReport(false, "Production checkpoint did not restore both windows."); app.allowExit = true; app.exit(1); return;
        }
        startupSelected = sessions.first();
        startupSelected->window->raise(); // One normal activation, never a forced frame.
        startupSelected->window->requestActivate();
        startupStage = 1; return;
    }
    if (startupStage == 1 || startupStage == 2) {
        if (!startupReady(startupSelected)) { startupStable = 0; return; }
        if (++startupStable < 3) return;
        startupObservations.append(startupSnapshot(startupSelected));
        startupStable = 0;
        if (startupStage == 1) {
            startupSelected = sessions.last();
            // Opening a minimized saved window is an ordinary user operation.
            if (startupSelected->window->windowState() == Qt::WindowMinimized) startupSelected->window->showNormal();
            startupSelected->window->raise(); startupSelected->window->requestActivate();
            startupStage = 2; return;
        }
        startupStage = 3;
        startupQuietSince = startupElapsed.elapsed();
        for (const auto &session : sessions) startupQuietCounts.insert(session->backend, startupThemeChanges.value(session->backend));
        return;
    }
    if (startupStage == 3 && startupElapsed.elapsed() - startupQuietSince >= 2000) {
        for (const auto &session : sessions) {
            if (!startupReady(session) || startupThemeChanges.value(session->backend) != startupQuietCounts.value(session->backend)) {
                startupPoll.stop(); startupWriteReport(false, "A settled preview refreshed again while its theme folder was untouched.");
                app.allowExit = true; app.exit(1); return;
            }
        }
        startupStage = 4; startupPoll.stop();
        startupWriteReport(true, QString());
        app.guardedQuit(); // Real close prompts/checkpoint/commit path for saved fixtures.
    }
});
startupPoll.start();
QTimer::singleShot(18000, &app, [&] { startupWriteReport(false, "Normal guarded quit did not complete."); app.allowExit = true; app.exit(2); });
