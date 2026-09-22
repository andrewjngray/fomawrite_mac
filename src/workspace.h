#pragma once
#include <QObject>
#include <QStringList>
#include <QJsonArray>
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

class WorkspaceStore {
public:
    static QJsonArray read(const QString &path);
    static bool write(const QString &path, const QJsonArray &windows);
};
