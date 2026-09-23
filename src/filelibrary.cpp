#include <QDirIterator>
#include <QMap>
#include <QGuiApplication>
#include <QClipboard>
#include <QProcess>
#include <QDesktopServices>
#include "filelibrary.h"
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QRegularExpression>
#include <algorithm>
#include <QtConcurrent>
#include <QFutureWatcher>

FileLibrary::FileLibrary(QObject *parent) : QObject(parent) {
    QSettings settings;
    m_locationNames = settings.value("library/locationNames").toMap();
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
QVariantList FileLibrary::locations() const {
    auto entries = organizerEntries(m_locations);
    for (auto &entry : entries) {
        auto map = entry.toMap();
        const QString label = m_locationNames.value(map.value("url").toUrl().toLocalFile()).toString();
        if (!label.isEmpty()) map["name"] = label;
        map["directory"] = true;
        entry = map;
    }
    return entries;
}
bool FileLibrary::renameLocation(const QUrl &url, const QString &name) {
    const QString label = name.trimmed();
    if (!url.isLocalFile() || !m_locations.contains(url.toLocalFile()) || label.isEmpty()
        || label.size() > 200 || label.contains(QRegularExpression("[\\x00-\\x1f]"))) return false;
    m_locationNames[url.toLocalFile()] = label;
    saveOrganizer();
    return true;
}
bool FileLibrary::copyPath(const QUrl &url) {
    if (!url.isLocalFile() || !QDir::isAbsolutePath(url.toLocalFile())) return false;
    QGuiApplication::clipboard()->setText(url.toLocalFile());
    return true;
}
bool FileLibrary::showInFileManager(const QUrl &url) {
    if (!url.isLocalFile() || !QFileInfo::exists(url.toLocalFile())) return false;
#ifdef Q_OS_MACOS
    return QProcess::startDetached(QStringLiteral("/usr/bin/open"), {QStringLiteral("-R"), url.toLocalFile()});
#else
    return QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(url.toLocalFile()).absolutePath()));
#endif
}
QVariantList FileLibrary::favorites() const { return organizerEntries(m_favorites); }
QVariantList FileLibrary::recentFiles() const { return organizerEntries(m_recentFiles); }
void FileLibrary::saveOrganizer() {
    QSettings settings;
    settings.setValue("library/locationNames", m_locationNames);
    settings.setValue("library/locations", m_locations);
    settings.setValue("library/favorites", m_favorites);
    settings.setValue("library/recents", m_recentFiles);
    emit organizerChanged();
}
void FileLibrary::removeLocation(const QUrl &url) {
    m_locations.removeAll(url.toLocalFile());
    m_locationNames.remove(url.toLocalFile());
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
void FileLibrary::relocatedPath(const QUrl &oldUrl, const QUrl &newUrl) {
    const QString oldPath = oldUrl.toLocalFile(), newPath = newUrl.toLocalFile();
    auto relocate = [&](const QString &path) {
        return path == oldPath || path.startsWith(oldPath + "/") ? newPath + path.mid(oldPath.size()) : path;
    };
    for (auto *paths : {&m_locations, &m_favorites, &m_recentFiles})
        for (auto &path : *paths) path = relocate(path);
    QVariantMap labels;
    // A physical rename supersedes the legacy sidebar-only alias for this folder.
    for (auto it = m_locationNames.cbegin(); it != m_locationNames.cend(); ++it)
        if (it.key() != oldPath) labels[relocate(it.key())] = it.value();
    m_locationNames = labels;
    auto searches = savedSearches();
    for (auto &entry : searches) {
        auto search = entry.toMap();
        const QUrl root = search.value("root").toUrl();
        if (root.isLocalFile()) search["root"] = QUrl::fromLocalFile(relocate(root.toLocalFile()));
        entry = search;
    }
    QSettings().setValue("library/savedSearches", searches);
    emit savedSearchesChanged();
    for (auto &entry : m_history) entry = QUrl::fromLocalFile(relocate(entry.toLocalFile()));
    QSet<QString> expanded;
    for (const auto &path : m_expanded) expanded.insert(relocate(path));
    m_expanded = expanded;
    if (!m_rootFolder.isEmpty()) {
        m_rootFolder = QUrl::fromLocalFile(relocate(m_rootFolder.toLocalFile()));
        QSettings().setValue("library/root", m_rootFolder);
        emit rootFolderChanged();
    }
    saveOrganizer(); refresh();
}

void FileLibrary::renamedFile(const QUrl &oldUrl, const QUrl &newUrl) {
    const QString oldPath = QDir::cleanPath(oldUrl.toLocalFile());
    const QString newPath = QFileInfo(newUrl.toLocalFile()).canonicalFilePath();
    if (newPath.isEmpty()) return;
    for (QStringList *paths : {&m_favorites, &m_recentFiles}) {
        for (QString &path : *paths)
            if (QDir::cleanPath(path) == oldPath) path = newPath;
        paths->removeDuplicates();
    }
    saveOrganizer();
    refresh();
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
    cancelQuickSearch();
    if(m_tagCanceled) m_tagCanceled->store(true);
    ++m_tagGeneration; m_tagIndex.clear(); m_tagStatus="Refresh to scan saved files"; emit tagIndexChanged();
    m_rootFolder = normalized;
    if (!m_navigatingHistory) {
        while (m_history.size() > m_historyIndex + 1) m_history.removeLast();
        m_history.append(normalized);
        m_historyIndex = m_history.size() - 1;
        emit historyChanged();
    }
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

int FileLibrary::showFile(const QUrl &url) {
    const QFileInfo info(url.toLocalFile());
    if (!url.isLocalFile() || !info.isFile() || !isTextFile(info.fileName())) {
        setError(QStringLiteral("This document is no longer available in the library."));
        return -1;
    }
    const QString path = info.canonicalFilePath();
    if (!containsPath(path))
        setRootFolder(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
    setFilter(QString());
    revealFile(QUrl::fromLocalFile(path));
    for (int i = 0; i < m_entries.size(); ++i)
        if (m_entries.at(i).toMap().value("url").toUrl() == QUrl::fromLocalFile(path))
            return i;
    setError(QStringLiteral("This document could not be shown within the library's scan limits."));
    return -1;
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

bool FileLibrary::navigateHistory(int direction) {
    if (direction != -1 && direction != 1) return false;
    const int target = m_historyIndex + direction;
    if (target < 0 || target >= m_history.size() || !QFileInfo(m_history.at(target).toLocalFile()).isDir()) return false;
    m_navigatingHistory = true;
    setRootFolder(m_history.at(target));
    m_navigatingHistory = false;
    m_historyIndex = target;
    emit historyChanged();
    return true;
}

void FileLibrary::enclosingFolder() {
    QDir directory(m_rootFolder.toLocalFile());
    if (m_rootFolder.isLocalFile() && directory.cdUp()) setRootFolder(QUrl::fromLocalFile(directory.absolutePath()));
}

void FileLibrary::cancelQuickSearch() {
    if (m_searchCanceled) m_searchCanceled->store(true);
    ++m_searchGeneration;
    m_quickResults.clear();
    m_quickStatus.clear();
    emit quickSearchChanged();
}

void FileLibrary::quickSearch(const QString &query, bool contents) {
    cancelQuickSearch();
    const QString root = m_rootFolder.toLocalFile();
    if (root.isEmpty()) return;
    m_quickStatus = contents ? QStringLiteral("Searching saved file contents…") : QStringLiteral("Searching filenames…");
    emit quickSearchChanged();
    const int generation = m_searchGeneration;
    const auto canceled = std::make_shared<std::atomic_bool>(false);
    m_searchCanceled = canceled;
    auto *watcher = new QFutureWatcher<QVariantMap>(this);
    connect(watcher, &QFutureWatcher<QVariantMap>::finished, this, [this, watcher, generation] {
        const auto result = watcher->result();
        watcher->deleteLater();
        if (generation != m_searchGeneration) return;
        m_quickResults = result.value("results").toList();
        m_contentIndex = result.value("index").toMap();
        m_quickStatus = result.value("limited").toBool() ? QStringLiteral("Search limit reached: narrow the folder/query (100 results / 20,000 entries / 32 MiB).")
            : QStringLiteral("%1 matching files; %2 unreadable/oversized files skipped").arg(m_quickResults.size()).arg(result.value("skipped").toInt());
        emit quickSearchChanged();
    });
    watcher->setFuture(QtConcurrent::run([root, query, contents, canceled] {
        QVariantList results;
        QStringList folders{root};
        int visited = 0, skipped = 0;
        qint64 indexedBytes = 0;
        QVariantMap index;
        bool limited = false;
        while (!folders.isEmpty() && !canceled->load()) {
            const QDir directory(folders.takeLast());
            const auto entries = directory.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name);
            for (const auto &entry : entries) {
                if (canceled->load()) break;
                if (++visited > 20000 || results.size() >= 100) { limited = true; break; }
                if (entry.isDir()) {
                    if (entry.fileName() != "node_modules" && entry.fileName() != "build" && entry.fileName() != "build-tests" && entry.fileName() != "dist") folders.append(entry.absoluteFilePath());
                } else if (isTextFile(entry.fileName())) {
                    bool matches = entry.fileName().contains(query, Qt::CaseInsensitive);
                    if (contents) {
                        if (entry.size() > 262144) { ++skipped; continue; }
                        if (indexedBytes + entry.size() > 32 * 1024 * 1024) { limited = true; break; }
                        indexedBytes += entry.size();
                        const QString key = entry.absoluteFilePath();
                        QVariantMap cached;
                        // Revalidate every query; no stale filename or content results after edits.
                        QFile file(key);
                        if (!file.open(QIODevice::ReadOnly)) { ++skipped; continue; }
                        const QByteArray bytes = file.read(262145);
                        if (bytes.size() > 262144 || file.error() != QFile::NoError) { ++skipped; continue; }
                        const QString text = QString::fromUtf8(bytes);
                        cached = QVariantMap{{"text", text}, {"modified", entry.lastModified()}, {"size", entry.size()}};
                        index.insert(key, cached);
                        matches = query.startsWith("#") ? tagsIn(text).contains(query.mid(1).toCaseFolded())
                            : matches || text.contains(query, Qt::CaseInsensitive);
                    }
                    if (!matches) continue;
                    results.append(QVariantMap{{"name", entry.fileName()}, {"path", QDir(root).relativeFilePath(entry.absoluteFilePath())}, {"url", QUrl::fromLocalFile(entry.absoluteFilePath())}});
                }
            }
            if (limited) break;
        }
        return QVariantMap{{"results", results}, {"limited", limited}, {"skipped", skipped}, {"index", index}};
    }));
}

QVariantList FileLibrary::savedSearches() const {
    return QSettings().value("library/savedSearches").toList();
}
void FileLibrary::saveSearch(const QString &query, bool contents) {
    if (query.trimmed().isEmpty() || !m_rootFolder.isLocalFile()) return;
    auto list = savedSearches();
    const QVariantMap item{{"query", query.trimmed()}, {"contents", contents}, {"root", m_rootFolder}};
    list.removeAll(item);
    list.prepend(item);
    while (list.size() > 30) list.removeLast();
    QSettings().setValue("library/savedSearches", list);
    emit savedSearchesChanged();
}
void FileLibrary::removeSearch(int index) {
    auto list = savedSearches();
    if (index < 0 || index >= list.size()) return;
    list.removeAt(index);
    QSettings().setValue("library/savedSearches", list);
    emit savedSearchesChanged();
}
QStringList FileLibrary::tagsIn(const QString &markdown) {
    QStringList tags;
    QChar fence;
    int fenceLength = 0;
    const QRegularExpression fenceRe("^ {0,3}(`{3,}|~{3,})(.*)$");
    const QRegularExpression tagRe(QStringLiteral("(?:^|\\s)#([\\p{L}\\p{N}_-]+)"));
    const QRegularExpression inlineCode("(`+).*?\\1");
    for (const QString &rawLine : markdown.split('\n')) {
        QString line=rawLine;
        line.remove(QRegularExpression("^ {0,3}(?:> ?)+"));
        QString fenceLine=line; fenceLine.remove(QRegularExpression("^ {0,3}(?:[-+*]|[0-9]+[.)]) +"));
        const auto match = fenceRe.match(fenceLine);
        if (match.hasMatch()) {
            const auto marker = match.captured(1);
            if (fence.isNull()) { fence = marker.at(0); fenceLength = marker.size(); }
            else if (marker.at(0) == fence && marker.size() >= fenceLength && match.captured(2).trimmed().isEmpty()) fence = QChar();
            continue;
        }
        if (!fence.isNull() || line.startsWith("    ") || line.startsWith('\t')) continue;
        QString prose = line;
        prose.remove(inlineCode);
        auto matches = tagRe.globalMatch(prose);
        while (matches.hasNext()) tags.append(matches.next().captured(1).toCaseFolded());
    }
    tags.removeDuplicates();
    return tags;
}

FileLibrary::~FileLibrary() {
    if(m_searchCanceled) m_searchCanceled->store(true);
    if(m_tagCanceled) m_tagCanceled->store(true);
}

void FileLibrary::refreshTags() {
    if(m_tagCanceled) m_tagCanceled->store(true);
    const int generation=++m_tagGeneration; const auto root=m_rootFolder.toLocalFile();
    m_tagIndex.clear(); m_tagStatus="Scanning saved-file tags…"; emit tagIndexChanged();
    const auto canceled=std::make_shared<std::atomic_bool>(false); m_tagCanceled=canceled;
    auto *watcher=new QFutureWatcher<QVariantMap>(this);
    connect(watcher,&QFutureWatcher<QVariantMap>::finished,this,[this,watcher,generation] {
        const auto result=watcher->result(); watcher->deleteLater(); if(generation!=m_tagGeneration) return;
        m_tagIndex=result["tags"].toList(); m_tagStatus=result["status"].toString(); emit tagIndexChanged();
    });
    watcher->setFuture(QtConcurrent::run([root,canceled] {
        QStringList folders; if(!root.isEmpty()) folders.append(root);
        QMap<QString,int> counts; int visited=0, skipped=0, files=0; qint64 bytes=0; bool limited=false;
        while(!folders.isEmpty() && !canceled->load() && !limited) {
            QDirIterator entries(folders.takeLast(),QDir::AllEntries|QDir::NoDotAndDotDot|QDir::NoSymLinks);
            while(entries.hasNext() && !canceled->load()) {
                entries.next(); const auto entry=entries.fileInfo();
                if(++visited>20000) { limited=true; break; }
                if(entry.isDir()) { if(!QStringList{"node_modules","build","build-tests","dist"}.contains(entry.fileName())) folders.append(entry.absoluteFilePath()); continue; }
                if(!isTextFile(entry.fileName())) continue;
                if(entry.size()>262144) { ++skipped; continue; }
                if(bytes+entry.size()>32*1024*1024) { limited=true; break; }
                QFile file(entry.absoluteFilePath()); if(!file.open(QIODevice::ReadOnly)) { ++skipped; continue; }
                const auto content=file.read(262145); if(content.size()>262144 || file.error()!=QFile::NoError) { ++skipped; continue; }
                bytes+=content.size(); ++files;
                for(const auto &tag:tagsIn(QString::fromUtf8(content))) {
                    if(!counts.contains(tag) && counts.size()>=2000) { limited=true; break; }
                    ++counts[tag];
                }
                if(limited) break;
            }
        }
        QVariantList tags; for(auto it=counts.cbegin();it!=counts.cend();++it) tags.append(QVariantMap{{"tag",it.key()},{"count",it.value()}});
        const QString status=QString("%1 files scanned; %2 skipped%3").arg(files).arg(skipped).arg(limited?"; scan limit reached":"");
        return QVariantMap{{"tags",tags},{"status",status}};
    }));
}
