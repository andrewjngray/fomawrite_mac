// Test-only seed, included before any Backend or QML window is constructed.
// Reproduce the old presentation seen in the review screenshot: Book/Preview,
// hidden navigation and a chosen export template. All data is disposable.
const QString smokeLegacyText = QString::fromUtf8(
    "# A saved writing workspace\n\nPreserve this café note and its output template.\n");
const QUrl smokeLegacyFile = QUrl::fromLocalFile(smokeDirectory.filePath("Before upgrade.md"));
{
    QFile legacyFile(smokeLegacyFile.toLocalFile());
    if (!legacyFile.open(QIODevice::WriteOnly)
        || legacyFile.write(smokeLegacyText.toUtf8()) != smokeLegacyText.toUtf8().size())
        qFatal("Unable to prepare disposable pre-upgrade document");
    legacyFile.close();
    QSettings legacySettings;
    legacySettings.setValue("workspace/writingAppearance", "book");
    legacySettings.setValue("workspace/layoutMode", 2);
    legacySettings.setValue("workspace/organizerVisible", false);
    legacySettings.setValue("workspace/libraryVisible", false);
    legacySettings.setValue("workspace/titleBarMode", 0);
    legacySettings.setValue("appearance/theme", "light");
    legacySettings.setValue("output/style", 6);
    legacySettings.sync();
    const QString legacyStateDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString legacyWorkspacePath = QDir(legacyStateDirectory).filePath(
        "workspace-" + workspaceIdentity(QCoreApplication::applicationFilePath()) + ".json");
    const QJsonObject panes{{"version", 1}, {"organizerVisible", false}, {"filesVisible", false},
        {"layoutMode", 2}, {"liveEditEnabled", false}, {"organizerWidth", 244.0},
        {"fileWidth", 320.0}, {"previewWidth", 460.0}};
    const QJsonObject window{{"url", smokeLegacyFile.toString()}, {"cursor", 4},
        {"x", 80}, {"y", 80}, {"width", 1100}, {"height", 800}, {"state", 0},
        {"active", true}, {"root", QUrl::fromLocalFile(smokeDirectory.path()).toString()},
        {"paneState", panes}};
    if (!QDir().mkpath(legacyStateDirectory)
        || !WorkspaceStore::write(legacyWorkspacePath, QJsonArray{window}))
        qFatal("Unable to prepare disposable pre-upgrade workspace checkpoint");
}
