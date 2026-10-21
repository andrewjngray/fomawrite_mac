// Windows M1: capability truth, pinned Harper policy, and Qt appearance.
#include "systemtheme.h"
#include <QStyleHints>

void FomawriteTest::platformWritingCheckerCapabilitiesAndBundledPolicy() {
    Cycle146Sandbox sandbox;
    HarperUpdater updater;
#ifdef Q_OS_WIN
    QVERIFY(!updater.updatesSupported());
    const QString bundled = updater.bundledVersion();
    QVERIFY(HarperSchemeHandler::isValidVersion(bundled));
    // A complete pre-existing override must not bypass the disabled updater.
    const QString current = HarperSchemeHandler::harperRoot() + QStringLiteral("/current");
    for (const char *name : {"index.js", "binary.js", "BinaryModule.js", "harper_wasm_bg.wasm"})
        QVERIFY(cycle146Write(current + QLatin1Char('/') + QString::fromLatin1(name), "untrusted override"));
    QVERIFY(cycle146Write(current + QStringLiteral("/VERSION"), "99.0.1\n"));
    QVERIFY(HarperSchemeHandler::overrideDirectory().isEmpty());
    QVERIFY(updater.installedVersion().isEmpty());
    QCOMPARE(updater.servedVersion(), bundled);
    QVERIFY(!updater.hasOverride());
    QVERIFY(HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/%1/index.js").arg(bundled))).startsWith(QLatin1String(":/editor/dist/harper/")));
    QVERIFY(HarperSchemeHandler::resolvePath(QUrl(QStringLiteral("fomawrite://harper/99.0.1/index.js"))).isEmpty());
    Cycle146Server server; QVERIFY(server.start());
    updater.setRegistryUrl(server.url(QStringLiteral("/harper.js/latest")));
    QSettings().setValue(QStringLiteral("harper/autoCheck"), true);
    const QVariant beforeAuto = QSettings().value(QStringLiteral("harper/autoCheck"));
    QSignalSpy failed(&updater, &HarperUpdater::failed), notices(&updater, &HarperUpdater::notice);
    QSignalSpy served(&updater, &HarperUpdater::servedChanged);
    updater.setAutoCheck(false);
    QCOMPARE(QSettings().value(QStringLiteral("harper/autoCheck")), beforeAuto);
    QVERIFY(!updater.autoCheck());
    updater.runScheduledCheck();
    QCOMPARE(failed.count(), 0);
    QCOMPARE(notices.count(), 0);
    updater.checkNow(); updater.checkAndInstall(); updater.install(); updater.rollback();
    QCOMPARE(failed.count(), 4);
    QCOMPARE(notices.count(), 4);
    QVERIFY(updater.status().contains(QStringLiteral("unavailable on Windows")));
    QTest::qWait(100);
    QVERIFY(server.requests.isEmpty());
    QVERIFY(!updater.checking() && !updater.installing() && !updater.hasUpdate());
    QVERIFY(!updater.lastCheck().isValid());
    QCOMPARE(served.count(), 0);
    QCOMPARE(cycle146Slurp(current + QStringLiteral("/VERSION")), QStringLiteral("99.0.1\n"));
    QCOMPARE(updater.servedVersion(), bundled);
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
    const auto before = hints->colorScheme();
    SystemTheme theme;
    hints->setColorScheme(Qt::ColorScheme::Dark);
    QTRY_VERIFY(theme.darkMode());
    hints->setColorScheme(Qt::ColorScheme::Light);
    QTRY_VERIFY(!theme.darkMode());
    QCOMPARE(theme.textScale(), 1.0); // Qt handles Windows display DPI itself.
    hints->setColorScheme(before);
#endif
}
