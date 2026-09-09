#include "filelibrary.h"
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

FileLibrary::FileLibrary(QObject *parent) : QObject(parent) {
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(100);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, [this] { m_refreshTimer.start(); });
    connect(&m_refreshTimer, &QTimer::timeout, this, &FileLibrary::refresh);
    const QUrl saved = QSettings().value(QStringLiteral("library/root")).toUrl();
    if (saved.isLocalFile() && QFileInfo(saved.toLocalFile()).isDir())
        setRootFolder(saved);
}

QString FileLibrary::rootName() const {
    const QString name = QFileInfo(m_rootFolder.toLocalFile()).fileName();
    return name.isEmpty() ? m_rootFolder.toLocalFile() : name;
}

bool FileLibrary::isTextFile(const QString &path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == "md" || suffix == "markdown" || suffix == "mdown"
        || suffix == "txt" || suffix == "text";
}

void FileLibrary::setError(const QString &error) {
    if (m_error == error) return;
    m_error = error;
    emit errorChanged();
}

void FileLibrary::setRootFolder(const QUrl &folder) {
    const QFileInfo info(folder.toLocalFile());
    if (!folder.isLocalFile() || !info.isDir() || !info.isReadable()) {
        setError(QStringLiteral("Choose a readable local folder."));
        return;
    }
    const QUrl normalized = QUrl::fromLocalFile(info.canonicalFilePath());
    if (normalized == m_rootFolder) { refresh(); return; }
    m_rootFolder = normalized;
    m_expanded.clear();
    m_filter.clear();
    QSettings().setValue(QStringLiteral("library/root"), m_rootFolder);
    emit rootFolderChanged();
    emit filterChanged();
    refresh();
}

bool FileLibrary::containsPath(const QString &path) const {
    if (m_rootFolder.isEmpty() || path.isEmpty()) return false;
    const QString relative = QDir(m_rootFolder.toLocalFile()).relativeFilePath(path);
    return !relative.startsWith("../") && relative != ".." && !QDir::isAbsolutePath(relative);
}

void FileLibrary::toggleFolder(const QUrl &folder) {
    if (!folder.isLocalFile()) return;
    const QString path = QFileInfo(folder.toLocalFile()).canonicalFilePath();
    if (!containsPath(path) || !QFileInfo(path).isDir()) return;
    if (m_expanded.contains(path)) m_expanded.remove(path);
    else m_expanded.insert(path);
    refresh();
}

void FileLibrary::setFilter(const QString &filter) {
    if (filter == m_filter) return;
    m_filter = filter;
    emit filterChanged();
    refresh();
}

void FileLibrary::appendDirectory(const QString &path, int depth, QStringList &watched) {
    if (depth > 24 || m_entries.size() >= 10000) return;
    watched.append(path);
    const auto items = QDir(path).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                                               QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &info : items) {
        if (m_entries.size() >= 10000) break;
        const bool directory = info.isDir();
        // Application bundles are not writing folders. Directory symlinks are
        // excluded to avoid loops and accidentally walking outside the library.
        if (directory && (info.isSymLink() || info.suffix() == "app")) continue;
        if (!directory && !isTextFile(info.fileName())) continue;
        if (!directory && !m_filter.isEmpty()
            && !info.fileName().contains(m_filter, Qt::CaseInsensitive)) continue;
        const QString childPath = info.absoluteFilePath();
        const bool expanded = directory && m_expanded.contains(childPath);
        m_entries.append(QVariantMap{{"name", info.fileName()},
            {"url", QUrl::fromLocalFile(childPath)}, {"directory", directory},
            {"depth", depth}, {"expanded", expanded},
            {"modified", info.lastModified().toString("d MMM")}});
        if (expanded) appendDirectory(childPath, depth + 1, watched);
    }
}

void FileLibrary::refresh() {
    QStringList watched;
    m_entries.clear();
    if (m_rootFolder.isLocalFile()) {
        if (!QFileInfo(m_rootFolder.toLocalFile()).isDir())
            setError(QStringLiteral("This folder is no longer available. Choose another folder."));
        else {
            setError({});
            appendDirectory(m_rootFolder.toLocalFile(), 0, watched);
        }
    }
    const QStringList previous = m_watcher.directories();
    if (!previous.isEmpty()) m_watcher.removePaths(previous);
    if (!watched.isEmpty()) m_watcher.addPaths(watched);
    emit entriesChanged();
}

static QString cleanName(const QString &name) {
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty() || trimmed == "." || trimmed == ".."
        || trimmed.contains('/') || trimmed.contains('\\') || trimmed.contains(QChar::Null)) return {};
    return trimmed;
}

QUrl FileLibrary::createDocument(const QString &name) {
    QString fileName = cleanName(name);
    if (!m_rootFolder.isLocalFile() || fileName.isEmpty()) {
        setError(QStringLiteral("Choose a folder and enter a filename without slashes."));
        return {};
    }
    if (!isTextFile(fileName)) fileName += QStringLiteral(".md");
    const QString path = QDir(m_rootFolder.toLocalFile()).filePath(fileName);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        setError(QStringLiteral("Could not create the file. Its name may already be in use."));
        return {};
    }
    file.close();
    refresh();
    return QUrl::fromLocalFile(path);
}

bool FileLibrary::createFolder(const QString &name) {
    const QString folderName = cleanName(name);
    if (!m_rootFolder.isLocalFile() || folderName.isEmpty()
        || !QDir(m_rootFolder.toLocalFile()).mkdir(folderName)) {
        setError(QStringLiteral("Could not create the folder. Check its name and permissions."));
        return false;
    }
    refresh();
    return true;
}

void FileLibrary::revealFile(const QUrl &url) {
    if (!url.isLocalFile()) return;
    const QString path = QFileInfo(url.toLocalFile()).canonicalFilePath();
    if (!containsPath(path)) return;
    QDir parent = QFileInfo(path).absoluteDir();
    while (parent.absolutePath() != m_rootFolder.toLocalFile() && containsPath(parent.absolutePath())) {
        m_expanded.insert(parent.absolutePath());
        if (!parent.cdUp()) break;
    }
    refresh();
}
