#include <QtTest>
#include <QFont>
#include <QTextBlock>
#include <QTextLayout>
#include <QTextDocument>
#include <QQuickTextDocument>
#include <QImage>
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
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        QStandardPaths::setTestModeEnabled(true);
        QQuickStyle::setStyle(QStringLiteral("Material"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           m_settingsDirectory.path());
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
