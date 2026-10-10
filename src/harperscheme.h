#pragma once

// fomawrite://harper/<file> — the engine files the editor page loads.
//
// The page lives on qrc:, and Chromium's fetch() refuses qrc, which the
// WebAssembly loader needs. This scheme is registered as secure, CORS-enabled
// and fetch-capable, and its handler serves each file from the first place it
// exists: an update the app downloaded into its data folder (Help → Check for
// Writing Checker Updates, Cycle 146), else the copy built into the app's
// resources (bin/fetch-harper, pinned in harper.lock.json).

#include <QObject>
#include <QString>
#include <QWebEngineUrlSchemeHandler>

class HarperSchemeHandler : public QWebEngineUrlSchemeHandler {
    Q_OBJECT
public:
    explicit HarperSchemeHandler(QObject *parent = nullptr);
    void requestStarted(QWebEngineUrlRequestJob *job) override;
    // Folder holding a downloaded engine (index.js, binary.js, BinaryModule.js,
    // harper_wasm_bg.wasm, VERSION); empty means none. Checked on every request.
    static QString overrideDirectory();
    // The version served: the override's VERSION file, else the bundled one.
    static QString servedVersion();
};

// Call before QtWebEngineQuick::initialize(): schemes register once, early.
void registerHarperScheme();
// Installs the handler on the default profile. Idempotent. Called from the
// Live pane when its view is created, not at startup: touching the default
// profile before any window exists left WebEngine windows painting blank in
// the offscreen tests (the capture fixtures), so the profile is first used
// where it is first needed.
void installHarperSchemeHandler();
