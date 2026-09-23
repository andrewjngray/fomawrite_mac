#include "workspace.h"
#include <QLocalSocket>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTimer>
#include <QThread>

InstanceBroker::InstanceBroker(QObject *parent) : QObject(parent) {
    m_server.setSocketOptions(QLocalServer::UserAccessOption);
    connect(&m_server, &QLocalServer::newConnection, this, [this] {
        while (auto *socket = m_server.nextPendingConnection()) {
            auto bytes = std::make_shared<QByteArray>();
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            QTimer::singleShot(10000, socket, [socket] { socket->disconnectFromServer(); });
            const auto receive = [this, socket, bytes] {
                bytes->append(socket->readAll());
                if (bytes->size() > 65536) { socket->abort(); return; }
                if (!bytes->endsWith('\n')) return;
                QJsonParseError parse;
                const auto document = QJsonDocument::fromJson(*bytes, &parse);
                if (parse.error != QJsonParseError::NoError || !document.isArray() || document.array().size() > 100) {
                    socket->abort(); return;
                }
                QStringList paths;
                for (const auto value : document.array()) {
                    if (!value.isString() || !QDir::isAbsolutePath(value.toString())) { socket->abort(); return; }
                    paths.append(value.toString());
                }
                emit requested(paths);
                socket->write("OK\n"); socket->flush(); socket->disconnectFromServer();
            };
            connect(socket, &QLocalSocket::readyRead, socket, receive);
            if (socket->bytesAvailable()) receive();
        }
    });
}

InstanceBroker::~InstanceBroker() { m_server.close(); m_lock.reset(); }

InstanceBroker::Result InstanceBroker::start(const QString &directory, const QString &identity, const QStringList &paths) {
    QDir().mkpath(directory);
    m_lock = std::make_unique<QLockFile>(QDir(directory).filePath(identity + ".lock"));
    m_lock->setStaleLockTime(0); // Only reclaim a dead process, never a slow live owner.
    const QString endpoint = QDir(directory).filePath(identity + ".socket");
    if (m_lock->tryLock()) {
        QLocalServer::removeServer(endpoint); // Only the lock owner can clear a stale socket.
        if (m_server.listen(endpoint)) return Owner;
        m_error = m_server.errorString(); m_lock.reset(); return Failed;
    }
    QJsonArray message;
    for (const auto &path : paths) message.append(path);
    const QByteArray bytes = QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n';
    if (bytes.size() > 65536) { m_error = "Too many file paths to forward."; return Failed; }
    for (int attempt = 0; attempt < 20; ++attempt) {
        QLocalSocket socket;
        socket.connectToServer(endpoint);
        if (!socket.waitForConnected(100)) { QThread::msleep(100); continue; }
        socket.write(bytes);
        if (!socket.waitForBytesWritten(2000)) break;
        QByteArray reply;
        while (!reply.contains('\n') && socket.waitForReadyRead(10000)) reply += socket.readAll();
        if (reply == "OK\n") return Forwarded;
        break;
    }
    m_error = "The running Omawrite could not receive this request. Try again after it finishes its current dialog.";
    return Failed;
}

QJsonArray WorkspaceStore::read(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 1024 * 1024) return {};
    const auto object = QJsonDocument::fromJson(file.readAll()).object();
    if (object["version"].toInt() != 1) return {};
    const auto windows = object["windows"].toArray();
    if (windows.size() > 100) return {};
    for (const auto value : windows) {
        if (!value.isObject() || !value.toObject().value("url").isString()) return {};
    }
    return windows;
}
bool WorkspaceStore::write(const QString &path, const QJsonArray &windows) {
    if (windows.size() > 100) return false;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    const auto bytes = QJsonDocument(QJsonObject{{"version", 1}, {"windows", windows}}).toJson(QJsonDocument::Compact);
    return file.write(bytes) == bytes.size() && file.commit();
}


QRect WorkspaceStore::visibleGeometry(const QRect &saved,const QRect &available,const QSize &minimum) {
    const int width=qBound(minimum.width(),saved.width(),qMax(minimum.width(),available.width()));
    const int height=qBound(minimum.height(),saved.height(),qMax(minimum.height(),available.height()));
    return QRect(qBound(available.left(),saved.x(),qMax(available.left(),available.right()-width+1)),
                 qBound(available.top(),saved.y(),qMax(available.top(),available.bottom()-height+1)),width,height);
}
