#pragma once

// Keeping Harper current (Cycle 146).
//
// Harper's rules and dictionary are compiled into the engine, so "updating the
// rules" means a newer harper.js release: this object asks the npm registry for
// the latest version, downloads its tarball, verifies the published sha512,
// unpacks the files the page needs into <AppDataLocation>/harper/<version>/ and
// points <AppDataLocation>/harper/current at it. HarperSchemeHandler serves that
// folder to the editor page in place of the bundled copy (bin/fetch-harper,
// harper.lock.json), the page imports it from its own versioned URL, and
// HarperEngine::reload() makes the page do that. Rolling back removes the link;
// the folders stay, and the bundled engine serves again.
//
// What is verified: the tarball's sha512 equals the `dist.integrity` the
// registry publishes for that version (the tarball is the one npm published;
// npm's TLS and the registry vouch for that string), every tarball entry is
// confined to package/ (files directly in it, or anything under package/dist/),
// no entry is a link, and the files are copied out by name. What is not: the
// registry's answer is trusted, so this does not prove who wrote the release
// (npm's signatures are not checked), and the engine is not run before it is
// loaded in the page. The unpacked folder is never executed by the app; it is
// only served to Chromium's sandbox.
//
// Only https registry and tarball URLs are accepted; the tarball must be on the
// registry's own host. (A registryUrl set for tests may be http on loopback.)

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <memory>

class QNetworkAccessManager;
class QNetworkReply;
class QFile;
class QProcess;
class QTemporaryDir;
class QCryptographicHash;

class HarperUpdater : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString bundledVersion READ bundledVersion NOTIFY changed)
    Q_PROPERTY(QString installedVersion READ installedVersion NOTIFY changed)
    Q_PROPERTY(QString servedVersion READ servedVersion NOTIFY changed)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed)
    Q_PROPERTY(bool checking READ checking NOTIFY changed)
    Q_PROPERTY(bool installing READ installing NOTIFY changed)
    Q_PROPERTY(QDateTime lastCheck READ lastCheck NOTIFY changed)
    Q_PROPERTY(bool autoCheck READ autoCheck WRITE setAutoCheck NOTIFY autoCheckChanged)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString registryUrl READ registryUrl WRITE setRegistryUrl NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    // A newer version than the one served is known (from the last check).
    Q_PROPERTY(bool hasUpdate READ hasUpdate NOTIFY changed)
    // A downloaded engine is serving (so "Use Built-in Writing Checker" has something to do).
    Q_PROPERTY(bool hasOverride READ hasOverride NOTIFY changed)
    // What the Review pane adds after "Harper 2.10.0": " · updating…", " · 2.12.0 available", then " (built-in)" or " (installed)".
    Q_PROPERTY(QString paneSuffix READ paneSuffix NOTIFY changed)
public:
    static constexpr int ScheduledCheckDays = 7;
    static constexpr qint64 MaxRegistryBytes = 1 << 20;        // the registry's answer is a few KB
    static constexpr qint64 MaxTarballBytes = 120LL << 20;     // harper.js 2.10.0 is 20 MB

    explicit HarperUpdater(QObject *parent = nullptr);
    ~HarperUpdater() override;

    QString bundledVersion() const;
    QString installedVersion() const;   // the override's, or empty
    QString servedVersion() const;
    QString latestVersion() const { return m_latest; }
    bool checking() const { return m_checking; }
    bool installing() const { return m_installing; }
    QDateTime lastCheck() const;
    bool autoCheck() const;
    void setAutoCheck(bool on);
    QString status() const { return m_status; }
    QString registryUrl() const { return m_registryUrl; }
    // Settable for tests; the default is https://registry.npmjs.org/harper.js/latest.
    void setRegistryUrl(const QString &url);
    QString error() const { return m_error; }
    bool hasUpdate() const;
    bool hasOverride() const { return !installedVersion().isEmpty(); }
    QString paneSuffix() const;

    // Reads the registry's `latest` for harper.js and compares it with the
    // version served: emits updateAvailable(version) or upToDate(), or failed().
    Q_INVOKABLE void checkNow();
    // Downloads, verifies and installs the version the last check found, makes
    // it the served one and emits installed(version) (and servedChanged).
    Q_INVOKABLE void install();
    // Help > Check for Writing Checker Updates: checkNow, then install when there is
    // something newer, with notices all the way ("up to date", "available —
    // installing…", the result, or the error).
    Q_INVOKABLE void checkAndInstall();
    // Help > Use Built-in Writing Checker: removes the `current` link (the version
    // folders stay) so the bundled engine serves again.
    Q_INVOKABLE void rollback();
    // Ten seconds after the window shows: when autoCheck is on and the last check
    // is older than a week, check and install silently (a notice only when something
    // was installed or a download failed verification; a network failure says
    // nothing). The same version is never installed automatically twice.
    Q_INVOKABLE void runScheduledCheck();
    // Re-reads what is on disk (another window may have installed something).
    Q_INVOKABLE void refresh();

    // Static helpers, public for the tests.
    // x.y.z > a.b.c; an unparseable side counts as 0.0.0.
    static bool isNewer(const QString &candidate, const QString &current);
    // Parses a `sha512-<base64>` integrity string (other tokens ignored) to its 64 digest bytes, or empty.
    static QByteArray digestFromIntegrity(const QString &integrity);
    // Whether `entry` (a path inside the tarball) is acceptable: package/<file>, or anything under package/dist/.
    static bool tarEntryAllowed(const QString &entry);

signals:
    void changed();
    void autoCheckChanged();
    void updateAvailable(const QString &version);
    void upToDate();
    void installed(const QString &version, const QString &previous);
    void rolledBack();
    void failed(const QString &message);
    // A line for the banner (Main.qml's navigation notice).
    void notice(const QString &message);
    // The version served may have changed (installed or rolled back, here or in
    // another window): the engine should reload.
    void servedChanged();

private:
    enum class Mode { Idle, Plain, Manual, Scheduled };
    void startCheck();
    void drain(QNetworkReply *reply, bool toFile);
    void startRequest(const QUrl &url, qint64 limit, int timeoutMs, bool toFile);
    void onRegistryFinished();
    void onTarballFinished();
    void finishCheck(bool ok, const QString &message, bool network);
    void buildAndActivate();
    void activate(const QString &version);
    void finishInstall(bool ok, const QString &message, bool network);
    void setStatus(const QString &status);
    bool registryAllowed(const QUrl &url, QString *why) const;
    bool tarballAllowed(const QUrl &url, QString *why) const;
    void runTar(const QStringList &arguments, int stage);
    void notifyServedChanged();
    void prune(const QString &keepA, const QString &keepB);
    QString root() const;
    void setLastCheck(const QDateTime &when);

    QNetworkAccessManager *network();

    QNetworkAccessManager *m_network = nullptr;
    QPointer<QNetworkReply> m_reply;
    QPointer<QProcess> m_process;
    std::unique_ptr<QTemporaryDir> m_work;
    std::unique_ptr<QFile> m_file;
    std::unique_ptr<QCryptographicHash> m_hash;
    QByteArray m_body;
    qint64 m_received = 0;
    qint64 m_limit = 0;
    bool m_overLimit = false;
    QString m_registryUrl;
    QString m_status, m_error;
    QString m_latest;
    QUrl m_tarball;
    QByteArray m_digest;
    QString m_previousServed;
    QString m_archivePath;
    bool m_checking = false;
    bool m_installing = false;
    Mode m_mode = Mode::Idle;
    static QList<HarperUpdater *> s_instances;
};
