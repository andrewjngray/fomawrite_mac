#include "harperscheme.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMultiMap>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVersionNumber>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlScheme>
#include <QtWebEngineQuick/QQuickWebEngineProfile>

namespace {
const char *const SchemeName = "fomawrite";
const char *const BundledRoot = ":/editor/dist/harper/";

QByteArray mimeFor(const QString &name) {
    if (name.endsWith(QLatin1String(".js"))) return "text/javascript";
    if (name.endsWith(QLatin1String(".wasm"))) return "application/wasm";
    if (name.endsWith(QLatin1String(".json"))) return "application/json";
    if (name == QLatin1String("VERSION") || name.startsWith(QLatin1String("LICENSE"))) return "text/plain";
    return "application/octet-stream";
}

QString readVersionFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    const QString version = QString::fromUtf8(file.read(64)).trimmed();
    return HarperSchemeHandler::isValidVersion(version) ? version : QString();
}
} // namespace

HarperSchemeHandler::HarperSchemeHandler(QObject *parent) : QWebEngineUrlSchemeHandler(parent) {}

bool HarperSchemeHandler::isValidVersion(const QString &version) {
    // \A…\z, not ^…$: PCRE's $ matches before a trailing newline, which would make "1.2.3\n" a folder name.
    static const QRegularExpression pattern(QStringLiteral("\\A\\d{1,6}\\.\\d{1,6}\\.\\d{1,6}\\z"));
    return pattern.match(version).hasMatch();
}

QString HarperSchemeHandler::harperRoot() {
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return root.isEmpty() ? QString() : root + QStringLiteral("/harper");
}

QString HarperSchemeHandler::overrideDirectory() {
    const QString root = harperRoot();
    if (root.isEmpty()) return {};
    const QDir dir(root + QStringLiteral("/current"));
    for (const char *name : {"harper_wasm_bg.wasm", "index.js", "binary.js", "BinaryModule.js"})
        if (!dir.exists(QString::fromLatin1(name))) return {};
    const QString version = readVersionFile(dir.filePath(QStringLiteral("VERSION")));
    if (version.isEmpty()) return {};
    // An app update may ship a newer engine than an earlier download: the
    // bundled copy wins unless the override is strictly newer.
    const QString bundled = bundledVersion();
    if (!bundled.isEmpty() && QVersionNumber::compare(QVersionNumber::fromString(version), QVersionNumber::fromString(bundled)) <= 0) return {};
    return dir.absolutePath();
}

QString HarperSchemeHandler::overrideVersion() {
    const QString override = overrideDirectory();
    return override.isEmpty() ? QString() : readVersionFile(override + QStringLiteral("/VERSION"));
}

QString HarperSchemeHandler::bundledVersion() { return readVersionFile(QString::fromLatin1(BundledRoot) + QStringLiteral("VERSION")); }

QString HarperSchemeHandler::servedVersion() {
    const QString override = overrideVersion();
    return override.isEmpty() ? bundledVersion() : override;
}

QString HarperSchemeHandler::resolvePath(const QUrl &url, QByteArray *mime) {
    if (url.scheme() != QLatin1String(SchemeName) || url.host() != QLatin1String("harper")) return {};
    const QString path = url.path();
    QString file, version;
    if (path == QLatin1String("/VERSION")) {
        file = QStringLiteral("VERSION");
    } else {
        // /<version>/<file>: two flat segments, no traversal.
        const QStringList parts = path.split(QLatin1Char('/'));
        if (parts.size() != 3 || !parts.at(0).isEmpty()) return {};
        version = parts.at(1);
        file = parts.at(2);
        if (!isValidVersion(version) || file.isEmpty() || file.contains(QLatin1String("..")) || file.contains(QLatin1Char('\\'))
            || file == QLatin1String("VERSION"))
            return {};
    }
    const QString override = overrideDirectory();
    const QString overrideVer = override.isEmpty() ? QString() : overrideVersion();
    QString resolved;
    if (version.isEmpty()) {
        // The version to use: the override's when valid, else the bundled one.
        resolved = !overrideVer.isEmpty() ? override + QStringLiteral("/VERSION") : QString::fromLatin1(BundledRoot) + QStringLiteral("VERSION");
    } else if (!overrideVer.isEmpty() && overrideVer == version) {
        resolved = override + QLatin1Char('/') + file;
    } else if (bundledVersion() == version) {
        resolved = QString::fromLatin1(BundledRoot) + file;
    }
    if (resolved.isEmpty() || !QFileInfo(resolved).isFile()) return {};
    if (mime) *mime = mimeFor(file);
    return resolved;
}

void HarperSchemeHandler::requestStarted(QWebEngineUrlRequestJob *job) {
    QByteArray mime;
    const QString path = resolvePath(job->requestUrl(), &mime);
    if (path.isEmpty()) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }
    auto *file = new QFile(path, job);
    if (!file->open(QIODevice::ReadOnly)) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }
    // The page is on qrc:, so module and fetch loads here are cross-origin.
    // no-store on everything: VERSION must be read fresh after an update, and the
    // versioned files are cheap (local) to serve again.
    job->setAdditionalResponseHeaders({{QByteArrayLiteral("Access-Control-Allow-Origin"), QByteArrayLiteral("*")},
                                       {QByteArrayLiteral("Cache-Control"), QByteArrayLiteral("no-store")}});
    job->reply(mime.isEmpty() ? QByteArrayLiteral("application/octet-stream") : mime, file);
}

void registerHarperScheme() {
    QWebEngineUrlScheme scheme(SchemeName);
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);
    scheme.setFlags(QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::CorsEnabled | QWebEngineUrlScheme::FetchApiAllowed);
    QWebEngineUrlScheme::registerScheme(scheme);
}

void installHarperSchemeHandler() {
    static HarperSchemeHandler *handler = nullptr;
    if (handler) return;
    handler = new HarperSchemeHandler(QQuickWebEngineProfile::defaultProfile());
    QQuickWebEngineProfile::defaultProfile()->installUrlSchemeHandler(SchemeName, handler);
}
