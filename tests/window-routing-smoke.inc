// Included only in the FOMAWRITE_CONTEXT_SMOKE integration-test build.
// Exercise actual context-menu dispatch, session creation and native tabbing.
const auto smokeCheck = [&](bool ok, const char *message) {
    if (!ok) { qCritical("FAIL: %s", message); app.exit(1); }
    else qInfo("PASS: %s", message);
    return ok;
};
const QUrl smokeFile = QUrl::fromLocalFile(smokeDirectory.filePath("Clicked.md"));
QFile smokeSource(smokeFile.toLocalFile());
smokeSource.open(QIODevice::WriteOnly);
smokeSource.write("# Window routing\n\nA disposable document.\n");
smokeSource.close();
const auto smokeTrigger = [&](const std::shared_ptr<Session> &session, const QString &label) {
    auto *menu = session->window->findChild<QObject *>("libraryContextMenu");
    if (!menu) return false;
    menu->setProperty("target", session->backend->libraryItemInfo(smokeFile));
    for (auto *item : menu->findChildren<QObject *>())
        if (item->property("text").toString() == label && item->metaObject()->indexOfSignal("triggered()") >= 0)
            return QMetaObject::invokeMethod(item, "triggered");
    return false;
};
const auto smokeWaitFor = [&](std::function<bool()> ready, std::function<void()> next) {
    auto *timer = new QTimer(&app);
    timer->setInterval(40);
    QObject::connect(timer, &QTimer::timeout, &app, [timer, ready, next] {
        if (!ready()) return;
        timer->stop(); timer->deleteLater(); next();
    });
    timer->start();
};
std::function<void(int)> smokePrint;
smokePrint = [&](int index) {
    const QStringList labels{"Print Rendered Document…", "Print Markdown Source…", "Paginated Preview…"};
    if (index == labels.size()) {
        if (!smokeCheck(sessions.first()->backend->libraryItemAction(smokeFile, "reveal"), "Finder reveal request accepted for sample")) return;
        if (!smokeCheck(sessions.first()->backend->libraryItemAction(smokeFile, "share"), "native sharing picker request accepted for sample")) return;
        QTimer::singleShot(300, &app, [&] {
            for (auto &session : sessions) session->backend->discardRecovery();
            app.guardedQuit();
        });
        return;
    }
    auto seen = std::make_shared<bool>(false);
    auto *cancel = new QTimer(&app);
    cancel->setInterval(100);
    QObject::connect(cancel, &QTimer::timeout, &app, [seen, cancel] {
        for (auto *widget : QApplication::topLevelWidgets()) {
            if (qobject_cast<QPrintDialog *>(widget) || qobject_cast<QPrintPreviewDialog *>(widget)) {
                *seen = true;
                static_cast<QDialog *>(widget)->reject();
                cancel->stop(); cancel->deleteLater();
                return;
            }
        }
    });
    cancel->start();
    if (!smokeCheck(smokeTrigger(sessions.first(), labels[index]), "dispatch print submenu action")) return;
    smokeWaitFor([seen] { return *seen; }, [&, index] {
        qInfo() << "PASS: opened and canceled" << QStringList{"rendered print", "source print", "paginated preview"}[index];
        smokePrint(index + 1);
    });
};
QTimer::singleShot(200, &app, [&] {
    if (!smokeCheck(sessions.size() == 1, "one initial test window")) return;
    auto original = sessions.first();
    if (!smokeCheck(original->backend->open(smokeFile), "open disposable source")) return;
    if (!smokeCheck(smokeTrigger(original, "Open in New Window"), "trigger New Window from context menu")) return;
    smokeWaitFor([&] { return sessions.size() >= 2; }, [&, original] {
        if (!smokeCheck(sessions.size() == 2, "New Window creates a second editor for the selected open file")) return;
        auto second = sessions.last();
        if (!smokeCheck(QFileInfo(second->backend->fileUrl().toLocalFile()).canonicalFilePath()
                       == QFileInfo(smokeFile.toLocalFile()).canonicalFilePath(), "new window loads the clicked file")) return;
#ifdef Q_OS_MACOS
        if (!smokeCheck(macWorkspaceState(second->window).isEmpty(), "New Window is separate from native tab groups")) return;
#endif
        if (!smokeCheck(smokeTrigger(original, "Open in New Tab"), "trigger New Tab from context menu")) return;
        smokeWaitFor([&] { return sessions.size() >= 3; }, [&, original, second] {
            if (!smokeCheck(sessions.size() == 3, "New Tab creates a third editor")) return;
            auto tab = sessions.last();
#ifdef Q_OS_MACOS
            const auto sourceGroup = macWorkspaceState(original->window).value("group").toString();
            if (!smokeCheck(!sourceGroup.isEmpty() && sourceGroup == macWorkspaceState(tab->window).value("group").toString(),
                            "new tab joins its source window")) return;
            if (!smokeCheck(macWorkspaceState(second->window).isEmpty(), "unrelated window stays separate")) return;
#endif
            createWindow(smokeFile);
            if (!smokeCheck(sessions.size() == 3, "ordinary opening still focuses an existing editor")) return;
            smokePrint(0);
        });
    });
});
QTimer::singleShot(15000, &app, [&] { qCritical("Window routing integration test timed out"); app.exit(2); });
