// Compile-only diagnostic: production session creation and restoration stay unchanged.
const QString startupRoot = qEnvironmentVariable("FOMAWRITE_STARTUP_PREVIEW_ROOT");
const QString startupPhase = qEnvironmentVariable("FOMAWRITE_STARTUP_PREVIEW_PHASE");
if (!QDir::isAbsolutePath(startupRoot) || !QFileInfo(startupRoot).isDir()
        || (startupPhase != "cold" && startupPhase != "restored")) {
    qCritical("Startup preview check requires a disposable root and cold/restored phase.");
    return 2;
}
QJsonArray startupQmlWarnings;
QMap<Backend *, int> startupThemeChanges;
qunsetenv("QML_DISABLE_DISK_CACHE");
QStandardPaths::setTestModeEnabled(true);
app.setOrganizationName("FomawriteStartupCheck");
app.setOrganizationDomain("invalid.fomawrite-startup-check");
app.setApplicationName("FomawriteStartupCheck-" + QString::fromLatin1(
    QCryptographicHash::hash(startupRoot.toUtf8(), QCryptographicHash::Sha256).toHex().left(16)));
app.setApplicationDisplayName("Fomawrite — isolated ordinary startup check");
QSettings::setDefaultFormat(QSettings::IniFormat);
QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir(startupRoot).filePath("settings"));
QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, QDir(startupRoot).filePath("system-settings"));
// Match the existing document checker: only a new test namespace may be linked.
struct StartupDataLink {
    QString path;
    QString cachePath;
    ~StartupDataLink() { if (!path.isEmpty()) QFile::remove(path); if (!cachePath.isEmpty()) QFile::remove(cachePath); }
} startupDataLink;
const QString startupDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
const QString startupDataTarget = QDir(startupRoot).filePath("data");
if (QFileInfo::exists(startupDataPath) || QFileInfo(startupDataPath).isSymLink()
        || !QDir().mkpath(QFileInfo(startupDataPath).absolutePath())
        || !QDir().mkpath(startupDataTarget) || !QFile::link(startupDataTarget, startupDataPath)) {
    qCritical("Cannot create isolated startup AppData link.");
    return 2;
}
startupDataLink.path = startupDataPath;
{ // Empty or copied disk cache also lives entirely inside the disposable root.
    const QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString cacheTarget = QDir(startupRoot).filePath("cache");
    if (QFileInfo::exists(cachePath) || QFileInfo(cachePath).isSymLink()
            || !QDir().mkpath(QFileInfo(cachePath).absolutePath())
            || !QDir().mkpath(cacheTarget) || !QFile::link(cacheTarget, cachePath)) {
        qCritical("Cannot link the isolated copy of the existing QML cache."); return 2;
    }
    startupDataLink.cachePath = cachePath;
}
if (startupPhase == "cold") {
    QSettings preferences;
    preferences.setValue("workspace/publishingFormat", "pdf");
    preferences.setValue("workspace/layoutMode", qEnvironmentVariableIntValue("FOMAWRITE_STARTUP_PREVIEW_LAYOUT") == 2 ? 2 : 1);
    preferences.setValue("workspace/previewZoom", 100);
    preferences.setValue("workspace/sourceZoom", 100);
    preferences.setValue("workspace/libraryVisible", false);
    preferences.setValue("workspace/organizerVisible", false);
    preferences.setValue("workspace/synchronizedScroll", true);
    preferences.setValue("workspace/autoHideChrome", true);
    preferences.setValue("workspace/referenceRevision/" + workspaceIdentity(app.applicationFilePath()), 1);
    preferences.setValue("output/style", 1);
    preferences.setValue("output/size", 12);
    preferences.setValue("output/font", "Helvetica Neue");
    preferences.setValue("output/pageFurniture", false);
    preferences.setValue("output/titlePage", false);
    preferences.setValue("output/header", "");
    preferences.setValue("output/footer", "");
    preferences.setValue("window/width", 1280);
    preferences.setValue("window/height", 820);
    preferences.sync();
}
