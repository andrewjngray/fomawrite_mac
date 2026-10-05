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
constexpr qint64 MaxTotal = 16 * 1024 * 1024;
constexpr int MaxFiles = 64;
constexpr int MaxDepth = 8;
struct Import {
    QString root, failure;
    QSet<QString> stack;
    qint64 bytes = 0;
    int files = 0;
    bool omittedExport = false;
    bool allowLocal = true;
    bool fail(const QString &message) { failure = message; return false; }
    bool read(const QString &path, qint64 limit, QByteArray &out) {
        QFileInfo info(path);
        if (!info.isFile() || info.isSymLink() || info.canonicalFilePath() != info.absoluteFilePath()
            || !info.canonicalFilePath().startsWith(root + QLatin1Char('/')))
            return fail(QStringLiteral("Theme resources must be regular files inside the selected CSS folder, without symlinks: %1").arg(info.fileName()));
        if (++files > MaxFiles || info.size() > limit || bytes + info.size() > MaxTotal)
            return fail(QStringLiteral("Theme exceeds the import limits (64 files, 1 MB per CSS file, 4 MB per asset, 16 MB total)."));
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return fail(QStringLiteral("Cannot read theme resource: %1").arg(info.fileName()));
        out = file.read(limit + 1);
        bytes += out.size();
        return out.size() <= limit && bytes <= MaxTotal ? true : fail(QStringLiteral("Theme resource exceeds the import size limit."));
    }
    bool localPath(const QString &value, const QString &base, QString &path) {
        if (!allowLocal) return fail(QStringLiteral("Stored publishing themes must contain only embedded assets, without local or remote resource references."));
        QUrl url(value);
        if (!url.isRelative() || value.startsWith('/') || value.contains('?') || value.contains('#') || value.contains(QChar(0)))
            return fail(QStringLiteral("Only relative local theme resources are supported: %1").arg(value.left(120)));
        const QString decoded = QUrl::fromPercentEncoding(value.toUtf8());
        if (decoded.contains('\\') || decoded.startsWith('/') || decoded.split('/').contains(QStringLiteral("..")))
            return fail(QStringLiteral("Theme resource paths cannot traverse parent folders."));
        path = QDir(base).absoluteFilePath(decoded);
        return true;
    }
    bool process(const QString &path, QString &out, int depth = 0) {
        if (depth > MaxDepth || stack.contains(path)) return fail(QStringLiteral("Theme CSS imports are recursive or deeper than eight levels."));
        QByteArray data;
        if (!read(path, MaxCss, data)) return false;
        QString source = QString::fromUtf8(data);
        if (source.toUtf8() != data && !(data.startsWith("\xef\xbb\xbf") && source.toUtf8() == data.mid(3)))
            return fail(QStringLiteral("Theme CSS must use UTF-8 encoding."));
        stack.insert(path);
        if (!transform(source, QFileInfo(path).absolutePath(), out, depth)) return false;
        stack.remove(path);
        return true;
    }
    bool transform(QString source, const QString &base, QString &out, int depth) {
        // Escapes are deliberately unsupported: they can hide URL functions, protocols,
        // at-rules and HTML style terminators from a bounded importer.
        if (source.contains('\\') || source.contains('<') || source.contains(QChar(0)))
            return fail(QStringLiteral("Theme CSS contains unsupported escapes or unsafe markup."));
        // Remove comments with a small lexer so comment delimiters inside strings survive.
        QString cleaned;
        QChar quote;
        for (qsizetype i = 0; i < source.size(); ++i) {
            const QChar c = source[i];
            if (!quote.isNull()) { cleaned += c; if (c == quote) quote = {}; continue; }
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
                if (!safeData.match(value).hasMatch() || value.size() > MaxAsset * 4 / 3 + 200)
                    return fail(QStringLiteral("Only bounded base64 raster images and fonts are supported in CSS data URLs."));
                continue;
            }
            QString assetPath;
            if (!localPath(value, base, assetPath)) return false;
            QByteArray asset;
            if (!read(assetPath, MaxAsset, asset)) return false;
            const QString suffix = QFileInfo(assetPath).suffix().toLower();
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
        if (source.toUtf8().size() > MaxTotal * 2) return fail(QStringLiteral("Expanded theme CSS exceeds 32 MB."));
        out = source;
        return true;
    }
};
QString displayName(QString id) {
    id.remove(QRegularExpression(QStringLiteral("-[0-9a-f]{16}$")));
    id.replace('-', ' ');
    for (int i = 0; i < id.size(); ++i)
        if (i == 0 || id[i - 1] == ' ') id[i] = id[i].toUpper();
    return id;
}
bool validId(const QString &id) {
    return QRegularExpression(QStringLiteral("^[a-z0-9][a-z0-9-]{0,100}$")).match(id).hasMatch();
}
}
namespace PublishingThemes {
QString directory() { return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/publishing-themes"); }
QVariantList catalog() {
    QVariantList result{QVariantMap{{"id", "claude-like"}, {"name", "Claude Like"}, {"imported", false}}};
    const QDir folder(directory());
    for (const QString &file : folder.entryList({QStringLiteral("*.css")}, QDir::Files | QDir::NoSymLinks, QDir::Name)) {
        const QString id = QFileInfo(file).completeBaseName();
        if (validId(id) && id != QStringLiteral("claude-like")) result.append(QVariantMap{{"id", id}, {"name", displayName(id)}, {"imported", true}});
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
    if (error && importer.omittedExport) *error = QStringLiteral("Imported the document CSS. Typora's @include-when-export directive was omitted; remote fonts are not downloaded.");
    return true;
}
std::optional<QString> css(QString id, QString *error) {
    if (error) error->clear();
    auto fail = [&](QString message) -> std::optional<QString> { if (error) *error = message; return std::nullopt; };
    if (!validId(id)) return fail(QStringLiteral("Invalid publishing theme identifier."));
    const QString path = id == QStringLiteral("claude-like") ? QStringLiteral(":/themes/claude-like.css") : QDir(directory()).filePath(id + QStringLiteral(".css"));
    if (QFileInfo(path).isSymLink()) return fail(QStringLiteral("Publishing themes cannot be symlinks."));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > MaxTotal * 2) return fail(QStringLiteral("Publishing theme is missing or exceeds the size limit."));
    QString source = QString::fromUtf8(file.readAll());
    // Revalidate managed CSS on each use, including externally modified files.
    Import validator;
    validator.allowLocal = false;
    QString sanitized;
    if (!validator.transform(source, QString(), sanitized, 0)) return fail(validator.failure);
    return sanitized;
}
}
