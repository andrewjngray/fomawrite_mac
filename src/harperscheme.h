#pragma once

// fomawrite://harper/<version>/<file> — the engine files the editor page loads.
//
// The page lives on qrc:, and Chromium's fetch() refuses qrc, which the
// WebAssembly loader needs. This scheme is registered as secure, CORS-enabled
// and fetch-capable.
//
// Paths are versioned (Cycle 146): ES module maps are keyed by URL, so an engine
// the app downloads later (Help -> Check for Writing Checker Updates) must have
// a URL the page has never imported. The handler serves
//
//     fomawrite://harper/VERSION            the version to use (the override's if
//                                           one is installed and valid, else the
//                                           bundled one); the only unversioned path
//     fomawrite://harper/<version>/<file>   from the override folder when
//                                           <version> is its VERSION, else from
//                                           the bundled resources when it is the
//                                           bundled VERSION, else 404
//
// and every reply carries Cache-Control: no-store. The override is the folder
// <AppDataLocation>/harper/current (a symlink to harper/<version>/, written by
// HarperUpdater); the bundled copy is bin/fetch-harper's, pinned in
// harper.lock.json. Nothing in the override folder is ever executed by the app:
// it is only served to the page, which runs in Chromium's sandbox.

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QWebEngineUrlSchemeHandler>

class HarperSchemeHandler : public QWebEngineUrlSchemeHandler {
    Q_OBJECT
public:
    explicit HarperSchemeHandler(QObject *parent = nullptr);
    void requestStarted(QWebEngineUrlRequestJob *job) override;
    // <AppDataLocation>/harper: where HarperUpdater keeps <version>/ folders and
    // the `current` link.
    static QString harperRoot();
    // Folder holding a downloaded engine (index.js, binary.js, BinaryModule.js,
    // harper_wasm_bg.wasm, VERSION) that is installed and valid: the files are
    // there and VERSION is a plain x.y.z. Empty means none. Checked on every request.
    static QString overrideDirectory();
    // The override's VERSION, or empty when none is installed.
    static QString overrideVersion();
    // The bundled engine's VERSION (from the app's resources).
    static QString bundledVersion();
    // The version served: the override's when one is installed, else the bundled one.
    static QString servedVersion();
    // A version the scheme and the updater accept: digits and two dots (2.12.0).
    static bool isValidVersion(const QString &version);
    // Where `url` is served from (a file path or a qrc: path), or empty when the
    // scheme answers 404; `mime` receives the content type. What requestStarted
    // does, without the job, so tests can ask.
    static QString resolvePath(const QUrl &url, QByteArray *mime = nullptr);
};

// Call before QtWebEngineQuick::initialize(): schemes register once, early.
void registerHarperScheme();
// Installs the handler on the default profile. Idempotent. Called from the
// Live pane when its view is created, not at startup: touching the default
// profile before any window exists left WebEngine windows painting blank in
// the offscreen tests (the capture fixtures), so the profile is first used
// where it is first needed.
void installHarperSchemeHandler();
