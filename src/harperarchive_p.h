#pragma once

// Private, deliberately limited USTAR reader for the verified Harper package.
// The trusted OS tar converts compressed input to this representation; no
// archive entry is extracted by tar or written before the whole stream passes.
#include <QByteArray>
#include <QList>
#include <QSet>
#include <QStringList>
#include <QRegularExpression>
#include <limits>

namespace HarperArchive {
constexpr qint64 MaxOutputBytes = 256LL << 20; // pinned engine is about 16 MiB
constexpr qint64 MaxFileBytes = 128LL << 20;
constexpr int MaxEntries = 64;
struct Entry { QString name; qsizetype offset; qsizetype size; bool directory; };

inline bool pathAllowed(const QString &entry) {
    if (entry.isEmpty() || entry.startsWith('/') || entry.contains('\\')) return false;
    for (const QChar c : entry)
        if (c.unicode() < 0x20 || c.unicode() == 0x7f || QStringLiteral(":<>\"|?*").contains(c)) return false;
    QStringList parts = entry.split('/');
    const bool directory = parts.last().isEmpty();
    if (directory) parts.removeLast();
    static const QRegularExpression device(QStringLiteral("\\A(?:CON|PRN|AUX|NUL|CLOCK\\$|CONIN\\$|CONOUT\\$|COM[1-9¹²³]|LPT[1-9¹²³])(?:\\..*)?\\z"), QRegularExpression::CaseInsensitiveOption);
    for (const QString &part : parts)
        if (part.isEmpty() || part == QLatin1String(".") || part == QLatin1String("..")
            || part.endsWith('.') || part.endsWith(' ') || part.contains('~')
            || device.match(part).hasMatch() || device.match(part.section('.', 0, 0).trimmed()).hasMatch()) return false;
    if (parts.isEmpty() || parts.first() != QLatin1String("package")) return false;
    if (parts.size() == 1) return directory;
    if (parts.size() == 2) return !directory || parts.at(1) == QLatin1String("dist");
    if (parts.size() != 3 || parts.at(1) != QLatin1String("dist") || directory) return false;
    const QString name = parts.last();
    return name == QLatin1String("harper_wasm_bg.wasm") || name == QLatin1String("harper_wasm_slim_bg.wasm")
        || name.startsWith(QLatin1String("LICENSE")) || name.endsWith(QLatin1String(".js"))
        || name.endsWith(QLatin1String(".d.ts")) || name.endsWith(QLatin1String(".json"));
}
inline bool number(const QByteArray &field, quint64 *value) {
    quint64 result = 0; bool digit = false, ended = false;
    for (char c : field) {
        if (c == ' ' && !digit && !ended) continue;
        if (c == '\0' || c == ' ') { ended = true; continue; }
        if (ended || c < '0' || c > '7' || result > (std::numeric_limits<quint64>::max() - 7) / 8) return false;
        digit = true; result = result * 8 + quint64(c - '0');
    }
    *value = result;
    return digit;
}

inline bool field(const QByteArray &bytes, QString *out) {
    const auto end = bytes.indexOf('\0');
    const QByteArray value = end < 0 ? bytes : bytes.left(end);
    if (end >= 0 && bytes.mid(end) != QByteArray(bytes.size() - end, '\0')) return false;
    *out = QString::fromUtf8(value);
    return out->toUtf8() == value;
}
inline bool parse(const QByteArray &bytes, QList<Entry> *entries, QString *error) {
    entries->clear();
    const auto fail = [&](const QString &why) { *error = QStringLiteral("unsafe Harper archive: ") + why; entries->clear(); return false; };
    if (bytes.size() > MaxOutputBytes || bytes.size() % 512 || bytes.size() < 1024) return fail(QStringLiteral("invalid or oversized USTAR stream"));
    QSet<QString> seen, files;
    qsizetype at = 0;
    while (at + 512 <= bytes.size()) {
        const QByteArray header = bytes.mid(at, 512);
        if (header == QByteArray(512, '\0')) {
            if (bytes.size() - at < 1024 || bytes.mid(at) != QByteArray(bytes.size() - at, '\0')) return fail(QStringLiteral("invalid archive end/padding"));
            for (const auto &entry : *entries) {
                QString parent = entry.name;
                while (parent.contains('/')) {
                    parent.truncate(parent.lastIndexOf('/'));
                    if (files.contains(parent.toCaseFolded())) return fail(QStringLiteral("file/directory collision"));
                }
            }
            return !entries->isEmpty() || fail(QStringLiteral("empty archive"));
        }
        if (entries->size() >= MaxEntries) return fail(QStringLiteral("too many entries"));
        if (header.mid(257, 6) != QByteArray("ustar\0", 6) || header.mid(263, 2) != "00") return fail(QStringLiteral("unsupported archive header"));
        quint64 expected = 0, size = 0;
        if (!number(header.mid(148, 8), &expected) || !number(header.mid(124, 12), &size)) return fail(QStringLiteral("invalid numeric header"));
        quint64 checksum = 0;
        for (int i = 0; i < 512; ++i) checksum += (i >= 148 && i < 156) ? ' ' : uchar(header.at(i));
        if (checksum != expected) return fail(QStringLiteral("header checksum mismatch"));
        QString name, prefix, link;
        if (!field(header.left(100), &name) || !field(header.mid(345, 155), &prefix)
            || !field(header.mid(157, 100), &link) || name.isEmpty()) return fail(QStringLiteral("invalid or empty entry name"));
        if (!prefix.isEmpty()) name = prefix + '/' + name;
        const char type = header.at(156);
        if ((type != '0' && type != '\0' && type != '5') || !link.isEmpty()) return fail(QStringLiteral("link or special file"));
        const bool directory = type == '5';
        if (directory && !name.endsWith('/')) name += '/';
        if ((!directory && name.endsWith('/')) || !pathAllowed(name)) return fail(QStringLiteral("entry outside package/dist or Windows path alias: ") + name.left(80));
        if (directory && size) return fail(QStringLiteral("directory payload"));
        if (size > quint64(MaxFileBytes)) return fail(QStringLiteral("file exceeds uncompressed bound"));
        const quint64 padded = ((size + 511) / 512) * 512;
        if (padded > quint64(bytes.size() - at - 512)) return fail(QStringLiteral("truncated entry"));
        const auto paddingSize = qsizetype(padded - size);
        if (bytes.mid(at + 512 + qsizetype(size), paddingSize) != QByteArray(paddingSize, '\0'))
            return fail(QStringLiteral("nonzero entry padding"));
        QString key = name; if (directory) key.chop(1);
        key = key.toCaseFolded();
        if (seen.contains(key)) return fail(QStringLiteral("duplicate or case-colliding entry"));
        seen.insert(key); if (!directory) files.insert(key);
        entries->append({name, at + 512, qsizetype(size), directory});
        at += 512 + qsizetype(padded);
    }
    return fail(QStringLiteral("missing archive end"));
}
} // namespace HarperArchive
