// Cycle 164 part 2 — re-anchoring (drawing only), the unplaced list, and
// removing a re-anchored mark. Fixture: six pages generated here
// (tests/pdffixtures.h): p1 "PAD ALPHA", p2 "PAD", p3 "OMEGA", p4 "PAD",
// p5 "PAD", p6 "FAR". Uses the cycle 160-164 helpers.

namespace {
QByteArray cycle164AnchorPdf() {
    using namespace PdfFixtures;
    const auto page = [](const QByteArray &content) { return Page{content, 0, {}, {}}; };
    return document({page(wordOps({"PAD", 100, 700}) + wordOps({"ALPHA", 100, 600})), page(wordOps({"PAD", 100, 700})),
                     page(wordOps({"OMEGA", 300, 200})), page(wordOps({"PAD", 100, 700})), page(wordOps({"PAD", 100, 700})),
                     page(wordOps({"FAR", 100, 500}))});
}
QString cycle164TextMark(const QString &kind, int page, const QString &quote, int start = 0) {
    return QStringLiteral("<!-- fomawrite:pdf %1 page=%2 colour=%3 text=%4+%5 -->\n> %6\n\n")
        .arg(kind).arg(page).arg(kind == QLatin1String("highlight") ? "green" : "red").arg(start).arg(quote.size()).arg(quote);
}
const QString cycle164Header = QStringLiteral("<!-- fomawrite:pdf-notes 1 -->\n# Notes\n\n");
bool cycle164OpenAnchor(Cycle161Pane &p, Cycle160Fixtures &f, const QString &notes, int layoutMode = 1) {
    if (!PdfFixtures::write(f.path("anchor.pdf"), cycle164AnchorPdf())) return false;
    if (!p.open(f.url("anchor.pdf"))) return false;
    p.w.editor->setProperty("text", notes);
    p.w.layout->setProperty("layoutMode", layoutMode);
    return QTest::qWaitFor([&] { return p.w.layout->property("effectiveLayoutMode").toInt() == layoutMode && p.page(0); }, 5000);
}
QVariantMap cycle164Item(Cycle161Pane &p, int index) { return cycle163Items(p).value(index).toMap(); }
int cycle164Unplaced(Cycle161Pane &p) {
    int count = 0;
    for (const QVariant &item : cycle163Items(p)) if (!item.toMap().value("placed").toBool()) ++count;
    return count;
}
}

// A mark whose text moved is drawn where the quote now is, without touching
// the notes: shifted on its page, then on another page within three pages.
// Removing a re-anchored mark takes its marker and quote out of the notes.
void FomawriteTest::pdfReanchoredMarksAreDrawnAtTheQuoteAndRemovable() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    const QString notes = cycle164Header + cycle164TextMark("highlight", 1, "ALPHA") + cycle164TextMark("underline", 1, "OMEGA");
    QVERIFY(cycle164OpenAnchor(p, f, notes));
    PdfSession *pdf = p.w.backend.pdf();
    QTRY_COMPARE(cycle163Items(p).size(), 2);
    const int alpha = int(pdf->pageText(0).indexOf("ALPHA"));
    QVERIFY(alpha > 0);
    // Same page, later in the text.
    const QVariantMap first = cycle164Item(p, 0);
    QVERIFY(first.value("placed").toBool());
    QCOMPARE(first.value("page").toInt(), 0);
    QCOMPARE(first.value("start").toInt(), alpha);
    QCOMPARE(first.value("rects").toList().size(), 1);
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 1);
    // Another page (two pages on).
    const QVariantMap second = cycle164Item(p, 1);
    QVERIFY(second.value("placed").toBool());
    QCOMPARE(second.value("page").toInt(), 2);
    QCOMPARE(second.value("start").toInt(), int(pdf->pageText(2).indexOf("OMEGA")));
    QVERIFY(!second.value("rects").toList().isEmpty());
    p.call("goToPage", 2);
    QVERIFY(p.settled());
    QTRY_COMPARE(cycle163Drawn(p, "underline"), 1);
    // Nothing was written: the notes are as they were.
    QCOMPARE(p.w.editor->property("text").toString(), notes);
    QCOMPARE(cycle164Unplaced(p), 0);
    // Removing the re-anchored underline: its marker and quote go, the rest stays.
    QVERIFY(p.w.backend.pdfMarks()->removeMark(1));
    QTRY_COMPARE(p.w.editor->property("text").toString(), cycle164Header + cycle164TextMark("highlight", 1, "ALPHA"));
    QTRY_COMPARE(cycle163Items(p).size(), 1);
    QTRY_COMPARE(cycle163Drawn(p, "underline"), 0);
    QVERIFY(p.w.backend.pdfMarks()->removeMark(0));
    QTRY_VERIFY(cycle163Items(p).isEmpty());
    QVERIFY(!p.w.editor->property("text").toString().contains("fomawrite:pdf highlight"));
}

// What cannot be placed is not drawn, stays in the file, and is listed: the
// quote gone, a quote more than three pages away, a page past the end, a note
// on a missing page or outside its page. The toolbar shows the count only then.
void FomawriteTest::pdfUnplacedItemsAreNotDrawnAndAreListed() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    const QString notes = cycle164Header
        + cycle164TextMark("highlight", 1, "ALPHA")                 // placed (shifted)
        + cycle164TextMark("highlight", 1, "GONE WITH THE WIND")    // removed from the PDF
        + cycle164TextMark("underline", 1, "FAR")                   // page 6: five pages away
        + cycle164TextMark("strike", 9, "ALPHA")                    // no page 9
        + QStringLiteral("<!-- fomawrite:pdf note page=9 at=10,10 -->\nA note on a missing page.\n\n")
        + QStringLiteral("<!-- fomawrite:pdf note page=1 at=-5,10 -->\nLeft of the page.\n\n")
        + QStringLiteral("<!-- fomawrite:pdf note page=1 at=9999,10 -->\nRight of the page.\n\n")
        + QStringLiteral("<!-- fomawrite:pdf note page=1 at=100,100 -->\nOn the page.\n");
    QVERIFY(cycle164OpenAnchor(p, f, notes));
    QTRY_COMPARE(cycle163Items(p).size(), 8);
    QTRY_COMPARE(cycle164Unplaced(p), 6);
    QCOMPARE(cycle164Notes(p).size(), 4);
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 1);
    QCOMPARE(cycle163Drawn(p, "underline"), 0);
    QCOMPARE(cycle163Drawn(p, "strike"), 0);
    QTRY_COMPARE(cycle164Pins(p).size(), 1);
    QCOMPARE(p.w.editor->property("text").toString(), notes); // the file is untouched
    // The toolbar's indicator and its list.
    QQuickItem *button = p.item("pdfUnplacedButton"); QVERIFY(button);
    QTRY_VERIFY(button->isVisible());
    QCOMPARE(button->property("text").toString(), QStringLiteral("6 unplaced"));
    QVERIFY(!p.reader->property("unplacedItems").toList().isEmpty());
    QTest::mouseClick(p.w.quick, Qt::LeftButton, Qt::NoModifier, cycle103SceneBounds(button).center().toPoint());
    auto *popup = p.w.window->findChild<QObject *>("pdfUnplacedList"); QVERIFY(popup);
    QTRY_VERIFY(popup->property("visible").toBool());
    const auto rows = cycle103VisualItems(qvariant_cast<QQuickItem *>(popup->property("contentItem")), "pdfUnplacedRow");
    QCOMPARE(rows.size(), 6);
    QStringList lines;
    for (QQuickItem *row : rows) lines << row->property("kindText").toString() + "|" + row->property("pageText").toString() + "|" + row->property("startText").toString();
    QVERIFY2(lines.contains("Highlight|p. 1|GONE WITH THE WIND"), qPrintable(lines.join("; ")));
    QVERIFY2(lines.contains("Underline|p. 1|FAR"), qPrintable(lines.join("; ")));
    QVERIFY2(lines.contains("Strikethrough|p. 9|ALPHA"), qPrintable(lines.join("; ")));
    QVERIFY2(lines.contains("Note|p. 9|A note on a missing page."), qPrintable(lines.join("; ")));
    QVERIFY2(lines.contains("Note|p. 1|Left of the page."), qPrintable(lines.join("; ")));
    QVERIFY2(lines.contains("Note|p. 1|Right of the page."), qPrintable(lines.join("; ")));
    popup->metaObject()->invokeMethod(popup, "close");
    // Delete the unplaced marks from the notes by hand: the indicator goes.
    p.w.editor->setProperty("text", cycle164Header + cycle164TextMark("highlight", 1, "ALPHA"));
    QTRY_COMPARE(cycle164Unplaced(p), 0);
    QTRY_VERIFY(!button->isVisible());
}

// A row of the list shows its item in the notes editor; with the PDF alone
// the layout switches to Split, since the item exists only in the notes.
void FomawriteTest::pdfClickingAnUnplacedRowRevealsItInTheNotes() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QString filler;
    for (int i = 0; i < 80; ++i) filler += QStringLiteral("Filler line %1 of the notes.\n\n").arg(i);
    const QString notes = filler + cycle164TextMark("highlight", 1, "ALPHA") + cycle164TextMark("underline", 1, "NOT IN THE PDF")
        + QStringLiteral("<!-- fomawrite:pdf note page=12 at=10,10 -->\nPinned to nowhere.\n");
    QVERIFY(cycle164OpenAnchor(p, f, notes, 2)); // PDF only
    QTRY_COMPARE(cycle164Unplaced(p), 2);
    QCOMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);
    p.w.editor->setProperty("cursorPosition", 0);
    QTest::qWait(700); // the window's own caret moves are quiet for a moment
    const int quote = int(notes.indexOf("> NOT IN THE PDF"));
    const int noteText = int(notes.indexOf("Pinned to nowhere."));
    QQuickItem *button = p.item("pdfUnplacedButton"); QVERIFY(button);
    const auto clickRow = [&](const QString &kind) -> bool {
        QTest::mouseClick(p.w.quick, Qt::LeftButton, Qt::NoModifier, cycle103SceneBounds(button).center().toPoint());
        if (!QTest::qWaitFor([&] { return p.w.window->findChild<QObject *>("pdfUnplacedList")->property("visible").toBool(); }, 3000)) return false;
        for (QQuickItem *row : cycle103VisualItems(qvariant_cast<QQuickItem *>(p.w.window->findChild<QObject *>("pdfUnplacedList")->property("contentItem")), "pdfUnplacedRow")) {
            if (row->property("kindText").toString() != kind) continue;
            QRectF before;
            QTest::qWaitFor([&] { const QRectF now = cycle103SceneBounds(row); const bool same = now == before; before = now; QTest::qWait(60); return same; }, 3000); // the popup has stopped opening
            QTest::mouseClick(p.w.quick, Qt::LeftButton, Qt::NoModifier, cycle103SceneBounds(row).center().toPoint());
            return true;
        }
        return false;
    };
    QVERIFY(clickRow("Underline"));
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    QTRY_COMPARE(p.w.editor->property("cursorPosition").toInt(), quote);
    QTRY_VERIFY(cycle164CaretVisible(p.w.editor));
    QCOMPARE(p.w.editor->property("text").toString(), notes);
    QTest::qWait(700);
    p.w.editor->setProperty("cursorPosition", 0);
    QTest::qWait(700);
    QVERIFY(clickRow("Note"));
    QTRY_COMPARE(p.w.editor->property("cursorPosition").toInt(), noteText);
}
