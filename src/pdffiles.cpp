#include "pdffiles.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPdfDocument>
#include <QTemporaryFile>
#ifdef Q_OS_WIN
#include <io.h>
#include <windows.h>
#else
#include <cstdio>
#include <unistd.h>
#endif

namespace PdfFiles {

namespace {
QString keyFor(const QString &pdfPath) {
    const QFileInfo info(pdfPath);
    QString key = info.exists() ? info.canonicalFilePath() : info.absoluteFilePath();
#ifdef Q_OS_WIN
    key = key.toLower();
#endif
    return QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1).toHex().left(20));
}

QString storeCopy(const QString &base, const QString &pdfPath, const QByteArray &bytes, int keep, QString *stored) {
    const QString folder = recoveryFolderFor(base, pdfPath);
    if (!QDir().mkpath(folder)) return QStringLiteral("the recovery folder could not be created");
    QFile named(QDir(folder).filePath(QStringLiteral("path.txt")));
    if (named.open(QIODevice::WriteOnly | QIODevice::Truncate)) named.write(QFileInfo(pdfPath).absoluteFilePath().toUtf8());
    qint64 stamp = QDateTime::currentMSecsSinceEpoch();
    QString file;
    do file = QDir(folder).filePath(QStringLiteral("%1.pdf").arg(stamp++, 16, 10, QLatin1Char('0')));
    while (QFileInfo::exists(file));
    QFile out(file);
    if (!out.open(QIODevice::WriteOnly) || out.write(bytes) != bytes.size() || !out.flush()) {
        out.close();
        QFile::remove(file);
        return QStringLiteral("the copy could not be written");
    }
    out.close();
    *stored = file;
    const QStringList all = QDir(folder).entryList({QStringLiteral("*.pdf")}, QDir::Files, QDir::Name);
    for (int i = 0; i < all.size() - qMax(1, keep); ++i) QFile::remove(QDir(folder).filePath(all.at(i)));
    return QString();
}

bool replaceFile(const QString &from, const QString &to) {
#ifdef Q_OS_WIN
    return MoveFileExW(reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(from).utf16()),
                       reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(to).utf16()),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(QFile::encodeName(from).constData(), QFile::encodeName(to).constData()) == 0;
#endif
}
}

QString recoveryFolderFor(const QString &base, const QString &pdfPath) {
    return QDir(base).filePath(keyFor(pdfPath));
}

QStringList recoveryCopies(const QString &base, const QString &pdfPath) {
    const QDir folder(recoveryFolderFor(base, pdfPath));
    QStringList copies;
    const QStringList names = folder.entryList({QStringLiteral("*.pdf")}, QDir::Files, QDir::Name | QDir::Reversed);
    for (const QString &name : names) copies << folder.filePath(name);
    return copies;
}

bool opens(const QByteArray &bytes, int pages, QString *why) {
    QByteArray copy = bytes;
    QBuffer buffer(&copy);
    buffer.open(QIODevice::ReadOnly);
    QPdfDocument document;
    document.load(&buffer);
    if (document.status() != QPdfDocument::Status::Ready) {
        if (why) *why = QStringLiteral("it could not be opened again");
        return false;
    }
    if (pages > 0 && document.pageCount() != pages) {
        if (why) *why = QStringLiteral("it has %1 pages instead of %2").arg(document.pageCount()).arg(pages);
        return false;
    }
    return true;
}

QString writeVerified(const Options &o) {
    const QFileInfo given(o.path);
    const bool exists = given.exists();
    const QString real = exists ? given.canonicalFilePath() : given.absoluteFilePath();
    const QFileInfo target(real);
    const QString name = target.fileName();
    if (exists && !target.isFile()) return QStringLiteral("%1 is not a file.").arg(name);
    if (exists && !target.isWritable()) return QStringLiteral("%1 is read-only. Nothing was saved.").arg(name);
    if (!target.absoluteDir().exists()) return QStringLiteral("The folder for %1 does not exist. Nothing was saved.").arg(name);
    QByteArray previous;
    if (exists) {
        QFile existing(real);
        if (!existing.open(QIODevice::ReadOnly)) return QStringLiteral("%1 could not be read before saving. Nothing was saved.").arg(name);
        previous = existing.readAll();
    }
    const QString changed = QStringLiteral("The PDF changed on disk. Your page changes were not saved.");
    if (o.requirePrevious && previous != o.previous) return changed;

    // The temporary file is removed with this object unless the replace succeeds.
    QTemporaryFile temporary(target.absoluteDir().filePath(QStringLiteral(".fomawrite-XXXXXX.tmp")));
    if (!temporary.open())
        return QStringLiteral("Could not create a temporary file next to %1; is the folder writable? Nothing was saved.").arg(name);
    const QString writeError = QStringLiteral("Could not write the new %1 (the disk may be full). The original is unchanged.").arg(name);
    if (o.failAt == QLatin1String("write")) { temporary.write(o.bytes.left(o.bytes.size() / 2)); return writeError; }
    if (temporary.write(o.bytes) != o.bytes.size() || !temporary.flush()) return writeError;
#ifdef Q_OS_WIN
    FlushFileBuffers(reinterpret_cast<HANDLE>(_get_osfhandle(temporary.handle())));
#else
    fsync(temporary.handle());
#endif
    temporary.close();

    // Verify what is on disk, not what we meant to write.
    {
        QFile back(temporary.fileName());
        QByteArray written = back.open(QIODevice::ReadOnly) ? back.readAll() : QByteArray();
        back.close();
        if (o.failAt == QLatin1String("verify")) written.truncate(written.size() / 2);
        QString why;
        if (written != o.bytes) why = QStringLiteral("it was not written completely");
        else if (!opens(written, o.expectedPages, &why)) {}
        if (!why.isEmpty())
            return QStringLiteral("The new PDF was not saved because it could not be verified (%1). The original is unchanged.").arg(why);
    }
    // The previous bytes are kept before they are replaced.
    QString kept;
    if (exists && !o.recoveryFolder.isEmpty()) {
        QString why = o.failAt == QLatin1String("recovery") ? QStringLiteral("test failure")
                                                           : storeCopy(o.recoveryFolder, real, previous, o.keepCopies, &kept);
        if (!why.isEmpty())
            return QStringLiteral("Could not keep a copy of the previous PDF (%1), so nothing was changed.").arg(why);
    }
    if (exists) QFile::setPermissions(temporary.fileName(), QFile::permissions(real));
    // Narrow the race with another writer: look again just before replacing.
    bool failed = o.failAt == QLatin1String("replace");
    if (!failed && o.requirePrevious) {
        QFile again(real);
        failed = !again.open(QIODevice::ReadOnly) || again.readAll() != o.previous;
        if (failed) { if (!kept.isEmpty()) QFile::remove(kept); return changed; }
    }
    if (failed || !replaceFile(temporary.fileName(), real)) {
        if (!kept.isEmpty()) QFile::remove(kept);
        return QStringLiteral("Could not replace %1 (is it open in another program?). The original is unchanged.").arg(name);
    }
    temporary.setAutoRemove(false);
    return QString();
}

}
