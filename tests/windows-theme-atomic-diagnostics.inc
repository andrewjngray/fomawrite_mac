// Synthetic Cycle125 fixture diagnostics only; never used by the application.
#ifdef Q_OS_WIN
#include <io.h>
#include <iterator>
#include <QCryptographicHash>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QSaveFile>
#include <QScopeGuard>
#include <restartmanager.h>

namespace {
QString cycle125FileTime(FILETIME value) {
    return QString::number((quint64(value.dwHighDateTime) << 32) | value.dwLowDateTime);
}
QJsonObject cycle125HandleSnapshot(HANDLE handle) {
    QJsonObject result;
    BY_HANDLE_FILE_INFORMATION info = {};
    if (GetFileInformationByHandle(handle, &info)) {
        result.insert("available", true);
        result.insert("volumeSerialHex", QString::number(info.dwVolumeSerialNumber, 16));
        result.insert("fileIndexHex", QString::number((quint64(info.nFileIndexHigh) << 32) | info.nFileIndexLow, 16));
        result.insert("attributesHex", QString::number(info.dwFileAttributes, 16));
        result.insert("readOnly", bool(info.dwFileAttributes & FILE_ATTRIBUTE_READONLY));
        result.insert("sizeBytes", QString::number((quint64(info.nFileSizeHigh) << 32) | info.nFileSizeLow));
        result.insert("lastWriteFiletime", cycle125FileTime(info.ftLastWriteTime));
    } else {
        const DWORD code = GetLastError();
        result.insert("available", false); result.insert("informationError", double(code));
    }
    WCHAR path[32768];
    const DWORD count = GetFinalPathNameByHandleW(handle, path, DWORD(std::size(path)), 0);
    if (count && count < std::size(path)) result.insert("nativePath", QString::fromWCharArray(path, int(count)));
    else {
        const DWORD code = count ? ERROR_INSUFFICIENT_BUFFER : GetLastError();
        result.insert("pathError", double(code));
    }
    return result;
}
QJsonObject cycle125TargetSnapshot(const QString &path) {
    const QString native = QDir::toNativeSeparators(path);
    QJsonObject result{{"path", path}};
    WIN32_FILE_ATTRIBUTE_DATA attributes = {};
    if (GetFileAttributesExW(reinterpret_cast<LPCWSTR>(native.utf16()), GetFileExInfoStandard, &attributes)) {
        result.insert("pathAttributesHex", QString::number(attributes.dwFileAttributes, 16));
        result.insert("pathSizeBytes", QString::number((quint64(attributes.nFileSizeHigh) << 32) | attributes.nFileSizeLow));
        result.insert("pathLastWriteFiletime", cycle125FileTime(attributes.ftLastWriteTime));
    } else { const DWORD code = GetLastError(); result.insert("attributesError", double(code)); }
    // Full sharing avoids adding a diagnostic handle that blocks replacement.
    const HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        const DWORD code = GetLastError(); result.insert("openError", double(code)); return result;
    }
    const auto close = qScopeGuard([&] { CloseHandle(handle); });
    result.insert("handle", cycle125HandleSnapshot(handle));
    // Only public synthetic fixture files are read; bound diagnostics to 1MiB.
    QCryptographicHash hash(QCryptographicHash::Sha256);
    char buffer[16384]; quint64 total = 0;
    constexpr quint64 limit = 1024 * 1024;
    while (total <= limit) {
        DWORD count = 0;
        if (!ReadFile(handle, buffer, DWORD(qMin<quint64>(sizeof(buffer), limit + 1 - total)), &count, nullptr)) {
            const DWORD code = GetLastError(); result.insert("readError", double(code)); break;
        }
        total += count;
        if (total > limit) { result.insert("hashUnavailable", "synthetic-file limit exceeded"); break; }
        if (!count) { result.insert("sha256", QString::fromLatin1(hash.result().toHex())); break; }
        hash.addData(QByteArrayView(buffer, count));
    }
    result.insert("bytesRead", QString::number(total));
    result.insert("snapshotNotAtomic", true);
    return result;
}
QJsonObject cycle125StagedSnapshot(QSaveFile &file) {
    // QFileDevice::handle() is a CRT descriptor, not a Win32 HANDLE:
    // doc.qt.io/qt-6/qfiledevice.html#handle; Microsoft _get_osfhandle docs.
    const int descriptor = file.handle();
    if (descriptor < 0) return {{"available", false}, {"reason", "QSaveFile CRT descriptor unavailable"}};
    const intptr_t native = _get_osfhandle(descriptor);
    if (native == -1 || native == -2) return {{"available", false}, {"reason", "CRT-to-native handle conversion unavailable"}};
    // Borrowed from Qt: do not close, flush, seek or change the descriptor.
    auto result = cycle125HandleSnapshot(reinterpret_cast<HANDLE>(native));
    result.insert("borrowedQtHandle", true);
    result.insert("nativeSizeMayExcludeQtBufferedWrites", true);
    return result;
}
QJsonObject cycle125ResourceOwners(const QString &path) {
    QJsonObject result{{"resource", path}, {"limit", 64},
        {"limitation", "Post-failure target-only snapshot may miss transient owners; listed users are not proven rename blockers. No processes are stopped or restarted."}};
    HMODULE library = LoadLibraryExW(L"Rstrtmgr.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!library) { const DWORD code = GetLastError(); result.insert("loadError", double(code)); return result; }
    const auto unload = qScopeGuard([&] { FreeLibrary(library); });
    using Start = decltype(&::RmStartSession);
    using Register = decltype(&::RmRegisterResources);
    using List = decltype(&::RmGetList);
    using End = decltype(&::RmEndSession);
    const auto start = reinterpret_cast<Start>(GetProcAddress(library, "RmStartSession"));
    const auto add = reinterpret_cast<Register>(GetProcAddress(library, "RmRegisterResources"));
    const auto list = reinterpret_cast<List>(GetProcAddress(library, "RmGetList"));
    const auto end = reinterpret_cast<End>(GetProcAddress(library, "RmEndSession"));
    if (!start || !add || !list || !end) { result.insert("unavailable", "Restart Manager exports missing"); return result; }
    DWORD session = 0; WCHAR key[CCH_RM_SESSION_KEY + 1] = {};
    const DWORD startError = start(&session, 0, key);
    result.insert("startError", double(startError));
    if (startError != ERROR_SUCCESS) return result;
    bool active = true;
    const auto finish = qScopeGuard([&] { if (active) end(session); });
    const QString native = QDir::toNativeSeparators(path);
    LPCWSTR resource = reinterpret_cast<LPCWSTR>(native.utf16());
    const DWORD registerError = add(session, 1, &resource, 0, nullptr, 0, nullptr);
    result.insert("registerError", double(registerError));
    if (registerError == ERROR_SUCCESS) {
        RM_PROCESS_INFO owners[64] = {};
        UINT needed = 0, count = UINT(std::size(owners)); DWORD rebootReasons = 0;
        // One bounded snapshot; ERROR_MORE_DATA is reported, never retried.
        const DWORD listError = list(session, &needed, &count, owners, &rebootReasons);
        result.insert("listError", double(listError)); result.insert("needed", double(needed));
        result.insert("rebootReasons", double(rebootReasons));
        QJsonArray processes;
        if (listError == ERROR_SUCCESS && count <= std::size(owners)) {
            for (UINT i = 0; i < count; ++i) {
                const auto &owner = owners[i];
                processes.append(QJsonObject{{"pid", double(owner.Process.dwProcessId)},
                    {"processStartFiletime", cycle125FileTime(owner.Process.ProcessStartTime)},
                    {"application", QString::fromWCharArray(owner.strAppName)},
                    {"service", QString::fromWCharArray(owner.strServiceShortName)},
                    {"applicationType", int(owner.ApplicationType)}, {"status", double(owner.AppStatus)},
                    {"sessionId", double(owner.TSSessionId)}, {"restartable", bool(owner.bRestartable)}});
            }
        } else if (listError == ERROR_MORE_DATA) result.insert("unavailable", "owner list exceeded fixed capacity; no retry");
        else if (listError == ERROR_SUCCESS) result.insert("unavailable", "Restart Manager returned an invalid count");
        result.insert("processes", processes);
    }
    result.insert("endError", double(end(session))); active = false;
    return result;
}
}
#endif
