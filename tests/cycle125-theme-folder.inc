#include "publishingthemes.h"

void FomawriteTest::publishingFolderThemesResolveNamesImportsAndAssets() {
    const QString folder = PublishingThemes::directory();
    QVERIFY(QDir().mkpath(folder + "/cycle125-assets"));
    const QString theme = folder + "/Cycle125_Theme.CSS";
    const QString imported = folder + "/cycle125-assets/base.css";
    const QString image = folder + "/cycle125-assets/paper.png";
    const QString reserved = folder + "/claude-like.css";
    const bool createReserved = !QFileInfo::exists(reserved);
    QString importedId;
    const auto cleanup = qScopeGuard([&] {
        QFile::remove(theme); QDir(folder + "/cycle125-assets").removeRecursively();
        if (createReserved) QFile::remove(reserved);
        if (!importedId.isEmpty()) QFile::remove(folder + '/' + importedId + ".css");
    });
    auto write = [](const QString &path, const QByteArray &bytes) {
        QFile file(path); return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(bytes) == bytes.size();
    };
    QImage paper(8, 8, QImage::Format_RGB32); paper.fill(Qt::blue); QVERIFY(paper.save(image));
    QVERIFY(write(imported, "#write{background-image:url(paper.png)}"));
    QVERIFY(write(theme, "/* Made with <3 */ @import 'cycle125-assets/base.css'; #write:before{content:'\\221A';} @font-face{font-family:Missing;src:url(cycle125-assets/missing.ttf)}"));
    if (createReserved) QVERIFY(write(reserved, "#write{color:#123456}"));
    QString id; bool bundled = false, folderReserved = false;
    for (const auto &entry : PublishingThemes::catalog()) {
        const auto item = entry.toMap();
        if (item["name"].toString() == "Cycle125 Theme") id = item["id"].toString();
        if (item["id"].toString() == "claude-like") bundled = true;
        if (item["id"].toString() == "file:claude-like.css") folderReserved = true;
    }
    QVERIFY(bundled); QVERIFY(folderReserved); QVERIFY(!id.isEmpty());
    QString error, advisory;
    const auto css = PublishingThemes::css(id, &error, &advisory); QVERIFY2(css, qPrintable(error));
    QVERIFY(css->contains("data:image/png;base64,")); QVERIFY(css->contains(QChar(0x221a)));
    QVERIFY(!css->contains("@import")); QVERIFY(!css->contains("<3")); QVERIFY(advisory.contains("missing.ttf"));
    // Import shares the optional-font fallback with folder selection; imported
    // output keeps embedded assets after its source resources are removed.
    QVERIFY2(PublishingThemes::importTheme(QUrl::fromLocalFile(theme), &importedId, &error), qPrintable(error));
    QVERIFY(error.contains("missing.ttf"));
    QVERIFY(QFile::remove(imported)); QVERIFY(QFile::remove(image));
    const auto portable = PublishingThemes::css(importedId, &error); QVERIFY2(portable, qPrintable(error));
    QVERIFY(portable->contains("data:image/png;base64,"));
    QVERIFY(portable->contains("local(\"Fomawrite unavailable theme font\")"));
    QVERIFY(portable->contains(QChar(0x221a)));
    for (const QByteArray &unsafe : {QByteArray("@import url(https://example.com/theme.css);"),
                                   QByteArray("#write{background:url(../outside.png)}"),
                                   QByteArray("#write{background:url('\\68 ttps://example.com/a.png')}"),
                                   QByteArray("#write{content:'</style><script>x</script>'}")}) {
        QVERIFY(write(theme, unsafe));
        QVERIFY(!PublishingThemes::css(id, &error)); QVERIFY(!error.isEmpty());
        QString rejectedId;
        QVERIFY(!PublishingThemes::importTheme(QUrl::fromLocalFile(theme), &rejectedId, &error));
        QVERIFY(rejectedId.isEmpty()); QVERIFY(!error.isEmpty());
    }
}

void FomawriteTest::publishingFolderThemesRefreshAfterExternalChanges() {
    Cycle103SettingsSnapshot settings;
    QSettings().remove("output/publishingTheme");
    Backend backend(nullptr, true);
    const QString path = PublishingThemes::directory() + "/cycle125-watched.css";
    const auto cleanup = qScopeGuard([&] { QFile::remove(path); });
    QSignalSpy changed(&backend, &Backend::publishingThemesChanged);
    auto write = [&](const QByteArray &bytes) {
        QSaveFile file(path); return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
    };
    QVERIFY(write("#write{color:#123456}"));
    QTRY_VERIFY_WITH_TIMEOUT(changed.count() > 0, 3000);
    QVERIFY(backend.selectPublishingTheme("cycle125-watched"));
    changed.clear();
    QVERIFY(write("#write{color:#654321}"));
    QTRY_VERIFY_WITH_TIMEOUT(changed.count() > 0, 3000);
    QString error;
    const auto updated = PublishingThemes::css(backend.publishingThemeId(), &error);
    QVERIFY2(updated, qPrintable(error)); QVERIFY(updated->contains("#654321"));
    changed.clear();
    QVERIFY(write("#write{background:url(https://example.com/no.png)}"));
    QTRY_VERIFY_WITH_TIMEOUT(!backend.publishingThemeError().isEmpty(), 3000);
    QVERIFY(!backend.selectPublishingTheme("file:../outside.css"));
    QCOMPARE(backend.publishingThemeId(), QString("cycle125-watched"));
    QVERIFY(QFile::remove(path));
    QTRY_VERIFY_WITH_TIMEOUT(backend.publishingThemeError().contains("missing"), 3000);
}
