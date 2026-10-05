#include "publishingthemes.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QVariantMap>

namespace {
constexpr qint64 MaxCss = 1024 * 1024;
constexpr qint64 MaxAsset = 4 * 1024 * 1024;
constexpr qint64 MaxFont = 32 * 1024 * 1024;
constexpr qint64 MaxTotal = 64 * 1024 * 1024;
constexpr int MaxFiles = 64;
constexpr int MaxDepth = 8;
struct Import {
    QString root, failure;
    QSet<QString> stack;
    qint64 bytes = 0;
    int files = 0;
    bool omittedExport = false;
    bool allowMissingFonts = false;
    QStringList missingFonts;
    bool fail(const QString &message) { failure = message; return false; }
    bool read(const QString &path, qint64 limit, QByteArray &out) {
        QFileInfo info(path);
        if (!info.isFile() || info.isSymLink() || info.canonicalFilePath() != info.absoluteFilePath()
            || !info.canonicalFilePath().startsWith(root + QLatin1Char('/')))
            return fail(QStringLiteral("Theme resources must be regular files inside the selected CSS folder, without symlinks: %1").arg(info.fileName()));
        if (++files > MaxFiles || info.size() > limit || bytes + info.size() > MaxTotal)
            return fail(QStringLiteral("Theme exceeds the import limits (64 files, 1 MB per imported CSS file, 4 MB per image, 32 MB per font, 64 MB total)."));
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return fail(QStringLiteral("Cannot read theme resource: %1").arg(info.fileName()));
        out = file.read(limit + 1);
        bytes += out.size();
        return out.size() <= limit && bytes <= MaxTotal ? true : fail(QStringLiteral("Theme resource exceeds the import size limit."));
    }
    bool localPath(const QString &value, const QString &base, QString &path) {
        QUrl url(value);
        if (!url.isRelative() || value.startsWith('/') || value.contains('?') || value.contains('#') || value.contains(QChar(0)))
            return fail(QStringLiteral("Only relative local theme resources are supported: %1").arg(value.left(120)));
        const QString decoded = QUrl::fromPercentEncoding(value.toUtf8());
        if (decoded.contains('\\') || decoded.startsWith('/') || decoded.split('/').contains(QStringLiteral("..")))
            return fail(QStringLiteral("Theme resource paths cannot traverse parent folders."));
        path = QDir::cleanPath(QDir(base).absoluteFilePath(decoded));
        return true;
    }
    bool process(const QString &path, QString &out, int depth = 0, qint64 limit = MaxCss) {
        if (depth > MaxDepth || stack.contains(path)) return fail(QStringLiteral("Theme CSS imports are recursive or deeper than eight levels."));
        QByteArray data;
        if (!read(path, limit, data)) return false;
        QString source = QString::fromUtf8(data);
        if (source.toUtf8() != data && !(data.startsWith("\xef\xbb\xbf") && source.toUtf8() == data.mid(3)))
            return fail(QStringLiteral("Theme CSS must use UTF-8 encoding."));
        stack.insert(path);
        if (!transform(source, QFileInfo(path).absolutePath(), out, depth)) return false;
        stack.remove(path);
        return true;
    }
    bool transform(QString source, const QString &base, QString &out, int depth) {
        // Remove comments with a small lexer so comment delimiters inside strings survive.
        QString cleaned;
        QChar quote;
        for (qsizetype i = 0; i < source.size(); ++i) {
            const QChar c = source[i];
            if (!quote.isNull()) {
                // Typora uses hexadecimal glyph escapes in content strings. Decode
                // these before URL checks; escapes outside strings stay unsupported.
                if (c == '\\') {
                    QString digits;
                    qsizetype end = i + 1;
                    while (end < source.size() && digits.size() < 6
                           && QStringLiteral("0123456789abcdefABCDEF").contains(source[end])) digits += source[end++];
                    bool ok = false;
                    const char32_t codepoint = static_cast<char32_t>(digits.toUInt(&ok, 16));
                    if (!ok || codepoint < 32 || codepoint > 0x10ffff || (codepoint >= 0xd800 && codepoint <= 0xdfff)
                        || codepoint == 34 || codepoint == 39 || codepoint == 92 || codepoint == 60)
                        return fail(QStringLiteral("Theme CSS contains an unsupported string escape."));
                    cleaned += QString::fromUcs4(&codepoint, 1);
                    if (end < source.size() && source[end].isSpace()) ++end;
                    i = end - 1;
                    continue;
                }
                cleaned += c; if (c == quote) quote = {}; continue;
            }
            if (c == '\'' || c == '"') { quote = c; cleaned += c; continue; }
            if (c == '/' && i + 1 < source.size() && source[i + 1] == '*') {
                const auto end = source.indexOf(QStringLiteral("*/"), i + 2);
                if (end < 0) return fail(QStringLiteral("Theme CSS has an unterminated comment."));
                cleaned += ' '; i = end + 1; continue;
            }
            cleaned += c;
        }
        if (!quote.isNull()) return fail(QStringLiteral("Theme CSS has an unterminated string."));
        source = cleaned;
        if (source.contains('\\') || source.contains('<') || source.contains(QChar(0)))
            return fail(QStringLiteral("Theme CSS contains unsupported escapes or unsafe markup."));
        const auto flags = QRegularExpression::CaseInsensitiveOption;
        // Typora URLs can contain semicolons (Google Fonts weight lists).
        // Only a semicolon outside quotes and parentheses ends the directive.
        QChar directiveQuote;
        const QString directive = QStringLiteral("@include-when-export");
        for (qsizetype i = 0; i < source.size(); ++i) {
            const QChar c = source[i];
            if (!directiveQuote.isNull()) { if (c == directiveQuote) directiveQuote = {}; continue; }
            if (c == '\'' || c == '"') { directiveQuote = c; continue; }
            if (c != '@' || source.mid(i, directive.size()).compare(directive, Qt::CaseInsensitive) != 0) continue;
            const auto afterName = i + directive.size();
            if (afterName < source.size() && (source[afterName].isLetterOrNumber() || source[afterName] == '-' || source[afterName] == '_')) continue;
            int parentheses = 0;
            QChar valueQuote;
            qsizetype end = afterName;
            for (; end < source.size(); ++end) {
                const QChar value = source[end];
                if (!valueQuote.isNull()) { if (value == valueQuote) valueQuote = {}; continue; }
                if (value == '\'' || value == '"') { valueQuote = value; continue; }
                if (value == '(') ++parentheses;
                else if (value == ')') { if (--parentheses < 0) break; }
                else if (value == ';' && parentheses == 0) break;
                else if (value == '{' || value == '}') break;
            }
            if (end >= source.size() || source[end] != ';' || parentheses != 0 || !valueQuote.isNull())
                return fail(QStringLiteral("Typora export directive is incomplete or malformed."));
            source.replace(i, end - i + 1, QStringLiteral(" "));
            omittedExport = true;
        }
        QRegularExpression imports(QStringLiteral("@import\\s+(?:url\\(\\s*(?:\"([^\"]*)\"|'([^']*)'|([^\\s)'\"]+))\\s*\\)|\"([^\"]*)\"|'([^']*)')\\s*([^;]*);"), flags);
        auto importMatches = imports.globalMatch(source);
        QList<QRegularExpressionMatch> matches;
        while (importMatches.hasNext()) matches.append(importMatches.next());
        for (auto it = matches.crbegin(); it != matches.crend(); ++it) {
            QString value;
            for (int n = 1; n <= 5; ++n) if (!it->captured(n).isNull()) { value = it->captured(n); break; }
            QString importedPath, imported;
            if (!localPath(value, base, importedPath) || !process(importedPath, imported, depth + 1)) return false;
            QString media = it->captured(6).trimmed();
            if (media.contains('{') || media.contains('}') || media.contains('@') || media.contains('('))
                return fail(QStringLiteral("CSS import layer/supports modifiers are not supported. Use plain or media imports."));
            if (!media.isEmpty()) imported = QStringLiteral("@media %1 {\n%2\n}").arg(media, imported);
            source.replace(it->capturedStart(), it->capturedLength(), imported);
        }
        // Reject remaining/obfuscated imports and network-bearing alternative image syntax.
        QRegularExpression unsafe(QStringLiteral("@(?:import|include-when-export|namespace|document)\\b|(?:image-set|image|expression)\\s*\\(|(?:-moz-binding|behavior)\\s*:"), flags);
        if (source.contains(unsafe)) return fail(QStringLiteral("Theme contains unsupported imports or active/network CSS features."));
        QRegularExpression urls(QStringLiteral("url\\(\\s*(?:\"([^\"]*)\"|'([^']*)'|([^\\s)'\"]*))\\s*\\)"), flags);
        matches.clear();
        auto urlMatches = urls.globalMatch(source);
        while (urlMatches.hasNext()) matches.append(urlMatches.next());
        for (auto it = matches.crbegin(); it != matches.crend(); ++it) {
            QString value;
            for (int n = 1; n <= 3; ++n) if (!it->captured(n).isNull()) { value = it->captured(n); break; }
            if (value.startsWith('#')) continue;
            QString replacement;
            if (value.startsWith(QStringLiteral("data:"), Qt::CaseInsensitive)) {
                QRegularExpression safeData(QStringLiteral("^data:(image/(?:png|jpeg|gif|webp)|font/(?:woff2?|ttf|otf)|application/(?:font-woff|x-font-ttf|x-font-opentype));base64,[A-Za-z0-9+/]*={0,2}$"), flags);
                if (!safeData.match(value).hasMatch() || value.size() > (value.startsWith("data:image/", Qt::CaseInsensitive) ? MaxAsset : MaxFont) * 4 / 3 + 200)
                    return fail(QStringLiteral("Only bounded base64 raster images and fonts are supported in CSS data URLs."));
                continue;
            }
            QString assetPath;
            if (!localPath(value, base, assetPath)) return false;
            const QString suffix = QFileInfo(assetPath).suffix().toLower();
            const bool font = QStringList{"woff", "woff2", "ttf", "otf"}.contains(suffix);
            // A missing optional font may use the theme's fallback families.
            // Existing unreadable, oversized or unsafe files still fail visibly.
            if (font && allowMissingFonts && !QFileInfo::exists(assetPath) && !QFileInfo(assetPath).isSymLink()) {
                missingFonts.append(QFileInfo(assetPath).fileName());
                source.replace(it->capturedStart(), it->capturedLength(), QStringLiteral("local(\"Fomawrite unavailable theme font\")"));
                continue;
            }
            QByteArray asset;
            if (!read(assetPath, font ? MaxFont : MaxAsset, asset)) return false;
            const QMap<QString, QString> mimes{{"png", "image/png"}, {"jpg", "image/jpeg"}, {"jpeg", "image/jpeg"}, {"gif", "image/gif"}, {"webp", "image/webp"}, {"woff", "font/woff"}, {"woff2", "font/woff2"}, {"ttf", "font/ttf"}, {"otf", "font/otf"}};
            if (!mimes.contains(suffix)) return fail(QStringLiteral("Unsupported theme asset %1. Supported assets are PNG/JPEG/GIF/WebP and WOFF/WOFF2/TTF/OTF fonts.").arg(QFileInfo(assetPath).fileName()));
            replacement = QStringLiteral("url(\"data:%1;base64,%2\")").arg(mimes.value(suffix), QString::fromLatin1(asset.toBase64()));
            source.replace(it->capturedStart(), it->capturedLength(), replacement);
        }
        // Any URL function that failed the parser is rejected rather than left to the browser.
        QString remainder = source;
        remainder.remove(urls);
        if (remainder.contains(QRegularExpression(QStringLiteral("\\burl\\s*\\("), flags)))
            return fail(QStringLiteral("Theme contains an unsupported URL expression."));
        if (source.toUtf8().size() > MaxTotal * 2) return fail(QStringLiteral("Expanded theme CSS exceeds 128 MB."));
        out = source;
        return true;
    }
};
QString displayName(QString id) {
    id.remove(QRegularExpression(QStringLiteral("-[0-9a-f]{16}$")));
    id.replace('-', ' ');
    id.replace('_', ' ');
    for (int i = 0; i < id.size(); ++i)
        if (i == 0 || id[i - 1] == ' ') id[i] = id[i].toUpper();
    return id;
}
bool validId(const QString &id) {
    return QRegularExpression(QStringLiteral("^[a-z0-9][a-z0-9-]{0,100}$")).match(id).hasMatch();
}
QString fileId(const QString &file) {
    const QString stem = QFileInfo(file).completeBaseName();
    return validId(stem) && stem != QStringLiteral("claude-like") && file.endsWith(".css")
        ? stem : QStringLiteral("file:") + file;
}
QStringList themeFiles() {
    QStringList files;
    const QDir folder(PublishingThemes::directory());
    for (const QString &file : folder.entryList(QDir::Files | QDir::NoSymLinks, QDir::Name))
        if (QFileInfo(file).suffix().compare("css", Qt::CaseInsensitive) == 0) files.append(file);
    return files;
}
}
namespace PublishingThemes {
QString directory() { return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/publishing-themes"); }
QVariantList catalog() {
    QVariantList result{QVariantMap{{"id", "claude-like"}, {"name", "Claude Like"}, {"imported", false}}};
    for (const QString &file : themeFiles()) {
        const QString stem = QFileInfo(file).completeBaseName();
        const QString name = displayName(stem) + (stem == "claude-like" ? QStringLiteral(" (Folder)") : QString());
        result.append(QVariantMap{{"id", fileId(file)}, {"name", name}, {"imported", true}});
    }
    return result;
}
bool importTheme(QUrl cssFile, QString *importedId, QString *error) {
    if (error) error->clear();
    if (importedId) importedId->clear();
    auto fail = [&](QString message) { if (error) *error = message; return false; };
    if (!cssFile.isLocalFile()) return fail(QStringLiteral("Choose a local CSS theme file."));
    const QFileInfo info(cssFile.toLocalFile());
    if (info.isSymLink()) return fail(QStringLiteral("Choose a regular CSS file, rather than a symlink."));
    if (info.suffix().compare(QStringLiteral("css"), Qt::CaseInsensitive) != 0) return fail(QStringLiteral("Choose a .css theme file."));
    Import importer;
    importer.allowMissingFonts = true;
    importer.root = info.dir().canonicalPath();
    QString content;
    if (importer.root.isEmpty() || !importer.process(QDir(importer.root).filePath(info.fileName()), content)) return fail(importer.failure.isEmpty() ? QStringLiteral("Cannot locate the theme folder.") : importer.failure);
    QString stem = info.completeBaseName().toLower();
    stem.replace(QRegularExpression(QStringLiteral("[^a-z0-9]+")), QStringLiteral("-"));
    while (stem.startsWith('-')) stem.remove(0, 1);
    while (stem.endsWith('-')) stem.chop(1);
    if (stem.isEmpty()) stem = QStringLiteral("theme");
    stem = stem.left(70);
    const QByteArray encoded = content.toUtf8();
    const QString id = stem + '-' + QString::fromLatin1(QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex().left(16));
    if (!QDir().mkpath(directory())) return fail(QStringLiteral("Cannot create the publishing themes folder."));
    QSaveFile file(QDir(directory()).filePath(id + QStringLiteral(".css")));
    if (!file.open(QIODevice::WriteOnly) || file.write(encoded) != encoded.size() || !file.commit()) return fail(QStringLiteral("Cannot save the imported publishing theme."));
    if (importedId) *importedId = id;
    QStringList notices;
    if (importer.omittedExport) notices << QStringLiteral("Imported the document CSS. Typora's @include-when-export directive was omitted; remote fonts are not downloaded.");
    if (!importer.missingFonts.isEmpty()) notices << QStringLiteral("Missing optional theme fonts use fallback families: %1.").arg(importer.missingFonts.join(", "));
    if (error) *error = notices.join(' ');
    return true;
}
std::optional<QString> css(QString id, QString *error, QString *advisory) {
    if (error) error->clear();
    if (advisory) advisory->clear();
    auto fail = [&](QString message) -> std::optional<QString> { if (error) *error = message; return std::nullopt; };
    Import validator;
    QString sanitized;
    if (id == QStringLiteral("claude-like")) {
        QFile file(QStringLiteral(":/themes/claude-like.css"));
        if (!file.open(QIODevice::ReadOnly)) return fail(QStringLiteral("Bundled publishing theme is missing."));
        if (!validator.transform(QString::fromUtf8(file.readAll()), QString(), sanitized, 0)) return fail(validator.failure);
    } else {
        QString filename;
        for (const QString &file : themeFiles()) if (fileId(file) == id) { filename = file; break; }
        if (filename.isEmpty()) return fail(QStringLiteral("Publishing theme is missing. Check the publishing themes folder."));
        validator.root = QDir(directory()).canonicalPath();
        validator.allowMissingFonts = true;
        if (!validator.process(QDir(validator.root).filePath(filename), sanitized, 0, MaxTotal * 2)) return fail(validator.failure);
    }
    QStringList notices;
    if (validator.omittedExport) notices << QStringLiteral("Typora export-only directives were omitted; remote fonts are not downloaded.");
    if (!validator.missingFonts.isEmpty()) notices << QStringLiteral("Missing optional theme fonts use fallback families: %1.").arg(validator.missingFonts.join(", "));
    if (advisory) *advisory = notices.join(' ');
    return sanitized;
}
}
