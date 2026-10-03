// Included inside main() by the native session-routing fixture.
// All documents are disposable; original/second/tab continue into workspace QA.
const auto smokeNavigation = [&](const std::shared_ptr<Session> &original,
                                 const std::shared_ptr<Session> &second,
                                 const std::shared_ptr<Session> &tab,
                                 const std::function<void()> &next) {
    Q_UNUSED(tab);
    const QUrl fromUrl = QUrl::fromLocalFile(smokeDirectory.filePath("Navigation source.md"));
    QFile fromFile(fromUrl.toLocalFile());
    if (!smokeCheck(fromFile.open(QIODevice::WriteOnly)
                   && fromFile.write("# Navigation source\n\nKeep this document open.\n") > 0,
                   "prepare native fragment source")) return;
    fromFile.close();
    if (!smokeCheck(original->backend->open(fromUrl), "load a distinct source for cross-window navigation")) return;
    auto *sourceEditor = original->window->findChild<QObject *>(QStringLiteral("sourceEditor"));
    auto *targetEditor = second->window->findChild<QObject *>(QStringLiteral("sourceEditor"));
    if (!smokeCheck(sourceEditor && targetEditor, "native navigation has both editors")) return;
    const QString sourceText = sourceEditor->property("text").toString();
    const QString targetText = targetEditor->property("text").toString();
    const QString append = QString::fromUtf8("\n# Café 你好\n\nNative unsaved target.\n");
    if (!smokeCheck(QMetaObject::invokeMethod(targetEditor, "insert", Q_ARG(int, targetText.size()),
                                            Q_ARG(QString, append)), "add an unsaved target heading")) return;
    QUrl link = smokeFile;
    link.setFragment(QString::fromUtf8("café-你好"));
    if (!smokeCheck(QMetaObject::invokeMethod(original->window, "requestOpen", Q_ARG(QVariant, link)),
                   "dispatch cross-window encoded heading link")) return;
    QTimer::singleShot(180, &app, [&, original, second, next, sourceText, targetText, append, fromUrl] {
        auto *sourceEditor = original->window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        auto *targetEditor = second->window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        if (!smokeCheck(sessions.size() == 3 && original->backend->fileUrl() == fromUrl
                       && sourceEditor->property("text").toString() == sourceText
                       && targetEditor->property("text").toString() == targetText + append
                       && targetEditor->property("cursorPosition").toInt() == targetText.size() + 1
                       && second->backend->modified() && targetEditor->property("canUndo").toBool(),
                       "cross-window navigation reaches live Unicode heading without reloading either buffer")) return;
        QUrl missing = smokeFile;
        missing.setFragment("missing-native-heading");
        if (!smokeCheck(QMetaObject::invokeMethod(original->window, "requestOpen", Q_ARG(QVariant, missing))
                       && second->window->property("navigationNotice").toString().contains("missing-native-heading")
                       && targetEditor->property("cursorPosition").toInt() == targetText.size() + 1,
                       "missing cross-window heading is visible and preserves destination caret")) return;
        if (!smokeCheck(QMetaObject::invokeMethod(sourceEditor, "insert", Q_ARG(int, sourceText.size()),
                                                 Q_ARG(QString, QStringLiteral("Unsaved source."))),
                       "prepare guarded native source")) return;
        QUrl valid = smokeFile;
        valid.setFragment("a-composed-writing-workspace");
        if (!smokeCheck(QMetaObject::invokeMethod(original->window, "requestOpen", Q_ARG(QVariant, valid))
                       && original->window->property("pendingAction").toString() == QStringLiteral("open"),
                       "dirty source prompts before any cross-window movement")) return;
        auto *prompt = original->window->findChild<QObject *>(QStringLiteral("unsavedChangesPrompt"));
        if (!smokeCheck(prompt && QMetaObject::invokeMethod(prompt, "cancelRequested")
                       && original->backend->modified()
                       && targetEditor->property("cursorPosition").toInt() == targetText.size() + 1,
                       "Cancel retains the draft and does not navigate the destination")) return;
        if (!smokeCheck(QMetaObject::invokeMethod(targetEditor, "undo")
                       && targetEditor->property("text").toString() == targetText,
                       "one Undo removes only the target edit after fragment navigation")) return;
        // Restore fixture state without sending an ordinary open back to another
        // view; this bypass is test-only and the production callback is restored.
        original->backend->reloadFromDisk();
        second->backend->reloadFromDisk();
        const auto originalRouting = original->backend->focusExistingDocument;
        original->backend->focusExistingDocument = {};
        const bool restored = original->backend->open(smokeFile);
        original->backend->focusExistingDocument = originalRouting;
        second->window->setProperty("navigationNotice", QString());
        if (!smokeCheck(restored && !original->backend->modified() && !second->backend->modified(),
                       "restore disposable native navigation fixture")) return;
        next();
    });
};
