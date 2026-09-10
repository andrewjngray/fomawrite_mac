#include "filelibrary.h"
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QRegularExpression>
#include <algorithm>

FileLibrary::FileLibrary(QObject *parent) : QObject(parent) {
    QSettings settings;
    m_locations = settings.value("library/locations").toStringList();
    m_favorites = settings.value("library/favorites").toStringList();
    m_recentFiles = settings.value("library/recents").toStringList();
    m_sortMode = qBound(0, settings.value("library/sortMode", 0).toInt(), 3);
    m_ascending = settings.value("library/ascending", true).toBool();
    m_foldersFirst = settings.value("library/foldersFirst", true).toBool();
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(100);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, [this] { m_refreshTimer.start(); });
    connect(&m_refreshTimer, &QTimer::timeout, this, &FileLibrary::refresh);
    const QUrl saved = QSettings().value(QStringLiteral("library/root")).toUrl();
    if (saved.isLocalFile() && QFileInfo(saved.toLocalFile()).isDir())
        setRootFolder(saved);
}

static QVariantList organizerEntries(const QStringList &paths) {
    QVariantList entries;
    for (const QString &path : paths) {
        const QFileInfo info(path);
        entries.append(QVariantMap{{"name", info.fileName().isEmpty() ? path : info.fileName()},
            {"url", QUrl::fromLocalFile(path)}, {"directory", info.isDir()}, {"available", info.exists()}});
    }
    return entries;
}
QVariantList FileLibrary::locations() const { return organizerEntries(m_locations); }
QVariantList FileLibrary::favorites() const { return organizerEntries(m_favorites); }
QVariantList FileLibrary::recentFiles() const { return organizerEntries(m_recentFiles); }
void FileLibrary::saveOrganizer() {
    QSettings settings;
    settings.setValue("library/locations", m_locations);
    settings.setValue("library/favorites", m_favorites);
    settings.setValue("library/recents", m_recentFiles);
    emit organizerChanged();
}
void FileLibrary::removeLocation(const QUrl &url) {
    m_locations.removeAll(url.toLocalFile());
    if (url == m_rootFolder) {
        m_rootFolder = QUrl();
        QSettings().remove("library/root");
        m_expanded.clear();
        emit rootFolderChanged();
        refresh();
    }
    saveOrganizer(); // Removes a shortcut only, never its folder or contents.
}
void FileLibrary::toggleFavorite(const QUrl &url) {
    if (!url.isLocalFile()) return;
    const QFileInfo info(url.toLocalFile());
    const QString path = info.exists() ? info.canonicalFilePath() : info.absoluteFilePath();
    if (path.isEmpty()) return;
    if (m_favorites.contains(path)) m_favorites.removeAll(path);
    else m_favorites.append(path);
    saveOrganizer();
}
void FileLibrary::recordRecentFile(const QUrl &url) {
    if (!url.isLocalFile()) return;
    const QString path = QFileInfo(url.toLocalFile()).canonicalFilePath();
    if (path.isEmpty()) return;
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > 20) m_recentFiles.removeLast();
    saveOrganizer();
}
void FileLibrary::clearRecentFiles() { m_recentFiles.clear(); saveOrganizer(); }
void FileLibrary::saveSorting() {
    QSettings settings;
    settings.setValue("library/sortMode", m_sortMode);
    settings.setValue("library/ascending", m_ascending);
    settings.setValue("library/foldersFirst", m_foldersFirst);
    emit sortingChanged();
    refresh();
}
void FileLibrary::setSortMode(int mode) { if (mode < 0 || mode > 3 || mode == m_sortMode) return; m_sortMode = mode; saveSorting(); }
void FileLibrary::setAscending(bool ascending) { if (ascending == m_ascending) return; m_ascending = ascending; saveSorting(); }
void FileLibrary::setFoldersFirst(bool enabled) { if (enabled == m_foldersFirst) return; m_foldersFirst = enabled; saveSorting(); }

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
    if (!m_locations.contains(normalized.toLocalFile())) {
        m_locations.append(normalized.toLocalFile());
        saveOrganizer();
    }
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
    auto items = QDir(path).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                                               QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    std::sort(items.begin(), items.end(), [this](const QFileInfo &a, const QFileInfo &b) {
        if (m_foldersFirst && a.isDir() != b.isDir()) return a.isDir();
        int comparison = 0;
        if (m_sortMode == 1 && a.lastModified() != b.lastModified()) comparison = a.lastModified() < b.lastModified() ? -1 : 1;
        else if (m_sortMode == 2 && a.birthTime() != b.birthTime()) comparison = a.birthTime() < b.birthTime() ? -1 : 1;
        else if (m_sortMode == 3) comparison = QString::compare(a.suffix(), b.suffix(), Qt::CaseInsensitive);
        if (!comparison) comparison = QString::compare(a.fileName(), b.fileName(), Qt::CaseInsensitive);
        if (!comparison) comparison = QString::compare(a.fileName(), b.fileName(), Qt::CaseSensitive);
        return m_ascending ? comparison < 0 : comparison > 0;
    });
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

QString FileLibrary::excerpt(const QUrl &url) const {
    if (!url.isLocalFile()) return {};
    const QFileInfo info(url.toLocalFile());
    if (!info.isFile() || !containsPath(info.canonicalFilePath()) || !isTextFile(info.fileName())) return {};
    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) return {};
    // Called only for instantiated rows; never scan full documents for a snippet.
    QString sample = QString::fromUtf8(file.read(1024));
    sample.remove(QRegularExpression(QStringLiteral("(?m)^ {0,3}#{1,6} +")));
    sample = sample.simplified();
    return sample.isEmpty() ? QStringLiteral("Empty document") : sample.left(160);
}
