#include "harperscheme.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMultiMap>
#include <QStandardPaths>
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
    return "application/octet-stream";
}
} // namespace

HarperSchemeHandler::HarperSchemeHandler(QObject *parent) : QWebEngineUrlSchemeHandler(parent) {}

QString HarperSchemeHandler::overrideDirectory() {
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (root.isEmpty()) return {};
    const QDir dir(root + QStringLiteral("/harper/current"));
    return dir.exists(QStringLiteral("harper_wasm_bg.wasm")) && dir.exists(QStringLiteral("index.js")) ? dir.absolutePath() : QString();
}

QString HarperSchemeHandler::servedVersion() {
    const QString override = overrideDirectory();
    QFile file((override.isEmpty() ? QString::fromLatin1(BundledRoot) : override + QLatin1Char('/')) + QStringLiteral("VERSION"));
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(file.readAll()).trimmed();
}

void HarperSchemeHandler::requestStarted(QWebEngineUrlRequestJob *job) {
    const QUrl url = job->requestUrl();
    // fomawrite://harper/<file>: one flat folder, no traversal.
    const QString name = QFileInfo(url.path()).fileName();
    if (url.host() != QLatin1String("harper") || name.isEmpty() || name.contains(QLatin1String(".."))
        || url.path() != QLatin1Char('/') + name) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }
    const QString override = overrideDirectory();
    QString path = override.isEmpty() ? QString() : override + QLatin1Char('/') + name;
    if (path.isEmpty() || !QFile::exists(path)) path = QString::fromLatin1(BundledRoot) + name;
    auto *file = new QFile(path, job);
    if (!file->open(QIODevice::ReadOnly)) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }
    // The page is on qrc:, so module and fetch loads here are cross-origin.
    job->setAdditionalResponseHeaders({{QByteArrayLiteral("Access-Control-Allow-Origin"), QByteArrayLiteral("*")}});
    job->reply(mimeFor(name), file);
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
