// Cycle 162 — PDF links and integration: #page=N links from Source, Live and
// the publishing Preview; Copy as Markdown Quote and the page's context menu;
// the Contents sidebar; checkpoint, restore and crash recovery of PDF
// windows; ownership across windows; rename, move and trash of a PDF with its
// notes; the Open dialog, drops and Open With. Fixtures: tests/pdffixtures.h,
// Cycle160Fixtures and the cycle 161 helpers.

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLockFile>
#include <QXmlStreamReader>
#ifdef Q_OS_MACOS
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {
// Four pages with a two-level outline: Part One (page 1) › Section A (page 2),
// Part Two (page 3) › Section B (page 4).
QByteArray cycle162OutlinedDocument() {
    QList<PdfFixtures::Page> pages;
    for (int n = 0; n < 4; ++n) pages.append(PdfFixtures::Page{PdfFixtures::wordsContent(), 0, {}, {}});
    return PdfFixtures::document(pages, [](PdfFixtures::Writer &pdf, const QList<int> &ids) {
        const int root = pdf.reserve(), one = pdf.reserve(), a = pdf.reserve(), two = pdf.reserve(), b = pdf.reserve();
        const auto ref = [](int id) { return QByteArray::number(id) + " 0 R"; };
        const auto dest = [&](int page) { return " /Dest [" + ref(ids.at(page)) + " /XYZ 0 792 0]"; };
        pdf.set(root, "<< /Type /Outlines /First " + ref(one) + " /Last " + ref(two) + " /Count 4 >>");
        pdf.set(one, "<< /Title (Part One) /Parent " + ref(root) + " /Next " + ref(two) + " /First " + ref(a) + " /Last " + ref(a) + " /Count 1" + dest(0) + " >>");
        pdf.set(a, "<< /Title (Section A) /Parent " + ref(one) + dest(1) + " >>");
        pdf.set(two, "<< /Title (Part Two) /Parent " + ref(root) + " /Prev " + ref(one) + " /First " + ref(b) + " /Last " + ref(b) + " /Count 1" + dest(2) + " >>");
        pdf.set(b, "<< /Title (Section B) /Parent " + ref(two) + dest(3) + " >>");
        return "/Outlines " + ref(root);
    });
}

// Recovery snapshots nobody holds (left by earlier tests) would be claimed
// before the one a test writes; live windows keep theirs locked.
void cycle162ClearOrphanSnapshots() {
    QDir data(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    for (const QString &name : data.entryList({QStringLiteral("recovery-*.json")}, QDir::Files)) {
        QLockFile lock(data.filePath(name.chopped(5) + QStringLiteral(".lock")));
        if (lock.tryLock(0)) { data.remove(name); lock.unlock(); }
    }
}
bool cycle162WriteSnapshot(const QJsonObject &snapshot) {
    QDir data(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    data.mkpath(QStringLiteral("."));
    for (int slot = 0; slot < 100; ++slot) {
        const QString path = data.filePath(QStringLiteral("recovery-%1.json").arg(slot));
        if (QFileInfo::exists(path)) continue;
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(QJsonDocument(snapshot).toJson()) > 0;
    }
    return false;
}
// The snapshot (on disk, by any window) whose text is `text`.
QJsonObject cycle162Snapshot(const QString &text) {
    QDir data(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    for (const QString &name : data.entryList({QStringLiteral("recovery-*.json")}, QDir::Files)) {
        QFile file(data.filePath(name));
        if (!file.open(QIODevice::ReadOnly)) continue;
        const QJsonObject object = QJsonDocument::fromJson(file.readAll()).object();
        if (object.value(QStringLiteral("text")).toString() == text) return object;
    }
    return {};
}

// A Backend with a Source document whose recovery snapshot outlives it, as
// after a crash (Cycle160Document discards its snapshot).
struct Cycle162Draft {
    std::unique_ptr<Backend> backend = std::make_unique<Backend>();
    QQmlEngine engine;
    QScopedPointer<QObject> editor;
    bool create() {
        engine.rootContext()->setContextProperty("backend", backend.get());
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nTextEdit { Component.onCompleted: backend.attachDocument(textDocument); onTextChanged: backend.editorTextChanged() }", QUrl());
        editor.reset(component.create());
        return !editor.isNull();
    }
    void crash() { editor.reset(); backend.reset(); }
};

// The window's real Main.qml without the reference workspace Cycle117Window
// applies, so the layout a window starts with comes from settings.
struct Cycle162Window {
    Backend backend;
    QQmlEngine engine;
    QScopedPointer<QObject> window;
    QQuickWindow *quick = nullptr;
    QQuickItem *editor = nullptr;
    QObject *layout = nullptr;
    QString error;
    bool start() {
        engine.rootContext()->setContextProperty("backend", &backend);
        QQmlComponent component(&engine, QUrl("qrc:/Main.qml"));
        window.reset(component.create()); error = component.errorString();
        if (!window) return false;
        quick = qobject_cast<QQuickWindow *>(window.data());
        editor = window->findChild<QQuickItem *>("sourceEditor");
        layout = window->findChild<QObject *>("workspaceLayout");
        if (!quick || !editor || !layout) return false;
        cycle106KeepNativeFocus(quick);
        quick->resize(1280, 720); quick->show();
        return QTest::qWaitForWindowExposed(quick);
    }
    ~Cycle162Window() { backend.discardRecovery(); }
};

QVariant cycle162PlistValue(QXmlStreamReader &xml) {
    const QString element = xml.name().toString();
    if (element == QLatin1String("dict")) {
        QVariantMap map; QString key;
        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("key")) key = xml.readElementText();
            else map.insert(key, cycle162PlistValue(xml));
        }
        return map;
    }
    if (element == QLatin1String("array")) {
        QVariantList list;
        while (xml.readNextStartElement()) list.append(cycle162PlistValue(xml));
        return list;
    }
    if (element == QLatin1String("true") || element == QLatin1String("false")) { xml.skipCurrentElement(); return element == QLatin1String("true"); }
    return xml.readElementText();
}
QVariantMap cycle162ReadPlist(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    QXmlStreamReader xml(&file);
    if (xml.readNextStartElement() && xml.name() == QLatin1String("plist") && xml.readNextStartElement())
        return cycle162PlistValue(xml).toMap();
    return {};
}

// A right-click; Qt turns it into the platform's context-menu request.
void cycle162ContextClick(QQuickWindow *window, QPoint at) {
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, at);
}

bool cycle162OpenLinkAt(QObject *window, QQuickItem *editor, const QString &text, const QString &inside) {
    editor->setProperty("cursorPosition", int(text.indexOf(inside)) + 1);
    return QMetaObject::invokeMethod(window, "openSourceLink");
}
}

void FomawriteTest::pdfLinksOpenFromSourceAndPreview() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    const QString text = QStringLiteral("# Links\n\n[Page 3](geometry.pdf#page=3) and [missing](absent.pdf#page=2) and [gone](absent.md)\n");
    const QString markdown = f.path("Links.md");
    QVERIFY(PdfFixtures::write(markdown, text.toUtf8()));
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.w.backend.open(QUrl::fromLocalFile(markdown)));
    QCOMPARE(p.w.editor->property("text").toString(), text);

    // Source: Go > Open Link on a PDF link opens it at the page, in the window.
    QVERIFY(cycle162OpenLinkAt(p.w.window.data(), p.w.editor, text, "Page 3"));
    QTRY_VERIFY(p.w.backend.pdf()->active());
    QCOMPARE(QFileInfo(p.w.backend.pdf()->url().toLocalFile()).fileName(), QStringLiteral("geometry.pdf"));
    p.reader = nullptr;
    QTRY_VERIFY((p.reader = p.w.window->findChild<QQuickItem *>("pdfReader")) && p.reader->property("ready").toBool());
    QTRY_COMPARE(p.currentPage(), 2);

    // The Markdown document again; modified, the link asks first.
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "requestOpen", Q_ARG(QVariant, QUrl::fromLocalFile(markdown))));
    QTRY_VERIFY(!p.w.backend.pdf()->active());
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "insert", Q_ARG(int, 0), Q_ARG(QString, "Draft. ")));
    const QString edited = p.w.editor->property("text").toString();
    auto *prompt = p.w.window->findChild<QObject *>("unsavedChangesPrompt"); QVERIFY(prompt);
    QVERIFY(cycle162OpenLinkAt(p.w.window.data(), p.w.editor, edited, "Page 3"));
    QCOMPARE(p.w.window->property("pendingAction").toString(), QStringLiteral("open"));
    QTRY_VERIFY(prompt->property("visible").toBool());
    QVERIFY(!p.w.backend.pdf()->active());
    QVERIFY(QMetaObject::invokeMethod(prompt, "cancelRequested"));
    QVERIFY(!p.w.backend.pdf()->active());
    QCOMPARE(p.w.editor->property("text").toString(), edited);
    QVERIFY(QMetaObject::invokeMethod(prompt, "close"));
    QTRY_VERIFY(!prompt->property("visible").toBool());
    p.w.backend.save();
    QVERIFY(!p.w.backend.modified());

    // A missing PDF is reported in the status bar as a missing Markdown file is; nothing changes.
    QVERIFY(cycle162OpenLinkAt(p.w.window.data(), p.w.editor, edited, "gone"));
    QVERIFY2(p.w.backend.status().contains("absent.md"), qPrintable(p.w.backend.status()));
    QVERIFY(cycle162OpenLinkAt(p.w.window.data(), p.w.editor, edited, "missing"));
    QVERIFY2(p.w.backend.status().contains("absent.pdf could not be found"), qPrintable(p.w.backend.status()));
    QVERIFY(!p.w.backend.pdf()->active());
    QCOMPARE(QFileInfo(p.w.backend.fileUrl().toLocalFile()).fileName(), QStringLiteral("Links.md"));
    QCOMPARE(p.w.editor->property("text").toString(), edited);

    // The publishing Preview's links take the same path.
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    QVERIFY(QMetaObject::invokeMethod(p.w.preview, "linkRequested", Q_ARG(QUrl, QUrl("geometry.pdf#page=4"))));
    QTRY_VERIFY(p.w.backend.pdf()->active());
    p.reader = nullptr;
    QTRY_VERIFY((p.reader = p.w.window->findChild<QQuickItem *>("pdfReader")) && p.reader->property("ready").toBool());
    QTRY_COMPARE(p.currentPage(), 3);
}

void FomawriteTest::pdfLinkInTheNotesScrollsThePdfBesideThem() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    const QString notes = QStringLiteral("Notes\n\n[Page 4](geometry.pdf#page=4)\n");
    QVERIFY(PdfFixtures::write(f.path("geometry.pdf.md"), notes.toUtf8()));
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    QCOMPARE(p.w.editor->property("text").toString(), notes);
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "insert", Q_ARG(int, 0), Q_ARG(QString, "Unsaved. ")));
    const QString edited = p.w.editor->property("text").toString();
    p.w.layout->setProperty("layoutMode", 0);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 0);

    // No prompt for the notes' own PDF; Notes only becomes Notes + PDF.
    QVERIFY(cycle162OpenLinkAt(p.w.window.data(), p.w.editor, edited, "Page 4"));
    QCOMPARE(p.w.window->property("pendingAction").toString(), QString());
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    QTRY_COMPARE(p.currentPage(), 3);
    QVERIFY(p.w.backend.modified());
    QCOMPARE(p.w.editor->property("text").toString(), edited);

    // Scrolled away, the same link scrolls back to its page.
    p.call("goToPage", 0);
    QTRY_COMPARE(p.currentPage(), 0);
    const qreal away = p.list->property("contentY").toReal();
    QVERIFY(cycle162OpenLinkAt(p.w.window.data(), p.w.editor, edited, "Page 4"));
    QTRY_COMPARE(p.currentPage(), 3);
    QVERIFY(p.list->property("contentY").toReal() > away + 100);
    // A fragment-only link in the notes does the same.
    p.call("goToPage", 0);
    QTRY_COMPARE(p.currentPage(), 0);
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "requestOpen", Q_ARG(QVariant, QUrl("#page=2"))));
    QTRY_COMPARE(p.currentPage(), 1);
    QVERIFY(p.w.backend.pdf()->active());
}

void FomawriteTest::pdfLinkFromLiveOpensThePdf() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    const QString text = QStringLiteral("# Live\n\nRead [Page 2](geometry.pdf#page=2) next.\n");
    const QString markdown = f.path("Live.md");
    QVERIFY(PdfFixtures::write(markdown, text.toUtf8()));
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.w.backend.open(QUrl::fromLocalFile(markdown)));
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    QVERIFY(p.w.layout->property("liveEditEnabled").toBool());
    auto *bridge = p.w.backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 30000);
    // The Source caret stays outside the link; the Live caret is inside it.
    p.w.editor->setProperty("cursorPosition", 0);
    const int inside = int(text.indexOf("Page 2")) + 1;
    bridge->placeCursor(inside);
    bridge->cursorChanged(inside, inside, true);
    auto *openLink = p.w.window->findChild<QObject *>("goOpenLink"); QVERIFY(openLink);
    QTRY_VERIFY(openLink->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(openLink, "triggered"));
    QTRY_VERIFY(p.w.backend.pdf()->active());
    QTRY_VERIFY((p.reader = p.w.window->findChild<QQuickItem *>("pdfReader")) && p.reader->property("ready").toBool());
    QTRY_COMPARE(p.currentPage(), 1);
    p.w.layout->setProperty("liveEditEnabled", false);
}

void FomawriteTest::pdfCopyAsMarkdownQuoteAndThePageMenu() {
    Cycle103SettingsSnapshot settings;
    // The clipboard text: lines quoted and tidied, hyphenated ends kept, the
    // page link after a blank line, labels and file names as Markdown needs them.
    QCOMPARE(PdfSession::markdownQuote(QStringLiteral("  The  quick\tbrown fox  \r\njumps over the well-\nknown\n\n\n lazy dog \n"),
                                       QStringLiteral("Report.pdf"), 2, QStringLiteral("3")),
             QStringLiteral("> The quick brown fox\n> jumps over the well-\n> known\n>\n> lazy dog\n\n[Page 3](Report.pdf#page=3)\n"));
    QCOMPARE(PdfSession::markdownQuote(QStringLiteral("Preface"), QStringLiteral("Report.pdf"), 2, QStringLiteral("iii")),
             QStringLiteral("> Preface\n\n[Page iii](Report.pdf#page=3)\n"));
    QCOMPARE(PdfSession::markdownQuote(QStringLiteral("Text"), QStringLiteral("My Report (draft) #2.pdf"), 0, QString()),
             QStringLiteral("> Text\n\n[Page 1](My%20Report%20%28draft%29%20%232.pdf#page=1)\n"));
    QCOMPARE(PdfSession::markdownQuote(QStringLiteral("x"), QStringLiteral("a.pdf"), 0, QStringLiteral("[A]")),
             QStringLiteral("> x\n\n[Page \\[A\\]](a.pdf#page=1)\n"));

    // From the pane: a labelled PDF whose name has a space, a two-line selection.
    Cycle160Fixtures f; QVERIFY(f.make());
    QVERIFY(PdfFixtures::write(f.path("Labelled Report.pdf"), cycle161LabelledDocument()));
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("Labelled Report.pdf")));
    auto *menuItem = p.w.window->findChild<QObject *>("pdfMenuCopyQuote"); QVERIFY(menuItem);
    QCOMPARE(menuItem->property("text").toString(), QStringLiteral("Copy as Markdown Quote"));
    QCOMPARE(menuItem->property("shortcut").toString(), QStringLiteral("Ctrl+Shift+C"));
    QVERIFY(!menuItem->property("enabled").toBool());
    PdfSession *pdf = p.w.backend.pdf();
    const QString page = pdf->pageText(0);
    QVERIFY(QMetaObject::invokeMethod(p.reader, "setSelection", Q_ARG(QVariant, 0), Q_ARG(QVariant, pdf->selectionAt(0, 0, int(page.size())))));
    const QString selected = p.reader->property("selectionText").toString();
    QVERIFY2(selected.contains("ALPHA") && selected.contains("OMEGA") && selected.contains('\n'), qPrintable(selected));
    QTRY_VERIFY(menuItem->property("enabled").toBool());
    QGuiApplication::clipboard()->setText("before");
    QVERIFY(QMetaObject::invokeMethod(menuItem, "triggered"));
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("> ALPHA\n> OMEGA\n\n[Page i](Labelled%20Report.pdf#page=1)\n"));
    // Page 3 is labelled "1": its label differs from its number, so it names the link.
    QVERIFY(QMetaObject::invokeMethod(p.reader, "setSelection", Q_ARG(QVariant, 2), Q_ARG(QVariant, pdf->selectionAt(2, int(page.indexOf("OMEGA")), 5))));
    QVERIFY(QMetaObject::invokeMethod(menuItem, "triggered"));
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("> OMEGA\n\n[Page 1](Labelled%20Report.pdf#page=3)\n"));

    // The page's context menu: Copy, Copy as Markdown Quote and Find with a selection.
    auto *menu = p.object("pdfPageMenu"); QVERIFY(menu);
    auto *copy = p.object("pdfContextCopy"), *quote = p.object("pdfContextCopyQuote"), *find = p.object("pdfContextFind"), *copyLink = p.object("pdfContextCopyLink");
    QVERIFY(copy && quote && find && copyLink);
    const QRectF alpha = PdfFixtures::userToView(PdfFixtures::userBox(PdfFixtures::Alpha), 0, PdfFixtures::geometryCrop(0));
    p.reveal(2, alpha.center());
    cycle162ContextClick(p.w.quick, p.scene(2, alpha.center()).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    QCOMPARE(p.reader->property("selectionText").toString(), QStringLiteral("OMEGA")); // kept
    QVERIFY(copy->property("enabled").toBool() && quote->property("enabled").toBool());
    QVERIFY(find->property("visible").toBool());
    QCOMPARE(find->property("text").toString(), QStringLiteral("Find “OMEGA” in PDF"));
    QVERIFY(!copyLink->property("visible").toBool());
    QGuiApplication::clipboard()->setText("before");
    QVERIFY(QMetaObject::invokeMethod(copy, "click"));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("OMEGA"));
    cycle162ContextClick(p.w.quick, p.scene(2, alpha.center()).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(quote, "click"));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("> OMEGA\n\n[Page 1](Labelled%20Report.pdf#page=3)\n"));
    cycle162ContextClick(p.w.quick, p.scene(2, alpha.center()).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(find, "click"));
    QTRY_VERIFY(p.reader->property("findOpen").toBool());
    QCOMPARE(p.object("pdfFindBar")->property("query").toString(), QStringLiteral("OMEGA"));
    QTRY_VERIFY(!menu->property("visible").toBool());
    p.call("closeFind");
    // Without a selection: Copy entries disabled, no Find.
    p.call("clearSelection");
    QTRY_VERIFY(!menuItem->property("enabled").toBool());
    p.reveal(2, alpha.center()); // Find scrolled to its match
    cycle162ContextClick(p.w.quick, p.scene(2, alpha.center()).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(!copy->property("enabled").toBool() && !quote->property("enabled").toBool());
    QVERIFY(!find->property("visible").toBool());
    QVERIFY(!copyLink->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(menu, "close"));
    QTRY_VERIFY(!menu->property("opened").toBool());

    // On a link: Copy Link Address (an internal link copies File.pdf#page=N).
    QVERIFY(p.open(f.url("navigation.pdf")));
    QTRY_VERIFY(p.page(0));
    const QRectF internal = p.w.backend.pdf()->links(0).first().toMap().value("rects").toList().first().toRectF();
    p.reveal(0, internal.center());
    cycle162ContextClick(p.w.quick, p.scene(0, internal.center()).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(copyLink->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(copyLink, "click"));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("navigation.pdf#page=2"));
    QCOMPARE(p.currentPage(), 0); // a right-click never follows the link
}

void FomawriteTest::pdfContentsSidebarSwitchesNavigatesAndMarksTheSection() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    QVERIFY(PdfFixtures::write(f.path("outlined.pdf"), cycle162OutlinedDocument()));
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("outlined.pdf")));
    p.call("toggleThumbnails");
    auto *sidebar = p.item("pdfThumbnails"); QVERIFY(sidebar);
    QTRY_VERIFY(sidebar->isVisible());
    auto *sidebarSwitch = p.item("pdfSidebarSwitch"); QVERIFY(sidebarSwitch);
    QTRY_VERIFY(sidebarSwitch->isVisible());
    QCOMPARE(p.reader->property("sidebarView").toString(), QStringLiteral("thumbnails"));
    QVERIFY(p.item("pdfThumbnailList")->isVisible());

    // Contents: the outline's titles, indented by level.
    const auto tab = [&](const char *name) { const auto tabs = cycle103VisualItems(sidebar, name); return tabs.isEmpty() ? nullptr : tabs.first(); };
    QVERIFY(tab("pdfSidebarThumbnailsTab") && tab("pdfSidebarThumbnailsTab")->property("selected").toBool());
    QVERIFY(cycle102Click(p.w.quick, tab("pdfSidebarContentsTab")));
    QTRY_VERIFY(tab("pdfSidebarContentsTab")->property("selected").toBool());
    QTRY_COMPARE(p.reader->property("sidebarView").toString(), QStringLiteral("contents"));
    QCOMPARE(QSettings().value("pdf/sidebarView").toString(), QStringLiteral("contents"));
    auto *contents = p.item("pdfContentsList"); QVERIFY(contents);
    QTRY_VERIFY(contents->isVisible());
    QVERIFY(!p.item("pdfThumbnailList")->isVisible());
    QTRY_COMPARE(contents->property("count").toInt(), 4);
    QStringList titles; QMap<QString, qreal> indent;
    QTRY_COMPARE(cycle103VisualItems(contents, "pdfContentsTitle").size(), 4);
    for (auto *title : cycle103VisualItems(contents, "pdfContentsTitle")) {
        titles << title->property("text").toString();
        indent.insert(title->property("text").toString(), title->x());
    }
    QCOMPARE(titles, (QStringList{"Part One", "Section A", "Part Two", "Section B"}));
    QVERIFY(indent["Section A"] > indent["Part One"] + 5 && indent["Section B"] > indent["Part Two"] + 5);
    const auto current = [&] {
        const auto items = cycle103VisualItems(contents, "pdfContentsCurrent");
        return items.size() == 1 ? items.first()->findChild<QQuickItem *>("pdfContentsTitle")->property("text").toString() : QString::number(items.size());
    };
    QTRY_COMPARE(current(), QStringLiteral("Part One"));

    // A click goes to the entry's page, and the section is marked as the pages move.
    QQuickItem *sectionB = nullptr;
    for (auto *title : cycle103VisualItems(contents, "pdfContentsTitle")) if (title->property("text").toString() == "Section B") sectionB = title;
    QVERIFY(cycle102Click(p.w.quick, sectionB));
    QTRY_COMPARE(p.currentPage(), 3);
    QTRY_COMPARE(current(), QStringLiteral("Section B"));
    p.call("goToPage", 1);
    QTRY_COMPARE(current(), QStringLiteral("Section A"));
    p.call("goToPage", 2);
    QTRY_COMPARE(current(), QStringLiteral("Part Two"));

    // Remembered: the next PDF with an outline opens on Contents.
    QVERIFY(p.open(f.url("navigation.pdf")));
    QTRY_COMPARE(p.reader->property("sidebarView").toString(), QStringLiteral("contents"));
    QTRY_COMPARE(p.item("pdfContentsList")->property("count").toInt(), 2);
    // Without an outline there is no switch, and the sidebar shows thumbnails.
    QVERIFY(p.open(f.url("geometry.pdf")));
    QTRY_VERIFY(!p.item("pdfSidebarSwitch")->isVisible());
    QCOMPARE(p.reader->property("sidebarView").toString(), QStringLiteral("thumbnails"));
    QTRY_VERIFY(p.item("pdfThumbnailList")->isVisible());
    QVERIFY(!p.item("pdfContentsList")->isVisible());
    QCOMPARE(QSettings().value("pdf/sidebarView").toString(), QStringLiteral("contents"));

    // The pane's sidebar button no longer looks like the window's sidebar button.
    auto *toggle = p.item("pdfThumbnailsToggle"); QVERIFY(toggle);
    QCOMPARE(toggle->property("iconName").toString(), QStringLiteral("pages"));
    QQmlComponent icons(&p.w.engine);
    icons.setData("import QtQuick\nRow { LineIcon { objectName: 'a'; name: 'pages'; width: 48; height: 48; ink: 'black' } "
                  "LineIcon { objectName: 'b'; name: 'library'; width: 48; height: 48; ink: 'black' } "
                  "LineIcon { objectName: 'c'; name: 'thumbnails'; width: 48; height: 48; ink: 'black' } }", QUrl("qrc:/"));
    QScopedPointer<QObject> row(icons.create()); QVERIFY2(row, qPrintable(icons.errorString()));
    auto *rowItem = qobject_cast<QQuickItem *>(row.data());
    rowItem->setParentItem(p.w.quick->contentItem());
    rowItem->setZ(10000);
    QTest::qWait(100);
    const QImage frame = p.w.quick->grabWindow();
    const auto inkOf = [&](const char *name) {
        auto *icon = row->findChild<QQuickItem *>(name);
        const QRect area = icon->mapRectToScene(icon->boundingRect()).toAlignedRect();
        const QImage image = frame.copy(QRect(area.topLeft() * frame.devicePixelRatio(), area.size() * frame.devicePixelRatio()))
            .scaled(48, 48).convertToFormat(QImage::Format_Grayscale8);
        return image;
    };
    const QImage pages = inkOf("a"), library = inkOf("b"), oldThumbnails = inkOf("c");
    const auto difference = [](const QImage &a, const QImage &b) {
        int differing = 0;
        for (int y = 0; y < a.height(); ++y) for (int x = 0; x < a.width(); ++x)
            differing += qAbs(qGray(a.pixel(x, y)) - qGray(b.pixel(x, y))) > 96;
        return differing;
    };
    const int fromLibrary = difference(pages, library), fromOld = difference(oldThumbnails, library);
    QVERIFY2(fromLibrary > 2 * fromOld && fromLibrary > 150,
             qPrintable(QStringLiteral("pages vs library %1 px, old thumbnails vs library %2 px").arg(fromLibrary).arg(fromOld)));
}

void FomawriteTest::pdfWindowCheckpointRestoresPdfModeAndKeepsTheMarkdownLayout() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    QVERIFY(PdfFixtures::write(f.path("geometry.pdf.md"), "Saved notes\n"));
    const QString markdown = f.path("Draft.md");
    QVERIFY(PdfFixtures::write(markdown, "# Draft\n"));
    const QString store = f.path("workspace.json");
    QVariant markdownLayoutSetting;

    {
        // Before the PDF: a Markdown document in Single.
        Cycle161Pane a; QVERIFY2(a.start(), qPrintable(a.w.error));
        QVERIFY(a.w.backend.open(QUrl::fromLocalFile(markdown)));
        a.w.layout->setProperty("layoutMode", 0);
        QTRY_COMPARE(a.w.layout->property("effectiveLayoutMode").toInt(), 0);
        const QJsonObject markdownEntry = WorkspaceStore::documentEntry(a.w.backend, a.w.window.data());
        QCOMPARE(markdownEntry["url"].toString(), QUrl::fromLocalFile(markdown).toString());
        QCOMPARE(markdownEntry["paneState"].toObject()["layoutMode"].toInt(), 0);
        QVERIFY(!markdownEntry["paneState"].toObject().contains("pdfLayoutMode"));

        markdownLayoutSetting = QSettings().value("workspace/layoutMode");
        // The PDF with notes, read in PDF only at page 4 and 150%.
        QVERIFY(a.open(f.url("geometry.pdf")));
        a.w.layout->setProperty("layoutMode", 2);
        QTRY_COMPARE(a.w.layout->property("effectiveLayoutMode").toInt(), 2);
        a.call("goToPage", 3);
        QVERIFY(a.settled());
        QVariant ignored;
        QVERIFY(QMetaObject::invokeMethod(a.reader, "zoomAround", Q_RETURN_ARG(QVariant, ignored), Q_ARG(QVariant, 1.5),
                                          Q_ARG(QVariant, a.list->width() / 2), Q_ARG(QVariant, a.list->height() / 2)));
        QTRY_COMPARE(a.zoom(), 1.5);
        QVERIFY(a.settled());
        QTRY_COMPARE(a.currentPage(), 3);
        QVERIFY(QMetaObject::invokeMethod(a.reader, "rememberSoon"));
        QVERIFY(QMetaObject::invokeMethod(a.reader, "rememberNow"));
        const QJsonObject entry = WorkspaceStore::documentEntry(a.w.backend, a.w.window.data());
        // Checkpointed by the PDF; the pane state keeps the Markdown arrangement.
        QCOMPARE(QUrl(entry["url"].toString()), a.w.backend.pdf()->url());
        const QJsonObject pane = entry["paneState"].toObject();
        QCOMPARE(pane["layoutMode"].toInt(), 0);
        QCOMPARE(pane["pdfLayoutMode"].toInt(), 2);
        QVERIFY(WorkspaceStore::write(store, QJsonArray{entry}));
    }
    // Nothing PDF mode did reached the layout new windows start with.
    QCOMPARE(QSettings().value("workspace/layoutMode", 1).toInt(), 1);

    // Relaunch: a fresh window (starting Split, the default) restored as main.cpp does.
    const QJsonArray saved = WorkspaceStore::read(store);
    QCOMPARE(saved.size(), 1);
    const QJsonObject entry = saved.first().toObject();
    Cycle161Pane b; QVERIFY2(b.start(), qPrintable(b.w.error));
    b.w.layout->setProperty("layoutMode", 1);
    QVERIFY(b.w.backend.open(QUrl(entry["url"].toString())));
    WorkspaceStore::restoreDocumentEntry(b.w.window.data(), entry, 1440);
    QVERIFY(b.w.backend.pdf()->active());
    QCOMPARE(QFileInfo(b.w.backend.fileUrl().toLocalFile()).fileName(), QStringLiteral("geometry.pdf.md"));
    QCOMPARE(b.w.editor->property("text").toString(), QStringLiteral("Saved notes\n"));
    QTRY_COMPARE(b.w.layout->property("effectiveLayoutMode").toInt(), 2);
    QTRY_VERIFY((b.reader = b.w.window->findChild<QQuickItem *>("pdfReader")) && b.reader->property("ready").toBool());
    b.list = b.reader->findChild<QQuickItem *>("pdfPageList");
    QTRY_COMPARE(b.zoom(), 1.5);
    QTRY_COMPARE(b.currentPage(), 3);
    // Its checkpoint again is the same; leaving PDF mode returns to the Markdown arrangement.
    const QJsonObject again = WorkspaceStore::documentEntry(b.w.backend, b.w.window.data());
    QCOMPARE(again["paneState"].toObject()["layoutMode"].toInt(), 0);
    QCOMPARE(again["paneState"].toObject()["pdfLayoutMode"].toInt(), 2);
    QVERIFY(QMetaObject::invokeMethod(b.w.window.data(), "requestOpen", Q_ARG(QVariant, QUrl::fromLocalFile(markdown))));
    QTRY_VERIFY(!b.w.backend.pdf()->active());
    QTRY_COMPARE(b.w.layout->property("effectiveLayoutMode").toInt(), 0);

    // Untitled notes: still checkpointed by the PDF, and restored in PDF mode with a save target.
    Cycle161Pane c; QVERIFY2(c.start(), qPrintable(c.w.error));
    QVERIFY(c.open(f.url("plain.pdf")));
    QVERIFY(c.w.backend.fileUrl().isEmpty());
    const QJsonObject untitled = WorkspaceStore::documentEntry(c.w.backend, c.w.window.data());
    QCOMPARE(QUrl(untitled["url"].toString()), c.w.backend.pdf()->url());
    QCOMPARE(untitled["paneState"].toObject()["pdfLayoutMode"].toInt(), 2);
    Cycle161Pane d; QVERIFY2(d.start(), qPrintable(d.w.error));
    QVERIFY(d.w.backend.open(QUrl(untitled["url"].toString())));
    WorkspaceStore::restoreDocumentEntry(d.w.window.data(), untitled, 1440);
    QVERIFY(d.w.backend.pdf()->active());
    QCOMPARE(d.w.backend.fileName(), QStringLiteral("plain.pdf.md"));
    QTRY_COMPARE(d.w.layout->property("effectiveLayoutMode").toInt(), 2);
}

void FomawriteTest::pdfRecoveryReopensUnsavedNotesWithTheirPdf() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    cycle162ClearOrphanSnapshots();
    const QString recovered = QStringLiteral("Unsaved notes on plain.pdf %1\n").arg(QUuid::createUuid().toString());
    {
        Cycle162Draft draft; QVERIFY(draft.create());
        QVERIFY(draft.backend->open(f.url("plain.pdf")));
        QVERIFY(draft.backend->fileUrl().isEmpty());
        QVERIFY(QMetaObject::invokeMethod(draft.editor.data(), "insert", Q_ARG(int, 0), Q_ARG(QString, recovered)));
        QVERIFY(draft.backend->modified());
        QTRY_VERIFY(!cycle162Snapshot(recovered).isEmpty());
        const QJsonObject snapshot = cycle162Snapshot(recovered);
        QCOMPARE(snapshot["fileUrl"].toString(), QString());
        QCOMPARE(QUrl(snapshot["pdfUrl"].toString()), draft.backend->pdf()->url());
        draft.crash();
    }

    // The next window recovers the notes with their PDF: PDF mode, split, modified.
    QSettings().setValue("workspace/layoutMode", 0);
    {
        Cycle162Window w; QVERIFY2(w.start(), qPrintable(w.error));
        QVERIFY(w.backend.pdf()->active());
        QCOMPARE(QFileInfo(w.backend.pdf()->url().toLocalFile()).fileName(), QStringLiteral("plain.pdf"));
        QCOMPARE(w.editor->property("text").toString(), recovered);
        QVERIFY(w.backend.modified());
        QVERIFY(w.backend.fileUrl().isEmpty());
        QCOMPARE(w.backend.fileName(), QStringLiteral("plain.pdf.md"));
        QTRY_COMPARE(w.layout->property("effectiveLayoutMode").toInt(), 1);
        QTRY_VERIFY(w.window->findChild<QQuickItem *>("pdfReader"));
        // Save writes plain.pdf.md without a dialog.
        QSignalSpy dialog(&w.backend, &Backend::saveDialogRequested);
        w.backend.save();
        QCOMPARE(dialog.count(), 0);
        QVERIFY(!w.backend.modified());
        QCOMPARE(cycle160Read(f.path("plain.pdf.md")), recovered.toUtf8());
        // Leaving PDF mode returns to the layout the window had (Single, from settings).
        QVERIFY(QMetaObject::invokeMethod(w.window.data(), "requestNewDocument"));
        QTRY_VERIFY(!w.backend.pdf()->active());
        QTRY_COMPARE(w.layout->property("effectiveLayoutMode").toInt(), 0);
    }

    // Titled notes: recovered in PDF mode with the notes file as the document.
    const QString titled = QStringLiteral("Edited geometry notes %1\n").arg(QUuid::createUuid().toString());
    QVERIFY(PdfFixtures::write(f.path("geometry.pdf.md"), "Saved\n"));
    {
        Cycle162Draft draft; QVERIFY(draft.create());
        QVERIFY(draft.backend->open(f.url("geometry.pdf")));
        QVERIFY(QMetaObject::invokeMethod(draft.editor.data(), "insert", Q_ARG(int, 0), Q_ARG(QString, titled)));
        QTRY_VERIFY(!cycle162Snapshot(titled + "Saved\n").isEmpty());
        draft.crash();
    }
    {
        Cycle160Document d; QVERIFY(d.create());
        QVERIFY(d.backend.pdf()->active());
        QCOMPARE(QFileInfo(d.backend.pdf()->url().toLocalFile()).fileName(), QStringLiteral("geometry.pdf"));
        QCOMPARE(QFileInfo(d.backend.fileUrl().toLocalFile()).fileName(), QStringLiteral("geometry.pdf.md"));
        QCOMPARE(d.text(), titled + "Saved\n");
        QVERIFY(d.backend.modified());
    }

    // Snapshots written before Cycle 162 still load: a plain draft stays Markdown...
    cycle162ClearOrphanSnapshots();
    QVERIFY(cycle162WriteSnapshot(QJsonObject{{"fileUrl", QString()}, {"text", QStringLiteral("Old draft\n")}}));
    {
        Cycle160Document d; QVERIFY(d.create());
        QCOMPARE(d.text(), QStringLiteral("Old draft\n"));
        QVERIFY(d.backend.modified());
        QVERIFY(!d.backend.pdf()->active());
        QVERIFY(d.backend.fileUrl().isEmpty());
    }
    // ...and an old snapshot of a notes file brings its PDF along, as opening it does.
    QVERIFY(cycle162WriteSnapshot(QJsonObject{{"fileUrl", f.url("geometry.pdf.md").toString()}, {"text", QStringLiteral("Old notes\n")}}));
    {
        Cycle160Document d; QVERIFY(d.create());
        QCOMPARE(d.text(), QStringLiteral("Old notes\n"));
        QVERIFY(d.backend.modified());
        QVERIFY(d.backend.pdf()->active());
        QCOMPARE(QFileInfo(d.backend.pdf()->url().toLocalFile()).fileName(), QStringLiteral("geometry.pdf"));
    }
}

void FomawriteTest::pdfWindowsOwnBothThePdfAndItsNotes() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    // Window A: plain.pdf with unsaved, untitled notes.
    Cycle160Document a; QVERIFY(a.create());
    QVERIFY(a.backend.open(f.url("plain.pdf")));
    QVERIFY(a.type("Unsaved."));
    QVERIFY(a.backend.modified() && a.backend.fileUrl().isEmpty());
    QVERIFY(a.backend.ownsDocument(QFileInfo(f.path("plain.pdf")).canonicalFilePath()));

    // Another view of the PDF is refused while its notes are unsaved.
    Backend other;
    QSignalSpy windows(&other, &Backend::newWindowRequested), tabs(&other, &Backend::newTabRequested);
    QVERIFY(!other.openInNewWindow(f.url("plain.pdf")));
    QVERIFY2(other.status().contains("unsaved changes"), qPrintable(other.status()));
    QVERIFY(!other.openInNewTab(f.url("plain.pdf")));
    QCOMPARE(windows.count() + tabs.count(), 0);
    // The explorer acts through the window that shows the PDF: Trash is refused, Rename follows the window.
    QVERIFY(!other.libraryItemAction(f.url("plain.pdf"), "trash"));
    QVERIFY2(other.status().contains("Save or close"), qPrintable(other.status()));
    QVERIFY(QFileInfo::exists(f.path("plain.pdf")));
    QVERIFY2(other.libraryItemAction(f.url("plain.pdf"), "rename", "Plain renamed.pdf"), qPrintable(other.status()));
    QCOMPARE(QFileInfo(a.backend.pdf()->url().toLocalFile()).fileName(), QStringLiteral("Plain renamed.pdf"));
    QCOMPARE(a.backend.fileName(), QStringLiteral("Plain renamed.pdf.md"));
    QVERIFY(a.backend.modified());
    QCOMPARE(a.text(), QStringLiteral("Unsaved."));

    // A second view of the same notes file (one window with untitled notes,
    // another that opened the file once it existed) is checked like any other.
    Cycle160Document first; QVERIFY(first.create());
    QVERIFY(first.backend.open(f.url("geometry.pdf")));
    QVERIFY(first.backend.fileUrl().isEmpty());
    QVERIFY(PdfFixtures::write(f.path("geometry.pdf.md"), "External\n"));
    Cycle160Document second; QVERIFY(second.create());
    QVERIFY(second.backend.open(f.url("geometry.pdf.md")));
    QVERIFY(second.backend.pdf()->active());
    QVERIFY(PdfFixtures::write(f.path("geometry.pdf.md"), "Changed by the other view\n"));
    QVERIFY(second.type("Mine."));
    second.backend.save();
    QVERIFY2(second.backend.status().contains("another view"), qPrintable(second.backend.status()));
    QCOMPARE(cycle160Read(f.path("geometry.pdf.md")), QByteArray("Changed by the other view\n"));
}

void FomawriteTest::pdfRenameMoveAndTrashCarryTheNotes() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    const QByteArray pdfBytes = cycle160Read(f.path("geometry.pdf"));
    const auto make = [&](const QString &name, const QByteArray &notes) {
        return PdfFixtures::write(f.path(name), pdfBytes) && (notes.isNull() || PdfFixtures::write(f.path(name + ".md"), notes));
    };
    QVERIFY(make("Report One.pdf", "Report notes\n"));
    QVERIFY(PdfFixtures::write(f.path(".Report One.pdf.md.omawrite-authors.json"), "{\"ranges\":[]}"));
    Backend backend;
    {
        PdfSession views;
        views.rememberView(f.url("Report One.pdf"), 3, "fixed", 1.5);
    }
    backend.library()->setProperty("rootFolder", QUrl::fromLocalFile(f.directory.path()));
    qobject_cast<FileLibrary *>(backend.library())->recordRecentFile(f.url("Report One.pdf"));

    // The explorer offers Rename and Move to Trash on a PDF row, not Duplicate.
    QVERIFY(backend.libraryItemInfo(f.url("Report One.pdf")).value("hasNotes").toBool());

    // Rename: the notes and their authorship sidecar follow; recents and the remembered view too.
    const QString one = QFileInfo(f.path("Report One.pdf")).canonicalFilePath();
    QVERIFY(!backend.libraryItemAction(f.url("Report One.pdf"), "rename", "Report One.md"));
    QVERIFY(PdfFixtures::write(f.path("Taken.pdf.md"), "x"));
    QVERIFY(!backend.libraryItemAction(f.url("Report One.pdf"), "rename", "Taken.pdf"));
    QVERIFY2(backend.status().contains("already in use"), qPrintable(backend.status()));
    QVERIFY(QFileInfo::exists(f.path("Report One.pdf")) && QFileInfo::exists(f.path("Report One.pdf.md")));
    QVERIFY2(backend.libraryItemAction(f.url("Report One.pdf"), "rename", "Report Two.pdf"), qPrintable(backend.status()));
    QVERIFY(!QFileInfo::exists(f.path("Report One.pdf")) && !QFileInfo::exists(f.path("Report One.pdf.md")));
    QCOMPARE(cycle160Read(f.path("Report Two.pdf")), pdfBytes);
    QCOMPARE(cycle160Read(f.path("Report Two.pdf.md")), QByteArray("Report notes\n"));
    QVERIFY(QFileInfo::exists(f.path(".Report Two.pdf.md.omawrite-authors.json")));
    QVERIFY(!QFileInfo::exists(f.path(".Report One.pdf.md.omawrite-authors.json")));
    const QString two = QFileInfo(f.path("Report Two.pdf")).canonicalFilePath();
    QVERIFY(QSettings().value("library/recents").toStringList().contains(two));
    QVERIFY(!QSettings().value("library/recents").toStringList().contains(one));
    {
        PdfSession views;
        QCOMPARE(views.rememberedView(f.url("Report Two.pdf")).value("page").toInt(), 3);
    }

#ifdef Q_OS_MACOS
    // If the notes cannot follow, the PDF is put back: never a half-moved pair.
    const QByteArray notesPath = QFile::encodeName(f.path("Report Two.pdf.md"));
    QCOMPARE(::chflags(notesPath.constData(), UF_IMMUTABLE), 0);
    const auto unlock = qScopeGuard([&] { ::chflags(notesPath.constData(), 0); });
    QVERIFY(!backend.libraryItemAction(f.url("Report Two.pdf"), "rename", "Report Three.pdf"));
    QVERIFY2(backend.status().contains("put back"), qPrintable(backend.status()));
    QVERIFY(QFileInfo::exists(f.path("Report Two.pdf")) && QFileInfo::exists(f.path("Report Two.pdf.md")));
    QVERIFY(!QFileInfo::exists(f.path("Report Three.pdf")) && !QFileInfo::exists(f.path("Report Three.pdf.md")));
    // Qt's copy fallback clones the locked file beside its destination and cannot remove the clone.
    for (const QString &name : QDir(f.directory.path()).entryList({QStringLiteral("Report Three.pdf.md.*")}, QDir::Files | QDir::Hidden)) {
        ::chflags(QFile::encodeName(f.path(name)).constData(), 0);
        QFile::remove(f.path(name));
    }
    QVERIFY(!backend.libraryItemAction(f.url("Report Two.pdf"), "trash"));
    QVERIFY2(backend.status().contains("put back"), qPrintable(backend.status()));
    QCOMPARE(cycle160Read(f.path("Report Two.pdf")), pdfBytes);
    QVERIFY(QFileInfo::exists(f.path("Report Two.pdf.md")));
    ::chflags(notesPath.constData(), 0);
#endif

    // Trash: both go, with the remembered view.
    QVERIFY2(backend.libraryItemAction(f.url("Report Two.pdf"), "trash"), qPrintable(backend.status()));
    QVERIFY(!QFileInfo::exists(f.path("Report Two.pdf")) && !QFileInfo::exists(f.path("Report Two.pdf.md")));
    QVERIFY(backend.status().contains("and its notes"));
    {
        PdfSession views;
        QVERIFY(views.rememberedView(QUrl::fromLocalFile(two)).isEmpty());
    }
    // Duplicate, export, print and the text copies stay unavailable.
    QVERIFY(make("Other.pdf", "Other notes\n"));
    for (const char *action : {"duplicate", "exportHtml", "print", "copyMarkdown", "copyText"})
        QVERIFY2(!backend.libraryItemAction(f.url("Other.pdf"), QString::fromLatin1(action), "Copy.pdf"), action);

    // The window showing a PDF follows Rename and Move To…, unsaved notes and its place kept.
    QVERIFY(make("Window.pdf", "Window notes\n"));
    QDir(f.directory.path()).mkdir("Moved");
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("Window.pdf")));
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "insert", Q_ARG(int, 0), Q_ARG(QString, "Unsaved. ")));
    const QString notes = p.w.editor->property("text").toString();
    p.call("goToPage", 2);
    QTRY_COMPARE(p.currentPage(), 2);
    const int layout = p.w.layout->property("layoutMode").toInt();
    QVERIFY2(p.w.backend.renameDocument("Window renamed.pdf"), qPrintable(p.w.backend.status()));
    QCOMPARE(QFileInfo(p.w.backend.pdf()->url().toLocalFile()).fileName(), QStringLiteral("Window renamed.pdf"));
    QCOMPARE(QFileInfo(p.w.backend.fileUrl().toLocalFile()).fileName(), QStringLiteral("Window renamed.pdf.md"));
    QVERIFY(QFileInfo::exists(f.path("Window renamed.pdf.md")) && !QFileInfo::exists(f.path("Window.pdf")));
    QVERIFY(p.w.backend.modified());
    QCOMPARE(p.w.editor->property("text").toString(), notes);
    QTRY_COMPARE(QUrl(p.reader->property("shownUrl").toUrl()), p.w.backend.pdf()->url());
    QTest::qWait(100);
    QCOMPARE(p.currentPage(), 2);
    QCOMPARE(p.w.layout->property("layoutMode").toInt(), layout);
    QCOMPARE(p.w.window->property("title").toString(), QStringLiteral("* Window renamed.pdf - Fomawrite"));
    QVERIFY2(p.w.backend.moveDocument(QUrl::fromLocalFile(f.path("Moved"))), qPrintable(p.w.backend.status()));
    QCOMPARE(QFileInfo(p.w.backend.pdf()->url().toLocalFile()).absoluteFilePath(), QFileInfo(f.path("Moved/Window renamed.pdf")).canonicalFilePath());
    QVERIFY(QFileInfo::exists(f.path("Moved/Window renamed.pdf.md")) && !QFileInfo::exists(f.path("Window renamed.pdf")));
    p.w.backend.save();
    QCOMPARE(cycle160Read(f.path("Moved/Window renamed.pdf.md")), notes.toUtf8());

    // Untitled notes keep their save target beside the renamed PDF.
    QVERIFY(make("Bare.pdf", QByteArray()));
    QVERIFY(p.open(f.url("Bare.pdf")));
    QVERIFY(p.w.backend.fileUrl().isEmpty());
    QVERIFY2(p.w.backend.renameDocument("Bare renamed.pdf"), qPrintable(p.w.backend.status()));
    QCOMPARE(p.w.backend.fileName(), QStringLiteral("Bare renamed.pdf.md"));
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "insert", Q_ARG(int, 0), Q_ARG(QString, "Bare notes\n")));
    p.w.backend.save();
    QCOMPARE(cycle160Read(f.path("Bare renamed.pdf.md")), QByteArray("Bare notes\n"));

    // Trash from the explorer while the window shows the (saved) PDF: the notes stay as an untitled draft.
    QVERIFY(!p.w.backend.modified());
    QVERIFY2(p.w.backend.libraryItemAction(f.url("Bare renamed.pdf"), "trash"), qPrintable(p.w.backend.status()));
    QTRY_VERIFY(!p.w.backend.pdf()->active());
    QVERIFY(p.w.backend.fileUrl().isEmpty());
    QVERIFY(p.w.backend.modified());
    QCOMPARE(p.w.editor->property("text").toString(), QStringLiteral("Bare notes\n"));
    QVERIFY(!QFileInfo::exists(f.path("Bare renamed.pdf.md")));
}

void FomawriteTest::pdfOpensFromTheOpenDialogDropsAndOpenWith() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle117Window w; QVERIFY2(w.start(), qPrintable(w.error));
    const QString markdown = f.path("Draft.md");
    QVERIFY(PdfFixtures::write(markdown, "# Draft\n"));

    // File > Open lists Markdown, text and PDF together; "All files" stays.
    auto *dialog = w.window->findChild<QObject *>("openFileDialog");
    QVERIFY(dialog);
    const QStringList filters = dialog->property("nameFilters").toStringList();
    QVERIFY2(filters.value(0).contains("*.pdf") && filters.value(0).contains("*.md") && filters.value(0).contains("*.txt"), qPrintable(filters.join(" | ")));
    QVERIFY(filters.contains("PDF files (*.pdf)"));
    QCOMPARE(filters.last(), QStringLiteral("All files (*)"));
    QVERIFY(w.backend.resolveOpenPath(f.path("geometry.pdf")).contains("url"));

    // A PDF dropped on the window opens there.
    const auto drop = [&](const QList<QUrl> &urls) {
        QMimeData mime; mime.setUrls(urls);
        const QPoint at(w.quick->width() / 2, w.quick->height() / 2);
        QDragEnterEvent enter(at, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(w.quick, &enter);
        if (!enter.isAccepted()) {
            QDragLeaveEvent leave; QCoreApplication::sendEvent(w.quick, &leave);
            return false;
        }
        QDragMoveEvent move(at, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(w.quick, &move);
        QDropEvent dropped(QPointF(at), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(w.quick, &dropped);
        return dropped.isAccepted();
    };
    QVERIFY(drop({f.url("geometry.pdf")}));
    QTRY_VERIFY(w.backend.pdf()->active());
    QTRY_VERIFY(w.window->findChild<QQuickItem *>("pdfReader"));
    // A Markdown file too; other files are not taken.
    QVERIFY(drop({QUrl::fromLocalFile(markdown)}));
    QTRY_VERIFY(!w.backend.pdf()->active());
    QCOMPARE(w.backend.fileUrl(), QUrl::fromLocalFile(markdown));
    QVERIFY(PdfFixtures::write(f.path("picture.png"), "not really"));
    QVERIFY(!drop({f.url("picture.png")}));
    QCOMPARE(w.backend.fileUrl(), QUrl::fromLocalFile(markdown));
    // With unsaved changes, a drop asks first.
    QVERIFY(QMetaObject::invokeMethod(w.editor, "insert", Q_ARG(int, 0), Q_ARG(QString, "Unsaved. ")));
    auto *prompt = w.window->findChild<QObject *>("unsavedChangesPrompt"); QVERIFY(prompt);
    QVERIFY(drop({f.url("picture.png"), f.url("geometry.pdf")}));
    QTRY_VERIFY(prompt->property("visible").toBool());
    QVERIFY(!w.backend.pdf()->active());
    QVERIFY(QMetaObject::invokeMethod(prompt, "discardRequested"));
    QTRY_VERIFY(w.backend.pdf()->active());
    QVERIFY(QMetaObject::invokeMethod(prompt, "close"));

    // Open With on macOS: a PDF viewer role that never becomes the default.
    const QVariantMap info = cycle162ReadPlist(QFINDTESTDATA("../macos/Info.plist"));
    QVERIFY(!info.isEmpty());
    QVariantMap pdfType;
    QStringList defaults;
    for (const QVariant &value : info.value("CFBundleDocumentTypes").toList()) {
        const QVariantMap type = value.toMap();
        const QStringList contentTypes = type.value("LSItemContentTypes").toStringList();
        if (contentTypes.contains("com.adobe.pdf")) pdfType = type;
        if (type.value("LSHandlerRank").toString() == "Default") defaults << contentTypes;
    }
    QCOMPARE(pdfType.value("CFBundleTypeRole").toString(), QStringLiteral("Viewer"));
    QCOMPARE(pdfType.value("LSHandlerRank").toString(), QStringLiteral("Alternate"));
    QCOMPARE(pdfType.value("CFBundleTypeExtensions").toStringList(), QStringList{"pdf"});
    QCOMPARE(defaults, QStringList{"net.daringfireball.markdown"});
    // The installer's default-app step names Markdown and plain text only.
    QFile defaultsScript(QFINDTESTDATA("../bin/set-default-editor"));
    QVERIFY(defaultsScript.open(QIODevice::ReadOnly));
    const QString script = QString::fromUtf8(defaultsScript.readAll());
    const auto loop = QRegularExpression(QStringLiteral("(?m)^for type in ([^;]+);")).match(script);
    QVERIFY(loop.hasMatch());
    QCOMPARE(loop.captured(1).split(' ', Qt::SkipEmptyParts), (QStringList{"net.daringfireball.markdown", "public.plain-text"}));
    QFile installScript(QFINDTESTDATA("../bin/install-mac"));
    QVERIFY(installScript.open(QIODevice::ReadOnly));
    const QString install = QString::fromUtf8(installScript.readAll());
    QVERIFY(!install.contains("pdf", Qt::CaseInsensitive) && install.contains("bin/set-default-editor"));
    // The Dev bundle offers itself for nothing, the PDF type included.
    QFile devScript(QFINDTESTDATA("../bin/prepare-dev-app"));
    QVERIFY(devScript.open(QIODevice::ReadOnly));
    const QString dev = QString::fromUtf8(devScript.readAll());
    for (int index = 0; index < info.value("CFBundleDocumentTypes").toList().size(); ++index)
        QVERIFY2(dev.contains(QStringLiteral("CFBundleDocumentTypes:%1:LSHandlerRank None").arg(index)), qPrintable(QString::number(index)));
}
