// The PDF menu on the Windows (Quick Controls) menu bar: present only in PDF
// mode, items through the bar's dispatch, Edit → Copy for the PDF selection,
// and keyboard routing by focus. Loads the Windows bar on any host, as the
// other windows-* tests do. Fixtures: Cycle160Fixtures, Cycle161Pane.

#include <QQmlListReference>
#include <QRegularExpression>

namespace {
struct WindowsPdfMenuFixture {
    Cycle161Pane p;
    QObject *bar = nullptr;
    QString markdown;
    bool start(const Cycle160Fixtures &f) {
        if (!p.start()) return false;
        markdown = f.path("Draft.md");
        if (!PdfFixtures::write(markdown, "# Draft\n\nSome café notes.\n")) return false;
        if (!p.w.backend.open(QUrl::fromLocalFile(markdown))) return false;
        p.w.layout->setProperty("layoutMode", 1);
        if (!QTest::qWaitFor([&] { return p.w.layout->property("effectiveLayoutMode").toInt() == 1; }, 2000)) return false;
        auto *loader = p.w.window->findChild<QObject *>("windowsMenuLoader");
        if (!loader) return false;
        if (!p.w.window->property("isWindows").toBool()) loader->setProperty("active", true);
        return QTest::qWaitFor([&] { return (bar = p.w.window->findChild<QObject *>("windowsMenuBar")) != nullptr; }, 5000);
    }
    QObject *pdfMenu() const { return bar->findChild<QObject *>("windowsPdfMenu"); }
    QObject *item(const QString &command) const { return bar->findChild<QObject *>("windows_pdf_" + command); }
    // The menus in the bar, in order, by title.
    QStringList titles() const {
        QQmlListReference menus(bar, "menus");
        QStringList out;
        for (qsizetype i = 0; i < menus.count(); ++i) out << menus.at(i)->property("title").toString();
        return out;
    }
    // Live MenuBarItems that show the PDF menu (deferred deletes flushed first).
    int pdfBarItems() const {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        int n = 0;
        for (auto *candidate : p.w.window->findChildren<QQuickItem *>())
            if (QByteArray(candidate->metaObject()->className()).contains("MenuBarItem")
                && candidate->property("menu").value<QObject *>() == pdfMenu()) ++n;
        return n;
    }
    int focusMenuEntries() const {
        int n = 0;
        const QVariantList list = bar->property("focusMenus").value<QJSValue>().toVariant().toList();
        for (const QVariant &entry : list) n += entry.value<QObject *>() == pdfMenu();
        return n;
    }
    QObject *topMenu(const QString &title) const {
        for (auto *candidate : bar->findChildren<QObject *>())
            if (candidate->property("title").toString() == title) return candidate;
        return nullptr;
    }
    QQuickItem *editItem(const QString &text) const {
        for (auto *candidate : topMenu("&Edit")->findChildren<QObject *>())
            if (candidate->property("text").toString() == text) return qobject_cast<QQuickItem *>(candidate);
        return nullptr;
    }
    bool openPdf(const QUrl &url) { return p.open(url); }
    bool backToMarkdown() {
        if (!QMetaObject::invokeMethod(p.w.window.data(), "requestOpen", Q_ARG(QVariant, QUrl::fromLocalFile(markdown)))) return false;
        return QTest::qWaitFor([&] { return !p.w.backend.pdf()->active(); }, 5000);
    }
};
const QStringList WindowsPdfMenuCommands{"goToPage", "zoomIn", "zoomOut", "actualSize", "fitWidth", "thumbnails", "inspector", "find"};
const QStringList WindowsMarkdownMenus{"&File", "&Edit", "F&ormat", "Authors", "&View", "Window", "F&ocus", "&Go", "&Help"};
}

void FomawriteTest::windowsPdfMenuIsPresentOnlyInPdfMode() {
    QTest::failOnWarning(QRegularExpression(QStringLiteral("WindowsMenuBar\\.qml")));
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    WindowsPdfMenuFixture m; QVERIFY2(m.start(f), qPrintable(m.p.w.error));
    auto *menu = m.pdfMenu();
    QVERIFY2(menu, "the Windows bar declares one PDF menu");
    QPointer<QObject> guard(menu);
    QCOMPARE(menu->property("title").toString(), QStringLiteral("&PDF"));
    QCOMPARE(m.titles(), WindowsMarkdownMenus);
    QCOMPARE(m.pdfBarItems(), 0);
    QCOMPARE(m.focusMenuEntries(), 1);

    const QStringList withPdf = QStringList(WindowsMarkdownMenus.mid(0, 5)) << "&PDF" << WindowsMarkdownMenus.mid(5);
    for (int round = 0; round < 2; ++round) {
        // A menu that is open stays open while the PDF menu arrives and leaves.
        auto *edit = m.topMenu("&Edit"); QVERIFY(edit);
        QVERIFY(QMetaObject::invokeMethod(edit, "open"));
        QTRY_VERIFY(edit->property("opened").toBool());
        QVERIFY(m.openPdf(f.url("geometry.pdf")));
        QTRY_COMPARE(m.titles(), withPdf);
        QVERIFY(edit->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(edit, "close"));
        QTRY_VERIFY(!edit->property("visible").toBool());
        QCOMPARE(m.pdfMenu(), menu);
        QCOMPARE(m.pdfBarItems(), 1);
        QCOMPARE(m.focusMenuEntries(), 1);
        // Eight items, in the Mac menu's order and wording, all enabled.
        QStringList texts;
        for (const QString &command : WindowsPdfMenuCommands) {
            auto *item = m.item(command);
            QVERIFY2(item, qPrintable(command));
            QVERIFY2(item->property("enabled").toBool(), qPrintable(command));
            texts << item->property("text").toString();
        }
        QCOMPARE(texts, QStringList({"Go to Page…", "Zoom In", "Zoom Out", "Actual Size", "Zoom to Fit Width",
                                     "Show Thumbnails", "Show Inspector", "Find in PDF…"}));
        // A second PDF replaces the first without a second menu.
        QVERIFY(m.openPdf(f.url("navigation.pdf")));
        QCOMPARE(m.titles(), withPdf);
        QCOMPARE(m.pdfBarItems(), 1);

        // Open, then back to Markdown: the open PDF menu closes and leaves the bar.
        QVERIFY(QMetaObject::invokeMethod(menu, "open"));
        QTRY_VERIFY(menu->property("opened").toBool());
        QVERIFY(m.backToMarkdown());
        QTRY_COMPARE(m.titles(), WindowsMarkdownMenus);
        QTRY_VERIFY(!menu->property("visible").toBool());
        QCOMPARE(m.pdfBarItems(), 0);
        QCOMPARE(m.focusMenuEntries(), 1);
        // The taken menu is kept, not left to the garbage collector.
        m.p.w.engine.collectGarbage();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(guard);
        QCOMPARE(m.pdfMenu(), menu);
    }
    QVERIFY2(m.p.w.error.isEmpty(), qPrintable(m.p.w.error));
}

void FomawriteTest::windowsPdfMenuItemsDriveThePane() {
    QTest::failOnWarning(QRegularExpression(QStringLiteral("WindowsMenuBar\\.qml")));
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    WindowsPdfMenuFixture m; QVERIFY2(m.start(f), qPrintable(m.p.w.error));
    auto &p = m.p;
    QVERIFY(m.openPdf(f.url("geometry.pdf")));
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    const auto trigger = [&](const char *command) { return QMetaObject::invokeMethod(m.item(command), "triggered"); };
    // No Ctrl+Alt (AltGr) chord and no shortcut at all on the PDF items.
    for (const QString &command : WindowsPdfMenuCommands) {
        const QVariant shortcut = m.item(command)->property("shortcut");
        QVERIFY2(!shortcut.isValid() || shortcut.toString().isEmpty(), qPrintable(command));
        QVERIFY2(!m.item(command)->property("text").toString().contains("Ctrl+Alt"), qPrintable(command));
    }

    // Through the real menu: the pane keeps its focus after Zoom In.
    p.call("focusPages");
    QTRY_VERIFY(p.w.window->property("pdfOwnsKeyboard").toBool());
    QVERIFY(trigger("actualSize"));
    QTRY_COMPARE(p.zoom(), 1.0);
    auto *menu = m.pdfMenu();
    QVERIFY(QMetaObject::invokeMethod(menu, "open"));
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(cycle102Click(p.w.quick, qobject_cast<QQuickItem *>(m.item("zoomIn"))));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QTRY_COMPARE(p.zoom(), 1.1);
    QTRY_VERIFY(p.w.window->property("pdfOwnsKeyboard").toBool());

    QVERIFY(trigger("zoomOut"));
    QTRY_COMPARE(p.zoom(), 1.0);
    QVERIFY(trigger("zoomIn")); QTRY_COMPARE(p.zoom(), 1.1);
    QVERIFY(trigger("actualSize"));
    QTRY_COMPARE(p.zoom(), 1.0);
    QCOMPARE(p.reader->property("zoomMode").toString(), QStringLiteral("fixed"));
    QVERIFY(trigger("fitWidth"));
    QTRY_COMPARE(p.reader->property("zoomMode").toString(), QStringLiteral("fit"));
    QCOMPARE(m.item("thumbnails")->property("text").toString(), QStringLiteral("Show Thumbnails"));
    QVERIFY(trigger("thumbnails"));
    QTRY_VERIFY(p.item("pdfThumbnails")->isVisible());
    QCOMPARE(m.item("thumbnails")->property("text").toString(), QStringLiteral("Hide Thumbnails"));
    QVERIFY(trigger("thumbnails"));
    QTRY_VERIFY(!p.item("pdfThumbnails")->isVisible());
    QCOMPARE(m.item("thumbnails")->property("text").toString(), QStringLiteral("Show Thumbnails"));
    // Go to Page through the real menu: the page field ends up focused, not the menu's restore target.
    QVERIFY(QMetaObject::invokeMethod(menu, "open"));
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(cycle102Click(p.w.quick, qobject_cast<QQuickItem *>(m.item("goToPage"))));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QTRY_VERIFY(p.item("pdfPageField")->hasActiveFocus());
    QTest::qWait(50);
    QVERIFY(p.item("pdfPageField")->hasActiveFocus());
    QVERIFY(trigger("inspector"));
    QTRY_VERIFY(p.object("pdfInspector")->property("opened").toBool());
    QMetaObject::invokeMethod(p.object("pdfInspector"), "close");
    QTRY_VERIFY(!p.object("pdfInspector")->property("opened").toBool());
    // From Notes only, Find in PDF brings the PDF back and opens its find bar.
    p.w.layout->setProperty("layoutMode", 0);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 0);
    QVERIFY(trigger("find"));
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    QTRY_VERIFY(p.item("pdfFindBar")->isVisible());
    QTRY_VERIFY(p.item("pdfFindField")->hasActiveFocus());
    QVERIFY(!p.w.window->property("searchOpen").toBool());
    p.call("closeFind");
    QVERIFY2(p.w.error.isEmpty(), qPrintable(p.w.error));
}

void FomawriteTest::windowsPdfMenuCopyAndKeysRouteByFocus() {
    QTest::failOnWarning(QRegularExpression(QStringLiteral("WindowsMenuBar\\.qml")));
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    WindowsPdfMenuFixture m; QVERIFY2(m.start(f), qPrintable(m.p.w.error));
    auto &p = m.p;
    auto *edit = m.topMenu("&Edit"); QVERIFY(edit);
    auto *copy = m.editItem("&Copy"); QVERIFY(copy);
    QCOMPARE(copy->objectName(), QStringLiteral("windows_editCopy"));

    // Markdown, no PDF: Copy is the editor's Markdown copy, as before.
    QTRY_VERIFY(p.w.editor->property("text").toString().contains(QString::fromUtf8("café")));
    QTest::qWait(100); // the opened document settles its caret first
    const QString notes = p.w.editor->property("text").toString();
    const int cafe = notes.indexOf(QString::fromUtf8("café"));
    p.w.editor->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "select", Q_ARG(int, cafe), Q_ARG(int, cafe + 4)));
    QCOMPARE(p.w.editor->property("selectedText").toString(), QString::fromUtf8("café"));
    QGuiApplication::clipboard()->setText("before");
    QVERIFY(QMetaObject::invokeMethod(edit, "open"));
    QTRY_VERIFY(edit->property("opened").toBool());
    QVERIFY(copy->property("enabled").toBool());
    QVERIFY(cycle102Click(p.w.quick, copy));
    QTRY_VERIFY(!edit->property("visible").toBool());
    QTRY_COMPARE(QGuiApplication::clipboard()->text(), QString::fromUtf8("café"));
    QVERIFY(QGuiApplication::clipboard()->mimeData()->hasFormat("text/markdown"));

    // A PDF with notes beside it: select ALPHA in the PDF.
    QVERIFY(m.openPdf(f.url("geometry.pdf")));
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    QTRY_VERIFY(p.page(0));
    const QString text = p.w.backend.pdf()->pageText(0);
    const QVariant alpha = p.w.backend.pdf()->selectionAt(0, int(text.indexOf("ALPHA")), 5);
    QMetaObject::invokeMethod(p.reader, "setSelection", Q_ARG(QVariant, 0), Q_ARG(QVariant, alpha));
    QCOMPARE(p.reader->property("selectionText").toString(), QStringLiteral("ALPHA"));
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "insert", Q_ARG(int, 0), Q_ARG(QString, QStringLiteral("Pdf notes here.\n"))));
    const QString draft = p.w.editor->property("text").toString();

    // The pane focused: Edit → Copy (through the open menu) copies the PDF selection.
    p.call("focusPages");
    QTRY_VERIFY(p.w.window->property("pdfOwnsKeyboard").toBool());
    QGuiApplication::clipboard()->setText("before");
    QVERIFY(QMetaObject::invokeMethod(edit, "open"));
    QTRY_VERIFY(edit->property("opened").toBool());
    QVERIFY(m.bar->property("pdfOwnsEditing").toBool());
    QVERIFY(copy->property("enabled").toBool());
    QVERIFY(cycle102Click(p.w.quick, copy));
    QTRY_VERIFY(!edit->property("visible").toBool());
    QTRY_COMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("ALPHA"));
    QTRY_VERIFY(p.w.window->property("pdfOwnsKeyboard").toBool());
    // With nothing selected in the pane, Copy is disabled rather than copying the notes.
    p.call("clearSelection");
    QVERIFY(!copy->property("enabled").toBool());
    QMetaObject::invokeMethod(p.reader, "setSelection", Q_ARG(QVariant, 0), Q_ARG(QVariant, alpha));
    QVERIFY(copy->property("enabled").toBool());

    // The notes editor focused: Copy copies the editor's selection, not the PDF's.
    p.w.editor->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "select", Q_ARG(int, 0), Q_ARG(int, 3)));
    QTRY_VERIFY(!p.w.window->property("pdfOwnsKeyboard").toBool());
    QVERIFY(!m.bar->property("pdfOwnsEditing").toBool());
    QGuiApplication::clipboard()->setText("before");
    QVERIFY(QMetaObject::invokeMethod(edit, "open"));
    QTRY_VERIFY(edit->property("opened").toBool());
    QVERIFY(copy->property("enabled").toBool());
    QVERIFY(cycle102Click(p.w.quick, copy));
    QTRY_VERIFY(!edit->property("visible").toBool());
    QTRY_COMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("Pdf"));
    QTRY_VERIFY(p.w.editor->hasActiveFocus());

    // Keys with the Windows bar loaded. Pane focused: Ctrl+=/-/0 zoom the PDF, Ctrl+F opens its find.
    p.call("focusPages");
    QTRY_VERIFY(p.w.window->property("pdfOwnsKeyboard").toBool());
    p.key(Qt::Key_0, Qt::ControlModifier);
    QTRY_COMPARE(p.zoom(), 1.0);
    p.key(Qt::Key_Equal, Qt::ControlModifier);
    QTRY_COMPARE(p.zoom(), 1.1);
    p.key(Qt::Key_Minus, Qt::ControlModifier);
    QTRY_COMPARE(p.zoom(), 1.0);
    p.key(Qt::Key_F, Qt::ControlModifier);
    QTRY_VERIFY(p.item("pdfFindBar")->isVisible());
    QVERIFY(!p.w.window->property("searchOpen").toBool());
    p.call("closeFind");
    QTRY_VERIFY(!p.item("pdfFindBar")->isVisible());
    // Editor focused: the PDF's zoom is untouched and Ctrl+F opens the editor's find.
    p.w.editor->forceActiveFocus();
    QTRY_VERIFY(!p.w.window->property("pdfOwnsKeyboard").toBool());
    p.key(Qt::Key_Equal, Qt::ControlModifier);
    p.key(Qt::Key_0, Qt::ControlModifier);
    QTest::qWait(50);
    QCOMPARE(p.zoom(), 1.0);
    QCOMPARE(p.w.editor->property("text").toString(), draft);
    p.key(Qt::Key_F, Qt::ControlModifier);
    QTRY_VERIFY(p.w.window->property("searchOpen").toBool());
    QVERIFY(!p.item("pdfFindBar")->isVisible());
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "closeSearch", Q_ARG(QVariant, false)));

    // Shortcut audit: Ctrl+Shift+G stays Find Previous only; the Windows bar adds no PDF chord
    // and no Ctrl+Alt chord beyond the existing Previous Issue.
    QStringList chords;
    for (auto *candidate : p.w.window->findChildren<QObject *>()) {
        if (candidate->metaObject()->indexOfSignal("activated()") < 0) continue;
        QStringList sequences{candidate->property("sequence").toString()};
        for (const QVariant &s : candidate->property("sequences").toList()) sequences << s.toString();
        for (const QString &s : sequences) if (!s.isEmpty()) chords << QKeySequence(s).toString(QKeySequence::PortableText);
    }
    QCOMPARE(chords.count(QKeySequence("Ctrl+Shift+G").toString(QKeySequence::PortableText)), 1);
    QStringList barAltChords;
    for (auto *candidate : m.bar->findChildren<QObject *>())
        if (candidate->metaObject()->indexOfSignal("activated()") >= 0
            && QKeySequence(candidate->property("sequence").toString())[0].keyboardModifiers().testFlags(Qt::ControlModifier | Qt::AltModifier))
            barAltChords << candidate->objectName();
    QCOMPARE(barAltChords, QStringList{"windowsShortcutPreviousIssue"});
    QVERIFY2(p.w.error.isEmpty(), qPrintable(p.w.error));
}

// Copy as Markdown Quote (Mac cycle 162) on the Windows PDF menu: first item,
// enabled only with a PDF selection, same clipboard text as the Mac menu.
void FomawriteTest::windowsPdfMenuCopiesAMarkdownQuote() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    WindowsPdfMenuFixture m; QVERIFY(m.start(f));
    auto &p = m.p;
    QVERIFY(m.openPdf(f.url("geometry.pdf")));
    QTRY_VERIFY(p.page(0));
    auto *quote = m.item("copyQuote"); QVERIFY(quote);
    QVERIFY(quote->property("text").toString().startsWith("Copy as Markdown Quote"));
    QVERIFY(!quote->property("enabled").toBool());
    const QString text = p.w.backend.pdf()->pageText(0);
    const QVariant alpha = p.w.backend.pdf()->selectionAt(0, int(text.indexOf("ALPHA")), 5);
    QMetaObject::invokeMethod(p.reader, "setSelection", Q_ARG(QVariant, 0), Q_ARG(QVariant, alpha));
    QTRY_VERIFY(quote->property("enabled").toBool());
    QGuiApplication::clipboard()->setText("before");
    QVERIFY(QMetaObject::invokeMethod(quote, "triggered"));
    QTRY_COMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("> ALPHA\n\n[Page 1](geometry.pdf#page=1)\n"));
    // Its shortcut exists once, for Windows only, and is not a Ctrl+Alt chord.
    auto *shortcut = m.bar->findChild<QObject *>("windowsShortcutPdfCopyQuote"); QVERIFY(shortcut);
    QCOMPARE(shortcut->property("sequence").value<QKeySequence>(), QKeySequence("Ctrl+Shift+C"));
    QCOMPARE(shortcut->property("enabled").toBool(), p.w.window->property("isWindows").toBool());
    p.call("clearSelection");
    QTRY_VERIFY(!quote->property("enabled").toBool());
    p.w.backend.discardRecovery();
}

// Highlight Text, Underline Text and Strike Through Text (Mac cycle 163) on the
// Windows PDF menu: present, disabled without a selection, enabled with one,
// and Highlight stores a mark in the notes. No shortcuts for them.
void FomawriteTest::windowsPdfMenuMarksTheSelection() {
    Cycle103SettingsSnapshot settings;
    QSettings().remove("pdf/markupTool");
    QSettings().remove("pdf/highlightColour");
    Cycle160Fixtures f; QVERIFY(f.make());
    WindowsPdfMenuFixture m; QVERIFY(m.start(f));
    auto &p = m.p;
    QVERIFY(m.openPdf(f.url("geometry.pdf")));
    QTRY_VERIFY(p.page(0));
    const QStringList commands{"highlight", "underline", "strike"};
    const QStringList texts{"Highlight Text", "Underline Text", "Strike Through Text"};
    QList<QObject *> items;
    for (int i = 0; i < 3; ++i) {
        auto *item = m.item(commands.at(i)); QVERIFY2(item, qPrintable(commands.at(i)));
        QCOMPARE(item->objectName(), "windows_pdf_" + commands.at(i));
        QCOMPARE(item->property("text").toString(), texts.at(i));
        QVERIFY(!item->property("enabled").toBool());
        items << item;
    }
    const int alpha = int(p.w.backend.pdf()->pageText(0).indexOf("ALPHA"));
    QVERIFY(cycle163Select(p, 0, "ALPHA"));
    for (auto *item : items) QTRY_VERIFY(item->property("enabled").toBool());
    QVERIFY(p.w.editor->property("text").toString().isEmpty());
    QVERIFY(QMetaObject::invokeMethod(items.at(0), "triggered"));
    QTRY_VERIFY(p.w.editor->property("text").toString().contains(
        QStringLiteral("<!-- fomawrite:pdf highlight page=1 colour=yellow text=%1+5 -->").arg(alpha)));
    QCOMPARE(cycle163Kinds(p), QStringList{QStringLiteral("highlight:yellow:%1").arg(alpha)});
    for (auto *item : items) QTRY_VERIFY(!item->property("enabled").toBool()); // the selection is used up
    p.w.backend.discardRecovery();
}

