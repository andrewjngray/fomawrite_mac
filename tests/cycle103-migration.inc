// Run after the production startup has restored the seeded workspace. These
// assertions exercise the real upgrade/checkpoint path, not a parallel helper.
const auto smokeReferenceMigration = [&](const std::shared_ptr<Session> &session) {
    auto *window = session->window.data();
    auto *settings = window->findChild<QObject *>(QStringLiteral("workspaceSettings"));
    auto *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
    if (!smokeCheck(settings && editor, "reference migration exposes the restored native writing workspace")) return false;
    if (!smokeCheck(QSettings().value(referenceRevisionKey).toInt() == 1
                   && !referenceWorkspaceUpgrade
                   && !session->engine->rootContext()->contextProperty(QStringLiteral("referenceWorkspaceUpgrade")).toBool()
                   && !window->property("adoptReferenceWorkspace").toBool(),
                   "completed reference migration clears the C++ and live QML upgrade flags")) return false;
    const QVariantMap referencePanes = smokePaneState(window);
    if (!smokeCheck(session->backend->themePreset() == QStringLiteral("studio")
                   && settings->property("writingAppearance").toString() == QStringLiteral("manuscript")
                   && settings->property("titleBarMode").toInt() == 1
                   && referencePanes.value("layoutMode").toInt() == 0
                   && referencePanes.value("organizerVisible").toBool()
                   && referencePanes.value("filesVisible").toBool()
                   && referencePanes.value("organizerWidth").toDouble() == 184
                   && referencePanes.value("fileWidth").toDouble() == 232,
                   "startup upgrades legacy Book/Preview and hidden panes to the approved source composition")) return false;
    if (!smokeCheck(session->backend->fileUrl() == smokeLegacyFile
                   && editor->property("text").toString() == smokeLegacyText
                   && !session->backend->modified() && session->backend->outputStyle() == 6,
                   "workspace migration preserves the saved Unicode document and Palatino output template")) return false;
    const auto upgradedCheckpoint = WorkspaceStore::read(workspacePath);
    if (!smokeCheck(upgradedCheckpoint.size() == 1
                   && upgradedCheckpoint[0].toObject()["paneState"].toObject().toVariantMap() == referencePanes,
                   "upgraded pane state is checkpointed before marking migration complete")) return false;

    const QString addition = QStringLiteral("\nKeep this unsaved review edit.\n");
    if (!smokeCheck(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, smokeLegacyText.size()), Q_ARG(QString, addition))
                   && QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 4), Q_ARG(int, 12))
                   && session->backend->modified() && editor->property("canUndo").toBool(),
                   "prepare an unsaved selected draft for the one-time migration regression")) return false;
    const int revision = session->backend->documentRevision();
    settings->setProperty("writingAppearance", QStringLiteral("book"));
    const QVariantMap customPanes{{"version", 2}, {"organizerVisible", false}, {"filesVisible", true},
        {"layoutMode", 2}, {"liveEditEnabled", false}, {"lastEditingLayoutMode", 0}, {"organizerWidth", 216.0},
        {"fileWidth", 296.0}, {"previewWidth", 380.0}};
    if (!smokeCheck(smokeRestorePanes(window, customPanes) && smokePaneState(window) == customPanes
                   && settings->property("writingAppearance").toString() == QStringLiteral("book"),
                   "restoring a later user checkpoint does not reapply the startup migration")) return false;
    if (!smokeCheck(session->backend->fileUrl() == smokeLegacyFile
                   && editor->property("text").toString() == smokeLegacyText + addition
                   && session->backend->documentRevision() == revision && session->backend->modified()
                   && editor->property("selectionStart").toInt() == 4 && editor->property("selectionEnd").toInt() == 12
                   && editor->property("canUndo").toBool() && session->backend->outputStyle() == 6,
                   "post-upgrade pane restoration preserves draft, selection, revision, Undo and output template")) return false;
    persist();
    const auto userCheckpoint = WorkspaceStore::read(workspacePath);
    if (!smokeCheck(userCheckpoint.size() == 1
                   && userCheckpoint[0].toObject()["paneState"].toObject().toVariantMap() == customPanes,
                   "subsequent production checkpoints retain the user's chosen preview layout")) return false;

    QVariantMap requestedReference = referencePanes;
    requestedReference["previewWidth"] = customPanes.value("previewWidth");
    if (!smokeCheck(QMetaObject::invokeMethod(window, "applyReferenceWorkspace")
                   && smokePaneState(window) == requestedReference
                   && session->backend->documentRevision() == revision
                   && editor->property("text").toString() == smokeLegacyText + addition
                   && session->backend->outputStyle() == 6,
                   "explicit reference-layout action also preserves an existing draft and export template")) return false;
    if (!smokeCheck(QMetaObject::invokeMethod(editor, "undo")
                   && editor->property("text").toString() == smokeLegacyText,
                   "one Undo removes only the draft edit after migration and layout restoration")) return false;
    session->backend->reloadFromDisk();
    if (!smokeCheck(!session->backend->modified()
                   && editor->property("text").toString() == smokeLegacyText,
                   "migration fixture leaves its disposable source clean for native routing checks")) return false;
    return true;
};
