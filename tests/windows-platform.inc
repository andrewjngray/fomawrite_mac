// Windows M1: capability truth, pinned Harper policy, and Qt appearance.
#include "systemtheme.h"
#include <QPalette>
#include <QScopeGuard>
#include <QStyleHints>

void FomawriteTest::platformWritingCheckerCapabilitiesAndBundledPolicy() {
    Cycle146Sandbox sandbox;
    HarperUpdater updater;
#ifdef Q_OS_WIN
    QVERIFY(updater.updatesSupported());
    QVERIFY(QDir::isAbsolutePath(HarperUpdater::archiveTool()));
    QVERIFY(HarperUpdater::archiveTool().endsWith(QStringLiteral("/tar.exe"), Qt::CaseInsensitive));
    const QString bundled = updater.bundledVersion(), root = HarperSchemeHandler::harperRoot();
    QVERIFY(HarperSchemeHandler::isValidVersion(bundled));
    // A legacy directory and an unactivated complete version cannot self-enable.
    QVERIFY(cycle146Write(root + "/current/VERSION", "99.0.1\n"));
    const QString folder = root + QStringLiteral("/99.0.1");
    for (const char *name : {"index.js", "binary.js", "BinaryModule.js"}) QVERIFY(cycle146Write(folder + '/' + name, "engine"));
    QVERIFY(cycle146Write(folder + "/VERSION", "99.0.1\n"));
    QVERIFY(cycle146Write(folder + "/harper_wasm_bg.wasm", Cycle146Wasm));
    QCOMPARE(updater.servedVersion(), bundled); QVERIFY(!updater.hasOverride());
    QString error; QVERIFY(HarperActivation::commit(root, "99.0.1", &error));
    QCOMPARE(updater.installedVersion(), QStringLiteral("99.0.1"));
    QCOMPARE(HarperSchemeHandler::resolvePath(QUrl("fomawrite://harper/99.0.1/index.js")), folder + "/index.js");
    QVERIFY(cycle146Write(root + "/current-version", "../99.0.1\n"));
    QCOMPARE(updater.servedVersion(), bundled);
    QVERIFY(HarperActivation::commit(root, "99.0.1", &error));
    QVERIFY(QFile::remove(folder + "/BinaryModule.js")); QCOMPARE(updater.servedVersion(), bundled);
    QVERIFY(HarperActivation::commit(root, "bundled", &error)); QCOMPARE(updater.servedVersion(), bundled);
    QSettings().setValue("harper/autoCheck", true); updater.setAutoCheck(false); QVERIFY(!updater.autoCheck());
#else
    QVERIFY(updater.updatesSupported());
#endif
}

void FomawriteTest::spellCheckPlatformAvailabilityFollowsHarper() {
    Cycle103SettingsSnapshot settings;
    QSettings().setValue(QStringLiteral("writing/engine"), QStringLiteral("macos"));
    Cycle145Fixture fixture(false);
    auto &spell = fixture.spell;
#ifdef Q_OS_MACOS
    QVERIFY(spell.nativeEngineAvailable());
    QVERIFY(spell.available());
#else
    QVERIFY(!spell.nativeEngineAvailable());
    QCOMPARE(spell.engine(), QStringLiteral("harper"));
    QCOMPARE(spell.effectiveEngine(), QStringLiteral("none"));
    QVERIFY(!spell.available());
    spell.setEngine(QStringLiteral("macos"));
    QCOMPARE(spell.engine(), QStringLiteral("harper"));
#endif
    spell.setEngine(QStringLiteral("harper"));
    QSignalSpy availability(&spell, &SpellCheck::availableChanged);
    fixture.bridge.ready(); fixture.bridge.harperReady(QStringLiteral("2.10.0-test"));
    QVERIFY(spell.available());
    QCOMPARE(spell.effectiveEngine(), QStringLiteral("harper"));
    const QString word = QStringLiteral("fomawriteplatformword");
    spell.learnWord(word);
    QVERIFY(spell.hasLearnedWord(word));
    QVERIFY(fixture.engine.words().contains(word));
    spell.unlearnWord(word);
    QVERIFY(!spell.hasLearnedWord(word));
    QVERIFY(!fixture.engine.words().contains(word));
    fixture.engine.reload();
    QCOMPARE(spell.available(), spell.nativeEngineAvailable());
    QCOMPARE(spell.effectiveEngine(), spell.nativeEngineAvailable() ? QStringLiteral("macos") : QStringLiteral("none"));
    if (!spell.nativeEngineAvailable()) QCOMPARE(availability.count(), 2);
}

void FomawriteTest::windowsAppearanceTracksQtColorScheme() {
#ifndef Q_OS_WIN
    QSKIP("Windows Qt appearance implementation; native Windows execution required");
#else
    auto *hints = QGuiApplication::styleHints();
    QVERIFY(hints);
    const QPalette beforePalette = QGuiApplication::palette();
    const auto restore = qScopeGuard([&] {
        hints->unsetColorScheme();
        QGuiApplication::setPalette(beforePalette);
    });
    SystemTheme theme;
    QCOMPARE(theme.textScale(), 1.0); // Qt handles Windows display DPI itself.
    if (QGuiApplication::platformName() != QStringLiteral("windows")) {
        // QStyleHints::setColorScheme is a platform request, not a setter for
        // its stored value. Qt's offscreen theme does not implement overrides.
        // Exercise the real fallback with actual Qt palette changes instead.
        QCOMPARE(hints->colorScheme(), Qt::ColorScheme::Unknown);
        QSignalSpy changed(&theme, &SystemTheme::darkModeChanged);
        QPalette palette = beforePalette;
        palette.setColor(QPalette::Window, Qt::white);
        QGuiApplication::setPalette(palette);
        theme.refresh();
        QVERIFY(!theme.darkMode());
        changed.clear();
        palette.setColor(QPalette::Window, Qt::black);
        QGuiApplication::setPalette(palette);
        theme.refresh();
        QVERIFY(theme.darkMode());
        QCOMPARE(changed.count(), 1);
        palette.setColor(QPalette::Window, Qt::white);
        QGuiApplication::setPalette(palette);
        theme.refresh();
        QVERIFY(!theme.darkMode());
        QCOMPARE(changed.count(), 2);
        qInfo("Verified unknown-scheme palette fallback; native Windows light/dark transitions require the separate windows-platform run.");
        return;
    }
    QSignalSpy changed(&theme, &SystemTheme::darkModeChanged);
    hints->setColorScheme(Qt::ColorScheme::Dark);
    QTRY_COMPARE(hints->colorScheme(), Qt::ColorScheme::Dark);
    QTRY_VERIFY(theme.darkMode());
    hints->setColorScheme(Qt::ColorScheme::Light);
    QTRY_COMPARE(hints->colorScheme(), Qt::ColorScheme::Light);
    QTRY_VERIFY(!theme.darkMode());
    hints->setColorScheme(Qt::ColorScheme::Dark);
    QTRY_COMPARE(hints->colorScheme(), Qt::ColorScheme::Dark);
    QTRY_VERIFY(theme.darkMode());
    QVERIFY(changed.count() >= 2);
#endif
}
