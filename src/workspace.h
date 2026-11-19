#pragma once
#include <QObject>
#include <QStringList>
#include <QJsonArray>
#include <QJsonObject>
#include <QRect>
#include <QSize>
#include <QLocalServer>
#include <QLockFile>
#include <memory>

// One owner per installed app; another launch forwards its open requests.
class InstanceBroker : public QObject {
    Q_OBJECT
public:
    enum Result { Owner, Forwarded, Failed };
    explicit InstanceBroker(QObject *parent = nullptr);
    ~InstanceBroker() override;
    Result start(const QString &directory, const QString &identity, const QStringList &paths);
    QString error() const { return m_error; }
signals:
    void requested(const QStringList &paths);
private:
    QLocalServer m_server;
    std::unique_ptr<QLockFile> m_lock;
    QString m_error;
};

class Backend;

class WorkspaceStore {
public:
    static QRect visibleGeometry(const QRect &saved, const QRect &available, const QSize &minimum);
    static QJsonArray read(const QString &path);
    static bool write(const QString &path, const QJsonArray &windows);
    // The document part of a window's checkpoint: {url, cursor, paneState}. A
    // window in PDF mode is checkpointed by its PDF (also with untitled notes),
    // which reopens it in PDF mode with its notes file.
    static QJsonObject documentEntry(Backend &backend, QObject *window);
    // Applies a checkpoint's pane state and cursor to a window whose document is
    // already open; the preview width is clamped to the screen's width.
    static void restoreDocumentEntry(QObject *window, const QJsonObject &entry, int screenWidth);
};
