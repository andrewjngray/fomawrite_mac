#pragma once

// Windows activation is a small atomic marker, never a directory or symlink.
// These helpers are platform-neutral so failure/rollback contracts run on Mac
// too; the Mac updater itself retains its existing atomic current symlink.
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace HarperActivation {
inline bool validVersion(const QString &version) {
    static const QRegularExpression pattern(QStringLiteral("\\A[0-9]{1,6}\\.[0-9]{1,6}\\.[0-9]{1,6}\\z"));
    return pattern.match(version).hasMatch();
}
inline bool plain(const QString &path, bool directory) {
    const QFileInfo info(path);
    if (info.isSymLink() || (directory ? !info.isDir() : !info.isFile())) return false;
#ifdef Q_OS_WIN
    const QString nativePath = QDir::toNativeSeparators(path);
    const auto native = reinterpret_cast<LPCWSTR>(nativePath.utf16());
    const DWORD attributes = GetFileAttributesW(native);
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
    if (!directory) {
        const HANDLE handle = CreateFileW(native, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) return false;
        BY_HANDLE_FILE_INFORMATION state{};
        const bool single = GetFileInformationByHandle(handle, &state) && state.nNumberOfLinks == 1;
        CloseHandle(handle);
        if (!single) return false;
    }
#endif
    return true;
}
inline QString selectedVersion(const QString &root) {
    if (!plain(root, true)) return {};
    const QString path = root + QStringLiteral("/current-version");
    if (!plain(path, false) || QFileInfo(path).size() > 64) return {};
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    const QByteArray bytes = file.readAll();
    QString value = QString::fromUtf8(bytes);
    if (value.endsWith('\n')) value.chop(1);
    if (value != QLatin1String("bundled") && !validVersion(value)) return {};
    return bytes == value.toUtf8() || bytes == value.toUtf8() + '\n' ? value : QString();
}
inline QString directory(const QString &root, const QString &version) {
    if (!validVersion(version) || !plain(root, true)) return {};
    const QString path = root + '/' + version;
    if (!plain(path, true) || QFileInfo(path).canonicalPath() != QFileInfo(root).canonicalFilePath()) return {};
    for (const char *name : {"index.js", "binary.js", "BinaryModule.js", "harper_wasm_bg.wasm", "VERSION"}) {
        const QString file = path + '/' + QString::fromLatin1(name);
        if (!plain(file, false) || QFileInfo(file).size() == 0 || QFileInfo(file).canonicalPath() != QFileInfo(path).canonicalFilePath()) return {};
    }
    QFile current(path + QStringLiteral("/VERSION"));
    if (current.size() > 64 || !current.open(QIODevice::ReadOnly) || QString::fromUtf8(current.readAll()).trimmed() != version) return {};
    QFile wasm(path + QStringLiteral("/harper_wasm_bg.wasm"));
    if (!wasm.open(QIODevice::ReadOnly) || wasm.read(4) != QByteArray("\0asm", 4)) return {};
    return path;
}
inline bool commit(const QString &root, const QString &version, QString *error) {
    if ((!validVersion(version) && version != QLatin1String("bundled")) || !plain(root, true)) {
        *error = QStringLiteral("the activation folder or version is invalid"); return false;
    }
    const QString path = root + QStringLiteral("/current-version");
    if (QFileInfo::exists(path) && !plain(path, false)) {
        *error = QStringLiteral("the activation marker is not a plain file"); return false;
    }
    QSaveFile file(path); file.setDirectWriteFallback(false);
    const QByteArray bytes = version.toUtf8() + '\n';
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        *error = QStringLiteral("the activation marker could not be committed: ") + file.errorString(); return false;
    }
    return true;
}
} // namespace HarperActivation
