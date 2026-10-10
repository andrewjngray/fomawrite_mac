// Cycle146 — keeping Harper current: HarperUpdater, the versioned fomawrite: scheme,
// and the Help menu's three commands.
//
// Nothing here reaches the real network: a tiny HTTP server on 127.0.0.1 (in this
// process) plays the npm registry and the tarball host, and the updater is pointed
// at it with setRegistryUrl (the one case where http, on loopback, is accepted).
// The packages are built here with /usr/bin/tar from small stand-in files, except in
// the last test, which packs the real bundled engine so the real page can load it.

#include "harperupdater.h"
#include "harperscheme.h"

#include <QCryptographicHash>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>

namespace {

// A clean <AppData>/harper and clean harper/* settings, before and after a test.
struct Cycle146Sandbox {
    static void clean() {
        const QString root = HarperSchemeHandler::harperRoot();
        if (!root.isEmpty()) QDir(root).removeRecursively();
        QSettings settings;
        for (const char *key : {"harper/autoCheck", "harper/lastCheck", "harper/autoAttempted"}) settings.remove(QString::fromLatin1(key));
        settings.sync();
    }
    Cycle146Sandbox() { clean(); }
    ~Cycle146Sandbox() { clean(); }
};

// GET-only HTTP on 127.0.0.1: fixed bodies by path, a log of what was asked for.
class Cycle146Server : public QObject {
public:
    explicit Cycle146Server(QObject *parent = nullptr) : QObject(parent) {
        connect(&m_server, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *socket = m_server.nextPendingConnection()) {
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                connect(socket, &QTcpSocket::readyRead, socket, [this, socket] {
                    QByteArray &buffer = m_buffers[socket];
                    buffer += socket->readAll();
                    if (!buffer.contains("\r\n\r\n")) return;
                    const QList<QByteArray> line = buffer.left(buffer.indexOf("\r\n")).split(' ');
                    buffer.clear();
                    const QString path = line.size() > 1 ? QString::fromLatin1(line.at(1)) : QString();
                    requests << path;
                    const bool known = routes.contains(path);
                    const QByteArray body = known ? routes.value(path) : QByteArrayLiteral("not found");
                    socket->write(QByteArray("HTTP/1.1 ") + (known ? "200 OK" : "404 Not Found") + "\r\nContent-Type: application/octet-stream\r\nContent-Length: "
                                  + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
            }
        });
    }
    bool start() { return m_server.listen(QHostAddress::LocalHost, 0); }
    void stop() { m_server.close(); }
    quint16 port() const { return m_server.serverPort(); }
    QString url(const QString &path) const { return QStringLiteral("http://127.0.0.1:%1%2").arg(port()).arg(path); }
    QHash<QString, QByteArray> routes;
    QStringList requests;
private:
    QTcpServer m_server;
    QHash<QTcpSocket *, QByteArray> m_buffers;
};

struct Cycle146Package { QString version; QByteArray bytes; QString integrity; };

QString cycle146Integrity(const QByteArray &bytes) {
    return QStringLiteral("sha512-") + QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha512).toBase64());
}

bool cycle146Write(const QString &path, const QByteArray &bytes) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}

const QByteArray Cycle146Wasm = QByteArray("\0asm\1\0\0\0stand-in", 16);

// A harper.js-shaped tarball made with tar: package/dist/{index.js, binary.js, BinaryModule-abc.js, harper_wasm_bg.wasm},
// package/LICENSE and package.json. `mode` plants a flaw: "evil-folder" (package/evil/x.txt), "dotdot" (../escape.txt),
// "link" (package/dist/link.js -> /etc/hosts), "no-glue", "bad-wasm".
Cycle146Package cycle146Package(const QString &version, const QString &mode = QString()) {
    Cycle146Package package;
    package.version = version;
    QTemporaryDir staging;
    const QString inner = staging.filePath(QStringLiteral("inner")), dist = inner + QStringLiteral("/package/dist");
    QDir().mkpath(dist);
    cycle146Write(dist + QStringLiteral("/index.js"), "import { glue } from \"./BinaryModule-abc.js\";\nexport const marker = \"" + version.toUtf8() + "\";\n");
    cycle146Write(dist + QStringLiteral("/binary.js"), "import \"./BinaryModule-abc.js\";\nexport const binary = 1;\n");
    if (mode != QLatin1String("no-glue")) cycle146Write(dist + QStringLiteral("/BinaryModule-abc.js"), "export const glue = \"glue " + version.toUtf8() + "\";\n");
    cycle146Write(dist + QStringLiteral("/harper_wasm_bg.wasm"), mode == QLatin1String("bad-wasm") ? QByteArrayLiteral("not wasm at all") : Cycle146Wasm);
    cycle146Write(inner + QStringLiteral("/package/LICENSE"), "Apache-2.0 stand-in\n");
    cycle146Write(inner + QStringLiteral("/package/package.json"), "{\"name\":\"harper.js\"}\n");
    QStringList arguments{QStringLiteral("-czf"), staging.filePath(QStringLiteral("harper.tgz")), QStringLiteral("-C"), inner, QStringLiteral("package")};
    if (mode == QLatin1String("evil-folder")) cycle146Write(inner + QStringLiteral("/package/evil/x.txt"), "x");
    if (mode == QLatin1String("link")) QFile::link(QStringLiteral("/etc/hosts"), dist + QStringLiteral("/link.js"));
    if (mode == QLatin1String("dotdot")) {
        cycle146Write(staging.filePath(QStringLiteral("escape.txt")), "escaped");
        arguments = {QStringLiteral("-czPf"), staging.filePath(QStringLiteral("harper.tgz")), QStringLiteral("-C"), inner, QStringLiteral("package"), QStringLiteral("../escape.txt")};
    }
    if (QProcess::execute(QStringLiteral("/usr/bin/tar"), arguments) != 0) return package;
    QFile file(staging.filePath(QStringLiteral("harper.tgz")));
    if (file.open(QIODevice::ReadOnly)) package.bytes = file.readAll();
    package.integrity = cycle146Integrity(package.bytes);
    return package;
}

// What the registry's `latest` says: the version and where its tarball is.
QByteArray cycle146Registry(const Cycle146Server &server, const QString &version, const QString &integrity, const QString &tarball = QString()) {
    const QJsonObject dist{{"tarball", tarball.isEmpty() ? server.url(QStringLiteral("/harper.js/-/harper.js-%1.tgz").arg(version)) : tarball}, {"integrity", integrity}};
    return QJsonDocument(QJsonObject{{"name", "harper.js"}, {"version", version}, {"dist", dist}}).toJson(QJsonDocument::Compact);
}

// Serves `package` as the registry's latest.
void cycle146Publish(Cycle146Server &server, const Cycle146Package &package) {
    server.routes.insert(QStringLiteral("/harper.js/latest"), cycle146Registry(server, package.version, package.integrity));
    server.routes.insert(QStringLiteral("/harper.js/-/harper.js-%1.tgz").arg(package.version), package.bytes);
}

QString cycle146Slurp(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QStringLiteral("<missing>");
}

QStringList cycle146Entries(const QString &folder) {
    return QDir(folder).entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDir::Name);
}

// The newest version no test registry could be behind.
const QString Cycle146Newer = QStringLiteral("99.0.1");
} // namespace

// ---- the scheme ------------------------------------------------------------------------------------------------------

void FomawriteTest::harperSchemeServesVersionedPathsFromTheOverrideThenTheBundle() {
    Cycle146Sandbox sandbox;
    const QString bundled = HarperSchemeHandler::bundledVersion();
    QVERIFY2(HarperSchemeHandler::isValidVersion(bundled), "the bundle has a VERSION (bin/fetch-harper)");
    QByteArray mime;
    // No override: VERSION is the bundled one, and its versioned files come from the resources.
    QCOMPARE(HarperSchemeHandler::servedVersion(), bundled);
    QVERIFY(HarperSchemeHandler::overrideDirectory().isEmpty());
    const QString version = HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/VERSION")), &mime);
    QVERIFY(version.startsWith(QStringLiteral(":/editor/dist/harper/")));
    QCOMPARE(cycle146Slurp(version).trimmed(), bundled);
    const QString index = HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/%1/index.js").arg(bundled)), &mime);
    QVERIFY(index.startsWith(QStringLiteral(":/editor/dist/harper/")));
    QCOMPARE(mime, QByteArrayLiteral("text/javascript"));
    HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/%1/harper_wasm_bg.wasm").arg(bundled)), &mime);
    QCOMPARE(mime, QByteArrayLiteral("application/wasm"));
    // The old unversioned paths, other versions, traversal and other hosts are 404.
    for (const char *url : {"fomawrite://harper/index.js", "fomawrite://harper/9.9.9/index.js", "fomawrite://harper/index.js/", "fomawrite://other/VERSION",
                            "fomawrite://harper//index.js", "fomawrite://harper/latest/index.js", "fomawrite://harper/nothing"})
        QVERIFY2(HarperSchemeHandler::resolvePath(QUrl(QString::fromLatin1(url))).isEmpty(), url);
    QVERIFY(HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/%1/missing.js").arg(bundled))).isEmpty());
    for (const QString &path : {QStringLiteral("/") + bundled + QStringLiteral("/..%2findex.js"), QStringLiteral("/") + bundled + QStringLiteral("/../index.js"), QStringLiteral("/") + bundled + QStringLiteral("/a/b.js"),
                                QStringLiteral("/") + bundled + QStringLiteral("/%2fetc%2fpasswd")})
        QVERIFY2(HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper") + path)).isEmpty(), qPrintable(path));
    // An override: VERSION says it, its versioned files come from it, the bundled version still resolves to the bundle.
    const QString root = HarperSchemeHandler::harperRoot();
    const QString folder = root + QStringLiteral("/") + Cycle146Newer;
    for (const char *name : {"index.js", "binary.js", "BinaryModule.js", "harper_wasm_bg.wasm", "LICENSE"}) QVERIFY(cycle146Write(folder + QLatin1Char('/') + QString::fromLatin1(name), name));
    QVERIFY(cycle146Write(folder + QStringLiteral("/VERSION"), (Cycle146Newer + QLatin1Char('\n')).toUtf8()));
    QVERIFY2(HarperSchemeHandler::overrideDirectory().isEmpty(), "a folder is not an override until `current` points at it");
    QVERIFY(QFile::link(folder, root + QStringLiteral("/current")));
    QCOMPARE(HarperSchemeHandler::overrideVersion(), Cycle146Newer);
    QCOMPARE(HarperSchemeHandler::servedVersion(), Cycle146Newer);
    QCOMPARE(cycle146Slurp(HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/VERSION")))).trimmed(), Cycle146Newer);
    const QString overridden = HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/%1/index.js").arg(Cycle146Newer)));
    QCOMPARE(QFileInfo(overridden).canonicalFilePath(), QFileInfo(folder + QStringLiteral("/index.js")).canonicalFilePath());
    QCOMPARE(cycle146Slurp(overridden), QStringLiteral("index.js"));
    QVERIFY(HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/%1/index.js").arg(bundled))).startsWith(QStringLiteral(":/editor/dist/harper/")));
    QVERIFY2(HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/9.9.9/index.js"))).isEmpty(), "a version that is neither served nor bundled");
    // A broken override (VERSION not a version) is not served: back to the bundle.
    QVERIFY(cycle146Write(folder + QStringLiteral("/VERSION"), "latest\n"));
    QVERIFY(HarperSchemeHandler::overrideDirectory().isEmpty());
    QCOMPARE(HarperSchemeHandler::servedVersion(), bundled);
    QVERIFY(HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/%1/index.js").arg(Cycle146Newer))).isEmpty());
    // A missing engine file does the same.
    QVERIFY(cycle146Write(folder + QStringLiteral("/VERSION"), (Cycle146Newer + QLatin1Char('\n')).toUtf8()));
    QCOMPARE(HarperSchemeHandler::servedVersion(), Cycle146Newer);
    QVERIFY(QFile::remove(folder + QStringLiteral("/BinaryModule.js")));
    QCOMPARE(HarperSchemeHandler::servedVersion(), bundled);
}

// ---- checking and installing ------------------------------------------------------------------------------------------

void FomawriteTest::harperUpdaterReportsUpToDateAndUpdateAvailable() {
    Cycle146Sandbox sandbox;
    Backend backend;
    HarperUpdater *updater = backend.harperUpdater();
    const QString bundled = HarperSchemeHandler::bundledVersion();
    QCOMPARE(updater->bundledVersion(), bundled);
    QCOMPARE(updater->servedVersion(), bundled);
    QVERIFY(updater->installedVersion().isEmpty() && !updater->hasOverride() && !updater->hasUpdate());
    QCOMPARE(updater->registryUrl(), QStringLiteral("https://registry.npmjs.org/harper.js/latest"));
    QVERIFY(updater->autoCheck()); // on by default
    QVERIFY(!updater->lastCheck().isValid());
    QCOMPARE(updater->paneSuffix(), QStringLiteral(" (built-in)"));

    Cycle146Server server; QVERIFY(server.start());
    updater->setRegistryUrl(server.url(QStringLiteral("/harper.js/latest")));
    QSignalSpy upToDate(updater, &HarperUpdater::upToDate), available(updater, &HarperUpdater::updateAvailable), failed(updater, &HarperUpdater::failed);
    QSignalSpy notices(updater, &HarperUpdater::notice);
    // The registry's latest is what is served: up to date.
    server.routes.insert(QStringLiteral("/harper.js/latest"), cycle146Registry(server, bundled, cycle146Integrity("x")));
    updater->checkNow();
    QVERIFY(updater->checking());
    QTRY_COMPARE_WITH_TIMEOUT(upToDate.count(), 1, 10000);
    QVERIFY(!updater->checking());
    QCOMPARE(available.count(), 0); QCOMPARE(failed.count(), 0);
    QCOMPARE(updater->latestVersion(), bundled);
    QCOMPARE(updater->status(), QStringLiteral("Harper is up to date (%1)").arg(bundled));
    QVERIFY(updater->lastCheck().isValid() && updater->lastCheck().secsTo(QDateTime::currentDateTime()) < 60);
    QCOMPARE(notices.count(), 0); // checkNow alone says nothing on the banner
    // An older registry version is not an update either (never a downgrade).
    server.routes.insert(QStringLiteral("/harper.js/latest"), cycle146Registry(server, QStringLiteral("0.0.1"), cycle146Integrity("x")));
    updater->checkNow();
    QTRY_COMPARE_WITH_TIMEOUT(upToDate.count(), 2, 10000);
    QVERIFY(!updater->hasUpdate());
    // A newer one is available, and checkNow does not install it.
    const Cycle146Package package = cycle146Package(Cycle146Newer);
    QVERIFY(!package.bytes.isEmpty());
    cycle146Publish(server, package);
    updater->checkNow();
    QTRY_COMPARE_WITH_TIMEOUT(available.count(), 1, 10000);
    QCOMPARE(available.at(0).at(0).toString(), Cycle146Newer);
    QCOMPARE(updater->latestVersion(), Cycle146Newer);
    QVERIFY(updater->hasUpdate());
    QCOMPARE(updater->status(), QStringLiteral("Harper %1 available").arg(Cycle146Newer));
    QCOMPARE(updater->paneSuffix(), QStringLiteral(" · %1 available (built-in)").arg(Cycle146Newer));
    QVERIFY(updater->installedVersion().isEmpty());
    QVERIFY(!QDir(HarperSchemeHandler::harperRoot() + QStringLiteral("/") + Cycle146Newer).exists());
    QCOMPARE(server.requests.count(QStringLiteral("/harper.js/latest")), 3);
    QVERIFY(!server.requests.contains(QStringLiteral("/harper.js/-/harper.js-%1.tgz").arg(Cycle146Newer)));
    // A registry that is down fails with a message; the version in use does not change.
    server.stop();
    updater->checkNow();
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 10000);
    QVERIFY(!updater->error().isEmpty());
    QVERIFY(updater->status().startsWith(QStringLiteral("Could not check for Harper updates: ")));
    QCOMPARE(updater->servedVersion(), bundled);
    // The answer must be a registry answer.
    Cycle146Server odd; QVERIFY(odd.start());
    updater->setRegistryUrl(odd.url(QStringLiteral("/harper.js/latest")));
    odd.routes.insert(QStringLiteral("/harper.js/latest"), "<html>not json</html>");
    updater->checkNow();
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 2, 10000);
    QVERIFY2(updater->error().contains(QStringLiteral("refused")), qPrintable(updater->error()));
}

void FomawriteTest::harperUpdaterInstallsAnUpdateServesItAndReloadsTheEngine() {
    Cycle146Sandbox sandbox;
    Backend backend;
    HarperUpdater *updater = backend.harperUpdater();
    const QString bundled = HarperSchemeHandler::bundledVersion();
    auto *bridge = backend.editorBridge();
    bridge->ready(); // a page is there: the engine sends harperLoad
    QSignalSpy loads(bridge, &EditorBridge::harperLoad);
    bridge->harperReady(bundled);
    QVERIFY(backend.spellCheck()->harperReady());
    QCOMPARE(backend.spellCheck()->harperVersion(), bundled);

    Cycle146Server server; QVERIFY(server.start());
    const Cycle146Package package = cycle146Package(Cycle146Newer);
    cycle146Publish(server, package);
    updater->setRegistryUrl(server.url(QStringLiteral("/harper.js/latest")));
    QSignalSpy available(updater, &HarperUpdater::updateAvailable), installed(updater, &HarperUpdater::installed), failed(updater, &HarperUpdater::failed);
    QSignalSpy served(updater, &HarperUpdater::servedChanged), notices(updater, &HarperUpdater::notice);
    // install() before a check has nothing to install.
    updater->install();
    QCOMPARE(failed.count(), 1);
    QVERIFY(failed.at(0).at(0).toString().contains(QStringLiteral("check for updates first")));
    QVERIFY(updater->hasOverride() == false);
    failed.clear();

    updater->checkNow();
    QTRY_COMPARE_WITH_TIMEOUT(available.count(), 1, 10000);
    updater->install();
    QVERIFY(updater->installing());
    QCOMPARE(updater->paneSuffix(), QStringLiteral(" · updating… (built-in)"));
    QTRY_COMPARE_WITH_TIMEOUT(installed.count(), 1, 20000);
    QCOMPARE(failed.count(), 0);
    QVERIFY(!updater->installing());
    QCOMPARE(installed.at(0).at(0).toString(), Cycle146Newer);
    QCOMPARE(installed.at(0).at(1).toString(), bundled);
    QCOMPARE(updater->status(), QStringLiteral("Harper %1 installed (was %2)").arg(Cycle146Newer, bundled));
    QCOMPARE(notices.count(), 0); // a plain install() is silent; the Help command and the weekly check use the banner

    // On disk: <AppData>/harper/<v>/ with the glue renamed and the imports rewritten, and VERSION.
    const QString root = HarperSchemeHandler::harperRoot(), folder = root + QStringLiteral("/") + Cycle146Newer;
    QCOMPARE(cycle146Entries(folder), (QStringList{"BinaryModule.js", "LICENSE", "VERSION", "binary.js", "harper_wasm_bg.wasm", "index.js"}));
    QCOMPARE(cycle146Slurp(folder + QStringLiteral("/index.js")), QStringLiteral("import { glue } from \"./BinaryModule.js\";\nexport const marker = \"%1\";\n").arg(Cycle146Newer));
    QCOMPARE(cycle146Slurp(folder + QStringLiteral("/binary.js")), QStringLiteral("import \"./BinaryModule.js\";\nexport const binary = 1;\n"));
    QCOMPARE(cycle146Slurp(folder + QStringLiteral("/BinaryModule.js")), QStringLiteral("export const glue = \"glue %1\";\n").arg(Cycle146Newer));
    QFile wasm(folder + QStringLiteral("/harper_wasm_bg.wasm")); QVERIFY(wasm.open(QIODevice::ReadOnly));
    QCOMPARE(wasm.readAll(), Cycle146Wasm);
    QCOMPARE(cycle146Slurp(folder + QStringLiteral("/VERSION")), Cycle146Newer + QLatin1Char('\n'));
    // Nothing else is left in the data folder: no download, no working folder, no temporary link.
    QCOMPARE(cycle146Entries(root), (QStringList{"99.0.1", "current"}));
    // `current` points at it, the scheme serves it, and the page would import its own versioned path.
    const QFileInfo current(root + QStringLiteral("/current"));
    QVERIFY(current.isSymLink());
    QCOMPARE(QFileInfo(current.symLinkTarget()).canonicalFilePath(), QFileInfo(folder).canonicalFilePath());
    QCOMPARE(updater->installedVersion(), Cycle146Newer);
    QCOMPARE(updater->servedVersion(), Cycle146Newer);
    QCOMPARE(HarperSchemeHandler::servedVersion(), Cycle146Newer);
    QVERIFY(updater->hasOverride() && !updater->hasUpdate());
    QCOMPARE(updater->paneSuffix(), QStringLiteral(" (installed)"));
    QCOMPARE(QFileInfo(HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/%1/index.js").arg(Cycle146Newer)))).canonicalFilePath(),
             QFileInfo(folder + QStringLiteral("/index.js")).canonicalFilePath());
    QCOMPARE(cycle146Slurp(HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/VERSION")))).trimmed(), Cycle146Newer);
    QVERIFY(HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/%1/index.js").arg(bundled))).startsWith(QStringLiteral(":/editor/dist/harper/")));
    // The engine was asked to load again (same dialect; the page re-reads VERSION) and is not ready until the page says so.
    QVERIFY(served.count() >= 1);
    QTRY_COMPARE(loads.count(), 1);
    QCOMPARE(loads.at(0).at(0).toString(), backend.spellCheck()->dialect());
    QVERIFY(!backend.spellCheck()->harperReady());
    bridge->harperReady(Cycle146Newer);
    QVERIFY(backend.spellCheck()->harperReady());
    QCOMPARE(backend.spellCheck()->harperVersion(), Cycle146Newer);
    // The package was fetched once, the registry once (no retry noise).
    QCOMPARE(server.requests.count(QStringLiteral("/harper.js/-/harper.js-%1.tgz").arg(Cycle146Newer)), 1);
}

void FomawriteTest::harperUpdaterRollbackRestoresTheBundledVersionAndKeepsTheFolders() {
    Cycle146Sandbox sandbox;
    Backend backend;
    HarperUpdater *updater = backend.harperUpdater();
    const QString bundled = HarperSchemeHandler::bundledVersion();
    auto *bridge = backend.editorBridge();
    bridge->ready(); bridge->harperReady(bundled);
    Cycle146Server server; QVERIFY(server.start());
    cycle146Publish(server, cycle146Package(Cycle146Newer));
    updater->setRegistryUrl(server.url(QStringLiteral("/harper.js/latest")));
    QSignalSpy installed(updater, &HarperUpdater::installed), rolledBack(updater, &HarperUpdater::rolledBack), notices(updater, &HarperUpdater::notice);
    updater->checkAndInstall();
    QTRY_COMPARE_WITH_TIMEOUT(installed.count(), 1, 20000);
    bridge->harperReady(Cycle146Newer);
    QCOMPARE(updater->servedVersion(), Cycle146Newer);
    QSignalSpy loads(bridge, &EditorBridge::harperLoad);
    const QString root = HarperSchemeHandler::harperRoot();
    updater->rollback();
    QCOMPARE(rolledBack.count(), 1);
    QVERIFY(!QFileInfo(root + QStringLiteral("/current")).isSymLink() && !QFileInfo::exists(root + QStringLiteral("/current")));
    QVERIFY2(QDir(root + QStringLiteral("/") + Cycle146Newer).exists(), "the folders stay");
    QVERIFY(updater->installedVersion().isEmpty() && !updater->hasOverride());
    QCOMPARE(updater->servedVersion(), bundled);
    QCOMPARE(HarperSchemeHandler::servedVersion(), bundled);
    QVERIFY(HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/%1/index.js").arg(bundled))).startsWith(QStringLiteral(":/editor/dist/harper/")));
    QVERIFY2(HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/%1/index.js").arg(Cycle146Newer))).isEmpty(), "the rolled-back version is not served");
    QCOMPARE(updater->status(), QStringLiteral("Using the built-in Harper %1").arg(bundled));
    QCOMPARE(notices.last().at(0).toString(), updater->status());
    // The engine loads again, and the pane says built-in.
    QTRY_COMPARE(loads.count(), 1);
    QVERIFY(!backend.spellCheck()->harperReady());
    bridge->harperReady(bundled);
    QCOMPARE(updater->paneSuffix(), QStringLiteral(" · %1 available (built-in)").arg(Cycle146Newer));
    // Rolling back with nothing installed is harmless.
    updater->rollback();
    QCOMPARE(updater->servedVersion(), bundled);
    // Installing again from the kept folder needs no download.
    const int fetched = server.requests.count(QStringLiteral("/harper.js/-/harper.js-%1.tgz").arg(Cycle146Newer));
    installed.clear();
    updater->install();
    QTRY_COMPARE_WITH_TIMEOUT(installed.count(), 1, 10000);
    QCOMPARE(server.requests.count(QStringLiteral("/harper.js/-/harper.js-%1.tgz").arg(Cycle146Newer)), fetched);
    QCOMPARE(updater->servedVersion(), Cycle146Newer);
}

// ---- refusals ----------------------------------------------------------------------------------------------------------

void FomawriteTest::harperUpdaterRefusesAWrongIntegrityAndLeavesNothingBehind() {
    Cycle146Sandbox sandbox;
    Backend backend;
    HarperUpdater *updater = backend.harperUpdater();
    Cycle146Server server; QVERIFY(server.start());
    const Cycle146Package package = cycle146Package(Cycle146Newer);
    // The registry vouches for different bytes than the ones the tarball URL serves.
    server.routes.insert(QStringLiteral("/harper.js/latest"), cycle146Registry(server, Cycle146Newer, cycle146Integrity("some other tarball")));
    server.routes.insert(QStringLiteral("/harper.js/-/harper.js-%1.tgz").arg(Cycle146Newer), package.bytes);
    updater->setRegistryUrl(server.url(QStringLiteral("/harper.js/latest")));
    QSignalSpy available(updater, &HarperUpdater::updateAvailable), installed(updater, &HarperUpdater::installed), failed(updater, &HarperUpdater::failed);
    updater->checkNow();
    QTRY_COMPARE_WITH_TIMEOUT(available.count(), 1, 10000);
    updater->install();
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 20000);
    QVERIFY2(failed.at(0).at(0).toString().contains(QStringLiteral("does not match")), qPrintable(failed.at(0).at(0).toString()));
    QCOMPARE(installed.count(), 0);
    QVERIFY(!updater->installing());
    QCOMPARE(updater->servedVersion(), HarperSchemeHandler::bundledVersion());
    QVERIFY(updater->status().startsWith(QStringLiteral("Harper update failed: ")));
    QVERIFY2(cycle146Entries(HarperSchemeHandler::harperRoot()).isEmpty(), qPrintable(cycle146Entries(HarperSchemeHandler::harperRoot()).join(',')));
    // A truncated download fails the same way.
    cycle146Publish(server, package);
    server.routes.insert(QStringLiteral("/harper.js/-/harper.js-%1.tgz").arg(Cycle146Newer), package.bytes.left(package.bytes.size() / 2));
    updater->checkNow();
    QTRY_COMPARE_WITH_TIMEOUT(available.count(), 2, 10000);
    updater->install();
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 2, 20000);
    QVERIFY(cycle146Entries(HarperSchemeHandler::harperRoot()).isEmpty());
    // And a correct one still installs afterwards (nothing was left in the way).
    cycle146Publish(server, package);
    updater->checkNow();
    QTRY_COMPARE_WITH_TIMEOUT(available.count(), 3, 10000);
    updater->install();
    QTRY_COMPARE_WITH_TIMEOUT(installed.count(), 1, 20000);
    QCOMPARE(updater->servedVersion(), Cycle146Newer);
}

void FomawriteTest::harperUpdaterRefusesPackagesThatReachOutsideDist() {
    Cycle146Sandbox sandbox;
    // The entry rules, directly.
    for (const char *entry : {"package/", "package/dist/", "package/dist/index.js", "package/dist/sub/deeper.js", "package/package.json", "package/LICENSE", "package/README.md"})
        QVERIFY2(HarperUpdater::tarEntryAllowed(QString::fromLatin1(entry)), entry);
    for (const char *entry : {"", "/etc/passwd", "../escape.txt", "package/../escape.txt", "package/dist/../../x", "other/dist/index.js", "package/evil/x.txt", "package/evil/", "package//dist/x",
                              "package/./dist/x", "packages/dist/x", "package", "/package/dist/x", "package/dist/a\\b", "package/dist/a\nb"})
        QVERIFY2(!HarperUpdater::tarEntryAllowed(QString::fromLatin1(entry)), entry);

    Backend backend;
    HarperUpdater *updater = backend.harperUpdater();
    Cycle146Server server; QVERIFY(server.start());
    updater->setRegistryUrl(server.url(QStringLiteral("/harper.js/latest")));
    QSignalSpy available(updater, &HarperUpdater::updateAvailable), installed(updater, &HarperUpdater::installed), failed(updater, &HarperUpdater::failed);
    int round = 0;
    const auto refused = [&](const QString &mode, const QString &needle) {
        const Cycle146Package package = cycle146Package(Cycle146Newer, mode);
        QVERIFY2(!package.bytes.isEmpty(), qPrintable("tar built " + mode));
        cycle146Publish(server, package); // the integrity is genuine: only the contents are wrong
        updater->checkNow();
        ++round;
        QTRY_COMPARE_WITH_TIMEOUT(available.count(), round, 10000);
        updater->install();
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), round, 20000);
        QVERIFY2(failed.last().at(0).toString().contains(needle), qPrintable(mode + ": " + failed.last().at(0).toString()));
        QCOMPARE(installed.count(), 0);
        QVERIFY2(cycle146Entries(HarperSchemeHandler::harperRoot()).isEmpty(), qPrintable(mode + ": " + cycle146Entries(HarperSchemeHandler::harperRoot()).join(',')));
        QCOMPARE(updater->servedVersion(), HarperSchemeHandler::bundledVersion());
    };
    refused(QStringLiteral("evil-folder"), QStringLiteral("outside package/dist"));
    refused(QStringLiteral("dotdot"), QStringLiteral("outside package/dist"));
    QVERIFY2(!QFileInfo::exists(QFileInfo(HarperSchemeHandler::harperRoot()).absolutePath() + QStringLiteral("/escape.txt")), "nothing was extracted");
    refused(QStringLiteral("link"), QStringLiteral("link or special file"));
    refused(QStringLiteral("no-glue"), QStringLiteral("layout"));
    refused(QStringLiteral("bad-wasm"), QStringLiteral("WebAssembly"));
    // The same package without the flaw installs.
    cycle146Publish(server, cycle146Package(Cycle146Newer));
    updater->checkNow();
    ++round;
    QTRY_COMPARE_WITH_TIMEOUT(available.count(), round, 10000);
    updater->install();
    QTRY_COMPARE_WITH_TIMEOUT(installed.count(), 1, 20000);
}

void FomawriteTest::harperUpdaterAcceptsOnlyHttpsAddressesOnTheRegistrysHost() {
    Cycle146Sandbox sandbox;
    QCOMPARE(HarperUpdater::digestFromIntegrity(QStringLiteral("sha512-s6BDuqtRbX8M/5exH1G5M7VzAuv6l0TagbRxMY37OsrT/1kWFjWmqHydWrkYFqTUW+fFhu4Hj2yUxvChp862Pg==")).size(), 64);
    QCOMPARE(HarperUpdater::digestFromIntegrity(QStringLiteral("sha1-abc sha512-s6BDuqtRbX8M/5exH1G5M7VzAuv6l0TagbRxMY37OsrT/1kWFjWmqHydWrkYFqTUW+fFhu4Hj2yUxvChp862Pg==")).size(), 64);
    for (const char *bad : {"", "sha1-s6BDuqtRbX8M/5exH1G5M7VzAuv6l0TagbRxMY37OsrT/1kWFjWmqHydWrkYFqTUW+fFhu4Hj2yUxvChp862Pg==", "sha512-short", "sha512-", "md5-AAAA",
                            "sha512-s6BDuqtRbX8M/5exH1G5M7VzAuv6l0TagbRxMY37OsrT/1kWFjWmqHydWrkYFqTUW+fFhu4Hj2yUxvChp862Pg==!!"})
        QVERIFY2(HarperUpdater::digestFromIntegrity(QString::fromLatin1(bad)).isEmpty(), bad);
    QVERIFY(HarperUpdater::isNewer(QStringLiteral("2.12.0"), QStringLiteral("2.10.0")));
    QVERIFY(HarperUpdater::isNewer(QStringLiteral("2.10.1"), QStringLiteral("2.10.0")));
    QVERIFY(HarperUpdater::isNewer(QStringLiteral("10.0.0"), QStringLiteral("9.9.9")));
    QVERIFY(!HarperUpdater::isNewer(QStringLiteral("2.10.0"), QStringLiteral("2.10.0")));
    QVERIFY(!HarperUpdater::isNewer(QStringLiteral("2.9.0"), QStringLiteral("2.10.0")));

    Backend backend;
    HarperUpdater *updater = backend.harperUpdater();
    QSignalSpy available(updater, &HarperUpdater::updateAvailable), failed(updater, &HarperUpdater::failed);
    // Not https, not on loopback: refused before anything is sent.
    for (const char *url : {"http://registry.example.invalid/harper.js/latest", "ftp://registry.example.invalid/harper.js/latest", "file:///etc/passwd", "https://user:pw@registry.example.invalid/x", "https:///x"}) {
        updater->setRegistryUrl(QString::fromLatin1(url));
        updater->checkNow();
        QVERIFY2(!updater->checking(), url);
        QVERIFY2(failed.count() >= 1, url);
        QVERIFY2(failed.last().at(0).toString().contains(QStringLiteral("https")) || failed.last().at(0).toString().contains(QStringLiteral("not valid")), qPrintable(failed.last().at(0).toString()));
        failed.clear();
    }
    // The registry's own answer is held to the same rules.
    Cycle146Server server; QVERIFY(server.start());
    updater->setRegistryUrl(server.url(QStringLiteral("/harper.js/latest")));
    const QString good = cycle146Integrity("x");
    struct Case { const char *name; QByteArray answer; QString needle; };
    const QList<Case> cases{
        {"an http tarball elsewhere", cycle146Registry(server, Cycle146Newer, good, QStringLiteral("http://registry.example.invalid/harper.js-99.0.1.tgz")), QStringLiteral("refused")},
        {"an https tarball on another host", cycle146Registry(server, Cycle146Newer, good, QStringLiteral("https://registry.npmjs.org/harper.js/-/harper.js-99.0.1.tgz")), QStringLiteral("registry's own host")},
        {"the registry's host on another port", cycle146Registry(server, Cycle146Newer, good, QStringLiteral("http://127.0.0.1:%1/x.tgz").arg(server.port() + 1)), QStringLiteral("registry's own host")},
        {"a file: tarball", cycle146Registry(server, Cycle146Newer, good, QStringLiteral("file:///etc/passwd")), QStringLiteral("refused")},
        {"no integrity", cycle146Registry(server, Cycle146Newer, QString()), QStringLiteral("integrity")},
        {"a sha1 integrity", cycle146Registry(server, Cycle146Newer, QStringLiteral("sha1-AAAAAAAAAAAAAAAAAAAAAAAAAAA=")), QStringLiteral("integrity")},
        {"a version with a path in it", cycle146Registry(server, QStringLiteral("99.0.1/../../x"), good), QStringLiteral("not a plain x.y.z")},
        {"a prerelease", cycle146Registry(server, QStringLiteral("99.0.1-beta.1"), good), QStringLiteral("not a plain x.y.z")},
        {"a tag", cycle146Registry(server, QStringLiteral("latest"), good), QStringLiteral("not a plain x.y.z")}};
    for (const Case &c : cases) {
        server.routes.insert(QStringLiteral("/harper.js/latest"), c.answer);
        updater->checkNow();
        QTRY_VERIFY_WITH_TIMEOUT(failed.count() == 1, 10000);
        QVERIFY2(failed.at(0).at(0).toString().contains(c.needle), qPrintable(QString::fromLatin1(c.name) + ": " + failed.at(0).at(0).toString()));
        failed.clear();
        QVERIFY2(updater->latestVersion().isEmpty(), c.name);
        updater->install();
        QCOMPARE(failed.count(), 1); // nothing accepted, so nothing to install
        failed.clear();
    }
    QCOMPARE(available.count(), 0);
    QVERIFY(cycle146Entries(HarperSchemeHandler::harperRoot()).isEmpty());
    // The default registry is https on registry.npmjs.org, and the tarball must be there too.
    updater->setRegistryUrl(QString());
    QCOMPARE(updater->registryUrl(), QStringLiteral("https://registry.npmjs.org/harper.js/latest"));
}

// ---- the weekly check --------------------------------------------------------------------------------------------------

void FomawriteTest::harperUpdaterWeeklyCheckRespectsLastCheckAndAutoCheckAndInstallsOnce() {
    Cycle146Sandbox sandbox;
    Backend backend;
    HarperUpdater *updater = backend.harperUpdater();
    const QString bundled = HarperSchemeHandler::bundledVersion();
    Cycle146Server server; QVERIFY(server.start());
    cycle146Publish(server, cycle146Package(Cycle146Newer));
    updater->setRegistryUrl(server.url(QStringLiteral("/harper.js/latest")));
    QSignalSpy installed(updater, &HarperUpdater::installed), failed(updater, &HarperUpdater::failed), notices(updater, &HarperUpdater::notice), available(updater, &HarperUpdater::updateAvailable);
    const auto setLast = [](const QDateTime &when) { QSettings settings; settings.setValue("harper/lastCheck", when.toString(Qt::ISODate)); settings.sync(); };
    const auto asked = [&] { QTest::qWait(300); return server.requests.size(); };

    // Checked yesterday: nothing is asked.
    setLast(QDateTime::currentDateTime().addDays(-1));
    updater->runScheduledCheck();
    QVERIFY(!updater->checking());
    QCOMPARE(asked(), 0);
    // Checked long ago but automatic checks are off: nothing is asked.
    setLast(QDateTime::currentDateTime().addDays(-30));
    updater->setAutoCheck(false);
    QVERIFY(!updater->autoCheck());
    updater->runScheduledCheck();
    QCOMPARE(asked(), 0);
    QCOMPARE(QSettings().value("harper/autoCheck").toBool(), false); // persisted
    updater->setAutoCheck(true);
    // Eight days: it checks, finds Harper 99.0.1, installs it and says so on the banner.
    setLast(QDateTime::currentDateTime().addDays(-8));
    updater->runScheduledCheck();
    QVERIFY(updater->checking());
    QTRY_COMPARE_WITH_TIMEOUT(installed.count(), 1, 20000);
    QCOMPARE(available.count(), 1);
    QCOMPARE(installed.at(0).at(0).toString(), Cycle146Newer);
    QCOMPARE(notices.count(), 1);
    QCOMPARE(notices.at(0).at(0).toString(), QStringLiteral("Harper %1 installed (was %2)").arg(Cycle146Newer, bundled));
    QCOMPARE(updater->servedVersion(), Cycle146Newer);
    QVERIFY2(updater->lastCheck().secsTo(QDateTime::currentDateTime()) < 60, "the check was recorded");
    // Just checked: not again for a week.
    const int before = server.requests.size();
    updater->runScheduledCheck();
    QCOMPARE(asked(), before);
    // A week on with the same version published: up to date (served), nothing installed, nothing said.
    setLast(QDateTime::currentDateTime().addDays(-8));
    updater->runScheduledCheck();
    QTRY_VERIFY_WITH_TIMEOUT(!updater->checking() && server.requests.size() > before, 10000);
    QCOMPARE(installed.count(), 1);
    QCOMPARE(notices.count(), 1);
    // The user rolls back: the same version is not installed again by the weekly check, though it is offered.
    updater->rollback();
    QCOMPARE(updater->servedVersion(), bundled);
    notices.clear();
    setLast(QDateTime::currentDateTime().addDays(-8));
    const int beforeRollbackCheck = server.requests.size();
    updater->runScheduledCheck();
    QTRY_VERIFY_WITH_TIMEOUT(!updater->checking() && server.requests.size() > beforeRollbackCheck, 10000);
    QCOMPARE(installed.count(), 1);
    QCOMPARE(updater->latestVersion(), Cycle146Newer);
    QVERIFY(updater->hasUpdate());
    QCOMPARE(notices.count(), 0);
    QVERIFY(!server.requests.mid(beforeRollbackCheck).contains(QStringLiteral("/harper.js/-/harper.js-%1.tgz").arg(Cycle146Newer)));
    // The manual command still installs it.
    updater->checkAndInstall();
    QTRY_COMPARE_WITH_TIMEOUT(installed.count(), 2, 20000);
    QCOMPARE(updater->servedVersion(), Cycle146Newer);

    // A newer release than the one installed is picked up by the weekly check, once.
    updater->rollback();
    QSettings().remove("harper/autoAttempted");
    const QString newest = QStringLiteral("99.0.2");
    cycle146Publish(server, cycle146Package(newest));
    notices.clear(); installed.clear();
    setLast(QDateTime::currentDateTime().addDays(-8));
    updater->runScheduledCheck();
    QTRY_COMPARE_WITH_TIMEOUT(installed.count(), 1, 20000);
    QCOMPARE(updater->servedVersion(), newest);
    QCOMPARE(notices.count(), 1);
    QCOMPARE(notices.at(0).at(0).toString(), QStringLiteral("Harper %1 installed (was %2)").arg(newest, bundled));
    // Only the version in use and the one that was in use are kept (16 MB each): 99.0.1 was rolled back before this install, so it goes.
    updater->rollback();
    QVERIFY(QDir(HarperSchemeHandler::harperRoot() + QStringLiteral("/") + newest).exists() && !QDir(HarperSchemeHandler::harperRoot() + QStringLiteral("/") + Cycle146Newer).exists());

    // Network failures are silent in the automatic path.
    server.stop();
    notices.clear(); failed.clear();
    setLast(QDateTime::currentDateTime().addDays(-8));
    updater->runScheduledCheck();
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 10000);
    QCOMPARE(notices.count(), 0);
    QVERIFY(!updater->error().isEmpty());
    // ...and shown in the manual one.
    updater->checkAndInstall();
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 2, 10000);
    QCOMPARE(notices.count(), 1);
    QVERIFY(notices.at(0).at(0).toString().startsWith(QStringLiteral("Could not check for Harper updates: ")));
    // A download that fails verification in the automatic path is not silent, and is not tried again for that version.
    Cycle146Server lying; QVERIFY(lying.start());
    const Cycle146Package good = cycle146Package(QStringLiteral("99.0.3"));
    lying.routes.insert(QStringLiteral("/harper.js/latest"), cycle146Registry(lying, good.version, cycle146Integrity("other")));
    lying.routes.insert(QStringLiteral("/harper.js/-/harper.js-99.0.3.tgz"), good.bytes);
    updater->setRegistryUrl(lying.url(QStringLiteral("/harper.js/latest")));
    notices.clear(); failed.clear();
    setLast(QDateTime::currentDateTime().addDays(-8));
    updater->runScheduledCheck();
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 20000);
    QCOMPARE(notices.count(), 1);
    QVERIFY(notices.at(0).at(0).toString().contains(QStringLiteral("does not match")));
    const int tarballs = lying.requests.count(QStringLiteral("/harper.js/-/harper.js-99.0.3.tgz"));
    setLast(QDateTime::currentDateTime().addDays(-8));
    updater->runScheduledCheck();
    QTRY_VERIFY_WITH_TIMEOUT(!updater->checking(), 10000);
    QTest::qWait(200);
    QCOMPARE(lying.requests.count(QStringLiteral("/harper.js/-/harper.js-99.0.3.tgz")), tarballs);
}

// ---- the Help menu, the banner and the pane ------------------------------------------------------------------------------

void FomawriteTest::helpMenuOffersWritingCheckerUpdatesWithBannerAndPaneStatus() {
    Cycle146Sandbox sandbox;
    Cycle103SettingsSnapshot settings;
    Cycle117Window f; QVERIFY2(f.start(1200), qPrintable(f.error));
    auto *spell = f.backend.spellCheck();
    if (!spell->available()) QSKIP("the system spell checker exists only on macOS");
    QObject *window = f.window.data();
    HarperUpdater *updater = f.backend.harperUpdater();
    QVERIFY2(!f.backend.checksHarperUpdates(), "the suite never runs the weekly check from a window (no network in tests)");
    const QString bundled = HarperSchemeHandler::bundledVersion();
    // The Help menu has the three commands, with state: check enabled, use-built-in dimmed until an update is installed, the toggle ticked.
    for (const char *name : {"helpCheckWritingCheckerUpdates", "helpUseBuiltInWritingChecker", "helpCheckWritingCheckerAutomatically"})
        QVERIFY2(window->findChild<QObject *>(QString::fromLatin1(name)), name);
    auto *check = window->findChild<QObject *>("helpCheckWritingCheckerUpdates");
    auto *builtIn = window->findChild<QObject *>("helpUseBuiltInWritingChecker");
    auto *automatic = window->findChild<QObject *>("helpCheckWritingCheckerAutomatically");
    QCOMPARE(check->property("text").toString(), QStringLiteral("Check for Writing Checker Updates…"));
    QCOMPARE(builtIn->property("text").toString(), QStringLiteral("Use Built-in Writing Checker"));
    QCOMPARE(automatic->property("text").toString(), QStringLiteral("Check for Updates Automatically"));
    QVERIFY(cycle145Enabled(window, "checkWritingCheckerUpdates") && check->property("enabled").toBool());
    QVERIFY(!cycle145Enabled(window, "useBuiltInWritingChecker") && !builtIn->property("enabled").toBool());
    QVERIFY(cycle145Checked(window, "checkWritingCheckerAutomatically") && automatic->property("checked").toBool());
    QVERIFY(automatic->property("checkable").toBool());
    QVERIFY(cycle144Run(window, "checkWritingCheckerAutomatically"));
    QVERIFY(!updater->autoCheck() && !cycle145Checked(window, "checkWritingCheckerAutomatically") && !automatic->property("checked").toBool());
    QVERIFY(cycle144Run(window, "checkWritingCheckerAutomatically"));
    QVERIFY(updater->autoCheck() && cycle145Checked(window, "checkWritingCheckerAutomatically"));

    // Harper is up in the page; the pane's status line says where it came from.
    spell->setEnabled(true);
    auto *bridge = f.backend.editorBridge();
    bridge->ready(); bridge->harperReady(bundled);
    QVERIFY(f.mode(0));
    QVERIFY(cycle144Run(window, "spelling"));
    QTRY_VERIFY(cycle144Control(window, "reviewFilterStyle"));
    QCOMPARE(cycle145Label(window, "reviewEngine"), QStringLiteral("Harper %1 (built-in)").arg(bundled));

    Cycle146Server server; QVERIFY(server.start());
    updater->setRegistryUrl(server.url(QStringLiteral("/harper.js/latest")));
    // Up to date: the banner says so.
    server.routes.insert(QStringLiteral("/harper.js/latest"), cycle146Registry(server, bundled, cycle146Integrity("x")));
    QVERIFY(cycle144Run(window, "checkWritingCheckerUpdates"));
    QTRY_COMPARE_WITH_TIMEOUT(window->property("navigationNotice").toString(), QStringLiteral("Harper is up to date (%1)").arg(bundled), 10000);
    // A newer one: "available — installing…", then the result; the pane and the menu follow.
    const Cycle146Package package = cycle146Package(Cycle146Newer);
    cycle146Publish(server, package);
    QSignalSpy notices(updater, &HarperUpdater::notice), installed(updater, &HarperUpdater::installed);
    QVERIFY(cycle144Run(window, "checkWritingCheckerUpdates"));
    QVERIFY(!cycle145Enabled(window, "checkWritingCheckerUpdates")); // a check is running
    QTRY_COMPARE_WITH_TIMEOUT(installed.count(), 1, 20000);
    QCOMPARE(notices.at(0).at(0).toString(), QStringLiteral("Harper %1 available — installing…").arg(Cycle146Newer));
    QCOMPARE(notices.at(1).at(0).toString(), QStringLiteral("Harper %1 installed (was %2)").arg(Cycle146Newer, bundled));
    QCOMPARE(window->property("navigationNotice").toString(), notices.at(1).at(0).toString());
    QTRY_VERIFY(cycle145Enabled(window, "checkWritingCheckerUpdates"));
    QVERIFY(cycle145Enabled(window, "useBuiltInWritingChecker") && builtIn->property("enabled").toBool());
    // The page reloads the engine and announces the new version.
    QTRY_VERIFY(cycle145Label(window, "reviewEngine").startsWith(QStringLiteral("macOS checker"))); // while the new engine loads
    bridge->harperReady(Cycle146Newer);
    QTRY_COMPARE(cycle145Label(window, "reviewEngine"), QStringLiteral("Harper %1 (installed)").arg(Cycle146Newer));
    // Back to the built-in one from the menu.
    QVERIFY(cycle144Run(window, "useBuiltInWritingChecker"));
    QCOMPARE(window->property("navigationNotice").toString(), QStringLiteral("Using the built-in Harper %1").arg(bundled));
    QVERIFY(!cycle145Enabled(window, "useBuiltInWritingChecker"));
    bridge->harperReady(bundled);
    QTRY_COMPARE(cycle145Label(window, "reviewEngine"), QStringLiteral("Harper %1 · %2 available (built-in)").arg(bundled, Cycle146Newer));
    // A failing check puts the reason on the banner.
    server.stop();
    QVERIFY(cycle144Run(window, "checkWritingCheckerUpdates"));
    QTRY_VERIFY_WITH_TIMEOUT(window->property("navigationNotice").toString().startsWith(QStringLiteral("Could not check for Harper updates: ")), 10000);
}

// ---- the real page -----------------------------------------------------------------------------------------------------

// The real engine in the real page, updated and rolled back: a package made from the bundled engine under a new version
// is installed, the page re-reads VERSION, imports fomawrite://harper/<new>/index.js (a URL it has never seen) and lints with it.
void FomawriteTest::harperUpdateLoadsInTheRealPageUnderItsOwnVersionedUrlAndRollsBack() {
    Cycle146Sandbox sandbox;
    Cycle103SettingsSnapshot settings;
    Backend backend;
    QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/Main.qml")));
    QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
    auto *quick = qobject_cast<QQuickWindow *>(window.data()); QVERIFY(quick);
    quick->resize(1200, 800); quick->show(); QVERIFY(QTest::qWaitForWindowExposed(quick));
    QVERIFY(QMetaObject::invokeMethod(window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    auto *bridge = backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 20000);
    auto *view = window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QTRY_VERIFY_WITH_TIMEOUT(!view->property("loading").toBool(), 15000);
    const QString bundled = HarperSchemeHandler::bundledVersion();
    QSignalSpy readied(bridge, &EditorBridge::harperReadied), replied(bridge, &EditorBridge::harperReplied);
    emit bridge->harperLoad(QStringLiteral("Australian"));
    QTRY_VERIFY_WITH_TIMEOUT(readied.count() >= 1, 60000);
    QCOMPARE(readied.last().at(0).toString(), bundled);
    // The scheme says no-store on what it serves, and VERSION is the unversioned path.
    const QVariant header = cycle145PageResult(view, QStringLiteral("window.__hdr = undefined; fetch('fomawrite://harper/VERSION').then(function(r) { return r.text().then(function(t) { window.__hdr = r.headers.get('cache-control') + '|' + t.trim(); }); }).catch(function(e) { window.__hdr = 'error ' + e; }); null"), 100);
    Q_UNUSED(header);
    QTRY_COMPARE_WITH_TIMEOUT(cycle145PageResult(view, QStringLiteral("window.__hdr || null")).toString(), QStringLiteral("no-store|%1").arg(bundled), 5000);

    // A package made from the real bundled engine, as a newer release.
    QTemporaryDir staging; QVERIFY(staging.isValid());
    const QString dist = staging.filePath(QStringLiteral("package/dist"));
    QDir().mkpath(dist);
    const auto bundledFile = [](const char *name) { QFile f(QStringLiteral(":/editor/dist/harper/") + QString::fromLatin1(name)); return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray(); };
    QByteArray index = bundledFile("index.js"), binary = bundledFile("binary.js");
    QVERIFY(!index.isEmpty() && !binary.isEmpty());
    index.replace("./BinaryModule.js", "./BinaryModule-real.js"); binary.replace("./BinaryModule.js", "./BinaryModule-real.js");
    index += "\nglobalThis.__harperMarker = \"99.0.1\"; // proves which module the page ran\n";
    QVERIFY(cycle146Write(dist + QStringLiteral("/index.js"), index) && cycle146Write(dist + QStringLiteral("/binary.js"), binary));
    QVERIFY(cycle146Write(dist + QStringLiteral("/BinaryModule-real.js"), bundledFile("BinaryModule.js")) && cycle146Write(dist + QStringLiteral("/harper_wasm_bg.wasm"), bundledFile("harper_wasm_bg.wasm")));
    QVERIFY(cycle146Write(staging.filePath(QStringLiteral("package/LICENSE")), "Apache-2.0\n"));
    const QString archive = staging.filePath(QStringLiteral("real.tgz"));
    QCOMPARE(QProcess::execute(QStringLiteral("/usr/bin/tar"), {QStringLiteral("-czf"), archive, QStringLiteral("-C"), staging.path(), QStringLiteral("package")}), 0);
    Cycle146Package package; package.version = Cycle146Newer;
    { QFile f(archive); QVERIFY(f.open(QIODevice::ReadOnly)); package.bytes = f.readAll(); package.integrity = cycle146Integrity(package.bytes); }
    Cycle146Server server; QVERIFY(server.start());
    cycle146Publish(server, package);
    backend.harperUpdater()->setRegistryUrl(server.url(QStringLiteral("/harper.js/latest")));
    QSignalSpy installed(backend.harperUpdater(), &HarperUpdater::installed), failed(backend.harperUpdater(), &HarperUpdater::failed);
    backend.harperUpdater()->checkAndInstall();
    QTRY_VERIFY_WITH_TIMEOUT(installed.count() == 1 || failed.count() == 1, 60000);
    QVERIFY2(failed.count() == 0, failed.isEmpty() ? "" : qPrintable(failed.at(0).at(0).toString()));
    // The engine reloads in the page: harperReady with the new version, from the new versioned URL.
    QTRY_COMPARE_WITH_TIMEOUT(readied.last().at(0).toString(), Cycle146Newer, 60000);
    // The module that ran is the new one, imported from its own versioned URL (its marker is set; the bundled module has none).
    QCOMPARE(cycle145PageResult(view, QStringLiteral("window.__harperMarker || null")).toString(), Cycle146Newer);
    // And it lints.
    const QString text = QStringLiteral("We recieve the the mail.");
    emit bridge->harperLint(41, text);
    QTRY_VERIFY_WITH_TIMEOUT(([&] { for (const auto &r : replied) if (r.at(0).toInt() == 41) return true; return false; })(), 20000);
    // Rolled back: the page loads the bundled version again, from its own versioned URL.
    backend.harperUpdater()->rollback();
    QTRY_COMPARE_WITH_TIMEOUT(readied.last().at(0).toString(), bundled, 60000);
    emit bridge->harperLint(42, text);
    QTRY_VERIFY_WITH_TIMEOUT(([&] { for (const auto &r : replied) if (r.at(0).toInt() == 42) return true; return false; })(), 20000);
    backend.discardRecovery();
}
