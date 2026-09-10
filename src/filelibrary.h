#pragma once

#include <QObject>
#include <QFileSystemWatcher>
#include <QSet>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

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
public:
    explicit FileLibrary(QObject *parent = nullptr);
    QVariantList locations() const;
    QVariantList favorites() const;
    QVariantList recentFiles() const;
    int sortMode() const { return m_sortMode; }
    bool ascending() const { return m_ascending; }
    bool foldersFirst() const { return m_foldersFirst; }
    void setSortMode(int mode);
    void setAscending(bool ascending);
    void setFoldersFirst(bool enabled);
    Q_INVOKABLE void removeLocation(const QUrl &url);
    Q_INVOKABLE void toggleFavorite(const QUrl &url);
    Q_INVOKABLE void clearRecentFiles();
    void recordRecentFile(const QUrl &url);
    QUrl rootFolder() const { return m_rootFolder; }
    QString rootName() const;
    QVariantList entries() const { return m_entries; }
    QString filter() const { return m_filter; }
    QString error() const { return m_error; }
    void setRootFolder(const QUrl &folder);
    void setFilter(const QString &filter);
    Q_INVOKABLE void toggleFolder(const QUrl &folder);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE QUrl createDocument(const QString &name);
    Q_INVOKABLE bool createFolder(const QString &name);
    Q_INVOKABLE void revealFile(const QUrl &url);
    static bool isTextFile(const QString &path);

signals:
    void organizerChanged();
    void sortingChanged();
    void rootFolderChanged();
    void entriesChanged();
    void filterChanged();
    void errorChanged();

private:
    void appendDirectory(const QString &path, int depth, QStringList &watched);
    void setError(const QString &error);
    bool containsPath(const QString &path) const;
    void saveOrganizer();
    void saveSorting();
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
