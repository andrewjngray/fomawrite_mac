#pragma once

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QTemporaryFile>

// An acceptance profile keeps native Windows settings, standard paths and
// restoration intact. Only the application's namespace changes; the supplied
// directory is the stable profile identity and the home of its launch receipt.
struct AcceptanceProfile {
    QString root;
    QString applicationName;
};

inline bool prepareAcceptanceProfile(const QString &requestedRoot,
                                     AcceptanceProfile *profile, QString *error) {
    *profile = {};
    if (!QDir::isAbsolutePath(requestedRoot) || !QFileInfo(requestedRoot).isDir()) {
        *error = QStringLiteral("FOMAWRITE_PROFILE_ROOT must name an existing absolute directory.");
        return false;
    }
    const QString canonicalRoot = QFileInfo(requestedRoot).canonicalFilePath();
    if (canonicalRoot.isEmpty()) {
        *error = QStringLiteral("Cannot resolve FOMAWRITE_PROFILE_ROOT.");
        return false;
    }
    // Probe actual file creation, rather than trusting permission metadata on
    // Windows shares. QTemporaryFile removes only this newly created probe.
    QTemporaryFile probe(QDir(canonicalRoot).filePath(QStringLiteral(".fomawrite-profile-write-XXXXXX")));
    if (!probe.open() || probe.write("1", 1) != 1 || !probe.flush()) {
        *error = QStringLiteral("FOMAWRITE_PROFILE_ROOT is not writable: %1").arg(probe.errorString());
        return false;
    }
    profile->root = canonicalRoot;
    profile->applicationName = QStringLiteral("FomawriteAcceptance-")
        + QString::fromLatin1(QCryptographicHash::hash(canonicalRoot.toUtf8(),
            QCryptographicHash::Sha256).toHex().left(20));
    error->clear();
    return true;
}
