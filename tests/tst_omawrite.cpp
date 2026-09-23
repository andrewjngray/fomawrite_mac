#include "markdownextensions.h"
#include "workspace.h"
#include <QtConcurrent>
#include <QtTest>
#include <cmath>
#include <QProcess>
#include <QScopeGuard>
#include <QUuid>
#include <cstdlib>
#include <QFont>
#include <QFontDatabase>
#include <QTextBlock>
#include <QTextLayout>
#include <QTextDocument>
#include <QQuickTextDocument>
#include <QImage>
#include <QDate>
#include <QClipboard>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>

#include "backend.h"
#include "markdownhighlighter.h"

class OmawriteTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("OmawriteTests");
        QCoreApplication::setApplicationName("OmawriteTests");
        QVERIFY(m_settingsDirectory.isValid());
        for (const QString &face : {"Regular", "Italic", "Bold", "BoldItalic"})
            QVERIFY(QFontDatabase::addApplicationFont(QFINDTESTDATA("../fonts/iAWriterMonoS-" + face + ".ttf")) >= 0);
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        QStandardPaths::setTestModeEnabled(true);
        QQuickStyle::setStyle(QStringLiteral("Material"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           m_settingsDirectory.path());
    }

    void contextFileAndFolderActionsUseClickedPath() {
        QTemporaryDir dir;
        const QUrl root = QUrl::fromLocalFile(dir.path());
        Backend backend;
        QVERIFY(backend.libraryItemAction(root, "newFolder", "Notes"));
        const QUrl folder = QUrl::fromLocalFile(dir.filePath("Notes"));
        QVERIFY(backend.libraryItemAction(folder, "newFile", "Target.md"));
        const QUrl file = QUrl::fromLocalFile(dir.filePath("Notes/Target.md"));
        QFile source(file.toLocalFile()); QVERIFY(source.open(QIODevice::WriteOnly));
        source.write("# Clicked target\n\n**Bold** text.\n"); source.close();
        QVERIFY(backend.libraryItemAction(file, "copyMarkdown"));
        QVERIFY(QGuiApplication::clipboard()->text().startsWith("# Clicked target"));
        QVERIFY(backend.fileUrl().isEmpty());
        QVERIFY(backend.libraryItemAction(file, "copyText"));
        QVERIFY(!QGuiApplication::clipboard()->text().contains("**"));
        QVERIFY(backend.libraryItemAction(file, "exportHtml", QUrl::fromLocalFile(dir.filePath("target.html")).toString()));
        QVERIFY(QFileInfo::exists(dir.filePath("target.html")));
        QVERIFY(!backend.libraryItemAction(file, "exportHtml", file.toString()));
        QVERIFY(backend.libraryItemAction(file, "duplicate", "Copy.md"));
        QVERIFY(!backend.libraryItemAction(file, "duplicate", "Copy.md"));
        QVERIFY(!backend.libraryItemAction(file, "rename", "../Escape.md"));
        QVERIFY(backend.libraryItemAction(file, "rename", "Renamed.md"));
        QVERIFY(QFileInfo::exists(dir.filePath("Notes/Renamed.md")));
        QVERIFY(!QFileInfo::exists(file.toLocalFile()));
        QVERIFY(backend.libraryItemAction(folder, "duplicate", "Notes copy"));
        QVERIFY(QFileInfo::exists(dir.filePath("Notes copy/Renamed.md")));
        auto *library = qobject_cast<FileLibrary *>(backend.library());
        library->setRootFolder(folder); library->toggleFavorite(QUrl::fromLocalFile(dir.filePath("Notes/Renamed.md")));
        QVERIFY(backend.libraryItemAction(folder, "rename", "Archive"));
        QCOMPARE(library->rootFolder(), QUrl::fromLocalFile(QFileInfo(dir.filePath("Archive")).canonicalFilePath()));
        QVERIFY(library->favorites().last().toMap().value("url").toUrl().toLocalFile().endsWith("Archive/Renamed.md"));
        QVERIFY(QFile::link(dir.filePath("Archive"), dir.filePath("Notes copy/link")));
        QVERIFY(!backend.libraryItemAction(QUrl::fromLocalFile(dir.filePath("Notes copy")), "duplicate", "Rejected"));
        QVERIFY(!QFileInfo::exists(dir.filePath("Rejected")));
    }

    void contextOperationsPreserveOpenUnsavedDocument() {
        QTemporaryDir dir;
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create()); QVERIFY(window);
        auto *editor = window->findChild<QObject *>("sourceEditor"); QVERIFY(editor);
        const QUrl file=QUrl::fromLocalFile(dir.filePath("Open.md"));
        editor->setProperty("text", "Saved"); backend.saveAs(file);
        editor->setProperty("text", "Unsaved text"); QVERIFY(backend.modified());
        QVERIFY(backend.libraryItemAction(file, "duplicate", "Copy.md"));
        QFile copy(dir.filePath("Copy.md")); QVERIFY(copy.open(QIODevice::ReadOnly)); QCOMPARE(copy.readAll(), QByteArray("Unsaved text"));
        QVERIFY(!backend.libraryItemAction(file, "trash"));
        QVERIFY(!backend.libraryItemAction(QUrl::fromLocalFile(dir.path()), "rename", "Nope"));
        QVERIFY(backend.libraryItemAction(file, "rename", "Renamed.md"));
        QCOMPARE(editor->property("text").toString(), QString("Unsaved text"));
        QVERIFY(backend.modified()); backend.discardRecovery();
    }

    void locationsRejectOverlapAndMigrateChildrenToFavorites() {
        QSettings settings;
        settings.beginGroup("library");
        QVariantMap original;
        for (const auto &key : settings.allKeys()) original[key] = settings.value(key);
        settings.remove("");
        const auto restore = qScopeGuard([&] {
            settings.remove("");
            for (auto it = original.cbegin(); it != original.cend(); ++it) settings.setValue(it.key(), it.value());
        });
        QTemporaryDir dir;
        QVERIFY(QDir(dir.path()).mkpath("Parent/Child"));
        QVERIFY(QDir(dir.path()).mkdir("Parent-other"));
        const QUrl parent = QUrl::fromLocalFile(QFileInfo(dir.filePath("Parent")).canonicalFilePath());
        const QUrl child = QUrl::fromLocalFile(parent.toLocalFile() + "/Child");
        const QUrl sibling = QUrl::fromLocalFile(QFileInfo(dir.filePath("Parent-other")).canonicalFilePath());
        {
            FileLibrary library;
            QSignalSpy rejected(&library, &FileLibrary::locationRejected);
            QVERIFY(library.addLocation(parent));
            QVERIFY(!library.addLocation(child));
            QVERIFY(!library.addLocation(parent));
            QCOMPARE(rejected.count(), 2);
            QCOMPARE(library.rootFolder(), parent);
            QCOMPARE(library.locations().size(), 1);
            QVERIFY(library.addFavorite(child));
            QVERIFY(library.addFavorite(child));
            QCOMPARE(library.favorites().size(), 1);
            library.setRootFolder(child);
            QCOMPARE(library.rootFolder(), child);
            QCOMPARE(library.locations().size(), 1);
            library.enclosingFolder();
            QCOMPARE(library.rootFolder(), parent);
            QVERIFY(library.navigateHistory(-1));
            QCOMPARE(library.rootFolder(), child);
            QCOMPARE(library.locations().size(), 1);
            QVERIFY(library.addLocation(sibling));
            QCOMPARE(library.locations().size(), 2);
            library.removeLocation(parent);
            QVERIFY(library.addLocation(child));
            QVERIFY(!library.addLocation(parent));
            QVERIFY(QFileInfo(child.toLocalFile()).isDir());
            QVERIFY(!library.addLocation(QUrl("https://example.com")));
            QVERIFY(QFile::link(parent.toLocalFile(), dir.filePath("Alias")));
            QVERIFY(!library.addLocation(QUrl::fromLocalFile(dir.filePath("Alias"))));
        }
        settings.setValue("locations", QStringList{child.toLocalFile(), parent.toLocalFile(), child.toLocalFile()});
        settings.setValue("favorites", QStringList{});
        settings.setValue("root", child);
        {
            FileLibrary migrated;
            QCOMPARE(migrated.locations().size(), 1);
            QCOMPARE(migrated.locations().first().toMap().value("url").toUrl(), parent);
            QCOMPARE(migrated.favorites().size(), 1);
            QCOMPARE(migrated.favorites().first().toMap().value("url").toUrl(), child);
            QCOMPARE(migrated.rootFolder(), child);
        }
        FileLibrary reopened;
        QCOMPARE(reopened.locations().size(), 1);
        QCOMPARE(reopened.favorites().size(), 1);
    }

    void locationPhysicalRenameReplacesLegacyAlias() {
        const QVariant searchesBefore = QSettings().value("library/savedSearches");
        const auto restoreSearches = qScopeGuard([&] { QSettings().setValue("library/savedSearches", searchesBefore); });
        QTemporaryDir dir;
        QVERIFY(QDir(dir.path()).mkdir("Original"));
        Backend backend;
        auto *library = qobject_cast<FileLibrary *>(backend.library());
        library->setRootFolder(QUrl::fromLocalFile(dir.filePath("Original")));
        const QUrl original = library->rootFolder();
        const QUrl document = library->createDocument("Keep.md");
        library->recordRecentFile(document);
        library->toggleFavorite(document);
        library->saveSearch("Keep", false);
        QVERIFY(library->renameLocation(original, "Old sidebar alias"));
        QVERIFY(QDir(dir.path()).mkdir("Occupied"));
        QVERIFY(!backend.libraryItemAction(original, "rename", "Occupied"));
        QCOMPARE(library->rootFolder(), original);
        QVERIFY(QFileInfo::exists(document.toLocalFile()));
        QVERIFY(backend.libraryItemAction(original, "rename", "Renamed folder"));
        const QUrl renamed = QUrl::fromLocalFile(QFileInfo(dir.filePath("Renamed folder")).canonicalFilePath());
        QVERIFY(!QFileInfo::exists(original.toLocalFile()));
        QVERIFY(QFileInfo::exists(renamed.toLocalFile() + "/Keep.md"));
        QCOMPARE(library->rootFolder(), renamed);
        QCOMPARE(library->savedSearches().first().toMap().value("root").toUrl(), renamed);
        QCOMPARE(library->recentFiles().first().toMap().value("url").toUrl().toLocalFile(), renamed.toLocalFile() + "/Keep.md");
        FileLibrary reopened;
        QCOMPARE(reopened.rootFolder(), renamed);
        bool found = false;
        for (const auto &item : reopened.locations()) {
            const auto entry = item.toMap();
            if (entry.value("url").toUrl() == renamed) {
                found = true;
                QCOMPARE(entry.value("name").toString(), QString("Renamed folder"));
            }
            QVERIFY(entry.value("url").toUrl() != original);
        }
        QVERIFY(found);
        QVERIFY(library->copyPath(renamed));
        QCOMPARE(QGuiApplication::clipboard()->text(), renamed.toLocalFile());
    }

    void locationContextActionsPreserveFiles() {
        QTemporaryDir dir;
        FileLibrary library;
        library.setRootFolder(QUrl::fromLocalFile(dir.path()));
        const QUrl url = library.rootFolder();
        const QUrl document = library.createDocument("Keep me.md");
        QVERIFY(library.renameLocation(url, "  Writing desk  "));
        auto labelFor = [&](FileLibrary &value) {
            for (const auto &item : value.locations())
                if (item.toMap().value("url").toUrl() == url) return item.toMap().value("name").toString();
            return QString();
        };
        QCOMPARE(labelFor(library), QString("Writing desk"));
        FileLibrary reopened;
        QCOMPARE(labelFor(reopened), QString("Writing desk"));
        QVERIFY(!library.renameLocation(url, "   "));
        QVERIFY(!library.renameLocation(document, "Wrong target"));
        QVERIFY(library.copyPath(document));
        QCOMPARE(QGuiApplication::clipboard()->text(), document.toLocalFile());
        QVERIFY(!library.copyPath(QUrl("https://example.com")));
        library.removeLocation(url);
        QVERIFY(QFileInfo::exists(document.toLocalFile()));
        library.setRootFolder(url);
        QVERIFY(labelFor(library) != "Writing desk");
    }

    void restoredWindowGeometryStaysOnAvailableScreen() {
        const QRect screen(1440,0,1920,1080);
        QCOMPARE(WorkspaceStore::visibleGeometry(QRect(-8000,-4000,1100,720),screen,QSize(720,520)),QRect(1440,0,1100,720));
        QCOMPARE(WorkspaceStore::visibleGeometry(QRect(4000,3000,4000,2000),screen,QSize(720,520)),screen);
        QCOMPARE(WorkspaceStore::visibleGeometry(QRect(1500,100,800,600),screen,QSize(720,520)),QRect(1500,100,800,600));
    }

    void tagIndexReportsBoundsAndSkipsOversizedFiles() {
        QTemporaryDir dir;
        QFile tags(dir.filePath("tags.md")); QVERIFY(tags.open(QIODevice::WriteOnly));
        for(int i=0;i<2001;++i) tags.write(("#tag"+QString::number(i)+" ").toUtf8()); tags.close();
        FileLibrary library; library.setRootFolder(QUrl::fromLocalFile(dir.path()));
        QElapsedTimer elapsed; elapsed.start(); library.refreshTags();
        QTRY_VERIFY_WITH_TIMEOUT(!library.tagStatus().startsWith("Scanning"),10000);
        QCOMPARE(library.tagIndex().size(),2000); QVERIFY(library.tagStatus().contains("limit reached"));
        qInfo() << "Bounded 2001-tag scan milliseconds:" << elapsed.elapsed();
        QVERIFY(tags.remove()); QFile large(dir.filePath("large.md")); QVERIFY(large.open(QIODevice::WriteOnly)); large.write(QByteArray(262145,'x')); large.close();
        library.refreshTags(); QTRY_VERIFY_WITH_TIMEOUT(!library.tagStatus().startsWith("Scanning"),10000);
        QVERIFY(library.tagIndex().isEmpty()); QVERIFY(library.tagStatus().contains("1 skipped"));
    }

    void extendedPaletteRoutesCommandsAndProtectsDisabledActions() {
        Backend backend; QQmlEngine engine; engine.rootContext()->setContextProperty("backend",&backend);
        QQmlComponent component(&engine,QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window,qPrintable(component.errorString()));
        auto *commands=window->findChild<QObject *>("workspaceCommands"); auto *editor=window->findChild<QObject *>("sourceEditor"); QVERIFY(commands && editor);
        QSignalSpy requested(commands,SIGNAL(commandRequested(QString)));
        QVERIFY(QMetaObject::invokeMethod(commands,"run",Q_ARG(QVariant,"rename"))); QCOMPARE(requested.count(),0);
        editor->setProperty("text","sample");
        QVERIFY(QMetaObject::invokeMethod(commands,"run",Q_ARG(QVariant,"themePaper"))); QCOMPARE(backend.themePreset(),QString("paper"));
        QVERIFY(QMetaObject::invokeMethod(commands,"run",Q_ARG(QVariant,"pageBreak"))); QVERIFY(editor->property("text").toString().contains("<!-- pagebreak -->"));
        QVERIFY(QMetaObject::invokeMethod(editor,"undo")); QCOMPARE(editor->property("text").toString(),QString("sample"));
        backend.setThemePreset("system"); backend.discardRecovery();
    }

    void writingReviewExcludesCodeAndCorrectsSafely() {
        const QString source="Very readable. `really`\n> ```\n> quite\n> ```\n    just\n[visible](https://example.com/very)\n";
        const auto prose=Backend::proseForReview(source);
        QCOMPARE(prose.size(),source.size()); QVERIFY(prose.startsWith("Very readable."));
        QVERIFY(!prose.contains("really")); QVERIFY(!prose.contains("quite")); QVERIFY(!prose.contains("just")); QVERIFY(!prose.contains("https"));
        Backend backend;
        const auto analysis=backend.writingAnalysis(source, "");
        int reviewCount=0; for(const auto &entry:analysis) if(entry.toMap()["label"]=="Review word") ++reviewCount;
        QCOMPARE(reviewCount,1);
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine,QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window,qPrintable(component.errorString()));
        auto *editor=window->findChild<QObject *>("sourceEditor"); QVERIFY(editor); editor->setProperty("text","A mispellled word.");
        QVERIFY(!backend.correctWriting(2,12,"stale","misspelled"));
        QVERIFY(backend.correctWriting(2,12,"mispellled","misspelled"));
        QCOMPARE(editor->property("text").toString(),QString("A misspelled word."));
        QVERIFY(QMetaObject::invokeMethod(editor,"undo")); QCOMPARE(editor->property("text").toString(),QString("A mispellled word."));
#ifdef Q_OS_MACOS
        QVERIFY(!backend.writingLanguages().isEmpty());
        const auto issues=backend.writingIssues("A mispellled word. `mispellled`", "en_US",false);
        bool found=false; for(const auto &entry:issues) { const auto issue=entry.toMap(); if(issue["word"]=="mispellled") { found=true; QCOMPARE(issue["start"].toInt(),2); QVERIFY(!issue["suggestions"].toStringList().isEmpty()); } }
        QVERIFY(found);
#endif
        backend.discardRecovery();
    }

    void customReviewSpansAreUnicodeSafeBoundedAndReadOnly() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        QVERIFY(editor);

        const QString source = QStringLiteral(
            "😀 Café caféine café. C++\n"
            "`café`\n"
            "```\ncafé C++\n```\n"
            "[visible](https://example.com/café)\n");
        editor->setProperty("text", source);
        const bool undoBefore = editor->property("canUndo").toBool();
        const QVariantList spans = backend.customReviewSpans(QStringLiteral(" café, C++, CAFÉ "));
        QCOMPARE(spans.size(), 3);
        QCOMPARE(spans.at(0).toMap().value("start").toInt(), 3);
        QCOMPARE(spans.at(0).toMap().value("word").toString(), QStringLiteral("Café"));
        QCOMPARE(spans.at(1).toMap().value("word").toString(), QStringLiteral("café"));
        QCOMPARE(spans.at(2).toMap().value("word").toString(), QStringLiteral("C++"));
        for (const QVariant &entry : spans) {
            const QVariantMap span = entry.toMap();
            QCOMPARE(span.value("label").toString(), QStringLiteral("Custom"));
            QCOMPARE(source.mid(span.value("start").toInt(),
                                span.value("end").toInt() - span.value("start").toInt()),
                     span.value("word").toString());
        }
        QCOMPARE(editor->property("text").toString(), source);
        QCOMPARE(editor->property("canUndo").toBool(), undoBefore);

        QString repeated;
        repeated.reserve(5500);
        for (int i = 0; i < 1100; ++i) repeated += QStringLiteral("word ");
        editor->setProperty("text", repeated);
        const QVariantList capped = backend.customReviewSpans(QStringLiteral("word"));
        QCOMPARE(capped.size(), 1000);
        QCOMPARE(capped.last().toMap().value("start").toInt(), 4995);

        editor->setProperty("text", QStringLiteral("very good"));
        const QVariantList overlapping = backend.customReviewSpans(
            QStringLiteral("very, very good, good"));
        QCOMPARE(overlapping.size(), 1);
        QCOMPARE(overlapping.first().toMap().value("start").toInt(), 0);
        QCOMPARE(overlapping.first().toMap().value("end").toInt(), 9);
        QCOMPARE(overlapping.first().toMap().value("word").toString(),
                 QStringLiteral("very good"));

        QStringList terms;
        for (int i = 0; i < 32; ++i) terms.append(QStringLiteral("unused%1").arg(i));
        terms.append(QStringLiteral("target"));
        editor->setProperty("text", QStringLiteral("target"));
        QVERIFY(backend.customReviewSpans(terms.join(QLatin1Char(','))).isEmpty());

        const QString tooLong(65, QLatin1Char('z'));
        editor->setProperty("text", tooLong);
        QVERIFY(backend.customReviewSpans(tooLong).isEmpty());

        editor->setProperty("text", QString(50000, QLatin1Char('x')) + QStringLiteral(" target"));
        QVERIFY(backend.customReviewSpans(QStringLiteral("target")).isEmpty());
        backend.discardRecovery();
    }

    void customReviewOverlayComposesWithMarkdownFocusAndSearch() {
        const QString source = QStringLiteral("**custom** [custom](url) `custom`\nplain custom");
        QTextDocument document;
        document.setPlainText(source);
        MarkdownHighlighter highlighter(&document);
        highlighter.setShowMarkup(false);
        const int boldStart = source.indexOf(QStringLiteral("custom"));
        const int linkStart = source.indexOf(QStringLiteral("custom"), boldStart + 1);
        const int codeStart = source.indexOf(QStringLiteral("custom"), linkStart + 1);
        const int plainStart = source.lastIndexOf(QStringLiteral("custom"));
        highlighter.setFocusRange(plainStart, plainStart + 6);
        highlighter.setReviewSpans({{0, 2}, {boldStart, 6}, {linkStart, 6},
                                    {codeStart, 6}, {plainStart, 6}});
        highlighter.rehighlight();

        const auto formatAt = [&](int position) {
            const QTextBlock block = document.findBlock(position);
            const int local = position - block.position();
            QTextCharFormat result;
            for (const auto &range : block.layout()->formats())
                if (local >= range.start && local < range.start + range.length) result = range.format;
            return result;
        };
        const QTextCharFormat marker = formatAt(0);
        const QTextCharFormat bold = formatAt(boldStart);
        const QTextCharFormat link = formatAt(linkStart);
        const QTextCharFormat code = formatAt(codeStart);
        const QTextCharFormat plain = formatAt(plainStart);
        QCOMPARE(marker.fontPointSize(), qreal(1));
        QCOMPARE(bold.fontWeight(), int(QFont::Bold));
        QVERIFY(link.fontUnderline());
        QVERIFY(bold.background().style() != Qt::NoBrush);
        QCOMPARE(link.background(), bold.background());
        QCOMPARE(plain.background(), bold.background());
        QVERIFY(code.background() != bold.background());
        QVERIFY(bold.foreground() != plain.foreground());

        const QBrush reviewBackground = plain.background();
        highlighter.setSearch(QStringLiteral("custom"), plainStart);
        highlighter.rehighlight();
        const QTextCharFormat searchedBold = formatAt(boldStart);
        const QTextCharFormat searchedLink = formatAt(linkStart);
        const QTextCharFormat currentSearch = formatAt(plainStart);
        QCOMPARE(searchedBold.fontWeight(), int(QFont::Bold));
        QVERIFY(searchedLink.fontUnderline());
        QVERIFY(searchedBold.background() != reviewBackground);
        QVERIFY(currentSearch.background() != reviewBackground);
        QVERIFY(currentSearch.background() != searchedBold.background());
        QCOMPARE(document.toPlainText(), source);
        QVERIFY(!document.isUndoAvailable());
    }

    void themesPersistWithoutEditingDocuments() {
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor"); QVERIFY(editor);
        editor->setProperty("text", "theme-independent Markdown");
        for (const auto &preset : {"light", "dark", "paper"}) {
            auto *choice = window->findChild<QObject *>(QString("theme%1").arg(preset == QString("paper") ? "Paper" : preset == QString("dark") ? "Dark" : "Light"));
            QVERIFY(choice); QVERIFY(QMetaObject::invokeMethod(choice, "triggered"));
            QCOMPARE(backend.themePreset(), QString(preset));
            backend.setDarkMode(true); // manual presets override system changes
            QCOMPARE(backend.darkMode(), preset == QString("dark"));
            Backend reopened; QCOMPARE(reopened.themePreset(), QString(preset));
            QCOMPARE(reopened.themeBackground(), backend.themeBackground());
            QCOMPARE(editor->property("text").toString(), QString("theme-independent Markdown"));
            QVERIFY(backend.modified());
            auto luminance = [](QColor c) {
                auto linear=[](double v) { return v <= 0.04045 ? v/12.92 : std::pow((v+0.055)/1.055, 2.4); };
                return .2126*linear(c.redF())+.7152*linear(c.greenF())+.0722*linear(c.blueF());
            };
            const auto palette = backend.palette();
            for (auto role : {"text", "muted"}) {
                const double a=luminance(QColor(palette[role].toString())), b=luminance(QColor(palette["panel"].toString()));
                QVERIFY((qMax(a,b)+.05)/(qMin(a,b)+.05) >= 4.5);
            }
        }
        backend.setThemePreset("invalid"); QCOMPARE(backend.themePreset(), QString("paper"));
        backend.setThemePreset("system"); backend.setDarkMode(false); QVERIFY(!backend.darkMode());
        backend.setDarkMode(true); QVERIFY(backend.darkMode());
        backend.discardRecovery();
    }

    void workspacePersistsAndRejectsCorruption() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const auto path = directory.filePath("workspace.json");
        const QJsonArray windows{QJsonObject{{"url", "file:///tmp/one.md"}, {"cursor", 17}, {"group", "1"}, {"order", 0}},
                                 QJsonObject{{"url", "file:///tmp/two.md"}, {"cursor", 8}, {"group", "1"}, {"order", 1}}};
        QVERIFY(WorkspaceStore::write(path, windows));
        QCOMPARE(WorkspaceStore::read(path), windows);
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("{broken"); file.close();
        QVERIFY(WorkspaceStore::read(path).isEmpty());
        QVERIFY(WorkspaceStore::write(path, windows));
        QJsonArray excessive; for (int i=0; i<101; ++i) excessive.append(QJsonObject{});
        QVERIFY(!WorkspaceStore::write(path, excessive));
        QCOMPARE(WorkspaceStore::read(path), windows); // rejected writes preserve prior state
    }

    void secondInstanceForwardsWithoutTakingOwnership() {
        QTemporaryDir directory("/tmp/ow-XXXXXX"); QVERIFY(directory.isValid());
        InstanceBroker owner;
        QCOMPARE(owner.start(directory.path(), "test", {}), InstanceBroker::Owner);
        QSignalSpy requests(&owner, &InstanceBroker::requested);
        auto future = QtConcurrent::run([path=directory.path()] {
            InstanceBroker client;
            return client.start(path, "test", {"/tmp/a file.md", "/tmp/東京.md"});
        });
        QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 15000);
        QCOMPARE(future.result(), InstanceBroker::Forwarded);
        QCOMPARE(requests.size(), 1);
        QCOMPARE(requests[0][0].toStringList(), QStringList({"/tmp/a file.md", "/tmp/東京.md"}));
        auto activate = QtConcurrent::run([path=directory.path()] {
            InstanceBroker client; return client.start(path, "test", {});
        });
        QTRY_VERIFY_WITH_TIMEOUT(activate.isFinished(), 15000);
        QCOMPARE(activate.result(), InstanceBroker::Forwarded);
        QCOMPARE(requests.size(), 2);
        QVERIFY(requests[1][0].toStringList().isEmpty());
    }

    void libraryFilterStaysInsideNarrowPane() {
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *settings = window->findChild<QObject *>("workspaceSettings");
        settings->setProperty("libraryVisible", true);
        settings->setProperty("organizerVisible", false);
        auto *pane = window->findChild<QQuickItem *>("libraryPane");
        auto *field = window->findChild<QQuickItem *>("libraryFilter");
        QVERIFY(pane); QVERIFY(field);
        pane->setProperty("showFilterBar", true);
        for (int width : {1100, 900, 720}) {
            window->setProperty("width", width);
            QTest::qWait(50);
            field->forceActiveFocus();
            const QPointF position = field->mapToItem(pane, QPointF());
            QVERIFY(position.x() >= 0);
            QVERIFY2(position.x() + field->width() <= pane->width(), "Filter extends past library edge");
            QVERIFY(field->width() > 0);
        }
        // Long filter text must scroll within the input, not enlarge the pill.
        field->setProperty("text", QString(300, 'x'));
        QTest::qWait(50);
        QVERIFY(field->mapToItem(pane, QPointF()).x() + field->width() <= pane->width());
        backend.discardRecovery();
    }

    void resolvesLocalFileAndFolderPaths() {
        QTemporaryDir directory;
        const QString path = directory.filePath("a # café.md");
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("sample"); file.close();
        Backend backend;
        const auto expected = QUrl::fromLocalFile(QFileInfo(path).canonicalFilePath());
        for (const QString &input : QStringList{path, "  "+path+"  ", "\""+path+"\"", "'"+path+"'", expected.toString(QUrl::FullyEncoded)}) {
            const auto result = backend.resolveOpenPath(input);
            QVERIFY2(!result.contains("error"), qPrintable(result["error"].toString()));
            QCOMPARE(result["url"].toUrl(), expected);
            QVERIFY(!result["folder"].toBool());
        }
        QVERIFY(backend.resolveOpenPath(directory.path())["folder"].toBool());
        QCOMPARE(backend.resolveOpenPath("~")["url"].toUrl(), QUrl::fromLocalFile(QFileInfo(QDir::homePath()).canonicalFilePath()));
        for (const QString &input : QStringList{"", "relative.md", "https://example.com/a.md", "file://server/a.md", "file:///tmp/a.md#fragment", directory.filePath("missing.md"), path+"\nother"})
            QVERIFY2(backend.resolveOpenPath(input).contains("error"), qPrintable(input));
        QFile binary(directory.filePath("sample.png")); QVERIFY(binary.open(QIODevice::WriteOnly)); binary.write("binary"); binary.close();
        QVERIFY(backend.resolveOpenPath(binary.fileName()).contains("error"));
        const QString link = directory.filePath("alias.md"); QVERIFY(QFile::link(path, link));
        QCOMPARE(backend.resolveOpenPath(link)["url"].toUrl(), expected);
    }

    void openByPathProtectsDirtyDocumentAndOpensFolder() {
        QTemporaryDir directory;
        QFile file(directory.filePath("sample.md")); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("file contents"); file.close();
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *dialog = window->findChild<QObject *>("openPathDialog");
        auto *input = window->findChild<QObject *>("openPathInput");
        QVERIFY(dialog); QVERIFY(input);
        editor->setProperty("text", "unsaved draft");
        input->setProperty("text", directory.path());
        QVERIFY(QMetaObject::invokeMethod(dialog, "submit"));
        QCOMPARE(backend.library()->property("rootFolder").toUrl(), QUrl::fromLocalFile(QFileInfo(directory.path()).canonicalFilePath()));
        QCOMPARE(editor->property("text").toString(), QString("unsaved draft")); QVERIFY(backend.modified());
        input->setProperty("text", directory.filePath("missing.md"));
        QVERIFY(QMetaObject::invokeMethod(dialog, "submit"));
        QVERIFY(!dialog->property("errorText").toString().isEmpty());
        input->setProperty("text", file.fileName());
        QVERIFY(QMetaObject::invokeMethod(dialog, "submit"));
        QCOMPARE(window->property("pendingAction").toString(), QString("open"));
        QCOMPARE(editor->property("text").toString(), QString("unsaved draft"));
        auto *prompt = window->findChild<QObject *>("unsavedChangesPrompt"); QVERIFY(prompt);
        QVERIFY(QMetaObject::invokeMethod(prompt, "cancelRequested"));
        QCOMPARE(editor->property("text").toString(), QString("unsaved draft"));
        QVERIFY(window->property("pendingAction").toString().isEmpty());
        QVERIFY(QMetaObject::invokeMethod(dialog, "submit"));
        QVERIFY(QMetaObject::invokeMethod(prompt, "discardRequested"));
        QCOMPARE(editor->property("text").toString(), QString("file contents"));
        QVERIFY(!backend.modified()); backend.discardRecovery();
    }

    void recoverySnapshotsSurviveProcessExit() {
        const bool child = qEnvironmentVariableIsSet("OMAWRITE_CRASH_FIXTURE");
        QTemporaryDir directory;
        const QString sampleRoot = child ? qEnvironmentVariable("OMAWRITE_CRASH_FIXTURE") : directory.path();
        const QString runId = child ? qEnvironmentVariable("OMAWRITE_CRASH_ID") : "Recovery-" + QUuid::createUuid().toString(QUuid::Id128);
        const auto oldName = QCoreApplication::applicationName();
        QCoreApplication::setApplicationName(runId);
        const auto restoreName = qScopeGuard([&] { QCoreApplication::setApplicationName(oldName); });
        if (!child) {
            for (int i=0; i<2; ++i) { QFile file(sampleRoot + QString("/%1.md").arg(i)); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("original"); }
            QProcess process; auto env=QProcessEnvironment::systemEnvironment();
            env.insert("OMAWRITE_CRASH_FIXTURE", sampleRoot); env.insert("OMAWRITE_CRASH_ID", runId);
            process.setProcessEnvironment(env);
            process.start(QCoreApplication::applicationFilePath(), {"recoverySnapshotsSurviveProcessExit"});
            QVERIFY(process.waitForFinished(15000)); QCOMPARE(process.exitCode(), 77);
            QFile external(sampleRoot + "/0.md"); QVERIFY(external.open(QIODevice::WriteOnly)); external.write("external writer"); external.close();
        }
        std::vector<std::unique_ptr<Backend>> backends;
        std::vector<std::unique_ptr<QQmlEngine>> engines;
        std::vector<std::unique_ptr<QObject>> windows;
        for (int i=0; i<2; ++i) {
            backends.push_back(std::make_unique<Backend>());
            auto *backend=backends.back().get();
            engines.push_back(std::make_unique<QQmlEngine>());
            auto *engine=engines.back().get(); engine->rootContext()->setContextProperty("backend", backend);
            QQmlComponent component(engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
            windows.emplace_back(component.create()); QVERIFY2(windows.back(), qPrintable(component.errorString()));
            auto *editor=windows.back()->findChild<QObject *>("sourceEditor"); QVERIFY(editor);
            if (child) {
                QVERIFY(backend->open(QUrl::fromLocalFile(sampleRoot + QString("/%1.md").arg(i))));
                editor->setProperty("text", QString("recovered draft %1").arg(i));
                backend->markAuthorship(0, 9, "Reference", "Synthetic source");
            } else {
                QVERIFY(backend->modified());
                const QString name=QFileInfo(backend->fileUrl().toLocalFile()).baseName();
                QCOMPARE(editor->property("text").toString(), "recovered draft " + name);
                QCOMPARE(backend->authorshipRanges().size(), 1);
                backend->autosave();
                QFile disk(backend->fileUrl().toLocalFile()); QVERIFY(disk.open(QIODevice::ReadOnly));
                if (name == "0") { QVERIFY(backend->modified()); QCOMPARE(disk.readAll(), QByteArray("external writer")); }
                else { QVERIFY(!backend->modified()); QCOMPARE(disk.readAll(), QByteArray("recovered draft 1")); }
                backend->discardRecovery();
            }
        }
        if (child) { QTest::qWait(1100); std::_Exit(77); } // real process exit without destructors or save prompts
    }

    void quitPreparationDoesNotCloseOrDiscard() {
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor=window->findChild<QObject *>("sourceEditor"); editor->setProperty("text", "keep this draft");
        QSignalSpy closed(&backend, &Backend::windowClosed), ready(&backend, &Backend::quitReady);
        QVERIFY(QMetaObject::invokeMethod(window.data(), "prepareQuit"));
        auto *prompt=window->findChild<QObject *>("unsavedChangesPrompt"); QVERIFY(prompt);
        QVERIFY(QMetaObject::invokeMethod(prompt, "discardRequested"));
        QCOMPARE(ready.size(), 1); QCOMPARE(closed.size(), 0);
        QVERIFY(backend.modified()); QCOMPARE(editor->property("text").toString(), QString("keep this draft"));
        const auto revision=backend.documentRevision();
        QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, 0), Q_ARG(QString, "new ")));
        QVERIFY(backend.documentRevision() != revision);
        backend.discardRecovery();
    }

    void tagIndexCountsDocumentsAndExcludesContainerCode() {
        QTemporaryDir directory;
        for(int i=0;i<110;++i) { QFile file(directory.filePath(QString::number(i)+".md")); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("#shared #shared\n> ```\n> #hidden\n> ```\n- ~~~\n#alsohidden\n  ~~~\n#visible"); }
        FileLibrary library; library.setRootFolder(QUrl::fromLocalFile(directory.path())); library.refreshTags();
        QTRY_VERIFY_WITH_TIMEOUT(!library.tagStatus().startsWith("Scanning"),10000);
        QMap<QString,int> counts; for(const auto &value:library.tagIndex()) counts[value.toMap()["tag"].toString()]=value.toMap()["count"].toInt();
        QCOMPARE(counts["shared"],110); QCOMPARE(counts["visible"],110); QVERIFY(!counts.contains("hidden")); QVERIFY(!counts.contains("alsohidden"));
        QTemporaryDir empty; library.setRootFolder(QUrl::fromLocalFile(empty.path())); QVERIFY(library.tagIndex().isEmpty()); library.refreshTags();
        QTRY_VERIFY_WITH_TIMEOUT(!library.tagStatus().startsWith("Scanning"),10000); QVERIFY(library.tagIndex().isEmpty());
    }

    void tagIndexAutomaticallyRefreshesSmallRoots() {
        QTemporaryDir directory;
        QFile file(directory.filePath("note.md"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("#first\n"), qint64(7));
        file.close();

        FileLibrary library;
        library.setRootFolder(QUrl::fromLocalFile(directory.path()));
        library.refreshTags();
        QTRY_VERIFY_WITH_TIMEOUT(!library.tagStatus().startsWith("Scanning"), 10000);
        QCOMPARE(library.tagIndex().size(), 1);
        QCOMPARE(library.tagIndex().first().toMap().value("tag").toString(), QString("first"));
        if (library.tagStatus().contains("automatic refresh unavailable"))
            QSKIP("QFileSystemWatcher registration is unavailable in this test environment");
        QVERIFY(library.tagStatus().contains("watching for changes"));

        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write("#second\n"), qint64(8));
        file.close();
        QTRY_VERIFY_WITH_TIMEOUT(library.tagStatus().startsWith("Saved-file changes detected")
                                 || library.tagStatus().startsWith("Scanning"), 3000);

        // A second write within the debounce window must be reflected by the one
        // eventual rescan rather than leaving the intermediate tag behind.
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write("#third\n"), qint64(7));
        file.close();
        QTRY_VERIFY_WITH_TIMEOUT(library.tagStatus().contains("watching for changes"), 10000);
        QVERIFY(library.tagStatus().contains("watching for changes"));
        QCOMPARE(library.tagIndex().size(), 1);
        QCOMPARE(library.tagIndex().first().toMap().value("tag").toString(), QString("third"));
    }

    void tagIndexRootSwitchRejectsStaleScanAndWatchEvents() {
        QTemporaryDir firstRoot;
        QTemporaryDir secondRoot;
        for (int i = 0; i < 180; ++i) {
            QFile file(firstRoot.filePath(QString::number(i) + ".md"));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("#oldroot\n");
        }
        QFile current(secondRoot.filePath("current.md"));
        QVERIFY(current.open(QIODevice::WriteOnly));
        current.write("#current\n");
        current.close();

        FileLibrary library;
        library.setRootFolder(QUrl::fromLocalFile(firstRoot.path()));
        library.refreshTags();
        library.setRootFolder(QUrl::fromLocalFile(secondRoot.path()));
        QVERIFY(library.tagIndex().isEmpty());
        QVERIFY(library.tagStatus().contains("not refreshed"));
        QTest::qWait(500);
        QVERIFY(library.tagIndex().isEmpty());
        QVERIFY(library.tagStatus().contains("not refreshed"));

        library.refreshTags();
        QTRY_VERIFY_WITH_TIMEOUT(!library.tagStatus().startsWith("Scanning"), 10000);
        QCOMPARE(library.tagIndex().first().toMap().value("tag").toString(), QString("current"));
        if (library.tagStatus().contains("automatic refresh unavailable")) return;
        QVERIFY(library.tagStatus().contains("watching for changes"));

        QFile old(firstRoot.filePath("0.md"));
        QVERIFY(old.open(QIODevice::WriteOnly | QIODevice::Truncate));
        old.write("#shouldnotappear\n");
        old.close();
        QTest::qWait(700);
        QCOMPARE(library.tagIndex().size(), 1);
        QCOMPARE(library.tagIndex().first().toMap().value("tag").toString(), QString("current"));
    }

    void tagIndexWatchBudgetFallsBackToManualRefresh() {
        QTemporaryDir directory;
        for (int i = 0; i < 260; ++i) {
            QFile file(directory.filePath(QString::number(i) + ".md"));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("#old\n");
        }
        FileLibrary library;
        library.setRootFolder(QUrl::fromLocalFile(directory.path()));
        library.refreshTags();
        QTRY_VERIFY_WITH_TIMEOUT(!library.tagStatus().startsWith("Scanning"), 10000);
        QVERIFY(library.tagStatus().contains("automatic refresh unavailable"));

        QFile changed(directory.filePath("0.md"));
        QVERIFY(changed.open(QIODevice::WriteOnly | QIODevice::Truncate));
        changed.write("#new\n");
        changed.close();
        QTest::qWait(700);
        QCOMPARE(library.tagIndex().size(), 1);
        QCOMPARE(library.tagIndex().first().toMap().value("tag").toString(), QString("old"));
        QCOMPARE(library.tagIndex().first().toMap().value("count").toInt(), 260);

        library.refreshTags();
        QTRY_VERIFY_WITH_TIMEOUT(!library.tagStatus().startsWith("Scanning"), 10000);
        QMap<QString, int> counts;
        for (const auto &value : library.tagIndex())
            counts[value.toMap().value("tag").toString()] = value.toMap().value("count").toInt();
        QCOMPARE(counts.value("old"), 259);
        QCOMPARE(counts.value("new"), 1);
        QVERIFY(library.tagStatus().contains("automatic refresh unavailable"));
    }

    void templatesSharePreviewAndExportWithoutEditingSource() {
        QTemporaryDir dir;
        Backend backend;
        const auto reset = qScopeGuard([&] { backend.setOutputStyle(0); backend.discardRecovery(); });
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *preview = window->findChild<QObject *>("renderedPreview");
        QVERIFY(editor); QVERIFY(preview);
        const QString source = "# Template specimen\n\nA paragraph with **bold**, *italic* and [a link](https://example.com).\n\n## Second heading\n\n> A quotation.\n\n- First item\n- Second item\n\n```cpp\nconst int answer = 42;\n```\n\n| Name | Value |\n| --- | --- |\n| Sample | 42 |\n";
        editor->setProperty("text", source);
        backend.saveAs(QUrl::fromLocalFile(dir.filePath("Source.md")));
        auto *quick = qvariant_cast<QQuickTextDocument *>(preview->property("textDocument")); QVERIFY(quick);
        const QString evidence = qEnvironmentVariable("OMAWRITE_TEMPLATE_EVIDENCE");
        if (!evidence.isEmpty()) QVERIFY(QDir().mkpath(evidence));
        for (int style : {0, 1, 2, 4, 5, 6, 7, 0}) {
            backend.setOutputStyle(style);
            QTRY_COMPARE(preview->property("font").value<QFont>().family(), backend.outputFont());
            QTRY_VERIFY(quick->textDocument()->toPlainText().contains("Template specimen"));
            QTRY_COMPARE(quick->textDocument()->begin().blockFormat().lineHeight(), qreal(style == 2 || style == 7 ? 200 : style == 4 ? 150 : 135));
            QCOMPARE(editor->property("text").toString(), source);
            QVERIFY(!backend.modified());
            const QString htmlPath = dir.filePath(QString::number(style) + ".html");
            QVERIFY(backend.exportDocument(QUrl::fromLocalFile(htmlPath), "html"));
            QFile html(htmlPath); QVERIFY(html.open(QIODevice::ReadOnly));
            const auto bytes = html.readAll();
            QVERIFY(bytes.contains(backend.outputFont().toUtf8()));
            QVERIFY(bytes.contains("<table"));
            QVERIFY(bytes.contains("answer"));
            if (style == 7) QVERIFY(bytes.contains("text-indent:36px"));
            if (!evidence.isEmpty()) {
                QFile::remove(evidence + "/template-" + QString::number(style) + ".html");
                QVERIFY(QFile::copy(htmlPath, evidence + "/template-" + QString::number(style) + ".html"));
                QVERIFY(backend.exportDocument(QUrl::fromLocalFile(evidence + "/template-" + QString::number(style) + ".pdf"), "pdf"));
            }
            Backend reopened; QCOMPARE(reopened.outputStyle(), style);
        }
    }

    void outputStylesPersistAndHtmlEmbedsLocalImages() {
        QTemporaryDir directory; Backend backend;
        const auto reset=qScopeGuard([&] { backend.setOutputStyle(0); });
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend",&backend);
        QQmlComponent component(&engine,QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window,qPrintable(component.errorString()));
        auto *editor=window->findChild<QObject *>("sourceEditor");
        QImage image(24,24,QImage::Format_RGB32); image.fill(Qt::blue); QVERIFY(image.save(directory.filePath("sample.png")));
        editor->setProperty("text","# First page\n\n![sample](sample.png)\n\n<!-- pagebreak -->\n\n# Second page\n\nFinal paragraph.");
        backend.saveAs(QUrl::fromLocalFile(directory.filePath("source.md")));
        backend.setOutputStyle(1); Backend reopened; QCOMPARE(reopened.outputStyle(),1);
        const auto html=QUrl::fromLocalFile(directory.filePath("output.html")); QVERIFY(backend.exportDocument(html,"html"));
        QFile file(html.toLocalFile()); QVERIFY(file.open(QIODevice::ReadOnly)); const auto bytes=file.readAll(); file.close();
        QVERIFY(bytes.contains("data:image/png;base64,")); QVERIFY(!bytes.contains("SENTINEL"));
        const auto pdf=QUrl::fromLocalFile(directory.filePath("output.pdf")); QVERIFY(backend.exportDocument(pdf,"pdf"));
        QFile pdfFile(pdf.toLocalFile()); QVERIFY(pdfFile.open(QIODevice::ReadOnly)); QVERIFY(pdfFile.readAll().contains("/Type /Page"));
        // Optional synthetic evidence location is explicitly set by the test runner.
        const auto evidence=qEnvironmentVariable("OMAWRITE_OUTPUT_EVIDENCE");
        if(!evidence.isEmpty()) { QDir().mkpath(evidence); QVERIFY(QFile::copy(pdf.toLocalFile(),evidence+"/pages.pdf")); QVERIFY(QFile::copy(html.toLocalFile(),evidence+"/portable.html")); }
        QFile style(directory.filePath("style.json")); QVERIFY(style.open(QIODevice::WriteOnly));
        style.write(R"({"fontFamily":"Georgia","pointSize":12,"header":"Sample header: {title}","footer":"Page {page} of {pages}","titlePage":true})"); style.close();
        QVERIFY(backend.loadOutputStyle(QUrl::fromLocalFile(style.fileName())));
        Backend customReopened; QCOMPARE(customReopened.outputStyle(),3);
        const auto customPdf=QUrl::fromLocalFile(directory.filePath("custom.pdf")); QVERIFY(backend.exportDocument(customPdf,"pdf"));
        if(!evidence.isEmpty()) QVERIFY(QFile::copy(customPdf.toLocalFile(),evidence+"/custom.pdf"));
        editor->setProperty("text","![missing](missing.png)"); QVERIFY(!backend.exportDocument(html,"html"));
        QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(),bytes); backend.discardRecovery();
    }

    void extendedContentBlocksRebaseLinksAndFootnotes() {
        QTemporaryDir directory; QDir(directory.path()).mkdir("nested");
        QFile included(directory.filePath("nested/chapter.md")); QVERIFY(included.open(QIODevice::WriteOnly)); included.write("[next](next.md#part) ![image](picture.png) `link [x](literal.md)`"); included.close();
        QFile csv(directory.filePath("table.csv")); QVERIFY(csv.open(QIODevice::WriteOnly)); csv.write("name,value\n\"a,b\",<tag>\n"); csv.close();
        QFile code(directory.filePath("sample.py")); QVERIFY(code.open(QIODevice::WriteOnly)); code.write("# literal [[wiki]]"); code.close();
        const auto output=expandedMarkdown("/nested/chapter.md\n/table.csv\n/sample.py\n\nText[^n] again[^n]\n[^n]: First\n    continued\n\n    next paragraph",QUrl::fromLocalFile(directory.path()+"/"));
        QVERIFY(output.contains("nested/next.md#part")); QVERIFY(output.contains("nested/picture.png")); QVERIFY(output.contains("`link [x](literal.md)`"));
        QVERIFY(output.contains("<td>a,b</td>")); QVERIFY(output.contains("&lt;tag&gt;")); QVERIFY(output.contains("```py\n# literal [[wiki]]"));
        QVERIFY(output.contains("continued\n\nnext paragraph")); QVERIFY(output.contains("-ref-2")); QVERIFY(output.contains("[↩2]"));
        const auto wiki=expandedMarkdown("[[next#part]]",QUrl::fromLocalFile(directory.path()+"/")); QVERIFY(wiki.contains("next.md#part"));
    }

    void clipboardAuthorshipRoundTripsAndRejectsStaleMetadata() {
        QTemporaryDir directory; Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine,QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window,qPrintable(component.errorString()));
        auto *editor=window->findChild<QObject *>("sourceEditor"); editor->setProperty("text","alpha beta");
        backend.markAuthorship(0,5,"Reference","Sample");
        QVERIFY(backend.copySelection(1,5,"markdown")); QCOMPARE(QGuiApplication::clipboard()->text(),QString("lpha"));
        QCOMPARE(backend.pasteWithAuthorship(10,10),14);
        QCOMPARE(editor->property("text").toString(),QString("alpha betalpha"));
        bool pasted=false; for (const auto &v : backend.authorshipRanges()) { const auto r=v.toMap(); if(r["start"].toInt()==10 && r["category"]=="Reference") pasted=true; } QVERIFY(pasted);
        QVERIFY(QMetaObject::invokeMethod(editor,"undo")); QCOMPARE(editor->property("text").toString(),QString("alpha beta"));
        auto *mime=new QMimeData; mime->setText("external"); mime->setData("application/x-omawrite-authorship+json",QGuiApplication::clipboard()->mimeData()->data("application/x-omawrite-authorship+json")); QGuiApplication::clipboard()->setMimeData(mime);
        backend.pasteWithAuthorship(0,5); QVERIFY(backend.authorshipRanges().isEmpty());
        QVERIFY(QMetaObject::invokeMethod(editor,"undo")); QVERIFY(!backend.authorshipRanges().isEmpty());
        const auto output=QUrl::fromLocalFile(directory.filePath("metadata.json")); QVERIFY(backend.exportAuthorship(output));
        QFile file(output.toLocalFile()); QVERIFY(file.open(QIODevice::ReadOnly)); const auto data=QJsonDocument::fromJson(file.readAll()).object(); QCOMPARE(data["ranges"].toArray().size(),1); QVERIFY(data["notice"].toString().contains("not verified"));
        backend.discardRecovery();
    }

    void automaticVersionsPreservePreviousSavedBytes() {
#ifdef Q_OS_MACOS
        QTemporaryDir directory; Backend backend;
        const auto reset=qScopeGuard([&] { backend.setAutomaticVersions(false); });
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor=window->findChild<QObject *>("sourceEditor");
        const auto url=QUrl::fromLocalFile(directory.filePath("history.md"));
        editor->setProperty("text", "before"); backend.saveAs(url);
        backend.setAutomaticVersions(true);
        editor->setProperty("text", "after"); backend.save(); QVERIFY(!backend.modified());
        bool found=false;
        for (const auto &item : backend.versions()) { QFile file(item.toMap()["url"].toUrl().toLocalFile()); if (file.open(QIODevice::ReadOnly) && file.readAll()=="before") found=true; }
        QVERIFY(found); const int count=backend.versions().size(); backend.save(); QCOMPARE(backend.versions().size(), count);
        backend.discardRecovery();
#endif
    }

    void unavailableSaveDestinationPreservesDraftAndOriginal() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor=window->findChild<QObject *>("sourceEditor");
        const auto original=QUrl::fromLocalFile(directory.filePath("original.md"));
        editor->setProperty("text", "saved original"); backend.saveAs(original);
        QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, 14), Q_ARG(QString, " draft")));
        QSignalSpy failed(&backend, &Backend::saveFailed);
        backend.saveAs(QUrl::fromLocalFile(directory.filePath("unavailable/new.md")));
        QCOMPARE(failed.size(), 1); QVERIFY(backend.modified()); QCOMPARE(backend.fileUrl(), original);
        QCOMPARE(editor->property("text").toString(), QString("saved original draft"));
        QFile file(original.toLocalFile()); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), QByteArray("saved original"));
        QVERIFY(QMetaObject::invokeMethod(editor, "undo")); QCOMPARE(editor->property("text").toString(), QString("saved original"));
        backend.discardRecovery();
    }

    void restoredVersionsRequireSaveAndDoNotInheritAuthorship() {
#ifdef Q_OS_MACOS
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const auto url=QUrl::fromLocalFile(directory.filePath("versioned.md"));
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor=window->findChild<QObject *>("sourceEditor");
        editor->setProperty("text", "historical text"); backend.saveAs(url); QVERIFY(!backend.modified());
        QVERIFY2(backend.createVersion(), qPrintable(backend.status()));
        const auto versions=backend.versions(); QVERIFY(!versions.isEmpty());
        editor->setProperty("text", "current labelled text");
        backend.markAuthorship(0, 7, "Human", "Synthetic author"); backend.save();
        const auto annotations=backend.authorshipRanges(); QVERIFY(!annotations.isEmpty());
        QVERIFY(backend.restoreVersion(versions.first().toMap()["url"].toUrl()));
        QCOMPARE(editor->property("text").toString(), QString("historical text"));
        QVERIFY(backend.authorshipRanges().isEmpty()); QVERIFY(backend.modified());
        backend.autosave(); QVERIFY(backend.modified());
        bool persistedPause=false;
        QDir recovery(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        for (const auto &name : recovery.entryList({"recovery-*.json"}, QDir::Files)) {
            QFile snapshot(recovery.filePath(name)); if (!snapshot.open(QIODevice::ReadOnly)) continue;
            const auto object=QJsonDocument::fromJson(snapshot.readAll()).object();
            if (object["fileUrl"].toString() == url.toString()) persistedPause=object["requiresExplicitSave"].toBool();
        }
        QVERIFY(persistedPause);
        QFile file(url.toLocalFile()); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), QByteArray("current labelled text")); file.close();
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), QString("current labelled text"));
        QCOMPARE(backend.authorshipRanges(), annotations);
        QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
        QVERIFY(backend.authorshipRanges().isEmpty());
        backend.save(); QVERIFY(!backend.modified());
        QVERIFY(backend.open(url)); QVERIFY(backend.authorshipRanges().isEmpty());
        backend.discardRecovery();
#endif
    }

    void autosaveRefusesExternalChanges() {
        QTemporaryDir directory;
        const auto url = QUrl::fromLocalFile(directory.filePath("sample.md"));
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        editor->setProperty("text", "initial"); backend.saveAs(url);
        QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, 7), Q_ARG(QString, " draft")));
        backend.autosave(); QVERIFY(!backend.modified());
        QFile file(url.toLocalFile()); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), QByteArray("initial draft")); file.close();
        QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, 13), Q_ARG(QString, " local")));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate)); file.write("external"); file.close();
        backend.autosave(); QVERIFY(backend.modified());
        QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), QByteArray("external"));
        QVERIFY(backend.status().contains("paused"));
        backend.discardRecovery();
    }

    void windowRequestsAndFailedSaveCancelQuit() {
        Backend backend;
        QSignalSpy requested(&backend, &Backend::newWindowRequested);
        backend.newWindow(); QCOMPARE(requested.size(), 1);
        QSignalSpy canceled(&backend, &Backend::quitCanceled);
        QSignalSpy failed(&backend, &Backend::saveFailed);
        backend.saveAs(QUrl("https://example.com/sample.md"));
        QCOMPARE(failed.size(), 1); QCOMPARE(canceled.size(), 1);
    }

    void authorshipSidecarsMatchSourceAndUndo() {
        QTemporaryDir directory;
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        editor->setProperty("text", "alpha beta");
        backend.markAuthorship(0, 5, "Human", "Sample author");
        QCOMPARE(backend.authorshipRanges().size(), 1);
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QVERIFY(backend.authorshipRanges().isEmpty());
        QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
        QCOMPARE(backend.authorshipRanges().size(), 1);
        const auto url = QUrl::fromLocalFile(directory.filePath("sample.md"));
        backend.saveAs(url); QVERIFY(!backend.modified());
        backend.newDocument(); QVERIFY(backend.open(url));
        QCOMPARE(backend.authorshipRanges().size(), 1);
        QFile file(url.toLocalFile()); QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate)); file.write("changed text"); file.close();
        QVERIFY(backend.open(url)); QVERIFY(backend.authorshipRanges().isEmpty());
        backend.discardRecovery();
    }

    void authorshipFollowsDuplicateRenameAndMove() {
        QTemporaryDir directory;
        QVERIFY(QDir(directory.path()).mkdir("moved"));
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        editor->setProperty("text", "alpha beta");
        backend.markAuthorship(0, 5, "Human", "Saved author");
        backend.saveAs(QUrl::fromLocalFile(directory.filePath("original.md")));
        QVERIFY(!backend.modified());
        backend.markAuthorship(6, 10, "Reference", "Unsaved source");
        const auto draftRanges = backend.authorshipRanges();
        QVERIFY(backend.duplicateDocument("copy.md"));
        QVERIFY(backend.modified());
        QCOMPARE(backend.authorshipRanges(), draftRanges);
        QVERIFY(backend.renameDocument("renamed.md"));
        QVERIFY(!QFileInfo::exists(directory.filePath(".original.md.omawrite-authors.json")));
        QVERIFY(backend.moveDocument(QUrl::fromLocalFile(directory.filePath("moved"))));
        QVERIFY(!QFileInfo::exists(directory.filePath(".renamed.md.omawrite-authors.json")));
        QCOMPARE(backend.authorshipRanges(), draftRanges);
        QVERIFY(backend.modified());
        // Moving a dirty document carries saved metadata with saved bytes.
        QFile sidecar(directory.filePath("moved/.renamed.md.omawrite-authors.json"));
        QVERIFY(sidecar.open(QIODevice::ReadOnly));
        QCOMPARE(QJsonDocument::fromJson(sidecar.readAll()).object()["ranges"].toArray().size(), 1);
        sidecar.close();
        backend.save(); QVERIFY(!backend.modified());
        backend.newDocument();
        QVERIFY(backend.open(QUrl::fromLocalFile(directory.filePath("moved/renamed.md"))));
        QCOMPARE(backend.authorshipRanges(), draftRanges);
        QVERIFY(backend.open(QUrl::fromLocalFile(directory.filePath("copy.md"))));
        QCOMPARE(backend.authorshipRanges(), draftRanges);
        backend.discardRecovery();
    }

    void renameDialogPreservesWindowAndDirtyAnnotations() {
        QTemporaryDir directory;
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        editor->setProperty("text", "alpha beta");
        backend.saveAs(QUrl::fromLocalFile(directory.filePath("original.md")));
        backend.markAuthorship(0, 5, "Human", "Draft author");
        const auto ranges = backend.authorshipRanges();
        auto *dialog = window->findChild<QObject *>("fileNameDialog"); QVERIFY(dialog);
        QSignalSpy closed(&backend, &Backend::windowClosed);
        QVERIFY(QMetaObject::invokeMethod(dialog, "showFor", Q_ARG(QVariant, QVariant(true))));
        window->findChild<QObject *>("fileNameInput")->setProperty("text", "renamed.md");
        QVERIFY(QMetaObject::invokeMethod(dialog, "submit"));
        QCoreApplication::processEvents();
        QCOMPARE(closed.size(), 0);
        QVERIFY(window->property("visible").toBool());
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QVERIFY(backend.modified());
        QCOMPARE(editor->property("text").toString(), QString("alpha beta"));
        QCOMPARE(backend.authorshipRanges(), ranges);
        QCOMPARE(backend.fileUrl(), QUrl::fromLocalFile(directory.filePath("renamed.md")));
        // Path operations synchronously update recovery with dirty annotations.
        bool recovered = false;
        QDir recovery(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        for (const auto &name : recovery.entryList({"recovery-*.json"}, QDir::Files)) {
            QFile file(recovery.filePath(name)); QVERIFY(file.open(QIODevice::ReadOnly));
            auto data = QJsonDocument::fromJson(file.readAll()).object();
            if (QUrl(data["fileUrl"].toString()) == backend.fileUrl()) {
                QCOMPARE(data["authorship"].toObject()["ranges"].toArray().size(), 1);
                recovered = true;
            }
        }
        QVERIFY(recovered);
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QVERIFY(backend.authorshipRanges().isEmpty());
        backend.discardRecovery();
    }

    void authorshipCollisionsLeaveOriginalIntact() {
        QTemporaryDir directory;
        Backend backend;
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        window->findChild<QObject *>("sourceEditor")->setProperty("text", "sample");
        backend.markAuthorship(0, 6, "Human", "Author");
        const auto original = QUrl::fromLocalFile(directory.filePath("sample.md"));
        backend.saveAs(original);
        QFile occupied(directory.filePath(".target.md.omawrite-authors.json"));
        QVERIFY(occupied.open(QIODevice::WriteOnly)); occupied.write("keep me"); occupied.close();
        QFile existing(directory.filePath("existing.md"));
        QVERIFY(existing.open(QIODevice::WriteOnly)); existing.write("untouched"); existing.close();
        QVERIFY(!backend.duplicateDocument("existing.md"));
        QVERIFY(!QFileInfo::exists(directory.filePath(".existing.md.omawrite-authors.json")));
        QVERIFY(!backend.renameDocument("target.md"));
        QVERIFY(!backend.duplicateDocument("target.md"));
        QCOMPARE(backend.fileUrl(), original);
        QVERIFY(!QFileInfo::exists(directory.filePath("target.md")));
        QVERIFY(occupied.open(QIODevice::ReadOnly)); QCOMPARE(occupied.readAll(), QByteArray("keep me"));
        QVERIFY(QDir(directory.path()).mkdir("destination"));
        QVERIFY(QFile::copy(occupied.fileName(), directory.filePath("destination/.sample.md.omawrite-authors.json")));
        QVERIFY(!backend.moveDocument(QUrl::fromLocalFile(directory.filePath("destination"))));
        QVERIFY(QFileInfo::exists(original.toLocalFile()));
        QVERIFY(!QFileInfo::exists(directory.filePath("destination/sample.md")));
        backend.discardRecovery();
    }

    void exportsPreserveSourceAndWriteHtmlPdf() {
        QTemporaryDir directory;
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        editor->setProperty("text", "# Export café\n\n**Bold** sample");
        const auto original = editor->property("text");
        QVERIFY(backend.exportDocument(QUrl::fromLocalFile(directory.filePath("sample.html")), "html"));
        QFile html(directory.filePath("sample.html")); QVERIFY(html.open(QIODevice::ReadOnly));
        QVERIFY(html.readAll().contains("Export caf"));
        QVERIFY(backend.exportDocument(QUrl::fromLocalFile(directory.filePath("sample.pdf")), "pdf"));
        QFile pdf(directory.filePath("sample.pdf")); QVERIFY(pdf.open(QIODevice::ReadOnly));
        QVERIFY(pdf.readAll().startsWith("%PDF"));
        const QString evidence = qEnvironmentVariable("OMAWRITE_EXPORT_EVIDENCE");
        if (!evidence.isEmpty()) {
            QVERIFY(backend.exportDocument(QUrl::fromLocalFile(evidence + "/sample.pdf"), "pdf"));
            QVERIFY(backend.exportDocument(QUrl::fromLocalFile(evidence + "/sample.html"), "html"));
        }
        QCOMPARE(editor->property("text"), original);
        QVERIFY(backend.modified());
        QVERIFY(!backend.exportDocument(QUrl("https://example.com/file.pdf"), "pdf"));
        backend.discardRecovery();
    }

    void markdownExtensionsPreserveCodeAndBoundIncludes() {
        QTemporaryDir directory;
        const QUrl base = QUrl::fromLocalFile(directory.path() + '/');
        const auto rendered = expandedMarkdown("[[note|Label]] ==bright== `==code==`\n[^n]\n[^n]: Footnote\n```\n[[literal]]\n```", base);
        QVERIFY(rendered.contains("Label"));
        QVERIFY(rendered.contains("note.md"));
        QVERIFY(rendered.contains("background-color"));
        QVERIFY(rendered.contains("`==code==`"));
        QVERIFY(rendered.contains("[[literal]]"));
        QVERIFY(rendered.contains("Footnote"));
        QFile file(directory.filePath("loop.md"));
        QVERIFY(file.open(QIODevice::WriteOnly)); file.write("/loop.md"); file.close();
        QVERIFY(expandedMarkdown("/loop.md", base).contains("Content block unavailable"));
        QVERIFY(expandedMarkdown("/../outside.md", base).contains("Content block unavailable"));
    }

    void tableOfContentsUsesUniqueAnchors() {
        Backend backend;
        const QString toc = backend.tableOfContents("# Hello **world**\n## Hello world\n```\n# hidden\n```\n# Café");
        QVERIFY(toc.contains("(#hello-world)"));
        QVERIFY(toc.contains("(#hello-world-1)"));
        QVERIFY(toc.contains("(#café)"));
        QVERIFY(!toc.contains("hidden"));
        QQmlEngine engine; engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create()); QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *preview = window->findChild<QObject *>("renderedPreview");
        editor->setProperty("text", "# Same\n\ntext\n\n# Same\n");
        auto *document = qvariant_cast<QQuickTextDocument *>(preview->property("textDocument"));
        QTRY_VERIFY(backend.previewAnchorPosition(document, "same-1") > 0);
        QCOMPARE(backend.previewAnchorPosition(document, "missing"), -1);
        editor->setProperty("text","Note.[^end] Again.[^end]\n\n"+QString("Filler paragraph.\n\n").repeated(100)+"[^end]: Destination.");
        QTRY_VERIFY(backend.previewAnchorPosition(document,"ow-note-0-end")>1000);
        const int first=backend.previewAnchorPosition(document,"ow-note-0-end-ref-1");
        const int second=backend.previewAnchorPosition(document,"ow-note-0-end-ref-2");
        QVERIFY(first>=0 && first<100); QVERIFY(second>first && second<100);
        backend.discardRecovery();
    }

    void tagsExcludeCodeAndSearchesPersist() {
        QCOMPARE(FileLibrary::tagsIn("# Heading\n#work #CAFÉ\n`#inline`\n```\n#hidden\n```\n    #indented"), QStringList({"work", "café"}));
        QTemporaryDir directory;
        FileLibrary library;
        library.setRootFolder(QUrl::fromLocalFile(directory.path()));
        library.saveSearch("#work", true);
        FileLibrary reopened;
        QCOMPARE(reopened.savedSearches().first().toMap()["query"].toString(), QString("#work"));
        reopened.removeSearch(0);
        QVERIFY(reopened.savedSearches().isEmpty());
    }

    void goMenuRoutesSavedQueriesHashtagsAndGuardedRecents() {
        QSettings settings;
        settings.beginGroup(QStringLiteral("library"));
        QVariantMap original;
        for (const QString &key : settings.allKeys()) original.insert(key, settings.value(key));
        settings.remove(QString());
        const auto restore = qScopeGuard([&] {
            settings.remove(QString());
            for (auto it = original.cbegin(); it != original.cend(); ++it)
                settings.setValue(it.key(), it.value());
        });

        QTemporaryDir directory;
        QVERIFY(QDir(directory.path()).mkpath(QStringLiteral("one")));
        QVERIFY(QDir(directory.path()).mkpath(QStringLiteral("two")));
        QVERIFY(QDir(directory.path()).mkpath(QStringLiteral("missing")));
        const QUrl rootOne = QUrl::fromLocalFile(QFileInfo(directory.filePath("one")).canonicalFilePath());
        const QUrl rootTwo = QUrl::fromLocalFile(QFileInfo(directory.filePath("two")).canonicalFilePath());
        const QUrl missingRoot = QUrl::fromLocalFile(QFileInfo(directory.filePath("missing")).canonicalFilePath());
        QFile tagged(directory.filePath("one/Tagged.md"));
        QVERIFY(tagged.open(QIODevice::WriteOnly));
        tagged.write("#work saved content");
        tagged.close();
        QFile available(directory.filePath("one/Available.md"));
        QVERIFY(available.open(QIODevice::WriteOnly)); available.write("available"); available.close();
        QFile unavailable(directory.filePath("one/Unavailable.md"));
        QVERIFY(unavailable.open(QIODevice::WriteOnly)); unavailable.write("gone"); unavailable.close();

        Backend backend;
        auto *library = qobject_cast<FileLibrary *>(backend.library());
        QVERIFY(library);
        library->setRootFolder(rootOne);
        library->saveSearch(QStringLiteral("#work"), true);
        library->recordRecentFile(QUrl::fromLocalFile(available.fileName()));
        library->recordRecentFile(QUrl::fromLocalFile(unavailable.fileName()));
        QVERIFY(unavailable.remove());

        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY(window);
        auto *dialog = window->findChild<QObject *>("quickOpenDialog");
        auto *query = window->findChild<QObject *>("quickQuery");
        auto *contents = window->findChild<QObject *>("quickContents");
        auto *savedChoice = window->findChild<QObject *>("savedSearchChoice");
        auto *saveSmartFolder = window->findChild<QObject *>("saveSmartFolder");
        auto *removeSmartFolder = window->findChild<QObject *>("removeSmartFolder");
        QVERIFY(dialog && query && contents && savedChoice && saveSmartFolder && removeSmartFolder);

        library->setRootFolder(rootTwo);
        auto *saved = window->findChild<QObject *>("goSavedSearch_0");
        QVERIFY(saved);
        QVERIFY(QMetaObject::invokeMethod(saved, "triggered"));
        QTRY_VERIFY(dialog->property("opened").toBool());
        QCOMPARE(library->rootFolder(), rootOne);
        QCOMPARE(query->property("text").toString(), QStringLiteral("#work"));
        QVERIFY(contents->property("checked").toBool());
        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));

        query->setProperty("text", QStringLiteral("stale"));
        contents->setProperty("checked", true);
        const int searchCount = library->savedSearches().size();
        auto *newSmartFolder = window->findChild<QObject *>("goNewSmartFolder");
        QVERIFY(newSmartFolder);
        QVERIFY(QMetaObject::invokeMethod(newSmartFolder, "triggered"));
        QTRY_VERIFY(dialog->property("opened").toBool());
        QVERIFY(dialog->property("creationMode").toBool());
        QCOMPARE(query->property("text").toString(), QString());
        QVERIFY(!contents->property("checked").toBool());
        QCOMPARE(library->savedSearches().size(), searchCount);
        query->setProperty("text", QStringLiteral("fresh query"));
        QVERIFY(QMetaObject::invokeMethod(saveSmartFolder, "clicked"));
        QTRY_COMPARE(library->savedSearches().size(), searchCount + 1);
        QTRY_COMPARE(window->findChild<QObject *>("goSavedSearch_0")->property("text").toString(),
                     QStringLiteral("fresh query — filenames"));
        savedChoice->setProperty("currentIndex", 0);
        QVERIFY(QMetaObject::invokeMethod(removeSmartFolder, "clicked"));
        QTRY_COMPARE(library->savedSearches().size(), searchCount);
        QTRY_COMPARE(window->findChild<QObject *>("goSavedSearch_0")->property("text").toString(),
                     QStringLiteral("#work — contents"));
        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));

        auto *refreshTags = window->findChild<QObject *>("goRefreshHashtags");
        QVERIFY(refreshTags && refreshTags->property("enabled").toBool());
        QVERIFY(QMetaObject::invokeMethod(refreshTags, "triggered"));
        QTRY_VERIFY_WITH_TIMEOUT(!library->tagStatus().startsWith(QStringLiteral("Scanning")), 10000);
        auto *hashtag = window->findChild<QObject *>("goHashtag_work");
        QVERIFY(hashtag);
        QVERIFY(QMetaObject::invokeMethod(hashtag, "triggered"));
        QTRY_VERIFY(dialog->property("opened").toBool());
        QCOMPARE(query->property("text").toString(), QStringLiteral("#work"));
        QVERIFY(contents->property("checked").toBool());
        QCOMPARE(library->rootFolder(), rootOne);
        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
        library->setRootFolder(rootTwo);
        QCOMPARE(library->tagIndex().size(), 0);
        QTRY_VERIFY(!window->findChild<QObject *>("goHashtag_work"));
        QVERIFY(window->findChild<QObject *>("goHashtagsEmpty")->property("visible").toBool());

        library->setRootFolder(missingRoot);
        library->saveSearch(QStringLiteral("missing root"), false);
        library->setRootFolder(rootTwo);
        QVERIFY(QDir(missingRoot.toLocalFile()).removeRecursively());
        QTRY_VERIFY(window->findChild<QObject *>("goSavedSearch_0"));
        QVERIFY(QMetaObject::invokeMethod(window->findChild<QObject *>("goSavedSearch_0"), "triggered"));
        QVERIFY(!dialog->property("opened").toBool());
        QCOMPARE(library->rootFolder(), rootTwo);
        QVERIFY(!library->error().isEmpty());

        const auto unavailableItems = window->findChildren<QObject *>(QStringLiteral("recentFile_0"));
        const auto availableItems = window->findChildren<QObject *>(QStringLiteral("recentFile_1"));
        QCOMPARE(unavailableItems.size(), 2); // File → Open Recent and Go → Smart Folders → Recents.
        QCOMPARE(availableItems.size(), 2);
        for (QObject *item : unavailableItems) QVERIFY(!item->property("enabled").toBool());
        for (QObject *item : availableItems) QVERIFY(item->property("enabled").toBool());
        QVERIFY(QMetaObject::invokeMethod(availableItems.first(), "triggered"));
        QCOMPARE(backend.fileUrl(), QUrl::fromLocalFile(QFileInfo(available.fileName()).canonicalFilePath()));
        backend.discardRecovery();
    }

    void contentSearchReflectsSavedChanges() {
        QTemporaryDir directory;
        QFile file(directory.filePath("sample.md"));
        QVERIFY(file.open(QIODevice::WriteOnly)); file.write("unique phrase"); file.close();
        FileLibrary library;
        library.setRootFolder(QUrl::fromLocalFile(directory.path()));
        library.quickSearch("unique", true);
        QTRY_COMPARE(library.quickResults().size(), 1);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate)); file.write("other text"); file.close();
        library.quickSearch("unique", true);
        QTRY_VERIFY(library.quickStatus().contains("matching files"));
        QVERIFY(library.quickResults().isEmpty());
        QVERIFY(file.rename(directory.filePath("renamed.md")));
        library.quickSearch("other", true);
        QTRY_COMPARE(library.quickResults().size(), 1);
        QCOMPARE(library.quickResults().first().toMap()["name"].toString(), QString("renamed.md"));
        QVERIFY(file.remove());
        library.quickSearch("other", true);
        QTRY_VERIFY(library.quickStatus().contains("matching files"));
        QVERIFY(library.quickResults().isEmpty());
    }

    void sentenceFocusBoundaries() {
        QCOMPARE(Backend::sentenceRange("First. Second!", 2), qMakePair(0, 7));
        QCOMPARE(Backend::sentenceRange("First. Second!", 10), qMakePair(7, 14));
        QCOMPARE(Backend::sentenceRange("First. Second!", 14), qMakePair(7, 14));
        QCOMPARE(Backend::sentenceRange("", 0), qMakePair(0, 0));
        QTextDocument document;
        document.setPlainText("First. Second!");
        MarkdownHighlighter highlighter(&document);
        highlighter.setFocusRange(7, 14);
        QVERIFY(!document.isUndoAvailable());
        QCOMPARE(document.toPlainText(), QString("First. Second!"));
    }

    void navigationHistoryAndQuickOpenStayIndependent() {
        QTemporaryDir directory;
        QVERIFY(QDir(directory.path()).mkpath("nested/deep"));
        QFile first(directory.filePath("First.md")), second(directory.filePath("nested/deep/Second.md"));
        QVERIFY(first.open(QIODevice::WriteOnly)); first.write("first document"); first.close();
        QVERIFY(second.open(QIODevice::WriteOnly)); second.write("second document"); second.close();
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        QVERIFY(backend.open(QUrl::fromLocalFile(first.fileName())));
        backend.rememberCursor(7);
        QVERIFY(backend.open(QUrl::fromLocalFile(second.fileName())));
        QVERIFY(backend.canGoBack());
        QCOMPARE(backend.navigateHistory(-1), 7);
        QCOMPARE(backend.fileUrl(), QUrl::fromLocalFile(first.fileName()));
        QVERIFY(backend.canGoForward());
        QCOMPARE(backend.navigateHistory(1), 0);
        QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, 0), Q_ARG(QString, "dirty ")));
        QVERIFY(QMetaObject::invokeMethod(window.data(), "requestHistory", Q_ARG(QVariant, -1)));
        QCOMPARE(window->property("pendingAction").toString(), QString("history"));
        QVERIFY(QMetaObject::invokeMethod(window->findChild<QObject *>("unsavedChangesPrompt"), "cancelRequested"));
        QCOMPARE(backend.fileUrl(), QUrl::fromLocalFile(second.fileName()));
        QVERIFY(backend.modified());
        QVERIFY(first.remove());
        QCOMPARE(backend.navigateHistory(-1), -1);
        QCOMPARE(backend.fileUrl(), QUrl::fromLocalFile(second.fileName()));
        QVERIFY(backend.modified());
        auto *library = qobject_cast<FileLibrary *>(backend.library());
        library->setRootFolder(QUrl::fromLocalFile(directory.path()));
        library->setRootFolder(QUrl::fromLocalFile(directory.filePath("nested")));
        QVERIFY(library->navigateHistory(-1));
        QCOMPARE(library->rootFolder(), QUrl::fromLocalFile(QFileInfo(directory.path()).canonicalFilePath()));
        QCOMPARE(backend.fileUrl(), QUrl::fromLocalFile(second.fileName()));
        library->quickSearch("Second");
        QTRY_COMPARE(library->quickResults().size(), 1);
        QCOMPARE(library->quickResults().first().toMap()["name"].toString(), QString("Second.md"));
        library->quickSearch("Second");
        library->quickSearch("missing");
        QTRY_VERIFY(library->quickStatus() != "Searching filenames…");
        QVERIFY(library->quickResults().isEmpty());
        editor->setProperty("text", "[local](First.md) [bad](javascript:alert)");
        QCOMPARE(backend.sourceLinkAt(2), backend.resolveDocumentLink("First.md"));
        QVERIFY(backend.sourceLinkAt(22).isEmpty());
        backend.discardRecovery();
    }

    void clipboardFormatsPreserveSelectionAndSource() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        editor->setProperty("text", "**bold** untouched");
        QVERIFY(backend.copySelection(0, 8, "formatted"));
        const auto *mime = QGuiApplication::clipboard()->mimeData();
        QVERIFY(mime->hasHtml());
        QCOMPARE(mime->text(), QString("bold"));
        QVERIFY(mime->html().contains("bold"));
        QVERIFY(backend.copySelection(0, 8, "markdown"));
        QCOMPARE(QGuiApplication::clipboard()->mimeData()->data("text/markdown"), QByteArray("**bold**"));
        QCOMPARE(backend.clipboardMarkdown(), QString("**bold**"));
        QVERIFY(backend.copySelection(0, 8, "html"));
        QVERIFY(QGuiApplication::clipboard()->text().contains("<html"));
        QCOMPARE(editor->property("text").toString(), QString("**bold** untouched"));
        auto *html = new QMimeData;
        html->setHtml("<p><strong>Hello</strong> <a href='https://example.com'>link</a></p>");
        QGuiApplication::clipboard()->setMimeData(html);
        const QString converted = backend.clipboardMarkdown();
        QVERIFY(converted.contains("**Hello**"));
        QVERIFY(converted.contains("[link](https://example.com)"));
        backend.replaceText(0, 8, converted);
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), QString("**bold** untouched"));
        QGuiApplication::clipboard()->setText("plain fallback");
        QCOMPARE(backend.clipboardMarkdown(), QString("plain fallback"));
        backend.discardRecovery();
    }

    void editingToolsPreserveProtectedTextAndUndo() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        editor->setProperty("text", QStringLiteral("straße café"));
        backend.editMarkdown("uppercase", 0, 11);
        QCOMPARE(editor->property("text").toString(), QStringLiteral("STRASSE CAFÉ"));
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), QStringLiteral("straße café"));
        editor->setProperty("text", "**~~words~~**");
        backend.editMarkdown("clearInline", 0, 13);
        QCOMPARE(editor->property("text").toString(), QString("words"));
        editor->setProperty("text", "[Label](https://example.com)");
        QVERIFY(backend.editMarkdown("uppercase", 0, 27).isEmpty());
        editor->setProperty("text", "");
        backend.editMarkdown("date", 0, 0);
        QCOMPARE(editor->property("text").toString(), QDate::currentDate().toString(Qt::ISODate));
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), QString());
        backend.editMarkdown("table", 0, 0);
        QVERIFY(editor->property("text").toString().contains("| --- | --- |"));
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), QString());
        backend.discardRecovery();
    }

    void currentDocumentCompletionsAreExplicitAndMarkdownAware() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *menuItem = window->findChild<QObject *>("native_showCompletions");
        auto *popup = window->findChild<QObject *>("completionPopup");
        QVERIFY(editor && menuItem && popup);

        const QString source = QStringLiteral("caféteria welcomes café.\nTry café");
        editor->setProperty("text", source);
        editor->setProperty("cursorPosition", source.size());
        const QVariantMap unicode = backend.wordCompletions(source.size());
        QCOMPARE(unicode.value("start").toInt(), source.size() - 4);
        QVERIFY(unicode.value("items").toStringList().contains(QStringLiteral("caféteria")));
        QVERIFY(menuItem->property("enabled").toBool());

        QVariant shown;
        QVERIFY(QMetaObject::invokeMethod(window.data(), "showCompletions",
                                          Q_RETURN_ARG(QVariant, shown)));
        QVERIFY(shown.toBool());
        QCOMPARE(editor->property("text").toString(), source);
        QTRY_VERIFY(popup->property("opened").toBool());
        const QStringList items = window->property("completionItems").toStringList();
        const int choice = items.indexOf(QStringLiteral("caféteria"));
        QVERIFY(choice >= 0);
        QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, source.size() - 4),
                                          Q_ARG(int, source.size())));
        QVariant accepted;
        QVERIFY(QMetaObject::invokeMethod(window.data(), "acceptCompletion",
                                          Q_RETURN_ARG(QVariant, accepted),
                                          Q_ARG(QVariant, choice)));
        QVERIFY(!accepted.toBool());
        QCOMPARE(editor->property("text").toString(), source);

        editor->setProperty("cursorPosition", source.size());
        QVERIFY(QMetaObject::invokeMethod(window.data(), "showCompletions",
                                          Q_RETURN_ARG(QVariant, shown)));
        QVERIFY(shown.toBool());
        QVERIFY(QMetaObject::invokeMethod(window.data(), "acceptCompletion",
                                          Q_RETURN_ARG(QVariant, accepted),
                                          Q_ARG(QVariant, choice)));
        QVERIFY(accepted.toBool());
        QCOMPARE(editor->property("text").toString(), QStringLiteral("caféteria welcomes café.\nTry caféteria"));
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), source);

        const QString protectedSource = QStringLiteral(
            "prose projection\n"
            "`prohibited pro`\n"
            "```\nprogramming pro\n```\n"
            "https://example.com/project\n"
            "[site](https://example.com/pro\n"
            "pro");
        editor->setProperty("text", protectedSource);
        const int inlineCode = protectedSource.indexOf(QStringLiteral("pro`")) + 3;
        QVERIFY(backend.wordCompletions(inlineCode).isEmpty());
        const int fencedCode = protectedSource.indexOf(QStringLiteral("pro\n```")) + 3;
        QVERIFY(backend.wordCompletions(fencedCode).isEmpty());
        const int bareUrl = protectedSource.indexOf(QStringLiteral("project")) + 7;
        QVERIFY(backend.wordCompletions(bareUrl).isEmpty());
        const int linkUrl = protectedSource.indexOf(QStringLiteral("/pro\n")) + 4;
        QVERIFY(backend.wordCompletions(linkUrl).isEmpty());
        const QVariantMap prose = backend.wordCompletions(protectedSource.size());
        const QStringList proseItems = prose.value("items").toStringList();
        QVERIFY(proseItems.contains(QStringLiteral("prose")));
        QVERIFY(proseItems.contains(QStringLiteral("projection")));
        QVERIFY(!proseItems.contains(QStringLiteral("prohibited")));
        QVERIFY(!proseItems.contains(QStringLiteral("programming")));
        QVERIFY(!proseItems.contains(QStringLiteral("project")));

        QString longDocument;
        longDocument.reserve(60000);
        for (int i = 0; i < 8000; ++i) longDocument += QStringLiteral("alpha ");
        for (int i = 0; i < 300; ++i) longDocument += QStringLiteral("completion ");
        longDocument += QStringLiteral("\nco");
        editor->setProperty("text", longDocument);
        QElapsedTimer completionTimer;
        completionTimer.start();
        const QVariantMap bounded = backend.wordCompletions(longDocument.size());
        QVERIFY(bounded.value("items").toStringList().contains(QStringLiteral("completion")));
        QVERIFY2(completionTimer.elapsed() < 2000, "Bounded completion lookup exceeded two seconds");
        backend.discardRecovery();
    }

    void inlineFormattingAndStructuralInsertionUndo() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        editor->setProperty("text", "alpha");
        auto selection = backend.wrapSelection(0, 5, "**", "**");
        QCOMPARE(editor->property("text").toString(), QString("**alpha**"));
        backend.wrapSelection(selection["start"].toInt(), selection["end"].toInt(), "**", "**");
        QCOMPARE(editor->property("text").toString(), QString("alpha"));
        editor->setProperty("text", "a`b");
        selection = backend.wrapSelection(0, 3, "`", "`");
        QCOMPARE(editor->property("text").toString(), QString("``a`b``"));
        backend.wrapSelection(selection["start"].toInt(), selection["end"].toInt(), "`", "`");
        QCOMPARE(editor->property("text").toString(), QString("a`b"));
        editor->setProperty("text", "before\n```\ninside");
        backend.editMarkdown("codeBlock", 0, 17);
        QVERIFY(editor->property("text").toString().startsWith("````\n"));
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), QString("before\n```\ninside"));
        backend.replaceText(0, 6, "[label](https://example.com)");
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), QString("before\n```\ninside"));
        backend.discardRecovery();
    }

    void blockFormattingPreservesLinesAndUndo() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        editor->setProperty("text", "");
        auto caret = backend.editMarkdown("heading2", 0, 0);
        QCOMPARE(editor->property("text").toString(), QString("## "));
        QCOMPARE(caret["start"].toInt(), 3);
        QCOMPARE(caret["end"].toInt(), 3);
        editor->setProperty("text", "alpha\nbeta\nlast");
        backend.editMarkdown("ordered", 10, 0);
        QCOMPARE(editor->property("text").toString(), QString("1. alpha\n2. beta\nlast"));
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), QString("alpha\nbeta\nlast"));
        backend.editMarkdown("heading2", 0, 0);
        QCOMPARE(editor->property("text").toString(), QString("## alpha\nbeta\nlast"));
        backend.editMarkdown("body", 0, 0);
        QCOMPARE(editor->property("text").toString(), QString("alpha\nbeta\nlast"));
        backend.editMarkdown("lineDown", 0, 0);
        QCOMPARE(editor->property("text").toString(), QString("beta\nalpha\nlast"));
        backend.editMarkdown("lineUp", 5, 5);
        QCOMPARE(editor->property("text").toString(), QString("alpha\nbeta\nlast"));
        editor->setProperty("text", "  - nested\n- [ ] task");
        backend.editMarkdown("ordered", 0, 10);
        QCOMPARE(editor->property("text").toString(), QString("  1. nested\n- [ ] task"));
        backend.editMarkdown("toggleTask", 12, 12);
        QCOMPARE(editor->property("text").toString(), QString("  1. nested\n- [x] task"));
        editor->setProperty("text", "```\nalpha\n```\n");
        QVERIFY(backend.editMarkdown("heading1", 4, 9).isEmpty());
        QCOMPARE(editor->property("text").toString(), QString("```\nalpha\n```\n"));
        backend.discardRecovery();
    }

    void orderedTaskListsPreserveStateAndUndo() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        QVERIFY(editor);

        const QString plain = QStringLiteral("first\nsecond");
        editor->setProperty("text", plain);
        backend.editMarkdown("orderedTask", 0, plain.size());
        QCOMPARE(editor->property("text").toString(), QStringLiteral("1. [ ] first\n2. [ ] second"));
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), plain);
        QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
        QCOMPARE(editor->property("text").toString(), QStringLiteral("1. [ ] first\n2. [ ] second"));

        const QString existing = QStringLiteral("  - [x] nested done\n- plain item");
        editor->setProperty("text", existing);
        backend.editMarkdown("orderedTask", 0, existing.size());
        QCOMPARE(editor->property("text").toString(), QStringLiteral("  1. [x] nested done\n2. [ ] plain item"));
        const int secondTask = editor->property("text").toString().indexOf(QStringLiteral("2. [ ]"));
        QVERIFY(secondTask >= 0);
        backend.editMarkdown("toggleTask", secondTask, secondTask);
        QCOMPARE(editor->property("text").toString(), QStringLiteral("  1. [x] nested done\n2. [x] plain item"));

        const QString fenced = QStringLiteral("before\n```\n- item\n```\nafter");
        editor->setProperty("text", fenced);
        QVERIFY(backend.editMarkdown("orderedTask", fenced.indexOf(QStringLiteral("- item")), fenced.indexOf(QStringLiteral("- item")) + 6).isEmpty());
        QCOMPARE(editor->property("text").toString(), fenced);
        backend.discardRecovery();
    }

    void clearStylesUsesAnExplicitSafeSelection() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        QVERIFY(editor);

        const QString styledHeading = QStringLiteral("## **Styled sample**");
        editor->setProperty("text", styledHeading);
        backend.editMarkdown("clearStyles", 0, styledHeading.size());
        QCOMPARE(editor->property("text").toString(), QStringLiteral("Styled sample"));
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), styledHeading);

        const QString styled = QStringLiteral("  ## Heading");
        editor->setProperty("text", styled);
        backend.editMarkdown("clearStyles", 0, styled.size());
        QCOMPARE(editor->property("text").toString(), QStringLiteral("  Heading"));
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), styled);
        QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
        QCOMPARE(editor->property("text").toString(), QStringLiteral("  Heading"));

        const QList<QPair<QString, QString>> blocks = {
            {QStringLiteral("> quoted"), QStringLiteral("quoted")},
            {QStringLiteral("- bullet"), QStringLiteral("bullet")},
            {QStringLiteral("- [x] task"), QStringLiteral("task")},
            {QStringLiteral("2. [ ] numbered task"), QStringLiteral("numbered task")}
        };
        for (const auto &block : blocks) {
            editor->setProperty("text", block.first);
            backend.editMarkdown("clearStyles", 0, block.first.size());
            QCOMPARE(editor->property("text").toString(), block.second);
        }
        const QList<QPair<QString, QString>> inlineStyles = {
            {QStringLiteral("**bold**"), QStringLiteral("bold")},
            {QStringLiteral("__strong__"), QStringLiteral("strong")},
            {QStringLiteral("*italic*"), QStringLiteral("italic")},
            {QStringLiteral("_emphasis_"), QStringLiteral("emphasis")},
            {QStringLiteral("~~strike~~"), QStringLiteral("strike")},
            {QStringLiteral("==highlight=="), QStringLiteral("highlight")}
        };
        for (const auto &inlineStyle : inlineStyles) {
            editor->setProperty("text", inlineStyle.first);
            backend.editMarkdown("clearStyles", 0, inlineStyle.first.size());
            QCOMPARE(editor->property("text").toString(), inlineStyle.second);
        }

        const QString code = QStringLiteral("**`code`**");
        editor->setProperty("text", code);
        QVERIFY(backend.editMarkdown("clearStyles", 0, code.size()).isEmpty());
        QCOMPARE(editor->property("text").toString(), code);
        const QString link = QStringLiteral("**[label](https://example.com)**");
        editor->setProperty("text", link);
        QVERIFY(backend.editMarkdown("clearStyles", 0, link.size()).isEmpty());
        QCOMPARE(editor->property("text").toString(), link);
        const QString stacked = QStringLiteral("**==mixed==**");
        editor->setProperty("text", stacked);
        backend.editMarkdown("clearStyles", 0, stacked.size());
        QCOMPARE(editor->property("text").toString(), QStringLiteral("mixed"));
        backend.discardRecovery();
    }

    void searchMenusWrapAndReplaceWithSingleUndo() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *query = window->findChild<QObject *>("searchField");
        auto *replacement = window->findChild<QObject *>("replaceField");
        QVERIFY(editor && query && replacement);
        const QString source = QStringLiteral("İ 😀 café CAFÉ café");
        editor->setProperty("text", source);
        const QVariantList matches = backend.searchPositions(QStringLiteral("café"));
        QCOMPARE(matches.size(), 3);
        QCOMPARE(matches.first().toInt(), source.indexOf(QStringLiteral("café")));
        QVERIFY(backend.searchPositions("").isEmpty());
        QVERIFY(backend.searchPositions("absent").isEmpty());
        auto trigger = [&](const char *name) {
            QObject *action = window->findChild<QObject *>(name);
            return action && QMetaObject::invokeMethod(action, "triggered");
        };
        QVERIFY(trigger("editReplace"));
        QVERIFY(window->property("searchOpen").toBool());
        QVERIFY(window->property("replaceOpen").toBool());
        query->setProperty("text", QStringLiteral("café"));
        QVERIFY(trigger("editFindPrevious"));
        QCOMPARE(editor->property("selectionStart").toInt(), matches.last().toInt());
        QVERIFY(trigger("editFindNext"));
        QCOMPARE(editor->property("selectionStart").toInt(), matches.first().toInt());
        replacement->setProperty("text", QStringLiteral("茶"));
        QVERIFY(QMetaObject::invokeMethod(window->findChild<QObject *>("replaceAllButton"), "clicked"));
        QCOMPARE(editor->property("text").toString(), QStringLiteral("İ 😀 茶 茶 茶"));
        QVERIFY(backend.modified());
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), source);
        QCOMPARE(backend.replaceMatches("absent", "x", -1), 0);
        QCOMPARE(backend.replaceMatches(QStringLiteral("café"), "", matches.at(1).toInt()), 1);
        QCOMPARE(editor->property("text").toString(), QStringLiteral("İ 😀 café  café"));
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), source);
        query->setProperty("text", "absent");
        QVERIFY(!window->findChild<QObject *>("editFindNext")->property("enabled").toBool());
        QVERIFY(QMetaObject::invokeMethod(query, "forceActiveFocus"));
        QVERIFY(QMetaObject::invokeMethod(query, "selectAll"));
        QVERIFY(trigger("editDelete"));
        QCOMPARE(query->property("text").toString(), QString());
        QCOMPARE(editor->property("text").toString(), source);
        QVERIFY(QMetaObject::invokeMethod(editor, "forceActiveFocus"));
        QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, matches.first().toInt()), Q_ARG(int, matches.first().toInt() + 4)));
        QVERIFY(trigger("editFindSelection"));
        QCOMPARE(query->property("text").toString(), QStringLiteral("café"));
        backend.discardRecovery();
    }

    void movePreservesStateAndRejectsUnsafeDestinations() {
        QTemporaryDir root;
        QVERIFY(QDir(root.path()).mkpath("destination"));
        const QUrl folder = QUrl::fromLocalFile(root.filePath("destination"));
        QFile original(root.filePath(QStringLiteral("Note 日本語.md")));
        QVERIFY(original.open(QIODevice::WriteOnly)); original.write("saved text"); original.close();
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        const QUrl oldUrl = QUrl::fromLocalFile(original.fileName());
        backend.open(oldUrl);
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *library = qobject_cast<FileLibrary *>(backend.library());
        QVERIFY(editor && library);
        library->toggleFavorite(oldUrl);
        QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, 10), Q_ARG(QString, QStringLiteral(" café"))));
        const QString dirty = editor->property("text").toString();
        QVERIFY(!backend.moveDocument(QUrl("https://example.com/")));
        QVERIFY(!backend.moveDocument(QUrl::fromLocalFile(root.filePath("missing"))));
        QVERIFY(backend.moveDocument(QUrl::fromLocalFile(root.path()))); // Same folder is a no-op.
        QCOMPARE(backend.fileUrl(), oldUrl);
        QFile destination(root.filePath(QStringLiteral("destination/Note 日本語.md")));
        QVERIFY(destination.open(QIODevice::WriteOnly)); destination.write("occupied"); destination.close();
        QVERIFY(!backend.moveDocument(folder));
        QVERIFY(destination.open(QIODevice::ReadOnly)); QCOMPARE(destination.readAll(), QByteArray("occupied")); destination.close();
        QVERIFY(destination.remove());
        // A dangling symlink is a collision, too.
        QVERIFY(QFile::link(root.filePath("missing-target"), destination.fileName()));
        QVERIFY(!backend.moveDocument(folder));
        QVERIFY(destination.remove());
        // Destination remains writable, but removing the original must fail.
        const auto permissions = QFile::permissions(root.path());
        QVERIFY(QFile::setPermissions(root.path(), QFileDevice::ReadOwner | QFileDevice::ExeOwner));
        const bool movedFromReadOnlyDirectory = backend.moveDocument(folder);
        QVERIFY(QFile::setPermissions(root.path(), permissions));
        QVERIFY(!movedFromReadOnlyDirectory);
        QVERIFY(original.exists());
        QVERIFY(destination.exists());
        QCOMPARE(backend.fileUrl(), oldUrl);
        QVERIFY(backend.modified());
        QVERIFY(backend.status().contains("both files remain"));
        QVERIFY(destination.remove());
        auto *moveDialog = window->findChild<QObject *>("moveFolderDialog");
        QVERIFY(moveDialog);
        QVERIFY(moveDialog->setProperty("currentFolder", QUrl::fromLocalFile(root.path())));
        QVERIFY(QMetaObject::invokeMethod(moveDialog, "open"));
        QTRY_VERIFY(moveDialog->property("visible").toBool());
        QVERIFY(moveDialog->setProperty("selectedFolder", folder));
        QTRY_COMPARE(moveDialog->property("selectedFolder").toUrl(), folder);
        QVERIFY(QMetaObject::invokeMethod(moveDialog, "accept"));
        QVERIFY2(!original.exists(), qPrintable(backend.status()));
        const QUrl moved = backend.fileUrl();
        QCOMPARE(moved, QUrl::fromLocalFile(QFileInfo(destination).canonicalFilePath()));
        QVERIFY(backend.modified());
        QCOMPARE(editor->property("text").toString(), dirty);
        QVERIFY(destination.open(QIODevice::ReadOnly)); QCOMPARE(destination.readAll(), QByteArray("saved text")); destination.close();
        QCOMPARE(library->favorites().last().toMap().value("url").toUrl(), moved);
        bool recentFound = false;
        for (const auto &entry : library->recentFiles()) {
            const QUrl url = entry.toMap().value("url").toUrl();
            QVERIFY(url != oldUrl);
            if (url == moved) recentFound = true;
        }
        QVERIFY(recentFound);
        bool snapshotFound = false;
        const QDir recovery(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        for (const QString &name : recovery.entryList({"recovery-*.json"}, QDir::Files)) {
            QFile snapshot(recovery.filePath(name)); QVERIFY(snapshot.open(QIODevice::ReadOnly));
            const auto data = QJsonDocument::fromJson(snapshot.readAll()).object();
            if (QUrl(data.value("fileUrl").toString()) == moved) {
                QCOMPARE(data.value("text").toString(), dirty); snapshotFound = true;
            }
        }
        QVERIFY(snapshotFound);
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), QStringLiteral("saved text"));
        QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
        QCOMPARE(editor->property("text").toString(), dirty);
        backend.save();
        QVERIFY(!backend.modified());
        QVERIFY(destination.open(QIODevice::ReadOnly)); QCOMPARE(destination.readAll(), dirty.toUtf8()); destination.close();
        QSignalSpy changed(&backend, &Backend::externalChangeDetected);
        QVERIFY(destination.open(QIODevice::WriteOnly | QIODevice::Truncate)); destination.write("external"); destination.close();
        QVERIFY(!backend.moveDocument(QUrl::fromLocalFile(root.path())));
        QCOMPARE(backend.fileUrl(), moved);
        QVERIFY(!original.exists());
        QTRY_COMPARE(changed.count(), 1);
        backend.discardRecovery();
    }

    void duplicateAndRenamePreserveDocumentState() {
        QTemporaryDir directory;
        QFile original(directory.filePath("Original.md"));
        QVERIFY(original.open(QIODevice::WriteOnly)); original.write("saved text"); original.close();
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        backend.open(QUrl::fromLocalFile(original.fileName()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *library = qobject_cast<FileLibrary *>(backend.library());
        QVERIFY(editor && library);
        library->toggleFavorite(backend.fileUrl());
        QVERIFY(QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, 10), Q_ARG(QString, QStringLiteral(" café 日本語"))));
        const QString dirty = editor->property("text").toString();
        QVERIFY(backend.modified());
        QVERIFY(backend.duplicateDocument("Copy.md"));
        QCOMPARE(backend.fileUrl(), QUrl::fromLocalFile(original.fileName()));
        QVERIFY(backend.modified());
        QFile copy(directory.filePath("Copy.md"));
        QVERIFY(copy.open(QIODevice::ReadOnly)); QCOMPARE(copy.readAll(), dirty.toUtf8()); copy.close();
        QVERIFY(!backend.duplicateDocument("Copy.md"));
        QVERIFY(!backend.duplicateDocument("../escape.md"));
        QVERIFY(!backend.renameDocument("Copy.md"));
        QVERIFY(!backend.renameDocument("../escape.md"));
        QVERIFY(backend.renameDocument("Renamed 日本語.md"));
        QVERIFY(!QFileInfo::exists(original.fileName()));
        QCOMPARE(editor->property("text").toString(), dirty);
        QVERIFY(backend.modified());
        const QUrl renamed = backend.fileUrl();
        bool recoveryUpdated = false;
        const QDir recoveryDirectory(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        for (const QString &name : recoveryDirectory.entryList({"recovery-*.json"}, QDir::Files)) {
            QFile snapshot(recoveryDirectory.filePath(name));
            QVERIFY(snapshot.open(QIODevice::ReadOnly));
            const auto data = QJsonDocument::fromJson(snapshot.readAll()).object();
            if (QUrl(data.value("fileUrl").toString()) == renamed) {
                QCOMPARE(data.value("text").toString(), dirty);
                recoveryUpdated = true;
            }
        }
        QVERIFY(recoveryUpdated);
        QFile disk(renamed.toLocalFile());
        QVERIFY(disk.open(QIODevice::ReadOnly)); QCOMPARE(disk.readAll(), QByteArray("saved text")); disk.close();
        QCOMPARE(library->favorites().last().toMap().value("url").toUrl(), QUrl::fromLocalFile(QFileInfo(disk).canonicalFilePath()));
        bool found = false;
        for (const auto &entry : library->recentFiles())
            if (entry.toMap().value("url").toUrl() == QUrl::fromLocalFile(QFileInfo(disk).canonicalFilePath())) found = true;
        QVERIFY(found);
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), QStringLiteral("saved text"));
        QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
        QCOMPARE(editor->property("text").toString(), dirty);
        backend.save();
        QVERIFY(!backend.modified());
        QVERIFY(disk.open(QIODevice::ReadOnly)); QCOMPARE(disk.readAll(), dirty.toUtf8()); disk.close();
        QSignalSpy externalChangeSpy(&backend, &Backend::externalChangeDetected);
        QVERIFY(disk.open(QIODevice::WriteOnly | QIODevice::Truncate)); disk.write("external edit"); disk.close();
        QVERIFY(!backend.renameDocument("Must-not-rename.md"));
        QCOMPARE(backend.fileUrl(), renamed);
        QVERIFY(QFileInfo::exists(renamed.toLocalFile()));
        QVERIFY(!QFileInfo::exists(directory.filePath("Must-not-rename.md")));
        QTRY_COMPARE(externalChangeSpy.count(), 1);
        backend.discardRecovery();
    }

    void explicitRevealClearsFilterAndFindsOutsideRoot() {
        QTemporaryDir first, second;
        QVERIFY(QDir(first.path()).mkpath("nested/deep"));
        QFile inside(first.filePath("nested/deep/Note.md"));
        QVERIFY(inside.open(QIODevice::WriteOnly)); inside.write("inside"); inside.close();
        QFile outside(second.filePath("Other.md"));
        QVERIFY(outside.open(QIODevice::WriteOnly)); outside.write("outside"); outside.close();
        FileLibrary library;
        library.setRootFolder(QUrl::fromLocalFile(first.path()));
        library.setFilter("does-not-match");
        int row = library.showFile(QUrl::fromLocalFile(inside.fileName()));
        QVERIFY(row >= 0);
        QVERIFY(library.filter().isEmpty());
        const auto entry = library.entries().at(row).toMap();
        QCOMPARE(entry.value("name").toString(), QStringLiteral("Note.md"));
        QVERIFY(entry.contains("modified"));
        QVERIFY(entry.contains("created"));
        row = library.showFile(QUrl::fromLocalFile(outside.fileName()));
        QVERIFY(row >= 0);
        QCOMPARE(library.rootFolder(), QUrl::fromLocalFile(QFileInfo(second.path()).canonicalFilePath()));
        QCOMPARE(library.showFile(QUrl::fromLocalFile(second.filePath("missing.md"))), -1);
        QVERIFY(!library.error().isEmpty());
        QCOMPARE(library.showFile(QUrl("https://example.com/note.md")), -1);
    }

    void libraryEntriesExposeCreatedAndModifiedDatesIndependently() {
        QTemporaryDir directory;
        QFile file(directory.filePath("dated.md"));
        QVERIFY(file.open(QIODevice::ReadWrite));
        QVERIFY(file.write("dated") > 0);
        const QDateTime oldModification(QDate(2001, 1, 2), QTime(12, 0));
        QVERIFY(file.setFileTime(oldModification, QFileDevice::FileModificationTime));
        file.close();

        const QFileInfo info(file.fileName());
        FileLibrary library;
        library.setRootFolder(QUrl::fromLocalFile(directory.path()));
        QCOMPARE(library.entries().size(), 1);
        const auto entry = library.entries().constFirst().toMap();
        QCOMPARE(entry.value("modified").toString(), info.lastModified().toString("d MMM"));
        QCOMPARE(entry.value("created").toString(), info.birthTime().isValid()
                     ? info.birthTime().toString("d MMM") : QStringLiteral("Unavailable"));
        // Some filesystems move birth time when an earlier modification time is set.
        // The live macOS fixture below exercises visibly distinct values.
    }

    void newAndCreateRespectUnsavedCancellation() {
        QTemporaryDir directory;
        QFile sample(directory.filePath("original.md"));
        QVERIFY(sample.open(QIODevice::WriteOnly)); sample.write("original"); sample.close();
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        backend.open(QUrl::fromLocalFile(sample.fileName()));
        backend.library()->setProperty("rootFolder", QUrl::fromLocalFile(directory.path()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        QVERIFY(editor);
        editor->setProperty("text", "unsaved text");
        QVERIFY(backend.modified());
        QVERIFY(QMetaObject::invokeMethod(window.data(), "requestCreateDocument",
                Q_ARG(QVariant, QStringLiteral("created.md")), Q_ARG(QVariant, false)));
        QCOMPARE(window->property("pendingAction").toString(), QStringLiteral("create"));
        QVERIFY(!QFileInfo::exists(directory.filePath("created.md")));
        // Cancel leaves the original buffer and filesystem intact.
        auto *prompt = window->findChild<QObject *>("unsavedChangesPrompt");
        QVERIFY(prompt);
        QVERIFY(QMetaObject::invokeMethod(prompt, "reject"));
        QVERIFY(window->property("pendingAction").toString().isEmpty());
        QCOMPARE(editor->property("text").toString(), QStringLiteral("unsaved text"));
        QVERIFY(!QFileInfo::exists(directory.filePath("created.md")));
        // Save successfully, then resume the requested creation.
        QVERIFY(QMetaObject::invokeMethod(window.data(), "requestCreateDocument",
                Q_ARG(QVariant, QStringLiteral("created.md")), Q_ARG(QVariant, false)));
        window->setProperty("awaitingPendingSave", true);
        backend.save();
        QVERIFY(QFileInfo::exists(directory.filePath("created.md")));
        QCOMPARE(backend.fileUrl(), QUrl::fromLocalFile(QFileInfo(directory.filePath("created.md")).canonicalFilePath()));
        QVERIFY(sample.open(QIODevice::ReadOnly));
        QCOMPARE(sample.readAll(), QByteArray("unsaved text")); sample.close();
        // A collision cannot replace either the file or the current dirty buffer.
        editor->setProperty("text", "keep me");
        QVERIFY(QMetaObject::invokeMethod(window.data(), "requestCreateDocument",
                Q_ARG(QVariant, QStringLiteral("original.md")), Q_ARG(QVariant, false)));
        QVERIFY(QMetaObject::invokeMethod(window.data(), "completePendingAction"));
        QCOMPARE(editor->property("text").toString(), QStringLiteral("keep me"));
        QVERIFY(backend.modified());
        // New is guarded, and accepted New resets URL, contents and undo.
        QVERIFY(QMetaObject::invokeMethod(window.data(), "requestNewDocument"));
        QCOMPARE(window->property("pendingAction").toString(), QStringLiteral("new"));
        QCOMPARE(editor->property("text").toString(), QStringLiteral("keep me"));
        QVERIFY(QMetaObject::invokeMethod(window.data(), "completePendingAction"));
        QVERIFY(backend.fileUrl().isEmpty());
        QVERIFY(editor->property("text").toString().isEmpty());
        QVERIFY(!backend.modified());
        QVERIFY(!editor->property("canUndo").toBool());
        QVERIFY(!backend.showInFinder());
        QVERIFY(!backend.openInNewWindow(QUrl::fromLocalFile(directory.filePath("absent.md"))));
    }

    void recentMenuTracksLibraryAndGuardsOpen() {
        QTemporaryDir directory;
        QFile sample(directory.filePath("Recent.md"));
        QVERIFY(sample.open(QIODevice::WriteOnly)); sample.write("recent"); sample.close();
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *library = qobject_cast<FileLibrary *>(backend.library());
        QVERIFY(library);
        library->clearRecentFiles();
        library->recordRecentFile(QUrl::fromLocalFile(sample.fileName()));
        QCoreApplication::processEvents();
#ifdef Q_OS_MACOS
        int count = 0;
        for (auto *item : window->findChildren<QObject *>()) {
            if (item->property("text").toString().startsWith("Recent.md — ")) {
                ++count;
                QVERIFY(QMetaObject::invokeMethod(item, "triggered"));
                break; // Opening reorders/recreates the dynamic menu delegates.
            }
        }
        QCOMPARE(count, 1);
        QCOMPARE(backend.fileUrl(), QUrl::fromLocalFile(QFileInfo(sample.fileName()).canonicalFilePath()));
#endif
        auto *editor = window->findChild<QObject *>("sourceEditor");
        QVERIFY(editor);
        editor->setProperty("text", "preserve dirty buffer");
        QVERIFY(sample.remove());
        QVERIFY(QMetaObject::invokeMethod(window.data(), "requestOpen", Q_ARG(QVariant, QUrl::fromLocalFile(sample.fileName()))));
        QCOMPARE(window->property("pendingAction").toString(), QStringLiteral("open"));
        QVERIFY(QMetaObject::invokeMethod(window.data(), "completePendingAction"));
        QCOMPARE(editor->property("text").toString(), QStringLiteral("preserve dirty buffer"));
        QVERIFY(backend.modified());
        QVERIFY(backend.status().startsWith("Could not open"));
        library->clearRecentFiles();
        QCoreApplication::processEvents();
        QVERIFY(library->recentFiles().isEmpty());
    }

    void dateDisplayMigratesLegacyPreference() {
        QSettings settings;
        const QStringList keys = {
            QStringLiteral("libraryDisplay/showDates"),
            QStringLiteral("libraryDisplay/dateMode"),
            QStringLiteral("libraryDisplay/dateModeRevision"),
            QStringLiteral("libraryDisplay/compactListRevision")};
        QVariantMap previous;
        QSet<QString> present;
        for (const auto &key : keys) {
            if (settings.contains(key)) present.insert(key);
            previous.insert(key, settings.value(key));
        }
        const auto restore = qScopeGuard([&] {
            for (const auto &key : keys) {
                if (present.contains(key)) settings.setValue(key, previous.value(key));
                else settings.remove(key);
            }
            settings.sync();
        });
        settings.setValue(QStringLiteral("libraryDisplay/showDates"), true);
        settings.setValue(QStringLiteral("libraryDisplay/compactListRevision"), 1);
        settings.remove(QStringLiteral("libraryDisplay/dateMode"));
        settings.remove(QStringLiteral("libraryDisplay/dateModeRevision"));
        settings.sync();

        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY(window);
        auto *pane = window->findChild<QObject *>("libraryPane");
        QVERIFY(pane);
        QCOMPARE(pane->property("dateMode").toInt(), 1);
        auto *commands = window->findChild<QObject *>("workspaceCommands");
        QVERIFY(commands);
        QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("dateCreated"))));
        QCOMPARE(pane->property("dateMode").toInt(), 2);
        window.reset();
        QCoreApplication::processEvents();
        QScopedPointer<QObject> reopened(component.create());
        QVERIFY(reopened);
        auto *reopenedPane = reopened->findChild<QObject *>("libraryPane");
        QVERIFY(reopenedPane);
        QCOMPARE(reopenedPane->property("dateMode").toInt(), 2);
    }

    void nativeWorkspaceMenusShareState() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY(window);
        auto *settings = window->findChild<QObject *>("workspaceSettings");
        auto *pane = window->findChild<QObject *>("libraryPane");
        QVERIFY(settings && pane);
        auto native = [&](const char *id) { return window->findChild<QObject *>(QStringLiteral("native_") + id); };
        auto *libraryAction = native("library");
#ifdef Q_OS_MACOS
        QVERIFY(libraryAction);
#else
        QSKIP("Native menus are macOS-specific");
#endif
        settings->setProperty("libraryVisible", true);
        settings->setProperty("organizerVisible", true);
        QVERIFY(QMetaObject::invokeMethod(libraryAction, "triggered"));
        QVERIFY(!settings->property("libraryVisible").toBool());
        QCOMPARE(libraryAction->property("text").toString(), QStringLiteral("Show Library"));
        QVERIFY(!native("organizer")->property("enabled").toBool());
        settings->setProperty("libraryVisible", true);
        QCOMPARE(libraryAction->property("text").toString(), QStringLiteral("Hide Library"));
        window->setProperty("width", 800);
        QTRY_VERIFY(!native("organizer")->property("enabled").toBool());
        window->setProperty("width", 1280);
        QTRY_VERIFY(native("organizer")->property("enabled").toBool());
        for (const auto &pair : {qMakePair("sortBar", "showSortBar"), qMakePair("filterBar", "showFilterBar"),
                                qMakePair("excerpts", "showExcerpts")}) {
            auto *action = native(pair.first);
            QVERIFY(action);
            pane->setProperty(pair.second, false);
            QVERIFY(QMetaObject::invokeMethod(action, "triggered"));
            QVERIFY(pane->property(pair.second).toBool());
            QVERIFY(action->property("checked").toBool());
            pane->setProperty(pair.second, false);
            QVERIFY(!action->property("checked").toBool());
            QVERIFY(QMetaObject::invokeMethod(action, "triggered"));
            QVERIFY(action->property("checked").toBool());
        }
        QVERIFY(QMetaObject::invokeMethod(native("sortCreated"), "triggered"));
        QCOMPARE(backend.library()->property("sortMode").toInt(), 2);
        QVERIFY(native("sortCreated")->property("checked").toBool());
        backend.library()->setProperty("sortMode", 0);
        QVERIFY(!native("sortCreated")->property("checked").toBool());
        QVERIFY(native("sortName")->property("checked").toBool());
        pane->setProperty("dateMode", 0);
        backend.library()->setProperty("sortMode", 1);
        QVERIFY(QMetaObject::invokeMethod(native("dateCreated"), "triggered"));
        QCOMPARE(pane->property("dateMode").toInt(), 2);
        QCOMPARE(backend.library()->property("sortMode").toInt(), 1);
        QVERIFY(native("dateCreated")->property("checked").toBool());
        QVERIFY(!native("dateModified")->property("checked").toBool());
        QVERIFY(QMetaObject::invokeMethod(native("dateNone"), "triggered"));
        QCOMPARE(pane->property("dateMode").toInt(), 0);
        QVERIFY(native("dateNone")->property("checked").toBool());
        backend.library()->setProperty("navigationMode", 0);
        QVERIFY(native("navigationTree"));
        QVERIFY(native("navigationList"));
        QVERIFY(native("navigationTree")->property("checked").toBool());
        QVERIFY(QMetaObject::invokeMethod(native("navigationList"), "triggered"));
        QCOMPARE(backend.library()->property("navigationMode").toInt(), 1);
        QVERIFY(native("navigationList")->property("checked").toBool());
        settings->setProperty("layoutMode", 0);
        QVERIFY(native("webPreview"));
        QVERIFY(QMetaObject::invokeMethod(native("webPreview"), "triggered"));
        QCOMPARE(settings->property("layoutMode").toInt(), 1);
        QVERIFY(QMetaObject::invokeMethod(native("preview"), "triggered"));
        QCOMPARE(settings->property("layoutMode").toInt(), 2);
        QVERIFY(QMetaObject::invokeMethod(native("split"), "triggered"));
        QCOMPARE(settings->property("layoutMode").toInt(), 1);
        auto *pdfPreview = window->findChild<QObject *>("native_pdfPreview");
        QVERIFY(pdfPreview);
        QCOMPARE(pdfPreview->property("text").toString(), QStringLiteral("Paginated Preview…"));
        QVERIFY(QMetaObject::invokeMethod(native("descending"), "triggered"));
        QVERIFY(!backend.library()->property("ascending").toBool());
        QVERIFY(!native("ascending")->property("checked").toBool());
        // Restore the shared test preferences for following existing tests.
        pane->setProperty("dateMode", 0);
        pane->setProperty("showExcerpts", false);
        backend.library()->setProperty("navigationMode", 0);
        backend.library()->setProperty("ascending", true);
    }

    void nativeFileAndEditMenuActionsRespectFocusAndGuards() {
        QTemporaryDir directory;
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY(window);
        auto *editor = window->findChild<QObject *>("sourceEditor");
        QVERIFY(editor);
        auto action = [&](const char *name) -> QObject * { return window->findChild<QObject *>(name); };

        auto *duplicate = action("fileDuplicate");
        auto *rename = action("fileRename");
        auto *move = action("fileMove");
        auto *createVersion = action("fileCreateVersion");
        auto *restoreVersion = action("fileRestoreVersion");
        auto *revealFinder = action("fileRevealFinder");
        auto *revealLibrary = action("fileRevealLibrary");
        QVERIFY(duplicate && rename && move && createVersion && restoreVersion && revealFinder && revealLibrary);
        QVERIFY(!duplicate->property("enabled").toBool());
        QVERIFY(!rename->property("enabled").toBool());
        QVERIFY(!move->property("enabled").toBool());
        QVERIFY(!createVersion->property("enabled").toBool());
        QVERIFY(!restoreVersion->property("enabled").toBool());
        QVERIFY(!revealFinder->property("enabled").toBool());
        QVERIFY(!revealLibrary->property("enabled").toBool());

        const QString source = QStringLiteral("**café** source");
        editor->setProperty("text", source);
        const QUrl saved = QUrl::fromLocalFile(directory.filePath("menu.md"));
        backend.saveAs(saved);
        QVERIFY(!backend.modified());
        QTRY_VERIFY(duplicate->property("enabled").toBool());
        QVERIFY(rename->property("enabled").toBool());
        QVERIFY(move->property("enabled").toBool());
        QVERIFY(createVersion->property("enabled").toBool());
        QVERIFY(restoreVersion->property("enabled").toBool());
        QVERIFY(revealFinder->property("enabled").toBool());
        QVERIFY(revealLibrary->property("enabled").toBool());

        auto *copyFormatted = action("editCopyFormatted");
        auto *copyHtml = action("editCopyHtml");
        auto *copyMarkdown = action("editCopyMarkdown");
        QVERIFY(copyFormatted && copyHtml && copyMarkdown);
        editor->setProperty("text", source);
        QVERIFY(QMetaObject::invokeMethod(editor, "forceActiveFocus"));
        QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 0), Q_ARG(int, 8)));
        QTRY_VERIFY(copyFormatted->property("enabled").toBool());
        QVERIFY(copyHtml->property("enabled").toBool());
        QVERIFY(copyMarkdown->property("enabled").toBool());
        const int revision = backend.documentRevision();
        QVERIFY(QMetaObject::invokeMethod(copyMarkdown, "triggered"));
        QCOMPARE(QGuiApplication::clipboard()->mimeData()->data("text/markdown"), QByteArray("**café**"));
        QVERIFY(QMetaObject::invokeMethod(copyHtml, "triggered"));
        QVERIFY(QGuiApplication::clipboard()->text().contains("<html"));
        QVERIFY(QMetaObject::invokeMethod(copyFormatted, "triggered"));
        QVERIFY(QGuiApplication::clipboard()->mimeData()->hasHtml());
        QCOMPARE(editor->property("text").toString(), source);
        QCOMPARE(backend.documentRevision(), revision);

        auto *find = action("editFind");
        QVERIFY(find);
        QVERIFY(QMetaObject::invokeMethod(find, "triggered"));
        auto *query = window->findChild<QObject *>("searchField");
        QVERIFY(query);
        QVERIFY(QMetaObject::invokeMethod(query, "forceActiveFocus"));
        QTRY_VERIFY(!copyFormatted->property("enabled").toBool());
        QVERIFY(!copyHtml->property("enabled").toBool());
        QVERIFY(!copyMarkdown->property("enabled").toBool());

        editor->setProperty("text", QStringLiteral("dirty but retained"));
        auto *close = action("fileClose");
        QVERIFY(close);
        QVERIFY(QMetaObject::invokeMethod(close, "triggered"));
        auto *prompt = window->findChild<QObject *>("unsavedChangesPrompt");
        QVERIFY(prompt);
        QTRY_VERIFY(prompt->property("opened").toBool());
        QVERIFY(QMetaObject::invokeMethod(prompt, "cancelRequested"));
        QCOMPARE(window->property("pendingAction").toString(), QString());
        QCOMPARE(editor->property("text").toString(), QStringLiteral("dirty but retained"));
        QVERIFY(backend.modified());
        backend.discardRecovery();
    }

    void workspacePresentationPreservesSourceAndUndo() {
        QTemporaryDir directory;
        QFile sample(directory.filePath("menu.md"));
        QVERIFY(sample.open(QIODevice::WriteOnly));
        const QByteArray source("# Menu check\n\nPlain **Markdown**.\n");
        QCOMPARE(sample.write(source), source.size());
        sample.close();
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        backend.open(QUrl::fromLocalFile(sample.fileName()));
        auto *commands = window->findChild<QObject *>("workspaceCommands");
        auto *settings = window->findChild<QObject *>("workspaceSettings");
        auto *editor = window->findChild<QObject *>("sourceEditor");
        QVERIFY(commands && settings && editor);
        auto run = [&](const char *id) { return QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QString::fromLatin1(id))); };
        for (const auto *id : {"editor", "split", "preview", "split", "paragraph", "typewriter", "serif", "mono", "sans", "markup", "reloadPreview"})
            QVERIFY(run(id));
        auto *preview = window->findChild<QObject *>("previewPane");
        QVERIFY(preview);
        QTRY_COMPARE(preview->property("renderedMarkdown").toString(), QString::fromUtf8(source));
        QCOMPARE(editor->property("text").toString(), QString::fromUtf8(source));
        QVERIFY(!backend.modified());
        QVERIFY(!editor->property("canUndo").toBool());
        settings->setProperty("writingSize", 32);
        QVERIFY(run("larger"));
        QCOMPARE(settings->property("writingSize").toInt(), 32);
        settings->setProperty("writingSize", 12);
        QVERIFY(run("smaller"));
        QCOMPARE(settings->property("writingSize").toInt(), 12);
        QVERIFY(run("resetSize"));
        QCOMPARE(settings->property("writingSize").toInt(), 16);
        settings->setProperty("paragraphFocus", false);
        settings->setProperty("typewriter", false);
        settings->setProperty("showMarkup", true);
    }

    void focusMenuKeepsModesExclusiveAndTypewriterIndependent() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *commands = window->findChild<QObject *>("workspaceCommands");
        auto *settings = window->findChild<QObject *>("workspaceSettings");
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *focusMenu = window->findChild<QObject *>("focusModeMenu");
        auto *sentence = window->findChild<QObject *>("native_sentence");
        auto *paragraph = window->findChild<QObject *>("native_paragraph");
        auto *typewriter = window->findChild<QObject *>("native_typewriter");
        auto *review = window->findChild<QObject *>("focusWritingReview");
        auto *reviewDialog = window->findChild<QObject *>("writingReviewDialog");
        QVERIFY(commands && settings && editor && focusMenu && sentence && paragraph
                && typewriter && review && reviewDialog);

        const bool originalSentence = settings->property("sentenceFocus").toBool();
        const bool originalParagraph = settings->property("paragraphFocus").toBool();
        const bool originalTypewriter = settings->property("typewriter").toBool();
        const auto restore = qScopeGuard([&] {
            settings->setProperty("sentenceFocus", originalSentence);
            settings->setProperty("paragraphFocus", originalParagraph);
            settings->setProperty("typewriter", originalTypewriter);
        });
        settings->setProperty("sentenceFocus", false);
        settings->setProperty("paragraphFocus", false);
        settings->setProperty("typewriter", false);
        editor->setProperty("text", QStringLiteral("Focus modes keep this source unchanged."));
        const QString source = editor->property("text").toString();

        QCOMPARE(focusMenu->property("title").toString(), QStringLiteral("Enable Focus Mode"));
        QCOMPARE(sentence->property("text").toString(), QStringLiteral("Sentence"));
        QCOMPARE(paragraph->property("text").toString(), QStringLiteral("Paragraph"));
        QCOMPARE(typewriter->property("text").toString(), QStringLiteral("Typewriter"));
        QVERIFY(sentence->property("checkable").toBool());
        QVERIFY(paragraph->property("checkable").toBool());
        QVERIFY(typewriter->property("checkable").toBool());

        auto run = [&](const QString &id) {
            return QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, id));
        };
        QVERIFY(run(QStringLiteral("sentence")));
        QVERIFY(settings->property("sentenceFocus").toBool());
        QVERIFY(!settings->property("paragraphFocus").toBool());
        QVERIFY(!settings->property("typewriter").toBool());
        QVERIFY(sentence->property("checked").toBool());

        QVERIFY(run(QStringLiteral("typewriter")));
        QVERIFY(settings->property("sentenceFocus").toBool());
        QVERIFY(settings->property("typewriter").toBool());
        QVERIFY(typewriter->property("checked").toBool());

        QVERIFY(run(QStringLiteral("paragraph")));
        QVERIFY(!settings->property("sentenceFocus").toBool());
        QVERIFY(settings->property("paragraphFocus").toBool());
        QVERIFY(settings->property("typewriter").toBool());
        QVERIFY(paragraph->property("checked").toBool());
        QVERIFY(!sentence->property("checked").toBool());

        QVERIFY(run(QStringLiteral("paragraph")));
        QVERIFY(!settings->property("sentenceFocus").toBool());
        QVERIFY(!settings->property("paragraphFocus").toBool());
        QVERIFY(settings->property("typewriter").toBool());
        QCOMPARE(editor->property("text").toString(), source);

        QCOMPARE(review->property("text").toString(), QStringLiteral("Writing Review…"));
        QVERIFY(QMetaObject::invokeMethod(review, "triggered"));
        QTRY_VERIFY(reviewDialog->property("opened").toBool());
        QVERIFY(QMetaObject::invokeMethod(reviewDialog, "close"));
        backend.discardRecovery();
    }

    void customStyleCheckPersistsAndRefreshesWithoutEditing() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *commands = window->findChild<QObject *>("workspaceCommands");
        auto *settings = window->findChild<QObject *>("workspaceSettings");
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *styleMenu = window->findChild<QObject *>("focusStyleCheckMenu");
        auto *custom = window->findChild<QObject *>("native_customStyleCheck");
        QVERIFY(commands && settings && editor && styleMenu && custom);

        const bool originalEnabled = settings->property("styleCheckCustom").toBool();
        const QString originalWords = settings->property("reviewWords").toString();
        settings->setProperty("styleCheckCustom", false);
        settings->setProperty("reviewWords", QStringLiteral("custom"));
        editor->setProperty("text", QStringLiteral("**custom** other"));
        QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 4), Q_ARG(int, 4)));
        const QString source = editor->property("text").toString();
        const bool canUndo = editor->property("canUndo").toBool();

        QCOMPARE(styleMenu->property("title").toString(), QStringLiteral("Enable Style Check"));
        QCOMPARE(custom->property("text").toString(), QStringLiteral("Custom"));
        QVERIFY(custom->property("checkable").toBool());
        QVERIFY(!custom->property("checked").toBool());
        QVERIFY(QMetaObject::invokeMethod(commands, "run",
                                          Q_ARG(QVariant, QStringLiteral("customStyleCheck"))));
        QVERIFY(settings->property("styleCheckCustom").toBool());
        QVERIFY(custom->property("checked").toBool());

        auto *quick = qvariant_cast<QQuickTextDocument *>(editor->property("textDocument"));
        QVERIFY(quick);
        auto backgroundAt = [&](int position) {
            const QTextBlock block = quick->textDocument()->findBlock(position);
            const int local = position - block.position();
            QBrush background;
            for (const auto &range : block.layout()->formats())
                if (local >= range.start && local < range.start + range.length)
                    background = range.format.background();
            return background;
        };
        QTRY_VERIFY_WITH_TIMEOUT(backgroundAt(2).style() != Qt::NoBrush, 1000);
        QCOMPARE(editor->property("text").toString(), source);
        QCOMPARE(editor->property("cursorPosition").toInt(), 4);
        QCOMPARE(editor->property("canUndo").toBool(), canUndo);

        editor->setProperty("text", QStringLiteral("other custom"));
        QTRY_VERIFY_WITH_TIMEOUT(backgroundAt(6).style() != Qt::NoBrush, 1000);
        settings->setProperty("reviewWords", QStringLiteral("other"));
        QTRY_VERIFY_WITH_TIMEOUT(backgroundAt(0).style() != Qt::NoBrush, 1000);
        QCOMPARE(backgroundAt(6).style(), Qt::NoBrush);
        settings->setProperty("reviewWords", QStringLiteral("custom"));
        QTRY_VERIFY_WITH_TIMEOUT(backgroundAt(6).style() != Qt::NoBrush, 1000);
        QCOMPARE(backgroundAt(0).style(), Qt::NoBrush);
        QVERIFY(QMetaObject::invokeMethod(commands, "run",
                                          Q_ARG(QVariant, QStringLiteral("customStyleCheck"))));
        QVERIFY(!settings->property("styleCheckCustom").toBool());
        QCOMPARE(backgroundAt(6).style(), Qt::NoBrush);
        QCOMPARE(editor->property("text").toString(), QStringLiteral("other custom"));

        QVERIFY(QMetaObject::invokeMethod(commands, "run",
                                          Q_ARG(QVariant, QStringLiteral("customStyleCheck"))));
        QVERIFY(QMetaObject::invokeMethod(settings, "sync"));
        window.reset();

        QScopedPointer<QObject> reopened(component.create());
        QVERIFY2(reopened, qPrintable(component.errorString()));
        auto *reopenedSettings = reopened->findChild<QObject *>("workspaceSettings");
        auto *reopenedCustom = reopened->findChild<QObject *>("native_customStyleCheck");
        QVERIFY(reopenedSettings && reopenedCustom);
        QCOMPARE(reopenedSettings->property("styleCheckCustom").toBool(), true);
        QCOMPARE(reopenedSettings->property("reviewWords").toString(), QStringLiteral("custom"));
        QVERIFY(reopenedCustom->property("checked").toBool());

        reopenedSettings->setProperty("reviewWords", originalWords);
        reopenedSettings->setProperty("styleCheckCustom", originalEnabled);
        QVERIFY(QMetaObject::invokeMethod(reopenedSettings, "sync"));
        backend.discardRecovery();
    }

    void authorshipSetupPersistsSafelyWithoutLabellingDocument() {
        QTemporaryDir directory;
        const QString documentPath = directory.filePath(QStringLiteral("authors.md"));
        QFile documentFile(documentPath);
        QVERIFY(documentFile.open(QIODevice::WriteOnly));
        const QByteArray source("# Synthetic authorship\n\nPlain local Markdown.\n");
        QCOMPARE(documentFile.write(source), source.size());
        documentFile.close();

        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        backend.open(QUrl::fromLocalFile(documentPath));

        auto *settings = window->findChild<QObject *>("workspaceSettings");
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *menu = window->findChild<QObject *>("authorsMenu");
        auto *setup = window->findChild<QObject *>("authorsSetupAction");
        auto *dialog = window->findChild<QObject *>("authorshipSetupDialog");
        auto *name = window->findChild<QObject *>("authorshipProfileName");
        auto *identifier = window->findChild<QObject *>("authorshipProfileIdentifier");
        auto *save = window->findChild<QObject *>("authorshipProfileSave");
        auto *cancel = window->findChild<QObject *>("authorshipProfileCancel");
        QVERIFY(settings && editor && menu && setup && dialog && name && identifier
                && save && cancel);

        const QString originalName = settings->property("authorshipProfileName").toString();
        const QString originalIdentifier = settings->property("authorshipProfileIdentifier").toString();
        settings->setProperty("authorshipProfileName", QString());
        settings->setProperty("authorshipProfileIdentifier", QString());

        QCOMPARE(menu->property("title").toString(), QStringLiteral("Authors"));
        QCOMPARE(setup->property("text").toString(), QStringLiteral("Set Up Authorship…"));
        QVERIFY(QMetaObject::invokeMethod(setup, "triggered"));
        QTRY_VERIFY(dialog->property("opened").toBool());
        QVERIFY(!save->property("enabled").toBool());
        name->setProperty("text", QStringLiteral("   "));
        QVERIFY(!save->property("enabled").toBool());
        name->setProperty("text", QString(101, QLatin1Char('n')));
        QVERIFY(!save->property("enabled").toBool());
        name->setProperty("text", QStringLiteral("Bad\tName"));
        QVERIFY(!save->property("enabled").toBool());
        name->setProperty("text", QStringLiteral("  Test Writer  "));
        identifier->setProperty("text", QString(201, QLatin1Char('i')));
        QVERIFY(!save->property("enabled").toBool());
        identifier->setProperty("text", QStringLiteral("  test@example.invalid  "));
        QVERIFY(save->property("enabled").toBool());

        const QString textBefore = editor->property("text").toString();
        const QString statusBefore = backend.status();
        const bool modifiedBefore = backend.modified();
        const bool undoBefore = editor->property("canUndo").toBool();
        const QDir recoveryDirectory(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        auto recoveryContents = [&] {
            QMap<QString, QByteArray> contents;
            for (const QString &fileName : recoveryDirectory.entryList(
                     {QStringLiteral("recovery-*.json")}, QDir::Files)) {
                QFile file(recoveryDirectory.filePath(fileName));
                if (file.open(QIODevice::ReadOnly)) contents.insert(fileName, file.readAll());
            }
            return contents;
        };
        const auto recoveryBefore = recoveryContents();
        QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
        QTRY_VERIFY(!dialog->property("opened").toBool());
        QCOMPARE(settings->property("authorshipProfileName").toString(),
                 QStringLiteral("Test Writer"));
        QCOMPARE(settings->property("authorshipProfileIdentifier").toString(),
                 QStringLiteral("test@example.invalid"));

        QCOMPARE(editor->property("text").toString(), textBefore);
        QCOMPARE(backend.status(), statusBefore);
        QCOMPARE(backend.modified(), modifiedBefore);
        QCOMPARE(editor->property("canUndo").toBool(), undoBefore);
        QVERIFY(backend.authorshipRanges().isEmpty());
        QVERIFY(!QFileInfo::exists(directory.filePath(
            QStringLiteral(".authors.md.omawrite-authors.json"))));
        QTest::qWait(850);
        QCOMPARE(recoveryContents(), recoveryBefore);

        QVERIFY(QMetaObject::invokeMethod(setup, "triggered"));
        QTRY_VERIFY(dialog->property("opened").toBool());
        QCOMPARE(name->property("text").toString(), QStringLiteral("Test Writer"));
        QCOMPARE(identifier->property("text").toString(), QStringLiteral("test@example.invalid"));
        name->setProperty("text", QStringLiteral("Cancelled Writer"));
        identifier->setProperty("text", QStringLiteral("cancelled@example.invalid"));
        QVERIFY(QMetaObject::invokeMethod(cancel, "clicked"));
        QTRY_VERIFY(!dialog->property("opened").toBool());
        QCOMPARE(settings->property("authorshipProfileName").toString(),
                 QStringLiteral("Test Writer"));
        QCOMPARE(settings->property("authorshipProfileIdentifier").toString(),
                 QStringLiteral("test@example.invalid"));
        QVERIFY(QMetaObject::invokeMethod(settings, "sync"));
        window.reset();

        QScopedPointer<QObject> reopened(component.create());
        QVERIFY2(reopened, qPrintable(component.errorString()));
        auto *reopenedSettings = reopened->findChild<QObject *>("workspaceSettings");
        QVERIFY(reopenedSettings);
        QCOMPARE(reopenedSettings->property("authorshipProfileName").toString(),
                 QStringLiteral("Test Writer"));
        QCOMPARE(reopenedSettings->property("authorshipProfileIdentifier").toString(),
                 QStringLiteral("test@example.invalid"));
        reopenedSettings->setProperty("authorshipProfileName", originalName);
        reopenedSettings->setProperty("authorshipProfileIdentifier", originalIdentifier);
        QVERIFY(QMetaObject::invokeMethod(reopenedSettings, "sync"));
        backend.discardRecovery();
    }

    void sharedFormattingCommandsPreserveUndo() {
        QTemporaryDir directory;
        QFile sample(directory.filePath("format.md"));
        QVERIFY(sample.open(QIODevice::WriteOnly));
        sample.write("sample");
        sample.close();
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto *commands = window->findChild<QObject *>("workspaceCommands");
        auto *editor = window->findChild<QObject *>("sourceEditor");
        QVERIFY(commands && editor);
        for (const auto &pair : {qMakePair("strike", "~~sample~~"), qMakePair("inlineCode", "`sample`")}) {
            backend.open(QUrl::fromLocalFile(sample.fileName()));
            QVERIFY(QMetaObject::invokeMethod(editor, "select", Q_ARG(int, 0), Q_ARG(int, 6)));
            QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QString::fromLatin1(pair.first))));
            QCOMPARE(editor->property("text").toString(), QString::fromLatin1(pair.second));
            QCOMPARE(editor->property("selectedText").toString(), QStringLiteral("sample"));
            QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
            QCOMPARE(editor->property("text").toString(), QStringLiteral("sample"));
            QVERIFY(QMetaObject::invokeMethod(editor, "redo"));
            QCOMPARE(editor->property("text").toString(), QString::fromLatin1(pair.second));
            QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        }
        // The same C++ mutation handles reversed and empty selections.
        auto selection = backend.wrapSelection(6, 0, "**", "**");
        QCOMPARE(editor->property("text").toString(), QStringLiteral("**sample**"));
        QCOMPARE(selection.value("start").toInt(), 2);
        QCOMPARE(selection.value("end").toInt(), 8);
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        selection = backend.wrapSelection(6, 6, "`", "`");
        QCOMPARE(editor->property("text").toString(), QStringLiteral("sample``"));
        QCOMPARE(selection.value("start").toInt(), 7);
        QCOMPARE(selection.value("end").toInt(), 7);
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QCOMPARE(editor->property("text").toString(), QStringLiteral("sample"));
    }

    void sortMenuAppliesFieldAndDirection() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY(window);
        auto *menu = window->findChild<QObject *>("librarySortMenu");
        QVERIFY(menu);
        auto trigger = [menu](const QString &label) {
            for (auto *item : menu->findChildren<QObject *>()) {
                if (item->property("text").toString() == label)
                    return QMetaObject::invokeMethod(item, "triggered");
            }
            return false;
        };
        QVERIFY(trigger("Date Modified"));
        QCOMPARE(backend.library()->property("sortMode").toInt(), 1);
        QVERIFY(trigger("Date Created"));
        QCOMPARE(backend.library()->property("sortMode").toInt(), 2);
        QVERIFY(trigger("Extension"));
        QCOMPARE(backend.library()->property("sortMode").toInt(), 3);
        QVERIFY(trigger("Name"));
        QCOMPARE(backend.library()->property("sortMode").toInt(), 0);
        QVERIFY(trigger("Z to A"));
        QVERIFY(!backend.library()->property("ascending").toBool());
        QVERIFY(trigger("A to Z"));
        QVERIFY(backend.library()->property("ascending").toBool());
        auto *previews = window->findChild<QObject *>("libraryPreviewToggle");
        QVERIFY(previews);
        QVERIFY(!previews->property("checked").toBool());
        QVERIFY(trigger("Show Text Excerpts"));
        QVERIFY(previews->property("checked").toBool());
        // A button click changes the same preference used by the menu.
        previews->setProperty("checked", false);
        QVERIFY(QMetaObject::invokeMethod(previews, "clicked"));
        QVERIFY(trigger("Show Text Excerpts"));
        QVERIFY(previews->property("checked").toBool());
        QVERIFY(trigger("Show Text Excerpts"));
        QVERIFY(!previews->property("checked").toBool());
    }

    void libraryBarsCanBeHiddenAndRestored() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY(window);
        for (const auto &pair : {qMakePair("toggleSortBar", "librarySortBar"), qMakePair("toggleFilterBar", "libraryFilter")}) {
            auto *action = window->findChild<QObject *>(pair.first);
            auto *bar = window->findChild<QObject *>(pair.second);
            QVERIFY(action);
            QVERIFY(bar);
            QVERIFY(QMetaObject::invokeMethod(action, "triggered"));
            QVERIFY(!bar->property("visible").toBool());
            QVERIFY(action->property("text").toString().startsWith("Show"));
            QVERIFY(QMetaObject::invokeMethod(action, "triggered"));
            QVERIFY(action->property("text").toString().startsWith("Hide"));
        }
        auto *sort = window->findChild<QObject *>("libraryOptionsSortMenu");
        QVERIFY(sort);
        for (auto *item : sort->findChildren<QObject *>()) {
            if (item->property("text").toString() == "Extension") {
                QVERIFY(QMetaObject::invokeMethod(item, "triggered"));
                QCOMPARE(backend.library()->property("sortMode").toInt(), 3);
                return;
            }
        }
        QFAIL("Missing Extension submenu action");
    }

    void organizerSectionsCollapseWithoutChangingShortcuts() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY(window);
        const auto favorites = backend.library()->property("favorites");
        const auto recents = backend.library()->property("recentFiles");
        for (const auto &name : {"favoritesDisclosure", "recentsDisclosure"}) {
            auto *button = window->findChild<QObject *>(name);
            QVERIFY(button);
            QCOMPARE(button->property("iconName").toString(), QString("down"));
            QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
            QCOMPARE(button->property("iconName").toString(), QString("right"));
            QVERIFY(button->property("hint").toString().startsWith("Expand"));
            QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
            QCOMPARE(button->property("iconName").toString(), QString("down"));
        }
        QCOMPARE(backend.library()->property("favorites"), favorites);
        QCOMPARE(backend.library()->property("recentFiles"), recents);
    }

    void boundsLibraryExcerptsAndPreservesFiles() {
        QTemporaryDir directory;
        QTemporaryDir outside;
        FileLibrary library;
        library.setRootFolder(QUrl::fromLocalFile(directory.path()));
        QFile file(directory.filePath("sample.md"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        const QByteArray source = "# Heading\n\nA readable note.\n" + QByteArray(4096, 'x');
        QCOMPARE(file.write(source), qint64(source.size()));
        file.close();
        const QString snippet = library.excerpt(QUrl::fromLocalFile(file.fileName()));
        QVERIFY(snippet.startsWith("Heading A readable note."));
        QVERIFY(snippet.size() <= 160);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), source);
        QVERIFY(library.excerpt(QUrl::fromLocalFile(outside.filePath("sample.md"))).isEmpty());
        QVERIFY(library.excerpt(QUrl("https://example.com/sample.md")).isEmpty());
    }

    void libraryTreeAndListNavigationPreserveReachability() {
        QSettings settings;
        const QStringList keys = {QStringLiteral("library/navigationMode"), QStringLiteral("library/root"),
                                  QStringLiteral("library/locations")};
        QVariantMap previous;
        QSet<QString> present;
        for (const auto &key : keys) {
            if (settings.contains(key)) present.insert(key);
            previous.insert(key, settings.value(key));
            settings.remove(key);
        }
        settings.setValue(QStringLiteral("library/navigationMode"), 0);
        settings.sync();
        const auto restore = qScopeGuard([&] {
            for (const auto &key : keys) {
                if (present.contains(key)) settings.setValue(key, previous.value(key));
                else settings.remove(key);
            }
            settings.sync();
        });

        QTemporaryDir directory;
        QVERIFY(QDir(directory.path()).mkpath(QStringLiteral("Notes/Deep")));
        for (const QString &relative : {QStringLiteral("Top.md"), QStringLiteral("Notes/Nested.md"),
                                        QStringLiteral("Notes/Deep/Deep.md")}) {
            QFile file(directory.filePath(relative));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QVERIFY(file.write(relative.toUtf8()) > 0);
        }
        const QUrl top = QUrl::fromLocalFile(QFileInfo(directory.path()).canonicalFilePath());
        const QUrl notes = QUrl::fromLocalFile(QFileInfo(directory.filePath("Notes")).canonicalFilePath());
        const QUrl deep = QUrl::fromLocalFile(QFileInfo(directory.filePath("Notes/Deep")).canonicalFilePath());

        FileLibrary library;
        QCOMPARE(library.navigationMode(), 0);
        library.setRootFolder(top);
        QCOMPARE(library.entries().size(), 2);
        library.toggleFolder(notes);
        QCOMPARE(library.entries().size(), 4);
        library.setNavigationMode(1);
        QCOMPARE(library.entries().size(), 2);
        for (const auto &entry : library.entries()) QCOMPARE(entry.toMap().value("depth").toInt(), 0);
        library.setNavigationMode(0);
        QCOMPARE(library.entries().size(), 4); // Same-root expansion remains in memory.

        library.setNavigationMode(1);
        library.toggleFolder(notes);
        QCOMPARE(library.rootFolder(), notes);
        QCOMPARE(library.entries().size(), 2);
        QVERIFY(library.canGoBack());
        QVERIFY(library.navigateHistory(-1));
        QCOMPARE(library.rootFolder(), top);

        const QUrl deepFile = QUrl::fromLocalFile(directory.filePath("Notes/Deep/Deep.md"));
        QVERIFY(library.showFile(deepFile) >= 0);
        QCOMPARE(library.rootFolder(), deep);
        QCOMPARE(library.entries().size(), 1);
        library.setRootFolder(top);
        library.revealFile(QUrl::fromLocalFile(directory.filePath("Notes/Nested.md")));
        QCOMPARE(library.rootFolder(), notes);
        QVERIFY(library.entries().size() == 2);

        library.setNavigationMode(1);
        FileLibrary reopened;
        QCOMPARE(reopened.navigationMode(), 1);
    }

    void browsesAndCreatesLibraryFilesSafely() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        FileLibrary library;
        library.setRootFolder(QUrl::fromLocalFile(directory.path()));
        QVERIFY(library.createFolder("Notes"));
        const QUrl draft = library.createDocument("Draft");
        QVERIFY(draft.isLocalFile());
        QFile file(draft.toLocalFile());
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("preserve me");
        file.close();
        QVERIFY(library.createDocument("Draft").isEmpty());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("preserve me"));
        QVERIFY(library.createDocument("../escape").isEmpty());
        QFile nested(directory.filePath("Notes/Nested.md"));
        QVERIFY(nested.open(QIODevice::WriteOnly)); nested.close();
        library.refresh();
        QCOMPARE(library.entries().size(), 2);
        library.toggleFolder(QUrl::fromLocalFile(directory.filePath("Notes")));
        QCOMPARE(library.entries().size(), 3);
        library.setFilter("nested");
        QCOMPARE(library.entries().size(), 2);
        QCOMPARE(library.entries().at(1).toMap().value("name").toString(), QString("Nested.md"));
        library.setRootFolder(QUrl("https://example.com"));
        QCOMPARE(library.rootFolder(), QUrl::fromLocalFile(QFileInfo(directory.path()).canonicalFilePath()));
        QVERIFY(!library.error().isEmpty());
    }

    void rendersPreviewWithoutChangingMarkdown() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        QObject *editor = window->findChild<QObject *>("sourceEditor");
        QObject *preview = window->findChild<QObject *>("renderedPreview");
        QObject *pane = window->findChild<QObject *>("previewPane");
        QVERIFY(editor && preview && pane);
        const QString text = "# Title\n\n**Bold** and café\n\n| A | B |\n|---|---|\n| 1 | 2 |\n";
        editor->setProperty("text", text);
        QTRY_COMPARE(pane->property("renderedMarkdown").toString(), text);
        QCOMPARE(preview->property("readOnly").toBool(), true);
        QCOMPARE(editor->property("text").toString(), text);
        backend.discardRecovery();
    }

    void typographyDoesNotDirtySavedDocument() {
        QTemporaryDir directory;
        QFile file(directory.filePath("saved.md"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("# Saved note\n\nSome **words**.\n");
        file.close();
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY(window);
        backend.open(QUrl::fromLocalFile(file.fileName()));
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *settings = window->findChild<QObject *>("workspaceSettings");
        const QString original = editor->property("text").toString();
        settings->setProperty("writingSize", 22);
        settings->setProperty("paragraphFocus", true);
        settings->setProperty("writingSize", 16);
        settings->setProperty("paragraphFocus", false);
        QCoreApplication::processEvents();
        QCOMPARE(editor->property("text").toString(), original);
        QVERIFY(!backend.modified());
    }

    void presentationKeepsSourceAndUndoIntact() {
        QTextDocument document;
        document.setPlainText("**Bold**\n\nSecond paragraph");
        MarkdownHighlighter highlighter(&document);
        highlighter.setDarkMode(false);
        highlighter.rehighlight();
        const QString original = document.toPlainText();
        auto markerSize = [&document]() {
            for (const auto &range : document.firstBlock().layout()->formats())
                if (range.start == 0) return range.format.fontPointSize();
            return qreal(-1);
        };
        QCOMPARE(markerSize(), qreal(1));
        highlighter.setShowMarkup(true);
        QVERIFY(markerSize() != 1);
        highlighter.setFocusBlock(0);
        auto ranges = document.lastBlock().layout()->formats();
        QVERIFY(!ranges.isEmpty());
        QCOMPARE(ranges.first().format.foreground().color(), QColor("#a1a6ad"));
        highlighter.setFocusBlock(2);
        ranges = document.lastBlock().layout()->formats();
        QVERIFY(ranges.isEmpty() || ranges.first().format.foreground().color() != QColor("#a1a6ad"));
        highlighter.setFocusBlock(-1);
        QCOMPARE(document.toPlainText(), original);
        QVERIFY(!document.isUndoAvailable());
    }

    void previewsRelativeImagesAndCentersTypewriter() {
        QTemporaryDir directory;
        QImage image(24, 24, QImage::Format_ARGB32);
        image.fill(Qt::cyan);
        QVERIFY(image.save(directory.filePath("asset.png")));
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY(window);
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *preview = window->findChild<QObject *>("renderedPreview");
        auto *settings = window->findChild<QObject *>("workspaceSettings");
        auto *scroll = window->findChild<QObject *>("editorScroll");
        QVERIFY(editor && preview && settings && scroll);
        const QString markdown = "# Image\n\n![Test](asset.png)\n\n" + QString("Paragraph.\n\n").repeated(40);
        editor->setProperty("text", markdown);
        backend.saveAs(QUrl::fromLocalFile(directory.filePath("draft.md")));
        auto *quick = qvariant_cast<QQuickTextDocument *>(preview->property("textDocument"));
        QVERIFY(quick);
        QTRY_VERIFY(!quick->textDocument()->resource(QTextDocument::ImageResource,
            QUrl::fromLocalFile(directory.filePath("asset.png"))).isNull());
        QCOMPARE(backend.resolveDocumentLink("sibling.md"), QUrl::fromLocalFile(directory.filePath("sibling.md")));
        settings->setProperty("typewriter", true);
        editor->setProperty("cursorPosition", markdown.size() - 2);
        QTRY_VERIFY(scroll->property("contentY").toReal() > 0);
        const QRectF caret = editor->property("cursorRectangle").toRectF();
        const qreal screenCenter = editor->property("y").toReal() + caret.center().y() - scroll->property("contentY").toReal();
        QVERIFY(qAbs(screenCenter - scroll->property("height").toReal() / 2) < 2);
        settings->setProperty("typewriter", false);
        QCOMPARE(editor->property("text").toString(), markdown);
        QVERIFY(!backend.modified());
        backend.discardRecovery();
    }

    void outlinesHeadingsOutsideCodeAndFrontMatter() {
        Backend backend;
        const QString text = "---\ntitle: Hidden\n---\n# First ###\n\n````md\n# Hidden\n```\n# Also hidden\n````\n\nSecond\n------\n\n### C#\n";
        const auto outline = backend.documentOutline(text);
        QCOMPARE(outline.size(), 3);
        QCOMPARE(outline.at(0).toMap().value("title").toString(), QString("First"));
        QCOMPARE(outline.at(1).toMap().value("level").toInt(), 2);
        QCOMPARE(outline.at(1).toMap().value("position").toInt(), text.indexOf("Second"));
        QCOMPARE(outline.at(2).toMap().value("title").toString(), QString("C#"));
        QCOMPARE(backend.documentOutline("---\n# Visible").size(), 1);
        const auto stats = backend.documentStatistics("**Hello** world");
        QCOMPARE(stats.value("words").toInt(), 2);
        QCOMPARE(stats.value("characters").toInt(), 11);
        QCOMPARE(stats.value("charactersWithoutSpaces").toInt(), 10);
        QCOMPARE(stats.value("readingMinutes").toInt(), 1);
        QCOMPARE(backend.documentStatistics("").value("readingMinutes").toInt(), 0);
    }

    void statisticsCountRenderedUnicodeTasksAndAuthorship() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY(window);
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *commands = window->findChild<QObject *>("workspaceCommands");
        auto *settings = window->findChild<QObject *>("workspaceSettings");
        auto *toolbarStatistic = window->findChild<QObject *>("toolbarStatistic");
        auto *statisticsDialog = window->findChild<QObject *>("statisticsDialog");
        QVERIFY(editor && commands && settings && toolbarStatistic && statisticsDialog);

        const int originalMode = settings->property("toolbarMode").toInt();
        QObject *restoreSettings = settings;
        QVariantMap originalMetrics;
        const QStringList metricProperties = {
            QStringLiteral("toolbarCharacters"), QStringLiteral("toolbarCharactersNoSpaces"),
            QStringLiteral("toolbarWords"), QStringLiteral("toolbarSentences"),
            QStringLiteral("toolbarReadingTime"), QStringLiteral("toolbarSpeakingTime"),
            QStringLiteral("toolbarTasks"), QStringLiteral("toolbarHuman"),
            QStringLiteral("toolbarAI"), QStringLiteral("toolbarReference")};
        for (const QString &property : metricProperties)
            originalMetrics.insert(property, settings->property(property.toUtf8().constData()));
        auto restore = qScopeGuard([&] {
            if (!restoreSettings) return;
            restoreSettings->setProperty("toolbarMode", originalMode);
            for (auto it = originalMetrics.cbegin(); it != originalMetrics.cend(); ++it)
                restoreSettings->setProperty(it.key().toUtf8().constData(), it.value());
        });

        const QString markdown = QString::fromUtf8(
            "**Alpha café.** Beta gamma!\n\n- [ ] Task one\n1. [x] Done two\n\n"
            "~~~md\n- [ ] hidden task\n~~~\n\n最後 sentence?");
        editor->setProperty("text", markdown);
        QTRY_COMPARE(backend.documentStatistics(markdown).value("words").toInt(), 12);
        QCOMPARE(backend.documentStatistics(markdown).value("sentences").toInt(), 3);
        QCOMPARE(backend.documentStatistics(markdown).value("tasks").toInt(), 2);
        QCOMPARE(backend.documentStatistics(markdown).value("readingMinutes").toInt(), 1);
        QCOMPARE(backend.documentStatistics(markdown).value("speakingMinutes").toInt(), 1);
        const auto unicode = backend.documentStatistics(QString::fromUtf8("**Hi** 😀"));
        QCOMPARE(unicode.value("characters").toInt(), 4);
        QCOMPARE(unicode.value("charactersWithoutSpaces").toInt(), 3);
        QCOMPARE(backend.documentStatistics(QString()).value("speakingMinutes").toInt(), 0);

        const int alpha = markdown.indexOf(QStringLiteral("Alpha"));
        const int cafeEnd = markdown.indexOf(QString::fromUtf8("café")) + QString::fromUtf8("café").size();
        const int beta = markdown.indexOf(QStringLiteral("Beta"));
        const int gammaEnd = markdown.indexOf(QStringLiteral("gamma")) + 5;
        const int task = markdown.indexOf(QStringLiteral("Task one"));
        backend.markAuthorship(alpha, cafeEnd, QStringLiteral("Human"), QStringLiteral("Writer"));
        backend.markAuthorship(beta, gammaEnd, QStringLiteral("AI"), QStringLiteral("Assistant"));
        backend.markAuthorship(task, task + 8, QStringLiteral("Reference"), QStringLiteral("Source"));
        const int done = markdown.indexOf(QStringLiteral("Done"));
        backend.markAuthorship(done, done + 2, QStringLiteral("Reference"), QStringLiteral("Partial source"));
        const auto annotated = backend.documentStatistics(markdown);
        QCOMPARE(annotated.value("humanWords").toInt(), 2);
        QCOMPARE(annotated.value("aiWords").toInt(), 2);
        QCOMPARE(annotated.value("referenceWords").toInt(), 2);
        QCOMPARE(backend.documentStatistics(markdown + QStringLiteral(" ")).value("humanWords").toInt(), 0);

        QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("statistics"))));
        QTRY_VERIFY(statisticsDialog->property("opened").toBool());
        const int before = statisticsDialog->property("statistics").toMap().value("words").toInt();
        editor->setProperty("text", markdown + QStringLiteral(" Extra."));
        QTRY_COMPARE(statisticsDialog->property("statistics").toMap().value("words").toInt(), before + 1);
        const int aiBefore = statisticsDialog->property("statistics").toMap().value("aiWords").toInt();
        const int extra = editor->property("text").toString().indexOf(QStringLiteral("Extra"));
        backend.markAuthorship(extra, extra + 5, QStringLiteral("AI"), QStringLiteral("Assistant"));
        QTRY_COMPARE(statisticsDialog->property("statistics").toMap().value("aiWords").toInt(), aiBefore + 1);
        QVERIFY(QMetaObject::invokeMethod(editor, "undo"));
        QTRY_COMPARE(statisticsDialog->property("statistics").toMap().value("aiWords").toInt(), aiBefore);

        settings->setProperty("toolbarMode", 0);
        for (const QString &property : metricProperties) settings->setProperty(property.toUtf8().constData(), true);
        QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("toolbarTasks"))));
        QCOMPARE(settings->property("toolbarMode").toInt(), 1);
        QVERIFY(!settings->property("toolbarTasks").toBool());
        QVERIFY(toolbarStatistic->property("text").toString().contains(QStringLiteral("words")));
        QVERIFY(!toolbarStatistic->property("text").toString().contains(QStringLiteral("tasks")));
#ifdef Q_OS_MACOS
        auto *tasksAction = window->findChild<QObject *>("native_toolbarTasks");
        auto *defaultAction = window->findChild<QObject *>("native_toolbarDefault");
        auto *statsOnlyAction = window->findChild<QObject *>("native_toolbarStatsOnly");
        QVERIFY(tasksAction && defaultAction && statsOnlyAction);
        QVERIFY(!tasksAction->property("checked").toBool());
        QVERIFY(QMetaObject::invokeMethod(tasksAction, "triggered"));
        QVERIFY(tasksAction->property("checked").toBool());
        QCOMPARE(settings->property("toolbarMode").toInt(), 1);
        QVERIFY(statsOnlyAction->property("checked").toBool());
        QVERIFY(QMetaObject::invokeMethod(defaultAction, "triggered"));
        QCOMPARE(settings->property("toolbarMode").toInt(), 0);
        QVERIFY(defaultAction->property("checked").toBool());
        QVERIFY(!statsOnlyAction->property("checked").toBool());
#endif

        settings->setProperty("toolbarMode", 1);
        const bool expectedTasks = settings->property("toolbarTasks").toBool();
        restoreSettings = nullptr;
        window.reset();
        QCoreApplication::processEvents();
        Backend reopenedBackend;
        QQmlEngine reopenedEngine;
        reopenedEngine.rootContext()->setContextProperty("backend", &reopenedBackend);
        QQmlComponent reopenedComponent(&reopenedEngine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> reopened(reopenedComponent.create());
        QVERIFY(reopened);
        auto *reopenedSettings = reopened->findChild<QObject *>("workspaceSettings");
        QVERIFY(reopenedSettings);
        restoreSettings = reopenedSettings;
        QCOMPARE(reopenedSettings->property("toolbarMode").toInt(), 1);
        QCOMPARE(reopenedSettings->property("toolbarTasks").toBool(), expectedTasks);
        reopenedSettings->setProperty("toolbarMode", originalMode);
        for (auto it = originalMetrics.cbegin(); it != originalMetrics.cend(); ++it)
            reopenedSettings->setProperty(it.key().toUtf8().constData(), it.value());
        QVERIFY(QMetaObject::invokeMethod(reopenedSettings, "sync"));
        restoreSettings = nullptr;
        restore.dismiss();
    }

    void chromePresentationModesPersistAndKeepCommandsReachable() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY(window);
        auto *settings = window->findChild<QObject *>("workspaceSettings");
        auto *commands = window->findChild<QObject *>("workspaceCommands");
        auto *chrome = window->findChild<QObject *>("topChrome");
        auto *title = window->findChild<QObject *>("topChromeTitle");
        auto *leading = window->findChild<QObject *>("topChromeToolbarLeading");
        auto *trailing = window->findChild<QObject *>("topChromeToolbarTrailing");
        auto *libraryButton = window->findChild<QObject *>("topChromeLibraryButton");
        auto *editor = window->findChild<QObject *>("sourceEditor");
        QVERIFY(settings && commands && chrome && title && leading && trailing && libraryButton && editor);

        const int originalTitleMode = settings->property("titleBarMode").toInt();
        const int originalToolbarVisibility = settings->property("toolbarVisibilityMode").toInt();
        QObject *restoreSettings = settings;
        auto restore = qScopeGuard([&] {
            if (!restoreSettings) return;
            restoreSettings->setProperty("titleBarMode", originalTitleMode);
            restoreSettings->setProperty("toolbarVisibilityMode", originalToolbarVisibility);
        });

        QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("titleBarFade"))));
        QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("toolbarFade"))));
        QCOMPARE(settings->property("titleBarMode").toInt(), 0);
        QCOMPARE(settings->property("toolbarVisibilityMode").toInt(), 0);
        QVERIFY(leading->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(libraryButton, "forceActiveFocus"));
        QTRY_VERIFY(chrome->property("keyboardReveal").toBool());
        QTRY_COMPARE(title->property("opacity").toReal(), 1.0);
        QTRY_COMPARE(leading->property("opacity").toReal(), 1.0);
        QVERIFY(QMetaObject::invokeMethod(editor, "forceActiveFocus"));
        QTRY_VERIFY(!chrome->property("keyboardReveal").toBool());

        QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("titleBarAlways"))));
        QTRY_COMPARE(title->property("opacity").toReal(), 1.0);
        QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("toolbarHide"))));
        QVERIFY(!leading->property("visible").toBool());
        QVERIFY(!trailing->property("visible").toBool());
        QCOMPARE(chrome->property("height").toInt(), 44);
        QVERIFY(title->property("visible").toBool());

#ifdef Q_OS_MACOS
        auto native = [&](const char *id) { return window->findChild<QObject *>(QStringLiteral("native_") + id); };
        QVERIFY(native("titleBarAlways") && native("titleBarAlways")->property("checked").toBool());
        QVERIFY(native("toolbarHide") && native("toolbarHide")->property("checked").toBool());
        QVERIFY(native("outline") && native("outline")->property("enabled").toBool());
        QVERIFY(QMetaObject::invokeMethod(native("toolbarAlways"), "triggered"));
        QCOMPARE(settings->property("toolbarVisibilityMode").toInt(), 1);
        QVERIFY(native("toolbarAlways")->property("checked").toBool());
#else
        QVERIFY(QMetaObject::invokeMethod(commands, "run", Q_ARG(QVariant, QStringLiteral("toolbarAlways"))));
#endif
        QVERIFY(leading->property("visible").toBool());
        QTRY_COMPARE(leading->property("opacity").toReal(), 1.0);

        settings->setProperty("titleBarMode", 0);
        settings->setProperty("toolbarVisibilityMode", 2);
        restoreSettings = nullptr;
        window.reset();
        QCoreApplication::processEvents();
        Backend reopenedBackend;
        QQmlEngine reopenedEngine;
        reopenedEngine.rootContext()->setContextProperty("backend", &reopenedBackend);
        QQmlComponent reopenedComponent(&reopenedEngine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QScopedPointer<QObject> reopened(reopenedComponent.create());
        QVERIFY(reopened);
        auto *reopenedSettings = reopened->findChild<QObject *>("workspaceSettings");
        QVERIFY(reopenedSettings);
        restoreSettings = reopenedSettings;
        QCOMPARE(reopenedSettings->property("titleBarMode").toInt(), 0);
        QCOMPARE(reopenedSettings->property("toolbarVisibilityMode").toInt(), 2);
        reopenedSettings->setProperty("titleBarMode", originalTitleMode);
        reopenedSettings->setProperty("toolbarVisibilityMode", originalToolbarVisibility);
        QVERIFY(QMetaObject::invokeMethod(reopenedSettings, "sync"));
        restoreSettings = nullptr;
        restore.dismiss();
    }

    void organizesShortcutsAndSortsWithoutMovingFiles() {
        QTemporaryDir directory;
        FileLibrary library;
        library.setRootFolder(QUrl::fromLocalFile(directory.path()));
        const QUrl root = library.rootFolder();
        const QUrl alpha = library.createDocument("Alpha.md");
        const QUrl beta = library.createDocument("Beta.txt");
        library.toggleFavorite(alpha);
        library.recordRecentFile(alpha);
        library.recordRecentFile(beta);
        library.recordRecentFile(alpha);
        QCOMPARE(library.recentFiles().first().toMap().value("url").toUrl(), alpha);
        int matches = 0;
        for (const auto &entry : library.recentFiles()) if (entry.toMap().value("url").toUrl() == alpha) ++matches;
        QCOMPARE(matches, 1);
        library.setSortMode(0);
        library.setAscending(false);
        QCOMPARE(library.entries().first().toMap().value("name").toString(), QString("Beta.txt"));
        {
            FileLibrary restored;
            QCOMPARE(restored.ascending(), false);
            QVERIFY(restored.favorites().contains(library.favorites().last()));
        }
        library.removeLocation(root);
        QVERIFY(QFileInfo::exists(alpha.toLocalFile()));
        QVERIFY(QFileInfo::exists(beta.toLocalFile()));
        QVERIFY(library.rootFolder().isEmpty());
        library.toggleFavorite(alpha);
        library.clearRecentFiles();
        QVERIFY(library.recentFiles().isEmpty());
        library.setAscending(true);
    }

    void headingAndFencePreviewMatchesSource() {
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY(window);
        auto *editor = window->findChild<QObject *>("sourceEditor");
        auto *preview = window->findChild<QObject *>("renderedPreview");
        const QString markdown = "#title\n\n# title\n\n## Second\n\n```md\n# Literal\n**Stars**\n```\n";
        editor->setProperty("text", markdown);
        auto *quick = qvariant_cast<QQuickTextDocument *>(preview->property("textDocument"));
        QVERIFY(quick);
        QTRY_VERIFY(quick->textDocument()->toPlainText().contains("# Literal"));
        auto *doc = quick->textDocument();
        QCOMPARE(doc->firstBlock().text(), QString("#title"));
        QCOMPARE(doc->firstBlock().blockFormat().headingLevel(), 0);
        bool heading = false, literal = false;
        for (auto block = doc->begin(); block.isValid(); block = block.next()) {
            if (block.text() == "title") { QCOMPARE(block.blockFormat().headingLevel(), 1); heading = true; }
            if (block.text() == "# Literal") { QCOMPARE(block.blockFormat().headingLevel(), 0); literal = true; }
        }
        QVERIFY(heading && literal);
        QCOMPARE(editor->property("text").toString(), markdown);
        backend.setShowMarkup(false);
        QVERIFY(backend.hiddenRangesAt(markdown.indexOf("**Stars**")).isEmpty());
        QTextDocument source;
        source.setPlainText("#title\n# title\n```\n# Literal\n**Stars**\n```\n  # Indented");
        MarkdownHighlighter highlighter(&source);
        highlighter.rehighlight();
        QVERIFY(source.firstBlock().layout()->formats().isEmpty());
        for (auto block = source.findBlockByNumber(3); block.blockNumber() <= 4; block = block.next())
            for (const auto &range : block.layout()->formats()) {
                QVERIFY(range.format.fontWeight() != QFont::Bold);
                QVERIFY(range.format.fontPointSize() != 1.0);
            }
        QVERIFY(!source.lastBlock().layout()->formats().isEmpty());
        backend.discardRecovery();
    }

    void countsWords() {
        QCOMPARE(Backend::countWords(QStringLiteral("one two-three don't 42")), 4);
        QCOMPARE(Backend::countWords(QStringLiteral("你好 世界")), 2);
        QCOMPARE(Backend::countWords(QString()), 0);
    }

    void normalizesLinks() {
        QCOMPARE(Backend::normalizedLinkUrl(QStringLiteral("www.example.com/path")),
                 QStringLiteral("https://www.example.com/path"));
        QCOMPARE(Backend::normalizedLinkUrl(QStringLiteral("mailto:writer@example.com")),
                 QStringLiteral("mailto:writer@example.com"));
        QVERIFY(Backend::normalizedLinkUrl(QStringLiteral("example.com")).isEmpty());
        QVERIFY(Backend::normalizedLinkUrl(QStringLiteral("file:///tmp/private")).isEmpty());
    }

    void suggestsSafeNames() {
        QCOMPARE(Backend::suggestedFileName(QStringLiteral("My first draft\nBody")),
                 QStringLiteral("My first draft.md"));
        QCOMPARE(Backend::suggestedFileName(QStringLiteral("A/B")), QStringLiteral("A-B.md"));
        QCOMPARE(Backend::suggestedFileName(QString()), QStringLiteral("Untitled.md"));
        QCOMPARE(Backend::suggestedFileName(QStringLiteral("Already.md")),
                 QStringLiteral("Already.md"));
    }

    void findsInlineMarkdownRanges() {
        const auto markup = MarkdownHighlighter::inlineMarkup(
            QStringLiteral("**bold** and *italic* and [site](https://example.com)"));
        QCOMPARE(markup.size(), 3);
        QCOMPARE(markup.at(0).content.start, 2);
        QCOMPARE(markup.at(0).content.length, 4);
        QCOMPARE(markup.at(2).content.length, 4);
        QCOMPARE(markup.at(2).markers[0].length, 1);
    }

    void loadsCurrentOmarchyTheme() {
        QTemporaryDir homeDirectory;
        QVERIFY(homeDirectory.isValid());

        const QByteArray originalHome = qgetenv("HOME");
        struct HomeRestorer {
            QByteArray value;
            ~HomeRestorer() { qputenv("HOME", value); }
        } restoreHome{originalHome};
        QVERIFY(qputenv("HOME", homeDirectory.path().toUtf8()));

        const QString themeDirectory = homeDirectory.path()
            + QStringLiteral("/.local/state/omarchy/current/theme");
        QVERIFY(QDir().mkpath(themeDirectory));

        QFile colorsFile(themeDirectory + QStringLiteral("/colors.toml"));
        QVERIFY(colorsFile.open(QIODevice::WriteOnly | QIODevice::Text));
        const QByteArray palette(
            "mode = \"light\"\n"
            "accent = \"#112233\"\n"
            "selection = \"#445566\"\n"
            "background = \"#fefefe\"\n"
            "foreground = \"#101010\"\n");
        QCOMPARE(colorsFile.write(palette), qint64(palette.size()));
        colorsFile.close();

        Backend backend;
        QCOMPARE(backend.themeBackground(), QStringLiteral("#fefefe"));
        QCOMPARE(backend.themeForeground(), QStringLiteral("#101010"));
        QCOMPARE(backend.themeAccent(), QStringLiteral("#112233"));
        QCOMPARE(backend.themeSelection(), QStringLiteral("#445566"));
        QVERIFY(!backend.darkMode());
    }

    void ignoresFileWatcherEventsForSavedContents() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString path = directory.filePath(QStringLiteral("first-save.md"));
        Backend backend;
        QSignalSpy externalChangeSpy(&backend, &Backend::externalChangeDetected);

        backend.saveAs(QUrl::fromLocalFile(path));
        QVERIFY(QFileInfo::exists(path));

        QFile sameContents(path);
        QVERIFY(sameContents.open(QIODevice::WriteOnly | QIODevice::Truncate));
        sameContents.close();
        QTest::qWait(100);
        QCOMPARE(externalChangeSpy.count(), 0);

        QFile changedContents(path);
        QVERIFY(changedContents.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(changedContents.write("changed elsewhere"), qint64(17));
        changedContents.close();
        QTRY_COMPARE(externalChangeSpy.count(), 1);
    }

    void keepsCursorAndSelectionStableAcrossInsertions() {
        const QString mutationsPath = QFINDTESTDATA("../src/EditorMutations.js");
        QVERIFY(!mutationsPath.isEmpty());

        QQmlEngine engine;
        QQmlComponent component(&engine);
        const QByteArray harness = R"QML(
            import QtQuick
            import "EditorMutations.js" as EditorMutations

            TextEdit {
                property string insertionText
                property int insertionCursor
                property string wrappedText
                property int wrappedSelectionStart
                property int wrappedSelectionEnd

                Component.onCompleted: {
                    text = "alpha omega";
                    cursorPosition = 5;
                    EditorMutations.replaceRange(this, 5, 5, "one\r\ntwo");
                    insertionText = text;
                    insertionCursor = cursorPosition;

                    text = "alpha beta omega";
                    select(6, 10);
                    EditorMutations.replaceRange(this, selectionStart, selectionEnd,
                                                 "**beta**", 2, 6);
                    wrappedText = text;
                    wrappedSelectionStart = selectionStart;
                    wrappedSelectionEnd = selectionEnd;
                }
            }
        )QML";
        const QUrl harnessUrl = QUrl::fromLocalFile(
            QFileInfo(mutationsPath).absolutePath() + QStringLiteral("/MutationHarness.qml"));
        component.setData(harness, harnessUrl);
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> editor(component.create());
        QVERIFY2(editor, qPrintable(component.errorString()));

        QCOMPARE(editor->property("insertionText").toString(),
                 QStringLiteral("alphaone\ntwo omega"));
        QCOMPARE(editor->property("insertionCursor").toInt(), 12);
        QCOMPARE(editor->property("wrappedText").toString(),
                 QStringLiteral("alpha **beta** omega"));
        QCOMPARE(editor->property("wrappedSelectionStart").toInt(), 8);
        QCOMPARE(editor->property("wrappedSelectionEnd").toInt(), 12);
    }

    void savesAndOpensFromFooterMenu() {
        const QString mainQmlPath = QFINDTESTDATA("../src/Main.qml");
        QVERIFY(!mainQmlPath.isEmpty());

        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(mainQmlPath));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));

        QVERIFY(window->findChild<QObject *>(QStringLiteral("sourceEditor")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("renderedPreview")));
        QVERIFY(!window->findChild<QObject *>(QStringLiteral("modeToggle")));

        QObject *saveButton = window->findChild<QObject *>(QStringLiteral("saveButton"));
        QObject *openButton = window->findChild<QObject *>(QStringLiteral("openButton"));
        QVERIFY(saveButton);
        QVERIFY(openButton);

        QSignalSpy saveDialogSpy(&backend, &Backend::saveDialogRequested);
        QVERIFY(QMetaObject::invokeMethod(saveButton, "triggered"));
        QCOMPARE(saveDialogSpy.count(), 1);

        QSignalSpy openDialogSpy(&backend, &Backend::openDialogRequested);
        QVERIFY(QMetaObject::invokeMethod(openButton, "triggered"));
        QCOMPARE(openDialogSpy.count(), 1);
    }

    void scalesTextWithDesktopTextSize() {
        const QString mainQmlPath = QFINDTESTDATA("../src/Main.qml");
        QVERIFY(!mainQmlPath.isEmpty());

        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(mainQmlPath));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QVERIFY(editor);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 16);

        // `omarchy display text size 16` sets the GNOME factor to 16/12.
        backend.setTextScale(16.0 / 12.0);
        QCOMPARE(window->property("editorFontPixelSize").toInt(), 21);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 21);

        backend.setTextScale(9.0 / 12.0);
        QCOMPARE(window->property("editorFontPixelSize").toInt(), 12);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 12);
    }

    void preservesMarkdownAndProtectsUnsavedOpen() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(
            QFINDTESTDATA("../src/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QVERIFY(editor);

        const QString markdown = QString::fromUtf8(
            "# Draft\n\n**Bold** and *italic* — café 你好\n\n- One\n- Two\n");
        QVERIFY(editor->setProperty("text", markdown));
        QVERIFY(backend.modified());
        const QUrl saved = QUrl::fromLocalFile(directory.filePath("draft.md"));
        backend.saveAs(saved);
        QVERIFY(!backend.modified());
        QFile file(saved.toLocalFile());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), markdown.toUtf8());
        file.close();

        QVERIFY(editor->setProperty("text", QStringLiteral("Unsaved work")));
        QVERIFY(backend.modified());
        QVERIFY(QMetaObject::invokeMethod(window.data(), "requestOpen",
            Q_ARG(QVariant, QVariant::fromValue(saved))));
        QCOMPARE(editor->property("text").toString(), QStringLiteral("Unsaved work"));
        QCOMPARE(window->property("pendingAction").toString(), QStringLiteral("open"));
        // Cancelling a pending Save As must never complete the document switch.
        backend.fileDialogCanceled();
        QCOMPARE(editor->property("text").toString(), QStringLiteral("Unsaved work"));
        backend.open(saved);
        QCOMPARE(editor->property("text").toString(), markdown);
        QVERIFY(!backend.modified());
        backend.discardRecovery();
    }

    void remembersLastSaveDirectory() {
        QTemporaryDir saveDirectory;
        QVERIFY(saveDirectory.isValid());

        const QString savedPath = saveDirectory.filePath(QStringLiteral("first.md"));
        Backend savedDocument;
        savedDocument.saveAs(QUrl::fromLocalFile(savedPath));

        Backend nextDocument;
        QSignalSpy saveDialogSpy(&nextDocument, &Backend::saveDialogRequested);
        nextDocument.saveAsDialog();
        QCOMPARE(saveDialogSpy.count(), 1);

        const QUrl suggestedUrl = saveDialogSpy.takeFirst().constFirst().toUrl();
        QCOMPARE(QFileInfo(suggestedUrl.toLocalFile()).absolutePath(),
                 saveDirectory.path());
        QCOMPARE(QFileInfo(suggestedUrl.toLocalFile()).fileName(),
                 QStringLiteral("Untitled.md"));

        QSettings().setValue(QStringLiteral("file/lastSaveDirectory"),
                             saveDirectory.filePath(QStringLiteral("missing")));
        Backend fallbackDocument;
        QSignalSpy fallbackDialogSpy(&fallbackDocument, &Backend::saveDialogRequested);
        fallbackDocument.saveAsDialog();
        const QUrl fallbackUrl = fallbackDialogSpy.takeFirst().constFirst().toUrl();
        QCOMPARE(QFileInfo(fallbackUrl.toLocalFile()).absolutePath(), QDir::homePath());
    }

private:
    QTemporaryDir m_settingsDirectory;
};

QTEST_MAIN(OmawriteTest)
#include "tst_omawrite.moc"
