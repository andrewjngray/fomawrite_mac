#include "harperupdater.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QVersionNumber>
#include <algorithm>

#include <unistd.h>
#include <cstdio>

#include "harperscheme.h"

QList<HarperUpdater *> HarperUpdater::s_instances;

namespace {
const char *const DefaultRegistry = "https://registry.npmjs.org/harper.js/latest";
const char *const TarProgram = "/usr/bin/tar";
const char *const AutoAttemptedKey = "harper/autoAttempted";

bool isLoopback(const QString &host) {
    return host == QLatin1String("127.0.0.1") || host == QLatin1String("localhost") || host == QLatin1String("::1") || host == QLatin1String("[::1]");
}

int portOf(const QUrl &url) { return url.port(url.scheme() == QLatin1String("https") ? 443 : 80); }

// The four files the page imports must be there, and the folder's VERSION must name it.
bool folderHolds(const QString &folder, const QString &version) {
    const QDir dir(folder);
    for (const char *name : {"index.js", "binary.js", "BinaryModule.js", "harper_wasm_bg.wasm"})
        if (!QFileInfo(dir.filePath(QString::fromLatin1(name))).isFile()) return false;
    QFile file(dir.filePath(QStringLiteral("VERSION")));
    return file.open(QIODevice::ReadOnly) && QString::fromUtf8(file.read(64)).trimmed() == version;
}

bool writeFile(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(bytes) == bytes.size();
}
} // namespace

HarperUpdater::HarperUpdater(QObject *parent) : QObject(parent), m_registryUrl(QString::fromLatin1(DefaultRegistry)) {
    s_instances.append(this);
}

HarperUpdater::~HarperUpdater() {
    s_instances.removeAll(this);
    if (m_reply) { m_reply->disconnect(this); m_reply->abort(); }
    if (m_process) { m_process->disconnect(this); m_process->kill(); m_process->waitForFinished(2000); }
}

QNetworkAccessManager *HarperUpdater::network() {
    if (!m_network) m_network = new QNetworkAccessManager(this);
    return m_network;
}

QString HarperUpdater::root() const { return HarperSchemeHandler::harperRoot(); }
QString HarperUpdater::bundledVersion() const { return HarperSchemeHandler::bundledVersion(); }
QString HarperUpdater::installedVersion() const { return HarperSchemeHandler::overrideVersion(); }
QString HarperUpdater::servedVersion() const { return HarperSchemeHandler::servedVersion(); }

QDateTime HarperUpdater::lastCheck() const {
    return QDateTime::fromString(QSettings().value(QStringLiteral("harper/lastCheck")).toString(), Qt::ISODate);
}
void HarperUpdater::setLastCheck(const QDateTime &when) {
    QSettings settings;
    settings.setValue(QStringLiteral("harper/lastCheck"), when.toString(Qt::ISODate));
    emit changed();
}

bool HarperUpdater::autoCheck() const { return QSettings().value(QStringLiteral("harper/autoCheck"), true).toBool(); }
void HarperUpdater::setAutoCheck(bool on) {
    if (on == autoCheck()) return;
    QSettings().setValue(QStringLiteral("harper/autoCheck"), on);
    emit autoCheckChanged();
    emit changed();
}

void HarperUpdater::setRegistryUrl(const QString &url) {
    if (url == m_registryUrl) return;
    m_registryUrl = url.isEmpty() ? QString::fromLatin1(DefaultRegistry) : url;
    m_latest.clear();
    m_tarball.clear();
    m_digest.clear();
    emit changed();
}

bool HarperUpdater::isNewer(const QString &candidate, const QString &current) {
    return QVersionNumber::compare(QVersionNumber::fromString(candidate), QVersionNumber::fromString(current)) > 0;
}

bool HarperUpdater::hasUpdate() const { return !m_latest.isEmpty() && isNewer(m_latest, servedVersion()); }

QString HarperUpdater::paneSuffix() const {
    QString suffix;
    if (m_installing) suffix = QStringLiteral(" · updating…");
    else if (hasUpdate()) suffix = QStringLiteral(" · ") + m_latest + QStringLiteral(" available");
    return suffix + (hasOverride() ? QStringLiteral(" (installed)") : QStringLiteral(" (built-in)"));
}

void HarperUpdater::refresh() { emit changed(); }

void HarperUpdater::setStatus(const QString &status) {
    m_status = status;
    emit changed();
}

// ---------------------------------------------------------------- what is accepted

QByteArray HarperUpdater::digestFromIntegrity(const QString &integrity) {
    for (const QString &token : integrity.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        if (!token.startsWith(QLatin1String("sha512-"))) continue;
        const QByteArray encoded = token.mid(7).toLatin1();
        const QByteArray digest = QByteArray::fromBase64(encoded, QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
        // The string must be exactly the base64 of 64 bytes (no stray characters).
        if (digest.size() == 64 && digest.toBase64() == encoded) return digest;
    }
    return {};
}

bool HarperUpdater::tarEntryAllowed(const QString &entry) {
    for (const QChar c : entry)
        if (c.unicode() < 0x20 || c == QLatin1Char('\\') || c.unicode() == 0x7f) return false;
    if (entry.isEmpty() || entry.startsWith(QLatin1Char('/'))) return false;
    const bool directory = entry.endsWith(QLatin1Char('/'));
    QStringList parts = entry.split(QLatin1Char('/'));
    if (directory) parts.removeLast();
    if (parts.isEmpty()) return false;
    for (const QString &part : parts)
        if (part.isEmpty() || part == QLatin1String(".") || part == QLatin1String("..")) return false;
    if (parts.first() != QLatin1String("package")) return false;
    if (parts.size() == 1) return directory;                                   // the package/ folder itself
    if (parts.size() == 2) return !directory || parts.at(1) == QLatin1String("dist"); // package.json, LICENSE, README; or package/dist/
    if (parts.at(1) != QLatin1String("dist")) return false;
    if (parts.size() != 3 || directory) return false;                           // nothing nested under dist (package/dist/ itself is the size-2 case)
    // Only the kinds of file a release holds; a package of other names is refused before extraction.
    const QString name = parts.at(2);
    return name == QLatin1String("harper_wasm_bg.wasm") || name == QLatin1String("harper_wasm_slim_bg.wasm")
        || name.startsWith(QLatin1String("LICENSE")) || name.endsWith(QLatin1String(".js")) || name.endsWith(QLatin1String(".d.ts"))
        || name.endsWith(QLatin1String(".json"));
}

bool HarperUpdater::registryAllowed(const QUrl &url, QString *why) const {
    const bool overridden = m_registryUrl != QLatin1String(DefaultRegistry);
    if (!url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty()) {
        if (why) *why = QStringLiteral("the registry address is not valid");
        return false;
    }
    if (url.scheme() == QLatin1String("https")) return true;
    if (url.scheme() == QLatin1String("http") && overridden && isLoopback(url.host())) return true; // tests only: the setting is not user-facing
    if (why) *why = QStringLiteral("only https registry addresses are accepted");
    return false;
}

bool HarperUpdater::tarballAllowed(const QUrl &url, QString *why) const {
    const QUrl registry(m_registryUrl);
    QString reason;
    if (!registryAllowed(url, &reason)) {
        if (why) *why = QStringLiteral("the package address is refused: ") + reason;
        return false;
    }
    // The tarball comes from the registry's own host, over the registry's own scheme.
    if (url.scheme() != registry.scheme() || url.host() != registry.host() || portOf(url) != portOf(registry)) {
        if (why) *why = QStringLiteral("the package is not on the registry's own host (%1)").arg(url.host());
        return false;
    }
    return true;
}

// ---------------------------------------------------------------- checking

void HarperUpdater::checkNow() {
    if (m_checking || m_installing) return;
    m_mode = Mode::Plain;
    startCheck();
}

void HarperUpdater::checkAndInstall() {
    if (m_checking || m_installing) return;
    m_mode = Mode::Manual;
    startCheck();
}

void HarperUpdater::runScheduledCheck() {
    if (!autoCheck() || m_checking || m_installing) return;
    const QDateTime last = lastCheck(), now = QDateTime::currentDateTime();
    // A recorded time in the future (a clock set wrong) must not silence the check until then.
    if (last.isValid() && last <= now && last.addDays(ScheduledCheckDays) > now) return;
    m_mode = Mode::Scheduled;
    startCheck();
}

void HarperUpdater::startCheck() {
    QString why;
    const QUrl url(m_registryUrl);
    if (!registryAllowed(url, &why)) {
        finishCheck(false, why, false);
        return;
    }
    m_checking = true;
    m_error.clear();
    setStatus(QStringLiteral("Checking for Harper updates…"));
    startRequest(url, MaxRegistryBytes, 20000, false);
}

void HarperUpdater::startRequest(const QUrl &url, qint64 limit, int timeoutMs, bool toFile) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Fomawrite (Harper update check)"));
    request.setRawHeader("Accept", toFile ? "application/octet-stream" : "application/json");
    request.setTransferTimeout(timeoutMs);
    // Redirects are not followed: the registry answers directly, and the integrity check is on the bytes, not the route.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    m_body.clear();
    m_received = 0;
    m_limit = limit;
    m_overLimit = false;
    QNetworkReply *reply = network()->get(request);
    m_reply = reply;
    connect(reply, &QNetworkReply::readyRead, this, [this, reply, toFile] { drain(reply, toFile); });
    connect(reply, &QNetworkReply::finished, this, [this, toFile] { if (toFile) onTarballFinished(); else onRegistryFinished(); });
}

void HarperUpdater::drain(QNetworkReply *reply, bool toFile) {
    const QByteArray chunk = reply->readAll();
    if (chunk.isEmpty() || m_overLimit) return;
    m_received += chunk.size();
    if (m_received > m_limit) {
        m_overLimit = true;
        reply->abort();
        return;
    }
    if (toFile) {
        if (m_hash) m_hash->addData(chunk);
        if (m_file) m_file->write(chunk);
    } else {
        m_body.append(chunk);
    }
}

void HarperUpdater::onRegistryFinished() {
    QNetworkReply *reply = m_reply;
    if (!reply) return;
    drain(reply, false);
    reply->deleteLater();
    m_reply.clear();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (m_overLimit) { finishCheck(false, QStringLiteral("the registry's answer was too large"), false); return; }
    if (reply->error() != QNetworkReply::NoError) { finishCheck(false, reply->errorString(), true); return; }
    if (status != 200) { finishCheck(false, QStringLiteral("the registry answered %1").arg(status), true); return; }
    const QJsonObject object = QJsonDocument::fromJson(m_body).object();
    const QString version = object.value(QStringLiteral("version")).toString();
    const QJsonObject dist = object.value(QStringLiteral("dist")).toObject();
    const QUrl tarball(dist.value(QStringLiteral("tarball")).toString());
    const QByteArray digest = digestFromIntegrity(dist.value(QStringLiteral("integrity")).toString());
    QString why;
    if (!HarperSchemeHandler::isValidVersion(version)) why = QStringLiteral("the registry's version is not a plain x.y.z");
    else if (digest.isEmpty()) why = QStringLiteral("the registry's answer has no sha512 integrity hash");
    else if (!tarballAllowed(tarball, &why)) {}
    if (!why.isEmpty()) {
        finishCheck(false, QStringLiteral("the registry's answer was refused: ") + why, false);
        return;
    }
    m_latest = version;
    m_tarball = tarball;
    m_digest = digest;
    setLastCheck(QDateTime::currentDateTime());
    finishCheck(true, QString(), false);
}

void HarperUpdater::finishCheck(bool ok, const QString &message, bool) {
    m_checking = false;
    const Mode mode = m_mode;
    if (!ok) {
        m_mode = Mode::Idle;
        m_error = message;
        setStatus(QStringLiteral("Could not check for Harper updates: ") + message);
        emit failed(message);
        if (mode == Mode::Manual) emit notice(m_status); // the automatic path says nothing
        return;
    }
    m_error.clear();
    if (!hasUpdate()) {
        m_mode = Mode::Idle;
        const QString line = QStringLiteral("Harper is up to date (%1)").arg(servedVersion());
        setStatus(line);
        emit upToDate();
        if (mode == Mode::Manual) emit notice(line);
        return;
    }
    setStatus(QStringLiteral("Harper %1 available").arg(m_latest));
    emit updateAvailable(m_latest);
    if (mode == Mode::Manual) {
        emit notice(QStringLiteral("Harper %1 available — installing…").arg(m_latest));
        install();
    } else if (mode == Mode::Scheduled && QSettings().value(QLatin1String(AutoAttemptedKey)).toString() != m_latest) {
        install(); // once per version: an automatic attempt (or a rollback) is remembered
    } else {
        m_mode = Mode::Idle;
    }
}

// ---------------------------------------------------------------- installing

void HarperUpdater::install() {
    if (m_checking || m_installing) return;
    // One install at a time across windows: each window has its own updater.
    for (HarperUpdater *other : std::as_const(s_instances)) if (other != this && other->m_installing) { finishInstall(false, QStringLiteral("another window is installing an update"), false); return; }
    if (m_mode == Mode::Idle) m_mode = Mode::Plain;
    if (m_latest.isEmpty() || !HarperSchemeHandler::isValidVersion(m_latest) || m_digest.size() != 64 || !m_tarball.isValid()) {
        finishInstall(false, QStringLiteral("check for updates first"), false);
        return;
    }
    const QString base = root();
    if (base.isEmpty() || !QDir().mkpath(base)) {
        finishInstall(false, QStringLiteral("the app's data folder is not available"), false);
        return;
    }
    m_installing = true;
    m_error.clear();
    m_previousServed = servedVersion();
    setStatus(QStringLiteral("Installing Harper %1…").arg(m_latest));
    // Leftovers of an interrupted install (the app quit mid-download) are not kept.
    for (const QFileInfo &stale : QDir(base).entryInfoList({QStringLiteral(".work-*")}, QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot))
        if (stale.lastModified().secsTo(QDateTime::currentDateTime()) > 3600) QDir(stale.absoluteFilePath()).removeRecursively();
    m_work = std::make_unique<QTemporaryDir>(base + QStringLiteral("/.work-XXXXXX"));
    if (!m_work->isValid()) {
        finishInstall(false, QStringLiteral("could not make a working folder"), false);
        return;
    }
    // A folder for this version that is already complete (a rollback kept it) needs no download.
    if (folderHolds(base + QLatin1Char('/') + m_latest, m_latest)) {
        QTimer::singleShot(0, this, [this] { if (m_installing) activate(m_latest); });
        return;
    }
    m_archivePath = m_work->filePath(QStringLiteral("harper.tgz"));
    m_file = std::make_unique<QFile>(m_archivePath);
    if (!m_file->open(QIODevice::WriteOnly)) {
        finishInstall(false, QStringLiteral("could not write the download"), false);
        return;
    }
    m_hash = std::make_unique<QCryptographicHash>(QCryptographicHash::Sha512);
    startRequest(m_tarball, MaxTarballBytes, 120000, true);
}

void HarperUpdater::onTarballFinished() {
    QNetworkReply *reply = m_reply;
    if (!reply) return;
    drain(reply, true);
    reply->deleteLater();
    m_reply.clear();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (m_file) m_file->close();
    if (m_overLimit) { finishInstall(false, QStringLiteral("the download was larger than expected"), false); return; }
    if (reply->error() != QNetworkReply::NoError) { finishInstall(false, QStringLiteral("the download failed: ") + reply->errorString(), true); return; }
    if (status != 200) { finishInstall(false, QStringLiteral("the download failed: the server answered %1").arg(status), true); return; }
    if (m_received == 0 || !m_hash) { finishInstall(false, QStringLiteral("the download was empty"), true); return; }
    if (m_hash->result() != m_digest) {
        finishInstall(false, QStringLiteral("the download does not match the sha512 hash npm publishes for Harper %1; it was discarded and nothing was installed").arg(m_latest), false);
        return;
    }
    runTar({QStringLiteral("-tzf"), m_archivePath}, 0);
}

void HarperUpdater::runTar(const QStringList &arguments, int stage) {
    if (!QFileInfo::exists(QString::fromLatin1(TarProgram))) {
        finishInstall(false, QStringLiteral("/usr/bin/tar is not available to unpack the download"), false);
        return;
    }
    auto *process = new QProcess(this);
    m_process = process;
    process->setProgram(QString::fromLatin1(TarProgram));
    process->setArguments(arguments);
    auto *timeout = new QTimer(process);
    timeout->setSingleShot(true);
    connect(timeout, &QTimer::timeout, process, [process] { process->kill(); });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        process->deleteLater();
        finishInstall(false, QStringLiteral("could not run /usr/bin/tar"), false);
    });
    connect(process, &QProcess::finished, this, [this, process, stage](int code, QProcess::ExitStatus exit) {
        const QByteArray out = process->readAllStandardOutput();
        const QString err = QString::fromUtf8(process->readAllStandardError()).trimmed().section(QLatin1Char('\n'), 0, 0);
        process->deleteLater();
        if (!m_installing) return;
        if (exit != QProcess::NormalExit || code != 0) {
            finishInstall(false, QStringLiteral("the download could not be unpacked") + (err.isEmpty() ? QString() : QStringLiteral(" (") + err + QLatin1Char(')')), false);
            return;
        }
        if (stage == 0) {
            // Every entry must be acceptable before anything is extracted, and
            // there must be few of them (the real package has 17).
            const QStringList entries = QString::fromUtf8(out).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            if (entries.size() > 64) {
                finishInstall(false, QStringLiteral("the package holds %1 entries; a Harper release has about 17; nothing was installed").arg(entries.size()), false);
                return;
            }
            for (const QString &entry : entries) {
                if (!tarEntryAllowed(entry)) {
                    finishInstall(false, QStringLiteral("the package holds an entry outside package/dist: %1; nothing was installed").arg(entry.left(80)), false);
                    return;
                }
            }
            QDir().mkpath(m_work->filePath(QStringLiteral("x")));
            runTar({QStringLiteral("-xzf"), m_archivePath, QStringLiteral("-C"), m_work->filePath(QStringLiteral("x"))}, 1);
        } else {
            buildAndActivate();
        }
    });
    timeout->start(60000);
    process->start();
}

void HarperUpdater::buildAndActivate() {
    const QString extracted = m_work->filePath(QStringLiteral("x"));
    const QString canonicalRoot = QFileInfo(extracted).canonicalFilePath();
    // Nothing that is not a plain file or folder, and nothing that resolves outside the folder.
    QDirIterator walk(extracted, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    while (walk.hasNext()) {
        const QFileInfo info(walk.next());
        if (info.isSymLink() || (!info.isFile() && !info.isDir()) || !info.canonicalFilePath().startsWith(canonicalRoot + QLatin1Char('/'))) {
            finishInstall(false, QStringLiteral("the package holds a link or special file; nothing was installed"), false);
            return;
        }
    }
    const QDir dist(extracted + QStringLiteral("/package/dist"));
    const QStringList glues = dist.entryList({QStringLiteral("BinaryModule-*.js")}, QDir::Files, QDir::Name);
    const QString indexPath = dist.filePath(QStringLiteral("index.js")), binaryPath = dist.filePath(QStringLiteral("binary.js")), wasmPath = dist.filePath(QStringLiteral("harper_wasm_bg.wasm"));
    if (glues.isEmpty() || !QFileInfo(indexPath).isFile() || !QFileInfo(binaryPath).isFile() || !QFileInfo(wasmPath).isFile()) {
        finishInstall(false, QStringLiteral("the package does not have the layout the app expects (index.js, binary.js, BinaryModule-*.js, harper_wasm_bg.wasm)"), false);
        return;
    }
    const QString glue = glues.first();
    // Stable file names, so the module paths never change: the same rewrite bin/fetch-harper makes.
    const QByteArray from = "./" + glue.toUtf8(), to = QByteArrayLiteral("./BinaryModule.js");
    QFile indexFile(indexPath), binaryFile(binaryPath), wasmFile(wasmPath);
    if (!indexFile.open(QIODevice::ReadOnly) || !binaryFile.open(QIODevice::ReadOnly) || !wasmFile.open(QIODevice::ReadOnly)) {
        finishInstall(false, QStringLiteral("the package's files could not be read"), false);
        return;
    }
    QByteArray index = indexFile.readAll(), binary = binaryFile.readAll();
    const QByteArray magic = wasmFile.read(4);
    wasmFile.close();
    if (index.isEmpty() || binary.isEmpty() || magic != QByteArrayLiteral("\0asm")) {
        finishInstall(false, QStringLiteral("the package's engine files are not what they should be (the WebAssembly binary has no header)"), false);
        return;
    }
    index.replace(from, to);
    binary.replace(from, to);
    const QString stage = m_work->filePath(QStringLiteral("stage"));
    QDir().mkpath(stage);
    bool wrote = writeFile(stage + QStringLiteral("/index.js"), index) && writeFile(stage + QStringLiteral("/binary.js"), binary)
        && QFile::copy(dist.filePath(glue), stage + QStringLiteral("/BinaryModule.js")) && QFile::copy(wasmPath, stage + QStringLiteral("/harper_wasm_bg.wasm"));
    for (const QString &licence : QDir(extracted + QStringLiteral("/package")).entryList({QStringLiteral("LICENSE*")}, QDir::Files))
        wrote = wrote && QFile::copy(extracted + QStringLiteral("/package/") + licence, stage + QLatin1Char('/') + licence);
    wrote = wrote && writeFile(stage + QStringLiteral("/VERSION"), m_latest.toUtf8() + '\n');
    if (!wrote) {
        finishInstall(false, QStringLiteral("the engine files could not be written"), false);
        return;
    }
    // One rename moves the finished folder into place.
    const QString target = root() + QLatin1Char('/') + m_latest;
    if (QFileInfo::exists(target)) QDir(target).removeRecursively(); // a half-made folder of this version
    if (!QDir().rename(stage, target)) {
        finishInstall(false, QStringLiteral("the new engine could not be moved into place"), false);
        return;
    }
    activate(m_latest);
}

// Points harper/current at harper/<version>: a symlink made beside it and renamed over it, so a reader sees either the
// old target or the new one, never none.
void HarperUpdater::activate(const QString &version) {
    const QString base = root();
    const QString current = base + QStringLiteral("/current");
    const QString previousTarget = HarperSchemeHandler::overrideVersion();
    const QString temporary = base + QStringLiteral("/.current-") + QString::number(::getpid());
    const QByteArray temporaryPath = QFile::encodeName(temporary), currentPath = QFile::encodeName(current);
    ::unlink(temporaryPath.constData());
    if (::symlink(QFile::encodeName(version).constData(), temporaryPath.constData()) != 0) {
        finishInstall(false, QStringLiteral("the new engine could not be made current"), false);
        return;
    }
    const QFileInfo existing(current);
    if (existing.isDir() && !existing.isSymLink()) QDir(current).removeRecursively(); // a folder where the link belongs
    if (::rename(temporaryPath.constData(), currentPath.constData()) != 0) {
        ::unlink(temporaryPath.constData());
        finishInstall(false, QStringLiteral("the new engine could not be made current"), false);
        return;
    }
    prune(version, previousTarget);
    finishInstall(true, version, false);
}

// Version folders add up (16 MB each): keep the one in use and the one before it.
void HarperUpdater::prune(const QString &keepA, const QString &keepB) {
    for (const QFileInfo &info : QDir(root()).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString name = info.fileName();
        if (HarperSchemeHandler::isValidVersion(name) && name != keepA && name != keepB && !info.isSymLink()) QDir(info.absoluteFilePath()).removeRecursively();
    }
}

void HarperUpdater::finishInstall(bool ok, const QString &message, bool network) {
    m_installing = false;
    m_work.reset();
    m_file.reset();
    m_hash.reset();
    const Mode mode = m_mode;
    m_mode = Mode::Idle;
    if (ok) {
        const QString version = message;
        m_error.clear();
        const QString line = QStringLiteral("Harper %1 installed (was %2)").arg(version, m_previousServed.isEmpty() ? QStringLiteral("none") : m_previousServed);
        if (mode == Mode::Scheduled) QSettings().setValue(QLatin1String(AutoAttemptedKey), version);
        setStatus(line);
        emit installed(version, m_previousServed);
        notifyServedChanged();
        if (mode == Mode::Manual || mode == Mode::Scheduled) emit notice(line);
        return;
    }
    m_error = message;
    setStatus(QStringLiteral("Harper update failed: ") + message);
    // An automatic install that failed verification is not tried again for this version; a network failure is.
    if (mode == Mode::Scheduled && !network) QSettings().setValue(QLatin1String(AutoAttemptedKey), m_latest);
    emit failed(message);
    if (mode == Mode::Manual || (mode == Mode::Scheduled && !network)) emit notice(m_status);
}

void HarperUpdater::rollback() {
    if (m_installing || root().isEmpty()) return;
    const QString current = root() + QStringLiteral("/current");
    const QString was = installedVersion();
    const QFileInfo info(current);
    if (info.isSymLink()) QFile::remove(current);
    else if (info.exists()) QDir(current).removeRecursively();
    // A version given up is not installed again by the weekly check (only by asking).
    if (!was.isEmpty()) QSettings().setValue(QLatin1String(AutoAttemptedKey), was);
    m_error.clear();
    const QString line = QStringLiteral("Using the built-in Harper %1").arg(bundledVersion());
    setStatus(line);
    emit rolledBack();
    notifyServedChanged();
    emit notice(line);
}

// Every window has its own updater and engine; all of them follow what is on disk.
void HarperUpdater::notifyServedChanged() {
    const QList<HarperUpdater *> all = s_instances;
    for (HarperUpdater *updater : all) {
        updater->refresh();
        emit updater->servedChanged();
    }
}
