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

namespace {
// A public, synthetic resource tree: one theme, nested imported CSS and an
// image. Shared windows must observe genuine edits without creating edits by
// watching the folder. No private themes are needed for this regression.
struct Cycle125WatchedThemeFixture {
    QString id = "cycle125-watcher-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString folder = PublishingThemes::directory();
    QString theme = folder + '/' + id + ".css";
    QString assets = folder + '/' + id + "-assets";
    QString imported = assets + "/styles/base.css";
    bool create() {
        if (!QDir().mkpath(assets + "/styles") || !QDir().mkpath(assets + "/styles/images")) return false;
        // Match a realistic installed theme catalog, not an empty folder: the
        // macOS watch re-registration loop depends on a populated watch tree.
        for (int pack = 0; pack < 7; ++pack) {
            const QString packPath = assets + QString("/pack%1/fonts").arg(pack);
            if (!QDir().mkpath(packPath)) return false;
            for (int font = 0; font < 12; ++font)
                if (!write(packPath + QString("/sample%1.woff").arg(font), "synthetic unreferenced font fixture")) return false;
            if (!write(assets + QString("/pack%1/theme.css").arg(pack), "#write{color:#123456}")) return false;
        }
        QImage image(8, 8, QImage::Format_RGB32); image.fill(Qt::blue);
        return image.save(assets + "/styles/images/paper.png")
            && write(imported, "#write{color:#123456;background-image:url(images/paper.png)}")
            && write(theme, ("@import '" + id + "-assets/styles/base.css';").toUtf8());
    }
    static bool write(const QString &path, const QByteArray &bytes) {
        QSaveFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
    }
    ~Cycle125WatchedThemeFixture() { QFile::remove(theme); QDir(assets).removeRecursively(); }
};
}

void FomawriteTest::publishingFolderThemesStayQuietAcrossWindows() {
    Cycle103SettingsSnapshot settings;
    QSettings().remove("output/publishingTheme");
    Cycle125WatchedThemeFixture fixture; QVERIFY(fixture.create());
    Backend first(nullptr, true), second(nullptr, true);
    QSignalSpy firstChanged(&first, &Backend::publishingThemesChanged);
    QSignalSpy secondChanged(&second, &Backend::publishingThemesChanged);
    // Let initial filesystem events settle, then measure an untouched folder
    // for several refresh/debounce intervals. No manual reload is performed.
    QTest::qWait(800);
    firstChanged.clear(); secondChanged.clear();
    QTest::qWait(1400);
    QCOMPARE(firstChanged.count(), 0);
    QCOMPARE(secondChanged.count(), 0);
}

void FomawriteTest::publishingFolderThemesReloadAndAtomicResourcesSettle() {
    Cycle103SettingsSnapshot settings;
    QSettings().remove("output/publishingTheme");
    Cycle125WatchedThemeFixture fixture; QVERIFY(fixture.create());
    Backend first(nullptr, true), second(nullptr, true);
    QVERIFY(first.selectPublishingTheme(fixture.id));
    QVERIFY(second.selectPublishingTheme(fixture.id));
    QSignalSpy firstChanged(&first, &Backend::publishingThemesChanged);
    QSignalSpy secondChanged(&second, &Backend::publishingThemesChanged);
    QTest::qWait(800);
    firstChanged.clear(); secondChanged.clear();
    // Explicit Reload is useful even without a detected disk change.
    first.reloadPublishingThemes();
    QCOMPARE(firstChanged.count(), 1);
    QTest::qWait(900);
    QCOMPARE(firstChanged.count(), 1);
    QCOMPARE(secondChanged.count(), 0);
    firstChanged.clear(); secondChanged.clear();
    // A real editor's atomic replacement changes a nested dependency, while
    // the theme entry itself stays unchanged. Every window must refresh once.
    QVERIFY(Cycle125WatchedThemeFixture::write(fixture.imported,
        "#write{color:#654321;background-image:url(images/paper.png)}"));
    QTRY_VERIFY_WITH_TIMEOUT(firstChanged.count() >= 1 && secondChanged.count() >= 1, 3000);
    QString error;
    const auto updatedFirst = PublishingThemes::css(first.publishingThemeId(), &error);
    QVERIFY2(updatedFirst, qPrintable(error)); QVERIFY(updatedFirst->contains("#654321"));
    const auto updatedSecond = PublishingThemes::css(second.publishingThemeId(), &error);
    QVERIFY2(updatedSecond, qPrintable(error)); QVERIFY(updatedSecond->contains("#654321"));
    QTest::qWait(1200);
    QCOMPARE(firstChanged.count(), 1);
    QCOMPARE(secondChanged.count(), 1);
    // The replacement remains watched after its old inode disappears.
    firstChanged.clear(); secondChanged.clear();
    QVERIFY(Cycle125WatchedThemeFixture::write(fixture.imported, "#write{color:#abcdef}"));
    QTRY_VERIFY_WITH_TIMEOUT(firstChanged.count() >= 1 && secondChanged.count() >= 1, 3000);
    const auto secondEdit = PublishingThemes::css(first.publishingThemeId(), &error);
    QVERIFY2(secondEdit, qPrintable(error)); QVERIFY(secondEdit->contains("#abcdef"));
    QTest::qWait(1200);
    QCOMPARE(firstChanged.count(), 1);
    QCOMPARE(secondChanged.count(), 1);
    // Editors may preserve coarse timestamps on a same-size in-place edit.
    // A real file event still has to inspect its content, not just metadata.
    firstChanged.clear(); secondChanged.clear();
    const auto previousTime = QFileInfo(fixture.imported).lastModified();
    QFile inPlace(fixture.imported);
    QVERIFY(inPlace.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(inPlace.write("#write{color:#fedcba}"), qint64(21));
    QVERIFY(inPlace.setFileTime(previousTime, QFileDevice::FileModificationTime));
    inPlace.close();
    QTRY_VERIFY_WITH_TIMEOUT(firstChanged.count() >= 1 && secondChanged.count() >= 1, 3000);
    const auto inPlaceEdit = PublishingThemes::css(first.publishingThemeId(), &error);
    QVERIFY2(inPlaceEdit, qPrintable(error)); QVERIFY(inPlaceEdit->contains("#fedcba"));
    QTest::qWait(900);
    QCOMPARE(firstChanged.count(), 1);
    QCOMPARE(secondChanged.count(), 1);

}
