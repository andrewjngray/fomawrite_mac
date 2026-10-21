// Cycle 152: the Source writing appearance "Omawrite" (IBM Plex Mono, 20 px, 140% lines, its own palette).
namespace {
QTextDocument *cycle152Document(Cycle117Window &fixture) {
    auto *quick = qvariant_cast<QQuickTextDocument *>(fixture.editor->property("textDocument"));
    return quick ? quick->textDocument() : nullptr;
}
QTextCharFormat cycle152FormatAt(QTextDocument *document, int position) {
    const auto block = document->findBlock(position);
    for (const auto &range : block.layout()->formats())
        if (position - block.position() >= range.start
                && position - block.position() < range.start + range.length)
            return range.format;
    return {};
}
bool cycle152RegisterPlexMono() {
    QStringList families;
    for (const QString &face : {QStringLiteral("Regular"), QStringLiteral("Italic"), QStringLiteral("Bold"), QStringLiteral("BoldItalic")}) {
        const int id = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/IBMPlexMono-") + face + QStringLiteral(".ttf"));
        if (id < 0) return false;
        families += QFontDatabase::applicationFontFamilies(id);
    }
    return families.contains(QStringLiteral("IBM Plex Mono"));
}
QColor cycle152Colour(QObject *object, const char *property) { return qvariant_cast<QColor>(object->property(property)); }
// Repeater and ListView delegates are visual children only; findChildren does not reach them.
QList<QQuickItem *> cycle152VisualItems(QQuickItem *root, const char *name) {
    QList<QQuickItem *> found;
    if (!root) return found;
    if (root->objectName() == QLatin1String(name)) found << root;
    for (auto *child : root->childItems()) found << cycle152VisualItems(child, name);
    return found;
}
}

void FomawriteTest::omawriteSourcePresetIsInTheMenuAndSelectsPlexMono() {
    Cycle103SettingsSnapshot isolatedSettings;
    QVERIFY2(cycle152RegisterPlexMono(), "IBM Plex Mono is not bundled in the resources");
    Cycle117Window f; QVERIFY2(f.start(1280), qPrintable(f.error));
    auto *settings = f.window->findChild<QObject *>("workspaceSettings");
    auto *menu = f.window->findChild<QObject *>("sourceAppearanceMenu");
    auto *appearance = f.item("sourceAppearanceButton");
    auto *zoom = f.window->findChild<QObject *>("paneZoomState");
    QVERIFY(settings && menu && appearance && zoom);
    QVERIFY(f.mode(0));
    QVERIFY(f.window->findChild<QQuickItem *>("sourceAppearanceOmawrite"));
    QCOMPARE(f.window->findChild<QQuickItem *>("sourceAppearanceOmawrite")->property("text").toString(), QString("Omawrite"));
    QVERIFY(cycle102Click(f.quick, appearance)); QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(cycle106MenuFits(f.quick, menu, appearance));
    QVERIFY(cycle102Click(f.quick, f.window->findChild<QQuickItem *>("sourceAppearanceOmawrite")));
    QTRY_COMPARE(settings->property("writingAppearance").toString(), QString("omawrite"));
    QCOMPARE(f.window->property("activeWritingAppearance").toString(), QString("omawrite"));
    QVERIFY(f.window->findChild<QQuickItem *>("sourceAppearanceOmawrite")->property("checked").toBool());
    QVERIFY(!f.window->findChild<QQuickItem *>("sourceAppearanceManuscript")->property("checked").toBool());
    QTRY_COMPARE(f.editor->property("font").value<QFont>().family(), QString("IBM Plex Mono"));
    // 20 px at 100%, scaled by the desktop text scale; Larger/Smaller text keep working.
    const qreal scale = f.window->property("textScale").toReal();
    QTRY_COMPARE(f.editor->property("font").value<QFont>().pixelSize(), qMax(1, qRound(20 * scale)));
    QVERIFY(QMetaObject::invokeMethod(zoom, "setZoom", Q_ARG(QVariant, QStringLiteral("source")), Q_ARG(QVariant, 150)));
    QTRY_COMPARE(f.editor->property("font").value<QFont>().pixelSize(), qMax(1, qRound(30 * scale)));
    QVERIFY(QMetaObject::invokeMethod(zoom, "resetZoom", Q_ARG(QVariant, QStringLiteral("source"))));
    QTRY_COMPARE(f.editor->property("font").value<QFont>().pixelSize(), qMax(1, qRound(20 * scale)));
    // The footer names the preset.
    QCOMPARE(f.footer->property("writingAppearance").toString(), QString("Omawrite"));
    // The native Writing Appearance menu and the command table carry it too.
    settings->setProperty("writingAppearance", "manuscript");
    QVERIFY(testDesktopMenusMatchPlatform(f.window.data()));
    auto *native = testDesktopCommand(f.window.data(), QStringLiteral("writingOmawrite"));
    QVERIFY(native);
    QCOMPARE(native->property("text").toString(), QString("Omawrite (Mono)"));
    QVERIFY(!native->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(native, "triggered"));
    QTRY_COMPARE(settings->property("writingAppearance").toString(), QString("omawrite"));
    QTRY_VERIFY(native->property("checked").toBool());
}

void FomawriteTest::omawriteSourceUsesItsPaletteInDarkAndLightAndManuscriptComesBack() {
    Cycle103SettingsSnapshot isolatedSettings;
    QVERIFY2(cycle152RegisterPlexMono(), "IBM Plex Mono is not bundled in the resources");
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const QString source = QString::fromUtf8("# A heading\n\nPlain words with a [link](https://example.com) and `code`.\n\n> A quote\n\n```js\nconst value = 42;\n```\n");
    const QString path = directory.filePath("Omawrite.md");
    QVERIFY(cycle103Write(path, source.toUtf8()));
    Cycle117Window f; QVERIFY2(f.start(1280), qPrintable(f.error));
    QVERIFY(f.backend.open(QUrl::fromLocalFile(path)));
    const auto cleanup = qScopeGuard([&] { f.backend.discardRecovery(); });
    auto *settings = f.window->findChild<QObject *>("workspaceSettings");
    auto *pane = f.window->findChild<QObject *>("editorPane");
    auto *document = cycle152Document(f);
    QVERIFY(settings && pane && document);
    QVERIFY(f.mode(0));
    // The Manuscript baseline to come back to.
    f.backend.setThemePreset("light");
    settings->setProperty("writingAppearance", "manuscript");
    QCoreApplication::processEvents();
    const QColor manuscriptPage = cycle152Colour(pane, "color");
    const QColor manuscriptText = cycle152Colour(f.editor, "color");
    const QColor manuscriptSelection = cycle152Colour(f.editor, "selectionColor");
    const QString manuscriptFamily = f.editor->property("font").value<QFont>().family();
    QCOMPARE(manuscriptPage, QColor(f.backend.palette().value("editor").toString()));
    QCOMPARE(manuscriptFamily, QString("iA Writer Mono S"));

    const int headingMarker = 0, headingText = source.indexOf("A heading");
    const int linkText = source.indexOf("link]") ;
    const int quoteText = source.indexOf("A quote");
    struct Palette { const char *theme; QString background, text, marker, link, selection, selectedText, panel; };
    for (const Palette &p : {Palette{"dark", "#101010", "#eeeeee", "#4f525a", "#5584aa", "#186a9a", "#eeeeee", "#1d1d1d"},
                             Palette{"light", "#ffffff", "#222324", "#aeb1b5", "#2077b2", "#2077b2", "#ffffff", "#f2f2f2"}}) {
        f.backend.setThemePreset(p.theme);
        settings->setProperty("writingAppearance", "omawrite");
        QTRY_COMPARE(f.editor->property("font").value<QFont>().family(), QString("IBM Plex Mono"));
        QCoreApplication::processEvents();
        QTRY_COMPARE(cycle152Colour(pane, "color"), QColor(p.background));
        QCOMPARE(cycle152Colour(f.editor, "color"), QColor(p.text));
        QCOMPARE(cycle152Colour(f.editor, "selectionColor"), QColor(p.selection));
        QCOMPARE(cycle152Colour(f.editor, "selectedTextColor"), QColor(p.selectedText));
        QCOMPARE(cycle152Colour(f.window.data(), "sourceTextColor"), QColor(p.text));
        // Every block is 140% of the font's natural line spacing, without extra paragraph spacing.
        for (auto block = document->begin(); block.isValid(); block = block.next()) {
            QCOMPARE(block.blockFormat().lineHeightType(), int(QTextBlockFormat::ProportionalHeight));
            QCOMPARE(block.blockFormat().lineHeight(), qreal(140));
            QCOMPARE(block.blockFormat().topMargin(), qreal(0));
            QCOMPARE(block.blockFormat().bottomMargin(), qreal(0));
            // Markers stay inline (no Manuscript gutter): headings and paragraphs are flush left; only
            // quote/list soft wraps hang, exactly as in Editorial and Book.
            if (!block.text().startsWith('>')) {
                QCOMPARE(block.blockFormat().leftMargin(), qreal(0));
                QCOMPARE(block.blockFormat().textIndent(), qreal(0));
            }
        }
        // Markers: dim, same size. Heading text: text colour, bold, body size.
        const auto marker = cycle152FormatAt(document, headingMarker);
        QCOMPARE(marker.foreground().color(), QColor(p.marker));
        QCOMPARE(marker.fontPointSize(), qreal(0));
        QCOMPARE(marker.fontWeight(), QFont::Normal);
        const auto heading = cycle152FormatAt(document, headingText);
        QCOMPARE(heading.foreground().color(), QColor(p.text));
        QCOMPARE(heading.fontWeight(), int(QFont::Bold));
        QCOMPARE(heading.fontPointSize(), qreal(0));            // no heading scale: it inherits the body size
        QVERIFY(!heading.hasProperty(QTextFormat::FontPixelSize));
        // Links: accent colour, underlined. Quotes: marker colour.
        const auto link = cycle152FormatAt(document, linkText);
        QCOMPARE(link.foreground().color(), QColor(p.link));
        QVERIFY(link.fontUnderline());
        QCOMPARE(cycle152FormatAt(document, quoteText).foreground().color(), QColor(p.marker));
        QCOMPARE(cycle152FormatAt(document, source.indexOf("> A quote")).foreground().color(), QColor(p.marker));
        // Code: inline code and fenced blocks sit on the code panel colour.
        const int inlineCode = source.indexOf("`code`");
        QCOMPARE(cycle152FormatAt(document, inlineCode + 1).background().color(), QColor(p.panel));
        QCOMPARE(document->findBlock(source.indexOf("const value")).blockFormat().background().color(), QColor(p.panel));
        // The panel actually drawn behind a fenced block (a QML rectangle, not the block format) matches.
        QTRY_VERIFY([&] {
            for (auto *panel : cycle152VisualItems(f.quick->contentItem(), "sourceCodeBlockBackground"))
                if (panel->isVisible()) return cycle152Colour(panel, "color") == QColor(p.panel);
            return false;
        }());
        // The header band above the page and the footer strip below it match the page.
        auto *header = f.window->findChild<QQuickItem *>("documentHeader"); QVERIFY(header);
        QTRY_COMPARE(cycle152Colour(header, "color"), QColor(p.background));
        auto *footer = f.window->findChild<QObject *>("documentFooter"); QVERIFY(footer);
        QTRY_COMPARE(footer->property("sourceSurfaceColor").value<QColor>(), QColor(p.background));
        QCOMPARE(document->toPlainText(), source);
        QVERIFY(!f.backend.modified()); QVERIFY(!document->isUndoAvailable());
    }

    // Back to Manuscript: the previous colours and font return.
    f.backend.setThemePreset("light");
    settings->setProperty("writingAppearance", "manuscript");
    QTRY_COMPARE(f.editor->property("font").value<QFont>().family(), manuscriptFamily);
    QTRY_COMPARE(cycle152Colour(pane, "color"), manuscriptPage);
    QCOMPARE(cycle152Colour(f.editor, "color"), manuscriptText);
    QCOMPARE(cycle152Colour(f.editor, "selectionColor"), manuscriptSelection);
    QCOMPARE(cycle152Colour(f.editor, "selectedTextColor"), QColor("#ffffff"));
    QTRY_COMPARE(document->findBlockByNumber(0).blockFormat().lineHeight(), qreal(155));
    QVERIFY(cycle152FormatAt(document, 0).foreground().color() != QColor("#aeb1b5"));
    QVERIFY(document->findBlock(source.indexOf("const value")).blockFormat().background().color() != QColor("#f2f2f2"));
}

void FomawriteTest::omawriteSourceKeepsAtMostSixtyFiveCharactersAndCentres() {
    Cycle103SettingsSnapshot isolatedSettings;
    QVERIFY2(cycle152RegisterPlexMono(), "IBM Plex Mono is not bundled in the resources");
    Cycle117Window f; QVERIFY2(f.start(1600), qPrintable(f.error));
    auto *settings = f.window->findChild<QObject *>("workspaceSettings");
    auto *pane = f.window->findChild<QQuickItem *>("editorPane");
    auto *zoom = f.window->findChild<QObject *>("paneZoomState");
    auto *scroll = f.item("editorScroll");
    QVERIFY(settings && pane && zoom && scroll);
    QVERIFY(f.mode(0));
    settings->setProperty("writingAppearance", "omawrite");
    QTRY_COMPARE(f.editor->property("font").value<QFont>().family(), QString("IBM Plex Mono"));
    const auto character = [&] {
        return QFontMetricsF(f.editor->property("font").value<QFont>()).horizontalAdvance(QLatin1Char('0'));
    };
    const auto widthInCharacters = [&] { return f.editor->width() / character(); };
    // min(65 characters, pane - 20 characters, pane - 48), centred.
    const auto expectedWidth = [&] {
        return qMax(180.0, qreal(qFloor(qMin(65 * character(), qMin(pane->width() - 20 * character(), pane->width() - 48)))));
    };
    QTRY_VERIFY(pane->width() > 800);
    QTRY_COMPARE(qRound(f.editor->width()), qRound(expectedWidth()));
    QVERIFY(widthInCharacters() <= 65.0 + 1e-6);
    QVERIFY(qAbs(f.editor->x() - (scroll->width() - f.editor->width()) / 2) <= 1.0);
    // Smaller text on a wide pane reaches the 65-character cap exactly.
    QVERIFY(QMetaObject::invokeMethod(zoom, "setZoom", Q_ARG(QVariant, QStringLiteral("source")), Q_ARG(QVariant, 75)));
    QTRY_VERIFY(qAbs(widthInCharacters() - 65.0) < 0.1);
    QVERIFY(qAbs(f.editor->x() - (scroll->width() - f.editor->width()) / 2) <= 1.0);
    // Larger text follows the same rule and never exceeds 65 characters.
    QVERIFY(QMetaObject::invokeMethod(zoom, "setZoom", Q_ARG(QVariant, QStringLiteral("source")), Q_ARG(QVariant, 130)));
    QTRY_COMPARE(qRound(f.editor->width()), qRound(expectedWidth()));
    QVERIFY(widthInCharacters() <= 65.0 + 1e-6);
    QVERIFY(QMetaObject::invokeMethod(zoom, "resetZoom", Q_ARG(QVariant, QStringLiteral("source"))));
    // A narrow pane: pane width minus 20 characters (never wider than the pane less its margins).
    f.quick->resize(640, 620);
    QTRY_VERIFY(pane->width() < 700);
    QTRY_COMPARE(qRound(f.editor->width()), qRound(expectedWidth()));
    QVERIFY(f.editor->width() <= qMax(180.0, pane->width() - 20 * character()) + 1.0);
    QVERIFY(f.editor->width() <= pane->width() - 48 + 1.0);
    QVERIFY(widthInCharacters() <= 65.0 + 1e-6);
    QVERIFY(qAbs(f.editor->x() - (scroll->width() - f.editor->width()) / 2) <= 1.0);
}
