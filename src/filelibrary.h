#pragma once

#include <QObject>
#include <QFileSystemWatcher>
#include <QSet>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <atomic>
#include <memory>

// A lazy, flattened directory tree. Only expanded folders are enumerated.
class FileLibrary : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList locations READ locations NOTIFY organizerChanged)
    Q_PROPERTY(QVariantList favorites READ favorites NOTIFY organizerChanged)
    Q_PROPERTY(QVariantList recentFiles READ recentFiles NOTIFY organizerChanged)
    Q_PROPERTY(int sortMode READ sortMode WRITE setSortMode NOTIFY sortingChanged)
    Q_PROPERTY(bool ascending READ ascending WRITE setAscending NOTIFY sortingChanged)
    Q_PROPERTY(bool foldersFirst READ foldersFirst WRITE setFoldersFirst NOTIFY sortingChanged)
    Q_PROPERTY(QUrl rootFolder READ rootFolder WRITE setRootFolder NOTIFY rootFolderChanged)
    Q_PROPERTY(QString rootName READ rootName NOTIFY rootFolderChanged)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY historyChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY historyChanged)
    Q_PROPERTY(QVariantList tagIndex READ tagIndex NOTIFY tagIndexChanged)
    Q_PROPERTY(QString tagStatus READ tagStatus NOTIFY tagIndexChanged)
    Q_PROPERTY(QVariantList savedSearches READ savedSearches NOTIFY savedSearchesChanged)
    Q_PROPERTY(QVariantList quickResults READ quickResults NOTIFY quickSearchChanged)
    Q_PROPERTY(QString quickStatus READ quickStatus NOTIFY quickSearchChanged)
public:
    explicit FileLibrary(QObject *parent = nullptr);
    ~FileLibrary() override;
    Q_INVOKABLE void refreshTags();
    QVariantList tagIndex() const { return m_tagIndex; }
    QString tagStatus() const { return m_tagStatus; }
    QVariantList locations() const;
    QVariantList favorites() const;
    QVariantList recentFiles() const;
    int sortMode() const { return m_sortMode; }
    bool ascending() const { return m_ascending; }
    bool foldersFirst() const { return m_foldersFirst; }
    void setSortMode(int mode);
    void setAscending(bool ascending);
    void setFoldersFirst(bool enabled);
    Q_INVOKABLE bool addLocation(const QUrl &url);
    Q_INVOKABLE bool addFavorite(const QUrl &url);
    Q_INVOKABLE bool renameLocation(const QUrl &url, const QString &name);
    Q_INVOKABLE bool copyPath(const QUrl &url);
    Q_INVOKABLE bool showInFileManager(const QUrl &url);
    Q_INVOKABLE void removeLocation(const QUrl &url);
    Q_INVOKABLE void toggleFavorite(const QUrl &url);
    Q_INVOKABLE void clearRecentFiles();
    void recordRecentFile(const QUrl &url);
    void relocatedPath(const QUrl &oldUrl, const QUrl &newUrl);
    void renamedFile(const QUrl &oldUrl, const QUrl &newUrl);
    QUrl rootFolder() const { return m_rootFolder; }
    QString rootName() const;
    QVariantList entries() const { return m_entries; }
    QString filter() const { return m_filter; }
    QString error() const { return m_error; }
    void setRootFolder(const QUrl &folder);
    void setFilter(const QString &filter);
    Q_INVOKABLE void toggleFolder(const QUrl &folder);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString excerpt(const QUrl &url) const;
    Q_INVOKABLE QUrl createDocument(const QString &name);
    Q_INVOKABLE bool createFolder(const QString &name);
    Q_INVOKABLE void revealFile(const QUrl &url);
    Q_INVOKABLE int showFile(const QUrl &url);
    bool canGoBack() const { return m_historyIndex > 0; }
    bool canGoForward() const { return m_historyIndex + 1 < m_history.size(); }
    QVariantList quickResults() const { return m_quickResults; }
    QString quickStatus() const { return m_quickStatus; }
    Q_INVOKABLE bool navigateHistory(int direction);
    Q_INVOKABLE void enclosingFolder();
    Q_INVOKABLE void quickSearch(const QString &query, bool contents = false);
    Q_INVOKABLE void cancelQuickSearch();
    QVariantList savedSearches() const;
    Q_INVOKABLE void saveSearch(const QString &query, bool contents);
    Q_INVOKABLE void removeSearch(int index);
    static QStringList tagsIn(const QString &markdown);
    static bool isTextFile(const QString &path);

signals:
    void locationRejected(const QUrl &url, const QString &message, bool canFavorite);
    void tagIndexChanged();
    void savedSearchesChanged();
    void historyChanged();
    void quickSearchChanged();
    void organizerChanged();
    void sortingChanged();
    void rootFolderChanged();
    void entriesChanged();
    void filterChanged();
    void errorChanged();

private:
    QVariantList m_tagIndex;
    QString m_tagStatus = "Refresh to scan saved files";
    int m_tagGeneration = 0;
    std::shared_ptr<std::atomic_bool> m_tagCanceled;
    QList<QUrl> m_history;
    int m_historyIndex = -1;
    bool m_navigatingHistory = false;
    int m_searchGeneration = 0;
    std::shared_ptr<std::atomic_bool> m_searchCanceled;
    QVariantList m_quickResults;
    QVariantMap m_contentIndex;
    QString m_quickStatus;
    void appendDirectory(const QString &path, int depth, QStringList &watched);
    void setError(const QString &error);
    bool containsPath(const QString &path) const;
    QString overlappingLocation(const QString &path) const;
    void normalizeLocations();
    void saveOrganizer();
    void saveSorting();
    QVariantMap m_locationNames;
    QStringList m_locations, m_favorites, m_recentFiles;
    int m_sortMode = 0;
    bool m_ascending = true;
    bool m_foldersFirst = true;
    QUrl m_rootFolder;
    QVariantList m_entries;
    QSet<QString> m_expanded;
    QString m_filter;
    QString m_error;
    QFileSystemWatcher m_watcher;
    QTimer m_refreshTimer;
};
