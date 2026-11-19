// Cycle 163 — notes model and text markup: PdfNotes (parse, insert, remove,
// recolour, place) on strings; marks applied to the notes through the
// editor's own edit path in Source and Live as one undo step each; the
// overlays on every fixture rotation; the highlighter, its menu, the PDF menu
// items and the page's context menu. Fixtures: tests/pdffixtures.h,
// Cycle160Fixtures and the cycle 161 and 162 helpers.

#include "pdfnotes.h"
#include "pdfmarks.h"

namespace {
// The notes file example of docs/pdf-plan.md §4.2.
const QString cycle163Canonical = QStringLiteral(
    "<!-- fomawrite:pdf-notes 1 -->\n"
    "# Notes on Report.pdf\n"
    "\n"
    "## [Page 1](Report.pdf#page=1)\n"
    "\n"
    "<!-- fomawrite:pdf highlight page=1 colour=yellow text=120+22 -->\n"
    "> Booking Number: B55577\n"
    "\n"
    "Check this matches the invoice.\n"
    "\n"
    "<!-- fomawrite:pdf note page=1 at=420,515 -->\n"
    "Ask about the return date.\n"
    "\n"
    "## [Page 3](Report.pdf#page=3)\n"
    "\n"
    "<!-- fomawrite:pdf underline page=3 colour=red text=48+31 -->\n"
    "> subject to board approval\n");

PdfNotes::Mark cycle163Mark(const QString &kind, int page, int start, const QString &text, const QString &colour = QString()) {
    PdfNotes::Mark mark;
    mark.kind = kind; mark.page = page; mark.start = start; mark.length = int(text.size()); mark.text = text; mark.colour = colour;
    return mark;
}

QString cycle163Insert(const QString &text, const PdfNotes::Mark &mark, const QString &label = QString()) {
    return PdfNotes::apply(text, PdfNotes::insertMark(text, mark, QStringLiteral("Report.pdf"), label.isEmpty() ? QString::number(mark.page) : label));
}

// The item whose marker is `marker`, or -1.
int cycle163ItemIndex(const PdfNotes::Document &document, const QString &text, const QString &marker) {
    for (int i = 0; i < document.items.size(); ++i) {
        const auto &item = document.items.at(i);
        if (text.mid(item.markerStart, item.markerEnd - item.markerStart).trimmed() == marker) return i;
    }
    return -1;
}

// Every change leaves the text before and after it alone.
bool cycle163Minimal(const QString &before, const PdfNotes::Edit &edit) {
    int previous = 0;
    for (const auto &change : edit) {
        if (change.position < previous || change.position + change.removed > before.size()) return false;
        previous = change.position + change.removed;
    }
    return true;
}
}

void FomawriteTest::pdfNotesParsesEveryFormatFeature() {
    const QString text = cycle163Canonical;
    const PdfNotes::Document document = PdfNotes::parse(text);
    QVERIFY(document.hasHeader);
    QCOMPARE(document.headerStart, 0);
    QCOMPARE(document.headerLineEnd, int(text.indexOf('\n')) + 1);
    QCOMPARE(document.items.size(), 3);
    const auto &highlight = document.items.at(0);
    QCOMPARE(highlight.kind, QStringLiteral("highlight"));
    QCOMPARE(highlight.page, 1);
    QCOMPARE(highlight.colour, QStringLiteral("yellow"));
    QCOMPARE(highlight.start, 120);
    QCOMPARE(highlight.length, 22);
    QVERIFY(highlight.hasQuote);
    QCOMPARE(highlight.quote, QStringLiteral("Booking Number: B55577"));
    QCOMPARE(text.mid(highlight.markerStart, highlight.markerEnd - highlight.markerStart),
             QStringLiteral("<!-- fomawrite:pdf highlight page=1 colour=yellow text=120+22 -->"));
    QCOMPARE(highlight.markerLineEnd, highlight.markerEnd + 1);
    QCOMPARE(text.mid(highlight.quoteStart, highlight.quoteEnd - highlight.quoteStart), QStringLiteral("> Booking Number: B55577\n"));
    QCOMPARE(highlight.end, int(text.indexOf("<!-- fomawrite:pdf note")));
    QCOMPARE(highlight.keys.size(), 3);
    QCOMPARE(highlight.keys.at(1).name, QStringLiteral("colour"));
    QCOMPARE(text.mid(highlight.keys.at(1).valueStart, highlight.keys.at(1).valueEnd - highlight.keys.at(1).valueStart), QStringLiteral("yellow"));
    const auto &note = document.items.at(1);
    QCOMPARE(note.kind, QStringLiteral("note"));
    QCOMPARE(note.page, 1);
    QCOMPARE(note.value("at"), QStringLiteral("420,515"));
    QVERIFY(!note.hasQuote); // a note's text is its own, not a quote
    QCOMPARE(note.end, int(text.indexOf("## [Page 3]")));
    const auto &underline = document.items.at(2);
    QCOMPARE(underline.kind, QStringLiteral("underline"));
    QCOMPARE(underline.page, 3);
    QCOMPARE(underline.colour, QStringLiteral("red"));
    QCOMPARE(underline.quote, QStringLiteral("subject to board approval"));
    QCOMPARE(underline.end, int(text.size()));
    QCOMPARE(underline.quoteEnd, int(text.size()));
    QCOMPARE(document.sections.size(), 2);
    QCOMPARE(document.sections.at(0).page, 1);
    QCOMPARE(document.sections.at(0).heading, QStringLiteral("## [Page 1](Report.pdf#page=1)"));
    QCOMPARE(document.sections.at(0).end, int(text.indexOf("## [Page 3]")));
    QCOMPARE(document.sections.at(1).page, 3);
    QCOMPARE(document.sections.at(1).end, int(text.size()));

    // Multi-line quotes keep their lines; a blank line inside is ">".
    const QString multi = QStringLiteral("<!-- fomawrite:pdf strike page=2 colour=red text=0+30 -->\n> first line\n>\n> second line\nA comment.\n");
    const auto item = PdfNotes::parse(multi).items.value(0);
    QCOMPARE(item.kind, QStringLiteral("strike"));
    QCOMPARE(item.quote, QStringLiteral("first line\n\nsecond line"));
    QCOMPARE(PdfNotes::normalised(item.quote), QStringLiteral("first line second line"));
    QCOMPARE(item.quoteEnd, int(multi.indexOf("A comment.")));

    // The writer's helpers: the same tidying and link encoding as Copy as Markdown Quote.
    QCOMPARE(PdfNotes::quoteBlock(QStringLiteral("  The  quick\tbrown \r\nfox\n\n\n jumps ")), QStringLiteral("> The quick brown\n> fox\n>\n> jumps"));
    QCOMPARE(PdfNotes::pageHeading(QStringLiteral("My Report (draft).pdf"), 3, QStringLiteral("iii")),
             QStringLiteral("## [Page iii](My%20Report%20%28draft%29.pdf#page=3)"));
    QCOMPARE(PdfNotes::pageHeading(QStringLiteral("Report.pdf"), 2, QStringLiteral("2")), QStringLiteral("## [Page 2](Report.pdf#page=2)"));
    QCOMPARE(PdfNotes::markerLine(cycle163Mark("underline", 3, 48, QStringLiteral("x"))),
             QStringLiteral("<!-- fomawrite:pdf underline page=3 colour=red text=48+1 -->"));
    QCOMPARE(PdfNotes::highlightColours(), QStringList({"yellow", "green", "blue", "pink", "purple"}));
}

void FomawriteTest::pdfNotesParseToleratesHostileInput() {
    // Windows line breaks: the same items, offsets into the CRLF text, no '\r' in quotes.
    QString crlf = cycle163Canonical;
    crlf.replace("\n", "\r\n");
    const auto windows = PdfNotes::parse(crlf);
    QVERIFY(windows.hasHeader);
    QCOMPARE(windows.items.size(), 3);
    QCOMPARE(windows.items.at(0).quote, QStringLiteral("Booking Number: B55577"));
    QVERIFY(crlf.mid(windows.items.at(0).markerStart).startsWith("<!-- fomawrite:pdf highlight"));
    QCOMPARE(crlf.mid(windows.items.at(0).markerEnd, 2), QStringLiteral("\r\n"));
    QCOMPARE(windows.sections.size(), 2);
    QCOMPARE(windows.sections.at(1).heading, QStringLiteral("## [Page 3](Report.pdf#page=3)"));

    // No header line: the marks are still read.
    const auto headless = PdfNotes::parse(cycle163Canonical.mid(cycle163Canonical.indexOf('\n') + 1));
    QVERIFY(!headless.hasHeader);
    QCOMPARE(headless.items.size(), 3);

    // Markers in fenced code blocks (backticks, tildes, longer fences, an unclosed fence) and indented code are text.
    const QString fenced = QStringLiteral(
        "```markdown\n<!-- fomawrite:pdf highlight page=1 text=0+5 -->\n> quoted\n```\n"
        "~~~~\n<!-- fomawrite:pdf highlight page=2 text=0+5 -->\n~~~\n## [Page 9](x.pdf#page=9)\n~~~~\n"
        "    <!-- fomawrite:pdf highlight page=3 text=0+5 -->\n"
        "<!-- fomawrite:pdf underline page=4 colour=red text=1+2 -->\n> ok\n"
        "````\n<!-- fomawrite:pdf highlight page=5 text=0+5 -->\n");
    const auto code = PdfNotes::parse(fenced);
    QCOMPARE(code.items.size(), 1);
    QCOMPARE(code.items.at(0).page, 4);
    QCOMPARE(code.items.at(0).quote, QStringLiteral("ok"));
    QVERIFY(code.sections.isEmpty());

    // Tabs and spaces inside and around a marker, up to three spaces of indentation.
    const auto spaced = PdfNotes::parse(QStringLiteral("   <!--\tfomawrite:pdf   highlight\tpage=2  colour=green text=3+4   -->  \n  >   spaced  quote\n"));
    QCOMPARE(spaced.items.size(), 1);
    QCOMPARE(spaced.items.at(0).page, 2);
    QCOMPARE(spaced.items.at(0).colour, QStringLiteral("green"));
    QCOMPARE(spaced.items.at(0).start, 3);
    QCOMPARE(PdfNotes::normalised(spaced.items.at(0).quote), QStringLiteral("spaced quote"));
    // Not markers: no kind, another tool's comment, a marker sharing its line with text.
    QVERIFY(PdfNotes::parse(QStringLiteral("<!-- fomawrite:pdf -->\n<!-- other:pdf highlight page=1 -->\nText <!-- fomawrite:pdf highlight page=1 text=0+1 -->\n")).items.isEmpty());

    // Reordered pages, edited and other headings, the header lower down.
    const QString edited = QStringLiteral(
        "# My own title\n\n<!-- fomawrite:pdf-notes 1 -->\n\n## Page 3 — what the board said\n\n"
        "<!-- fomawrite:pdf underline page=3 colour=red text=4+5 -->\n> board\n\n"
        "### [Pages](Report.pdf#page=4)\n\ntext\n\n## [Page 1](Report.pdf#page=1)\n\n## Summary\n");
    const auto reordered = PdfNotes::parse(edited);
    QVERIFY(reordered.hasHeader);
    QCOMPARE(reordered.headerStart, int(edited.indexOf("<!--")));
    QCOMPARE(reordered.sections.size(), 3);
    QCOMPARE(reordered.sections.at(0).page, 3);
    QCOMPARE(reordered.sections.at(0).end, int(edited.indexOf("### [Pages]"))); // a page heading ends it
    QCOMPARE(reordered.sections.at(1).page, 4);
    QCOMPARE(reordered.sections.at(1).level, 3);
    QCOMPARE(reordered.sections.at(2).page, 1);
    QCOMPARE(reordered.sections.at(2).end, int(edited.indexOf("## Summary")));
    QCOMPARE(reordered.items.at(0).end, int(edited.indexOf("### [Pages]"))); // headings end items

    // Unknown kinds and keys are read and kept by every edit.
    const QString unknown = QStringLiteral("<!-- fomawrite:pdf-notes 1 -->\n\n"
        "<!-- fomawrite:pdf star page=2 points=1,2,3 future=yes -->\nA star.\n\n"
        "<!-- fomawrite:pdf highlight page=2 future=kept colour=blue text=0+5 later=also -->\n> ALPHA\n");
    const auto kept = PdfNotes::parse(unknown);
    QCOMPARE(kept.items.size(), 2);
    QCOMPARE(kept.items.at(0).kind, QStringLiteral("star"));
    QCOMPARE(kept.items.at(0).value("points"), QStringLiteral("1,2,3"));
    QCOMPARE(kept.items.at(0).value("future"), QStringLiteral("yes"));
    QVERIFY(!kept.items.at(0).hasQuote);
    QCOMPARE(kept.items.at(1).keys.size(), 5);
    const QString recoloured = PdfNotes::apply(unknown, PdfNotes::recolourMark(unknown, kept.items.at(1), QStringLiteral("pink")));
    QCOMPARE(recoloured, QString(unknown).replace("colour=blue", "colour=pink"));
    const QString removed = PdfNotes::apply(unknown, PdfNotes::removeMark(unknown, kept.items.at(1), QStringLiteral("ALPHA")));
    QCOMPARE(removed, unknown.left(unknown.indexOf("\n<!-- fomawrite:pdf highlight") + 1).chopped(1)); // the star is untouched
}

void FomawriteTest::pdfNotesInsertsTheCanonicalExampleExactly() {
    // The first highlight on a PDF with no notes.
    const QString first = cycle163Insert(QString(), cycle163Mark("highlight", 1, 120, QStringLiteral("Booking Number:  B55577\n"), "yellow"));
    QCOMPARE(first, QStringLiteral(
        "<!-- fomawrite:pdf-notes 1 -->\n# Notes on Report.pdf\n\n## [Page 1](Report.pdf#page=1)\n\n"
        "<!-- fomawrite:pdf highlight page=1 colour=yellow text=120+24 -->\n> Booking Number: B55577\n"));
    // The writer comments and adds a note; then an underline on page 3 gives the plan's example byte for byte.
    QString text = cycle163Canonical.left(cycle163Canonical.indexOf("## [Page 3]") - 1);
    const QString underlined = cycle163Insert(text, cycle163Mark("underline", 3, 48, QStringLiteral("subject to board approval")));
    QCOMPARE(underlined, QString(cycle163Canonical).replace("text=48+31", "text=48+25"));
    const auto edit = PdfNotes::insertMark(text, cycle163Mark("underline", 3, 48, QStringLiteral("subject to board approval")), "Report.pdf", "3");
    QCOMPARE(edit.size(), 1);
    QCOMPARE(edit.first().position, int(text.size()));
    QCOMPARE(edit.first().removed, 0);
    // An underline on a page with a label: the heading names the label.
    QCOMPARE(cycle163Insert(first, cycle163Mark("underline", 3, 7, QStringLiteral("Preface")), QStringLiteral("iii")),
             first + QStringLiteral("\n## [Page iii](Report.pdf#page=3)\n\n<!-- fomawrite:pdf underline page=3 colour=red text=7+7 -->\n> Preface\n"));
    // Nothing to mark: no edit.
    QVERIFY(PdfNotes::insertMark(first, cycle163Mark("highlight", 1, 0, QStringLiteral(" \n ")), "Report.pdf", "1").isEmpty());
    QVERIFY(PdfNotes::insertMark(first, cycle163Mark("note", 1, 0, QStringLiteral("x")), "Report.pdf", "1").isEmpty());
    QVERIFY(PdfNotes::insertMark(first, cycle163Mark("highlight", 0, 0, QStringLiteral("x")), "Report.pdf", "1").isEmpty());
}

void FomawriteTest::pdfNotesInsertKeepsPagesAndTextOrder() {
    const QString h = PdfNotes::headerLine();
    // Free-form notes without marks: the header goes at the very top, the page at the end; nothing else moves.
    const QString freeForm = QStringLiteral("My thoughts on the report.\n\nMore later.\n");
    const auto freeEdit = PdfNotes::insertMark(freeForm, cycle163Mark("highlight", 2, 5, QStringLiteral("ALPHA"), "green"), "Report.pdf", "2");
    QCOMPARE(freeEdit.size(), 2);
    QVERIFY(cycle163Minimal(freeForm, freeEdit));
    QCOMPARE(freeEdit.at(0), (PdfNotes::Change{0, 0, h + "\n\n"}));
    QCOMPARE(PdfNotes::apply(freeForm, freeEdit), h + "\n\n" + freeForm
             + "\n## [Page 2](Report.pdf#page=2)\n\n<!-- fomawrite:pdf highlight page=2 colour=green text=5+5 -->\n> ALPHA\n");
    // A last line without a line break keeps having none.
    QCOMPARE(cycle163Insert(QStringLiteral("Unfinished"), cycle163Mark("strike", 1, 0, QStringLiteral("X"))),
             h + "\n\nUnfinished\n\n## [Page 1](Report.pdf#page=1)\n\n<!-- fomawrite:pdf strike page=1 colour=red text=0+1 -->\n> X");

    // Pages in ascending order; items in text order within a page; comments stay with their items.
    QString notes = cycle163Insert(QString(), cycle163Mark("highlight", 3, 40, QStringLiteral("three")));
    notes = cycle163Insert(notes, cycle163Mark("highlight", 1, 10, QStringLiteral("one")));
    notes = cycle163Insert(notes, cycle163Mark("highlight", 5, 0, QStringLiteral("five")));
    notes += QStringLiteral("\n## Summary\n\nTo do.\n");
    notes = cycle163Insert(notes, cycle163Mark("underline", 4, 0, QStringLiteral("four")));
    notes = cycle163Insert(notes, cycle163Mark("underline", 9, 0, QStringLiteral("nine")));
    notes.insert(notes.indexOf("\n\n## [Page 3]") + 1, QStringLiteral("\nMy comment on one.\n"));
    notes = cycle163Insert(notes, cycle163Mark("highlight", 1, 50, QStringLiteral("later")));
    notes = cycle163Insert(notes, cycle163Mark("highlight", 1, 2, QStringLiteral("earlier")));
    const QString expected = h + QStringLiteral("\n# Notes on Report.pdf\n\n"
        "## [Page 1](Report.pdf#page=1)\n\n"
        "<!-- fomawrite:pdf highlight page=1 colour=yellow text=2+7 -->\n> earlier\n\n"
        "<!-- fomawrite:pdf highlight page=1 colour=yellow text=10+3 -->\n> one\n\nMy comment on one.\n\n"
        "<!-- fomawrite:pdf highlight page=1 colour=yellow text=50+5 -->\n> later\n\n"
        "## [Page 3](Report.pdf#page=3)\n\n"
        "<!-- fomawrite:pdf highlight page=3 colour=yellow text=40+5 -->\n> three\n\n"
        "## [Page 4](Report.pdf#page=4)\n\n"
        "<!-- fomawrite:pdf underline page=4 colour=red text=0+4 -->\n> four\n\n"
        "## [Page 5](Report.pdf#page=5)\n\n"
        "<!-- fomawrite:pdf highlight page=5 colour=yellow text=0+4 -->\n> five\n\n"
        "## [Page 9](Report.pdf#page=9)\n\n"
        "<!-- fomawrite:pdf underline page=9 colour=red text=0+4 -->\n> nine\n\n"
        "## Summary\n\nTo do.\n");
    QCOMPARE(notes, expected);

    // An edited page heading still holds its page's items; reordered sections are left as they are.
    const QString reordered = h + QStringLiteral("\n\n## Page 3 (edited)\n\n<!-- fomawrite:pdf highlight page=3 colour=yellow text=0+1 -->\n> a\n\n"
                                                 "## [Page 1](Report.pdf#page=1)\n");
    QCOMPARE(cycle163Insert(reordered, cycle163Mark("highlight", 3, 9, QStringLiteral("b"))),
             h + QStringLiteral("\n\n## Page 3 (edited)\n\n<!-- fomawrite:pdf highlight page=3 colour=yellow text=0+1 -->\n> a\n\n"
                                "<!-- fomawrite:pdf highlight page=3 colour=yellow text=9+1 -->\n> b\n\n"
                                "## [Page 1](Report.pdf#page=1)\n"));
    // A deleted header line is not put back while the notes hold marks.
    const QString headless = cycle163Canonical.mid(cycle163Canonical.indexOf('\n') + 1);
    const auto headlessEdit = PdfNotes::insertMark(headless, cycle163Mark("highlight", 2, 0, QStringLiteral("x")), "Report.pdf", "2");
    QCOMPARE(headlessEdit.size(), 1);
    QVERIFY(headlessEdit.first().position > 0);
    // Windows line breaks are kept.
    QString crlf = cycle163Canonical;
    crlf.replace("\n", "\r\n");
    const QString crlfAfter = cycle163Insert(crlf, cycle163Mark("highlight", 2, 0, QStringLiteral("two")));
    QCOMPARE(crlfAfter.count("\r\n"), crlfAfter.count('\n'));
    QVERIFY(crlfAfter.contains("\r\n\r\n## [Page 2](Report.pdf#page=2)\r\n\r\n<!-- fomawrite:pdf highlight page=2 colour=yellow text=0+3 -->\r\n> two\r\n\r\n## [Page 3]"));
}

void FomawriteTest::pdfNotesRemoveAndRecolour() {
    const QString text = cycle163Canonical;
    const auto document = PdfNotes::parse(text);
    // An untouched quote: marker and quote go; the comment stays.
    const auto edit = PdfNotes::removeMark(text, document.items.at(0), QStringLiteral("Booking Number:\r\nB55577"));
    QVERIFY(cycle163Minimal(text, edit));
    QCOMPARE(edit.size(), 1);
    QCOMPARE(PdfNotes::apply(text, edit), QString(text).replace(
        QStringLiteral("<!-- fomawrite:pdf highlight page=1 colour=yellow text=120+22 -->\n> Booking Number: B55577\n\n"), QString()));
    // An edited quote: only the marker goes; the edited quote and the comment stay as plain Markdown.
    QString editedQuote = text;
    editedQuote.replace("> Booking Number: B55577", "> Booking Number: B55577 (my emphasis)");
    const auto editedEdit = PdfNotes::removeMark(editedQuote, PdfNotes::parse(editedQuote).items.at(0), QStringLiteral("Booking Number: B55577"));
    QCOMPARE(PdfNotes::apply(editedQuote, editedEdit),
             QString(editedQuote).replace("<!-- fomawrite:pdf highlight page=1 colour=yellow text=120+22 -->\n", QString()));
    // The page's last item takes its generated heading with it.
    const auto lastOnPage = PdfNotes::removeMark(text, document.items.at(2), QStringLiteral("subject to board approval"));
    QCOMPARE(PdfNotes::apply(text, lastOnPage), text.left(text.indexOf("\n## [Page 3]")));
    // ... but not a heading the writer has changed, nor one with their text under it.
    QString renamed = text;
    renamed.replace("## [Page 3](Report.pdf#page=3)", "## [Page 3](Report.pdf#page=3) — the board");
    QCOMPARE(PdfNotes::apply(renamed, PdfNotes::removeMark(renamed, PdfNotes::parse(renamed).items.at(2), "subject to board approval")),
             renamed.left(renamed.indexOf("<!-- fomawrite:pdf underline") - 1));
    QString commented = text + QStringLiteral("\nWhich board?\n");
    QCOMPARE(PdfNotes::apply(commented, PdfNotes::removeMark(commented, PdfNotes::parse(commented).items.at(2), "subject to board approval")),
             QString(commented).replace("<!-- fomawrite:pdf underline page=3 colour=red text=48+31 -->\n> subject to board approval\n\n", QString()));
    // A note has no quote: its marker goes and its text stays.
    QCOMPARE(PdfNotes::apply(text, PdfNotes::removeMark(text, document.items.at(1), QString())),
             QString(text).replace("<!-- fomawrite:pdf note page=1 at=420,515 -->\n", QString()));
    // A stale item (the text changed under it) is refused.
    QVERIFY(PdfNotes::removeMark(QStringLiteral("short"), document.items.at(2), "x").isEmpty());

    // Recolour: just the value; a marker without colour= gets one.
    const auto recolour = PdfNotes::recolourMark(text, document.items.at(0), QStringLiteral("purple"));
    QCOMPARE(recolour.size(), 1);
    QCOMPARE(recolour.first().removed, 6);
    QCOMPARE(PdfNotes::apply(text, recolour), QString(text).replace("highlight page=1 colour=yellow", "highlight page=1 colour=purple"));
    QVERIFY(PdfNotes::recolourMark(text, document.items.at(0), QStringLiteral("yellow")).isEmpty());
    const QString plain = QStringLiteral("<!-- fomawrite:pdf highlight page=1 text=0+5 -->\n> ALPHA\n");
    QCOMPARE(PdfNotes::apply(plain, PdfNotes::recolourMark(plain, PdfNotes::parse(plain).items.at(0), "green")),
             QStringLiteral("<!-- fomawrite:pdf highlight page=1 text=0+5 colour=green -->\n> ALPHA\n"));
}

void FomawriteTest::pdfNotesInsertThenRemoveRestoresTheBytes() {
    QString crlf = cycle163Canonical;
    crlf.replace("\n", "\r\n");
    const QStringList texts{
        QString(), QStringLiteral("\n"), QStringLiteral("  \n"), QStringLiteral("Free text"), QStringLiteral("Free text\n"),
        QStringLiteral("Free text\n\n"), QStringLiteral("Free text\n\n\n"), QStringLiteral("# Notes on Report.pdf\n"),
        QStringLiteral("# Title\nNo blank lines\n## [Page 2](Report.pdf#page=2)\n<!-- fomawrite:pdf highlight page=2 colour=yellow text=100+1 -->\n> q\n## Summary\nend"),
        cycle163Canonical, crlf, cycle163Canonical.mid(cycle163Canonical.indexOf('\n') + 1),
        cycle163Canonical + QStringLiteral("\n## Summary\n\nTo do.\n"),
        QStringLiteral("```\n<!-- fomawrite:pdf highlight page=1 text=0+5 -->\n```\n"),
        QStringLiteral("## [Page 3](Report.pdf#page=3)\n\nmy words only\n"),
        QStringLiteral("<!-- fomawrite:pdf underline page=1 colour=red text=0+3 -->\n> abc\nComment right under.\n<!-- fomawrite:pdf highlight page=1 colour=yellow text=200+3 -->\n> xyz")};
    const QList<PdfNotes::Mark> marks{
        cycle163Mark("highlight", 1, 0, QStringLiteral("First words")), cycle163Mark("highlight", 1, 150, QStringLiteral("Middle\nof two lines"), "blue"),
        cycle163Mark("underline", 2, 0, QStringLiteral("two")), cycle163Mark("strike", 3, 500, QStringLiteral("late on three")),
        cycle163Mark("highlight", 9, 0, QStringLiteral("nine"), "purple")};
    int cases = 0;
    for (const QString &text : texts) {
        for (const auto &mark : marks) {
            const QString where = QStringLiteral("text %1, mark %2").arg(texts.indexOf(text)).arg(PdfNotes::markerLine(mark));
            const auto insert = PdfNotes::insertMark(text, mark, QStringLiteral("Report.pdf"), QString::number(mark.page));
            QVERIFY2(!insert.isEmpty() && cycle163Minimal(text, insert), qPrintable(where));
            for (const auto &change : insert) QVERIFY2(change.removed == 0, qPrintable(where));
            const QString after = PdfNotes::apply(text, insert);
            const auto document = PdfNotes::parse(after);
            const int index = cycle163ItemIndex(document, after, PdfNotes::markerLine(mark));
            QVERIFY2(index >= 0, qPrintable(where + "\n" + after));
            QCOMPARE(document.items.at(index).quote, PdfNotes::quoteBlock(mark.text).remove(QRegularExpression("^> ?", QRegularExpression::MultilineOption)));
            const auto remove = PdfNotes::removeMark(after, document.items.at(index), mark.text);
            QVERIFY2(cycle163Minimal(after, remove), qPrintable(where));
            const QString restored = PdfNotes::apply(after, remove);
            QVERIFY2(restored == text, qPrintable(where + "\n--- after insert:\n" + after + "\n--- after remove:\n" + restored));
            ++cases;
        }
    }
    QCOMPARE(cases, int(texts.size() * marks.size()));
    // The one exception, as documented: notes holding the header line but no marks lose it.
    const QString headerOnly = PdfNotes::headerLine() + QStringLiteral("\n\nWords.\n");
    const QString after = cycle163Insert(headerOnly, cycle163Mark("highlight", 1, 0, QStringLiteral("x")));
    QCOMPARE(PdfNotes::apply(after, PdfNotes::removeMark(after, PdfNotes::parse(after).items.at(0), "x")), QStringLiteral("Words.\n"));
}

void FomawriteTest::pdfNotesPlaceFindsMovedText() {
    QHash<int, QString> pages{{1, QStringLiteral("Intro ALPHA beta\r\ngamma")}, {2, QStringLiteral("nothing here")},
                              {3, QStringLiteral("x ALPHA beta gamma")}, {6, QStringLiteral("delta far away")}};
    const auto pageText = [&](int page) { return pages.value(page); };
    const auto item = [](int page, int start, int length, const QString &quote) {
        const QString text = QStringLiteral("<!-- fomawrite:pdf highlight page=%1 colour=yellow text=%2+%3 -->\n").arg(page).arg(start).arg(length)
            + PdfNotes::quoteBlock(quote) + "\n";
        return PdfNotes::parse(text).items.value(0);
    };
    // Exact (whitespace-normalised, across the page's line break).
    auto placed = PdfNotes::place(item(1, 12, 11, QStringLiteral("beta gamma")), pageText);
    QVERIFY(placed.placed);
    QCOMPARE(placed.page, 1); QCOMPARE(placed.start, 12); QCOMPARE(placed.length, 11);
    // Shifted on the page: the nearest occurrence.
    placed = PdfNotes::place(item(1, 0, 5, QStringLiteral("ALPHA")), pageText);
    QVERIFY(placed.placed); QCOMPARE(placed.page, 1); QCOMPARE(placed.start, 6); QCOMPARE(placed.length, 5);
    pages[1] = QStringLiteral("ALPHA one, ALPHA two, ALPHA three");
    placed = PdfNotes::place(item(1, 13, 5, QStringLiteral("ALPHA")), pageText);
    QCOMPARE(placed.start, 11);
    // Across a changed line break.
    pages[1] = QStringLiteral("Intro ALPHA\nbeta");
    placed = PdfNotes::place(item(1, 0, 10, QStringLiteral("ALPHA beta")), pageText);
    QVERIFY(placed.placed); QCOMPARE(placed.start, 6); QCOMPARE(placed.length, 10);
    // Moved to a neighbouring page (two pages on), the next page first.
    placed = PdfNotes::place(item(1, 0, 16, QStringLiteral("ALPHA beta gamma")), pageText);
    QVERIFY(placed.placed); QCOMPARE(placed.page, 3); QCOMPARE(placed.start, 2); QCOMPARE(placed.length, 16);
    placed = PdfNotes::place(item(5, 0, 5, QStringLiteral("gamma")), pageText);
    QVERIFY(placed.placed); QCOMPARE(placed.page, 3);
    // Four pages away, or gone: unplaced.
    QVERIFY(!PdfNotes::place(item(2, 0, 5, QStringLiteral("delta")), pageText).placed);
    QVERIFY(!PdfNotes::place(item(1, 0, 5, QStringLiteral("omega")), pageText).placed);
    // No quote: the stored range when the page has it.
    const auto bare = PdfNotes::parse(QStringLiteral("<!-- fomawrite:pdf highlight page=2 text=0+7 -->\n")).items.value(0);
    placed = PdfNotes::place(bare, pageText);
    QVERIFY(placed.placed); QCOMPARE(placed.start, 0); QCOMPARE(placed.length, 7);
    QVERIFY(!PdfNotes::place(PdfNotes::parse(QStringLiteral("<!-- fomawrite:pdf highlight page=2 text=10+7 -->\n")).items.value(0), pageText).placed);
    QVERIFY(!PdfNotes::place(PdfNotes::parse(QStringLiteral("<!-- fomawrite:pdf highlight text=0+2 -->\n> no\n")).items.value(0), pageText).placed);
}

namespace {
QRectF cycle163WordRect(PdfSession *pdf, int page, const QString &word) {
    const int start = int(pdf->pageText(page).indexOf(word));
    return pdf->selectionAt(page, start, int(word.size())).value("rects").toList().value(0).toRectF();
}
bool cycle163Select(Cycle161Pane &p, int page, const QString &word) {
    PdfSession *pdf = p.w.backend.pdf();
    const int start = int(pdf->pageText(page).indexOf(word));
    return start >= 0 && QMetaObject::invokeMethod(p.reader, "setSelection", Q_ARG(QVariant, page),
                                                   Q_ARG(QVariant, pdf->selectionAt(page, start, int(word.size()))));
}
QVariantList cycle163Items(Cycle161Pane &p) { return p.w.backend.pdfMarks()->items(); }
int cycle163Drawn(Cycle161Pane &p, const char *kind) {
    return int(cycle103VisualItems(p.list, QStringLiteral("pdfMark_") + QLatin1String(kind)).size());
}
QString cycle163Marker(PdfSession *pdf, const QString &kind, const QString &colour, int page, const QString &word) {
    const int start = int(pdf->pageText(page).indexOf(word));
    return QStringLiteral("<!-- fomawrite:pdf %1 page=%2 colour=%3 text=%4+%5 -->").arg(kind).arg(page + 1).arg(colour).arg(start).arg(word.size());
}
}

// Risk 2 of the plan: a mark is a host edit of the notes document. In Source
// it is one QTextDocument edit block: one undo step, the editor's caret and
// selection kept on their text, focus left in the PDF pane.
void FomawriteTest::pdfMarksAreOneUndoStepInSource() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    QTRY_VERIFY(p.page(0));
    PdfSession *pdf = p.w.backend.pdf();
    const QString before = QStringLiteral("My notes\n");
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "insert", Q_ARG(int, 0), Q_ARG(QString, before)));
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "select", Q_ARG(int, 3), Q_ARG(int, 8)));
    QVERIFY(cycle163Select(p, 0, "ALPHA"));
    p.call("focusPages");
    QTRY_VERIFY(p.list->hasActiveFocus());
    QCOMPARE(p.call("markSelection", QStringLiteral("highlight"), QStringLiteral("green")).toString(), QStringLiteral("added"));
    const QString marked = PdfNotes::headerLine() + QStringLiteral("\n\nMy notes\n\n## [Page 1](geometry.pdf#page=1)\n\n")
        + cycle163Marker(pdf, "highlight", "green", 0, "ALPHA") + QStringLiteral("\n> ALPHA\n");
    QCOMPARE(p.w.editor->property("text").toString(), marked);
    QCOMPARE(p.reader->property("selectionText").toString(), QString()); // cleared
    // The editor's selection stays on "notes"; the PDF pane keeps focus; the notes are modified.
    QCOMPARE(p.w.editor->property("selectedText").toString(), QStringLiteral("notes"));
    QVERIFY(p.list->hasActiveFocus());
    QVERIFY(p.w.backend.modified());
    QCOMPARE(cycle163Items(p).size(), 1);
    QVERIFY(cycle163Items(p).first().toMap().value("placed").toBool());
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 1);
    // Cmd+Z with the PDF pane focused: one step takes both the header and the item back.
    QTest::keyClick(p.w.quick, Qt::Key_Z, Qt::ControlModifier);
    QTRY_COMPARE(p.w.editor->property("text").toString(), before);
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 0);
    QVERIFY(cycle163Items(p).isEmpty());
    QTest::keyClick(p.w.quick, Qt::Key_Z, Qt::ControlModifier | Qt::ShiftModifier);
    QTRY_COMPARE(p.w.editor->property("text").toString(), marked);
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 1);
    // Removing from the page is one step too, and undo puts the mark back.
    QVERIFY(p.w.backend.pdfMarks()->removeMark(0));
    QCOMPARE(p.w.editor->property("text").toString(), before);
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 0);
    QTest::keyClick(p.w.quick, Qt::Key_Z, Qt::ControlModifier);
    QTRY_COMPARE(p.w.editor->property("text").toString(), marked);
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 1);
    QVERIFY(p.list->hasActiveFocus());
}

namespace {
// The Live page's selection as it is now.
QPair<int, int> cycle163LiveSelection(EditorBridge *bridge) {
    QSignalSpy received(bridge, &EditorBridge::selectionReceived);
    const int token = bridge->fetchSelection();
    if (!QTest::qWaitFor([&] { for (const auto &args : received) if (args.at(0).toInt() == token) return true; return false; }, 5000)) return {-1, -1};
    for (const auto &args : received)
        if (args.at(0).toInt() == token) return {qMin(args.at(1).toInt(), args.at(2).toInt()), qMax(args.at(1).toInt(), args.at(2).toInt())};
    return {-1, -1};
}
}

// In Live the host edit reaches the page as one change set (header and item
// together), which the page records as one step of its own history: Undo with
// the PDF pane focused goes to the page, which takes the whole mark back.
void FomawriteTest::pdfMarksAreOneUndoStepInLive() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    QTRY_VERIFY(p.page(0));
    PdfSession *pdf = p.w.backend.pdf();
    const QString before = QStringLiteral("My notes\n");
    p.w.editor->setProperty("text", before);
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "selectWritingMode", Q_ARG(QVariant, QStringLiteral("live"))));
    QVERIFY(p.w.layout->property("liveEditEnabled").toBool());
    auto *bridge = p.w.backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 30000);
    auto *view = p.w.window->findChild<QObject *>("liveEditorView"); QVERIFY(view);
    QTRY_VERIFY_WITH_TIMEOUT(!view->property("loading").toBool(), 15000);
    QTRY_COMPARE_WITH_TIMEOUT(cycle144PageText(bridge), before, 10000);
    bridge->placeSelection(3, 8);
    QTRY_COMPARE(cycle163LiveSelection(bridge), qMakePair(3, 8));
    QVERIFY(cycle163Select(p, 0, "ALPHA"));
    p.call("focusPages");
    QTRY_VERIFY(p.list->hasActiveFocus());
    QCOMPARE(p.call("markSelection", QStringLiteral("highlight"), QStringLiteral("blue")).toString(), QStringLiteral("added"));
    const QString marked = PdfNotes::headerLine() + QStringLiteral("\n\nMy notes\n\n## [Page 1](geometry.pdf#page=1)\n\n")
        + cycle163Marker(pdf, "highlight", "blue", 0, "ALPHA") + QStringLiteral("\n> ALPHA\n");
    QCOMPARE(p.w.editor->property("text").toString(), marked);
    QTRY_COMPARE_WITH_TIMEOUT(cycle144PageText(bridge), marked, 10000);
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 1);
    // The page's selection follows its text ("notes" moved by the header); focus stays in the PDF pane.
    const int shift = int(PdfNotes::headerLine().size()) + 2;
    QCOMPARE(cycle163LiveSelection(bridge), qMakePair(3 + shift, 8 + shift));
    QVERIFY(p.list->hasActiveFocus());
    QVERIFY(p.w.backend.modified());
    {   // How a marker reads in Live today, for the cycle record (research/cycle-163/captures, local only).
        const QString directory = QDir(QFileInfo(QFINDTESTDATA("tst_fomawrite.cpp")).absolutePath()).filePath("../research/cycle-163/captures");
        QDir().mkpath(directory);
        QTest::qWait(300);
        p.w.quick->grabWindow().save(directory + "/live-marker.png");
    }
    // Cmd+Z in the PDF pane: the page undoes the whole mark in one step and the host follows.
    QTest::keyClick(p.w.quick, Qt::Key_Z, Qt::ControlModifier);
    QTRY_COMPARE_WITH_TIMEOUT(p.w.editor->property("text").toString(), before, 10000);
    QCOMPARE(cycle144PageText(bridge), before);
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 0);
    QVERIFY(cycle163Items(p).isEmpty());
    // The page's own history agrees: nothing earlier to undo there (an undo made
    // on the document underneath would have left the mark and its reversal in it).
    bridge->requestUndo();
    QTest::qWait(300);
    QCOMPARE(cycle144PageText(bridge), before);
    QCOMPARE(p.w.editor->property("text").toString(), before);
    QTest::keyClick(p.w.quick, Qt::Key_Z, Qt::ControlModifier | Qt::ShiftModifier);
    QTRY_COMPARE_WITH_TIMEOUT(p.w.editor->property("text").toString(), marked, 10000);
    QCOMPARE(cycle144PageText(bridge), marked);
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 1);
    QVERIFY(p.list->hasActiveFocus());
    // Recolouring and removing from the page are single steps there too, even
    // in quick succession (the page would otherwise group adjacent edits made
    // within half a second).
    QVERIFY(p.w.backend.pdfMarks()->recolourMark(0, QStringLiteral("pink")));
    QTRY_COMPARE_WITH_TIMEOUT(cycle144PageText(bridge), QString(marked).replace("colour=blue", "colour=pink"), 10000);
    QVERIFY(p.w.backend.pdfMarks()->removeMark(0));
    QTRY_COMPARE_WITH_TIMEOUT(cycle144PageText(bridge), before, 10000);
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 0);
    QTest::keyClick(p.w.quick, Qt::Key_Z, Qt::ControlModifier);
    QTRY_COMPARE_WITH_TIMEOUT(p.w.editor->property("text").toString(), QString(marked).replace("colour=blue", "colour=pink"), 10000);
    QTest::keyClick(p.w.quick, Qt::Key_Z, Qt::ControlModifier);
    QTRY_COMPARE_WITH_TIMEOUT(p.w.editor->property("text").toString(), marked, 10000);
    QTRY_COMPARE(cycle163Items(p).value(0).toMap().value("colour").toString(), QStringLiteral("blue"));
    p.w.layout->setProperty("liveEditEnabled", false);
}

// Untitled notes (the PDF had none): marking writes nothing to disk; recovery
// keeps the mark like typing; Save writes X.pdf.md with exactly the notes.
void FomawriteTest::pdfMarkInUntitledNotesIsWrittenOnlyBySave() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    QTRY_VERIFY(p.page(0));
    PdfSession *pdf = p.w.backend.pdf();
    const QString notes = f.path("geometry.pdf.md");
    QVERIFY(!QFileInfo::exists(notes));
    QVERIFY(p.w.backend.fileUrl().isEmpty());
    QVERIFY(cycle163Select(p, 0, "ALPHA"));
    QCOMPARE(p.call("markWithTool").toString(), QStringLiteral("added"));
    const QString expected = PdfNotes::headerLine() + QStringLiteral("\n# Notes on geometry.pdf\n\n## [Page 1](geometry.pdf#page=1)\n\n")
        + cycle163Marker(pdf, "highlight", "yellow", 0, "ALPHA") + QStringLiteral("\n> ALPHA\n");
    QCOMPARE(p.w.editor->property("text").toString(), expected);
    QVERIFY(p.w.backend.modified());
    QVERIFY(!QFileInfo::exists(notes));
    QTRY_VERIFY_WITH_TIMEOUT(!cycle162Snapshot(expected).isEmpty(), 5000); // recovery has it, as for typing
    QVERIFY(!QFileInfo::exists(notes));
    QVERIFY(QMetaObject::invokeMethod(&p.w.backend, "save"));
    QTRY_VERIFY(QFileInfo::exists(notes));
    QCOMPARE(cycle160Read(notes), expected.toUtf8());
    QTRY_VERIFY(!p.w.backend.modified());
    QCOMPARE(p.w.backend.fileUrl(), QUrl::fromLocalFile(notes));
    QCOMPARE(cycle163Items(p).size(), 1);
}

namespace {
struct Cycle163Probe {
    Cycle161Pane &p;
    QImage image;
    void grab() { image = p.w.quick->grabWindow(); }
    QColor at(int page, QPointF points) const {
        const QPointF scene = p.scene(page, points) * image.devicePixelRatio();
        return image.pixelColor(scene.toPoint());
    }
};
bool cycle163White(const QColor &c) { return c.red() > 235 && c.green() > 235 && c.blue() > 235; }
bool cycle163Red(const QColor &c) { return c.red() > 190 && c.green() < 120 && c.blue() < 120; }
bool cycle163Near(const QColor &c, const QColor &expected, int tolerance = 28) {
    return qAbs(c.red() - expected.red()) <= tolerance && qAbs(c.green() - expected.green()) <= tolerance && qAbs(c.blue() - expected.blue()) <= tolerance;
}
// A highlight colour drawn at half opacity over the white page.
QColor cycle163OnWhite(const QString &name) {
    const QColor c(name);
    return QColor((255 + c.red()) / 2, (255 + c.green()) / 2, (255 + c.blue()) / 2);
}
QString cycle163Describe(const QColor &c) { return c.name(); }
}

// Overlays from the notes on every fixture rotation and the CropBox page:
// a highlight covers the word with the line's height, an underline sits just
// under it and a strikethrough through its middle, along the text's direction.
void FomawriteTest::pdfMarkOverlaysFollowTheTextOnEveryRotation() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    QTRY_VERIFY(p.page(0));
    PdfSession *pdf = p.w.backend.pdf();
    PdfMarks *marks = p.w.backend.pdfMarks();
    const QMap<QString, QString> colours{{"yellow", "#ffd60a"}, {"green", "#34c759"}, {"blue", "#3d9df5"}, {"pink", "#ff5c9d"}, {"purple", "#a970ff"}};
    const int directions[] = {0, 90, 180, 270, 0};
    for (int page = 0; page < 5; ++page) QCOMPARE(pdf->textDirection(page), directions[page]);
    Cycle163Probe probe{p, {}};
    for (const QString kind : {QStringLiteral("highlight"), QStringLiteral("underline"), QStringLiteral("strike")}) {
        p.w.editor->setProperty("text", QString());
        for (int page = 0; page < 5; ++page)
            for (const QString word : {QStringLiteral("ALPHA"), QStringLiteral("OMEGA")})
                QCOMPARE(marks->mark(kind, kind == "highlight" ? "yellow" : "red", page, int(pdf->pageText(page).indexOf(word)), 5), QStringLiteral("added"));
        QCOMPARE(cycle163Items(p).size(), 10);
        for (const QVariant &item : cycle163Items(p)) QVERIFY(item.toMap().value("placed").toBool());
        for (int page = 0; page < 5; ++page) {
            const int direction = directions[page];
            const bool vertical = direction == 90 || direction == 270;
            // Unit vectors in view points: along the text, and from the baseline outwards (towards the descenders).
            const QPointF along = direction == 0 ? QPointF(1, 0) : direction == 90 ? QPointF(0, 1) : direction == 180 ? QPointF(-1, 0) : QPointF(0, -1);
            const QPointF out = direction == 0 ? QPointF(0, 1) : direction == 90 ? QPointF(-1, 0) : direction == 180 ? QPointF(0, -1) : QPointF(1, 0);
            for (const QString word : {QStringLiteral("ALPHA"), QStringLiteral("OMEGA")}) {
                const QString where = QStringLiteral("%1 %2 on page %3").arg(kind, word).arg(page + 1);
                const QRectF r = cycle163WordRect(pdf, page, word);
                const qreal h = vertical ? r.width() : r.height(), length = vertical ? r.height() : r.width();
                const QPointF baseline = direction == 0 ? QPointF(r.center().x(), r.bottom()) : direction == 90 ? QPointF(r.left(), r.center().y())
                    : direction == 180 ? QPointF(r.center().x(), r.top()) : QPointF(r.right(), r.center().y());
                const auto point = [&](qreal rise, qreal t) { return baseline - out * (rise * h) + along * (t * length); };
                p.reveal(page, r.center());
                QTRY_VERIFY(p.page(page) && p.page(page)->property("rendered").toBool());
                QTest::qWait(50);
                probe.grab();
                // The mark item for this word runs along the text.
                bool found = false;
                for (auto *item : cycle103VisualItems(p.list, "pdfMark_" + kind)) {
                    const QRectF drawn = cycle103SceneBounds(item);
                    const QRectF wordScene = QRectF(p.scene(page, r.topLeft()), p.scene(page, r.bottomRight())).normalized();
                    if (!drawn.intersects(wordScene.adjusted(-h * 2, -h * 2, h * 2, h * 2))) continue;
                    found = true;
                    if (kind != "highlight") QVERIFY2(vertical ? drawn.height() > 3 * drawn.width() : drawn.width() > 3 * drawn.height(), qPrintable(where));
                    const qreal drawnLength = vertical ? drawn.height() : drawn.width(), wordLength = vertical ? wordScene.height() : wordScene.width();
                    QVERIFY2(qAbs(drawnLength - wordLength) < h * p.scale() * 0.2 + 2, qPrintable(where));
                }
                QVERIFY2(found, qPrintable(where));
                for (const qreal t : {-0.35, 0.0, 0.35}) {
                    if (kind == "highlight") {
                        const QColor inside = probe.at(page, point(-0.16, t));
                        QVERIFY2(cycle163Near(inside, cycle163OnWhite(colours.value("yellow"))), qPrintable(where + " inside " + cycle163Describe(inside)));
                        QVERIFY2(cycle163White(probe.at(page, point(-0.9, t))), qPrintable(where + " below"));
                        QVERIFY2(cycle163White(probe.at(page, point(1.6, t))), qPrintable(where + " above"));
                    } else {
                        const qreal at = kind == "underline" ? -0.12 : 0.42;
                        bool red = false;
                        for (const qreal nudge : {-0.03, 0.0, 0.03}) red = red || cycle163Red(probe.at(page, point(at + nudge, t)));
                        QVERIFY2(red, qPrintable(where + " line " + cycle163Describe(probe.at(page, point(at, t)))));
                        QVERIFY2(!cycle163Red(probe.at(page, point(kind == "underline" ? 0.42 : -0.12, t))), qPrintable(where + " other line"));
                        QVERIFY2(cycle163White(probe.at(page, point(-0.6, t))), qPrintable(where + " below"));
                    }
                }
                // Nothing past the ends of the word.
                QVERIFY2(cycle163White(probe.at(page, baseline + along * (length / 2 + 0.6 * h) - out * (-0.16 * h))), qPrintable(where + " after"));
                QVERIFY2(cycle163White(probe.at(page, baseline - along * (length / 2 + 0.6 * h) - out * (-0.16 * h))), qPrintable(where + " before"));
            }
        }
    }
    // The five highlight colours, each distinct on the page.
    p.w.editor->setProperty("text", QString());
    const QString alpha = QStringLiteral("ALPHA");
    QCOMPARE(marks->mark("highlight", "yellow", 0, int(pdf->pageText(0).indexOf(alpha)), 5), QStringLiteral("added"));
    const QRectF r = cycle163WordRect(pdf, 0, alpha);
    p.reveal(0, r.center());
    QList<QColor> seen;
    for (const QString &name : PdfNotes::highlightColours()) {
        if (name != "yellow") QVERIFY(marks->recolourMark(0, name));
        QTRY_COMPARE(cycle163Items(p).value(0).toMap().value("colour").toString(), name);
        QTest::qWait(50);
        probe.grab();
        const QColor c = probe.at(0, QPointF(r.center().x(), r.bottom() + 0.16 * r.height()));
        QVERIFY2(cycle163Near(c, cycle163OnWhite(colours.value(name))), qPrintable(name + " " + c.name()));
        for (const QColor &other : seen) {
            const int hue = qAbs(c.hslHue() - other.hslHue());
            QVERIFY2(qMin(hue, 360 - hue) > 25, qPrintable(name + " too close to " + other.name()));
        }
        seen << c;
    }
}

// The overlays come from the notes text: typing a marker draws it, the quote
// re-anchors a stale range, an edited quote leaves it unplaced, deleting the
// marker takes it away, undo puts it back, and a reload from disk is followed.
void FomawriteTest::pdfMarksFollowEditsToTheNotes() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_VERIFY(p.page(1));
    PdfSession *pdf = p.w.backend.pdf();
    const int omega = int(pdf->pageText(1).indexOf("OMEGA"));
    const QString marker = cycle163Marker(pdf, "highlight", "green", 1, "OMEGA");
    const QString typed = QStringLiteral("# Mine\n\n") + marker + QStringLiteral("\n> OMEGA\n\nMy comment.\n");
    p.w.editor->setProperty("text", typed);
    QTRY_COMPARE(cycle163Items(p).size(), 1);
    auto item = cycle163Items(p).first().toMap();
    QVERIFY(item.value("placed").toBool());
    QCOMPARE(item.value("page").toInt(), 1);
    QCOMPARE(item.value("start").toInt(), omega);
    QCOMPARE(item.value("colour").toString(), QStringLiteral("green"));
    QCOMPARE(item.value("direction").toInt(), 90);
    QVERIFY(item.value("vertical").toBool());
    QCOMPARE(item.value("rects").toList().size(), 1);
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 1);
    // A stale range is placed by its quote.
    QString stale = typed;
    stale.replace(QStringLiteral("text=%1+5").arg(omega), QStringLiteral("text=0+5"));
    p.w.editor->setProperty("text", stale);
    QTRY_COMPARE(cycle163Items(p).value(0).toMap().value("start").toInt(), omega);
    QVERIFY(cycle163Items(p).value(0).toMap().value("placed").toBool());
    // An edited quote cannot be placed: not drawn, still listed.
    p.w.editor->setProperty("text", QString(typed).replace("> OMEGA", "> OMEGA, as I read it"));
    QTRY_VERIFY(!cycle163Items(p).value(0).toMap().value("placed").toBool());
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 0);
    // Typing in the editor: deleting the marker line takes the mark away; undo brings it back.
    p.w.editor->setProperty("text", typed);
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 1);
    p.w.editor->forceActiveFocus();
    const int from = int(typed.indexOf(marker));
    QVERIFY(QMetaObject::invokeMethod(p.w.editor, "remove", Q_ARG(int, from), Q_ARG(int, from + int(marker.size()) + 1)));
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 0);
    QVERIFY(cycle163Items(p).isEmpty());
    QVERIFY(QMetaObject::invokeMethod(p.w.window.data(), "undoEditing"));
    QTRY_COMPARE(p.w.editor->property("text").toString(), typed);
    QTRY_COMPARE(cycle163Drawn(p, "highlight"), 1);
    // Saved, then changed on disk and reloaded: the overlay follows the file.
    QVERIFY(QMetaObject::invokeMethod(&p.w.backend, "save"));
    QTRY_VERIFY(!p.w.backend.modified());
    const QString outside = QString(typed).replace("highlight", "strike").replace("colour=green", "colour=red");
    QVERIFY(PdfFixtures::write(f.path("geometry.pdf.md"), outside.toUtf8()));
    p.w.backend.reloadFromDisk(); // the writer's "Reload" answer to the external-change prompt
    QTRY_COMPARE_WITH_TIMEOUT(p.w.editor->property("text").toString(), outside, 5000);
    QTRY_COMPARE(cycle163Drawn(p, "strike"), 1);
    QCOMPARE(cycle163Drawn(p, "highlight"), 0);
}

namespace {
void cycle163Drag(Cycle161Pane &p, int page, const QString &word) {
    const QRectF r = cycle163WordRect(p.w.backend.pdf(), page, word);
    p.reveal(page, r.center());
    const QPoint from = p.scene(page, QPointF(r.left() + 0.5, r.center().y())).toPoint();
    const QPoint to = p.scene(page, QPointF(r.right() + 1, r.center().y())).toPoint();
    QTest::mousePress(p.w.quick, Qt::LeftButton, Qt::NoModifier, from);
    QTest::mouseMove(p.w.quick, (from + to) / 2);
    QTest::mouseMove(p.w.quick, to);
    QTest::mouseRelease(p.w.quick, Qt::LeftButton, Qt::NoModifier, to);
}
QStringList cycle163Kinds(Cycle161Pane &p) {
    QStringList kinds;
    for (const QVariant &item : cycle163Items(p))
        kinds << item.toMap().value("kind").toString() + ":" + item.toMap().value("colour").toString() + ":"
                 + QString::number(item.toMap().value("start").toInt());
    return kinds;
}
}

// Preview's highlighter: the pen marks a selection, or toggles marking mode
// (every drag or double-click is marked; Escape or the pen ends it); the menu
// sets the colour or underline/strikethrough, remembered in app settings.
void FomawriteTest::pdfHighlighterPenMenuAndMarkingMode() {
    Cycle103SettingsSnapshot settings;
    QSettings().remove("pdf/markupTool");
    QSettings().remove("pdf/highlightColour");
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    QTRY_VERIFY(p.page(0));
    PdfSession *pdf = p.w.backend.pdf();
    PdfMarks *marks = p.w.backend.pdfMarks();
    const int alpha = int(pdf->pageText(0).indexOf("ALPHA")), omega = int(pdf->pageText(0).indexOf("OMEGA"));
    auto *pen = p.item("pdfHighlighter"); QVERIFY(pen);
    auto *arrow = p.item("pdfHighlighterMenuButton"); QVERIFY(arrow);
    auto *swatch = p.item("pdfHighlighterSwatch"); QVERIFY(swatch);
    QCOMPARE(marks->tool(), QStringLiteral("highlight"));
    QCOMPARE(marks->highlightColour(), QStringLiteral("yellow"));
    QCOMPARE(swatch->property("color").value<QColor>(), QColor("#ffd60a"));
    QCOMPARE(pen->property("hint").toString(), QStringLiteral("Highlight text as you select it"));

    // With a selection the pen marks it and clears it.
    cycle163Drag(p, 0, "ALPHA");
    QTRY_COMPARE(p.reader->property("selectionText").toString(), QStringLiteral("ALPHA"));
    QCOMPARE(pen->property("hint").toString(), QStringLiteral("Highlight the selected text"));
    QVERIFY(cycle102Click(p.w.quick, pen));
    QCOMPARE(cycle163Kinds(p), QStringList{QStringLiteral("highlight:yellow:%1").arg(alpha)});
    QCOMPARE(p.reader->property("selectionText").toString(), QString());
    QVERIFY(!p.reader->property("markingMode").toBool());
    QVERIFY(p.list->hasActiveFocus()); // the pen does not take focus from the pages

    // Without one it turns marking mode on: a double-click marks at once, and
    // dragging over an existing mark of the same kind removes it (Preview's toggle).
    QVERIFY(cycle102Click(p.w.quick, pen));
    QVERIFY(p.reader->property("markingMode").toBool());
    QVERIFY(pen->property("checked").toBool());
    QCOMPARE(pen->property("hint").toString(), QStringLiteral("Stop marking"));
    const QRectF omegaRect = cycle163WordRect(pdf, 0, "OMEGA");
    p.reveal(0, omegaRect.center());
    QTest::mouseDClick(p.w.quick, Qt::LeftButton, Qt::NoModifier, p.scene(0, omegaRect.center()).toPoint());
    QTRY_COMPARE(cycle163Items(p).size(), 2);
    QCOMPARE(p.reader->property("selectionText").toString(), QString());
    cycle163Drag(p, 0, "ALPHA");
    QTRY_COMPARE(cycle163Kinds(p), QStringList{QStringLiteral("highlight:yellow:%1").arg(omega)});
    QVERIFY(p.reader->property("markingMode").toBool());
    // Escape ends it; so does another click on the pen.
    p.key(Qt::Key_Escape);
    QVERIFY(!p.reader->property("markingMode").toBool());
    QVERIFY(!pen->property("checked").toBool());
    QVERIFY(cycle102Click(p.w.quick, pen));
    QVERIFY(p.reader->property("markingMode").toBool());
    QVERIFY(cycle102Click(p.w.quick, pen));
    QVERIFY(!p.reader->property("markingMode").toBool());
    // Out of marking mode a drag only selects.
    cycle163Drag(p, 0, "ALPHA");
    QTRY_COMPARE(p.reader->property("selectionText").toString(), QStringLiteral("ALPHA"));
    QCOMPARE(cycle163Items(p).size(), 1);
    p.call("clearSelection");

    // The menu: five colours with the current one ticked, Underline, Strikethrough.
    auto *menu = p.object("pdfHighlighterMenu"); QVERIFY(menu);
    QVERIFY(cycle102Click(p.w.quick, arrow));
    QTRY_VERIFY(menu->property("opened").toBool());
    const QStringList entries{"pdfToolYellow", "pdfToolGreen", "pdfToolBlue", "pdfToolPink", "pdfToolPurple", "pdfToolUnderline", "pdfToolStrike"};
    const QStringList labels{"Yellow", "Green", "Blue", "Pink", "Purple", "Underline", "Strikethrough"};
    for (int i = 0; i < entries.size(); ++i) {
        auto *entry = p.object(entries.at(i).toUtf8().constData()); QVERIFY2(entry, qPrintable(entries.at(i)));
        QCOMPARE(entry->property("text").toString(), labels.at(i));
        QCOMPARE(entry->property("ticked").toBool(), i == 0);
        QCOMPARE(entry->property("swatch").value<QColor>().alpha() > 0, i < 5);
    }
    QVERIFY(QMetaObject::invokeMethod(p.object("pdfToolGreen"), "click"));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QCOMPARE(marks->highlightColour(), QStringLiteral("green"));
    QCOMPARE(QSettings().value("pdf/highlightColour").toString(), QStringLiteral("green"));
    QVERIFY(p.object("pdfToolGreen")->property("ticked").toBool());
    QVERIFY(!p.object("pdfToolYellow")->property("ticked").toBool());
    QCOMPARE(swatch->property("color").value<QColor>(), QColor("#34c759"));
    QCOMPARE(cycle163Items(p).size(), 1); // no selection: nothing marked
    // With a selection, choosing from the menu also applies it.
    cycle163Drag(p, 0, "ALPHA");
    QTRY_COMPARE(p.reader->property("selectionText").toString(), QStringLiteral("ALPHA"));
    QVERIFY(cycle102Click(p.w.quick, arrow));
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(p.object("pdfToolUnderline"), "click"));
    QTRY_COMPARE(cycle163Items(p).size(), 2);
    QVERIFY(cycle163Kinds(p).contains(QStringLiteral("underline:red:%1").arg(alpha)));
    QCOMPARE(marks->tool(), QStringLiteral("underline"));
    QCOMPARE(QSettings().value("pdf/markupTool").toString(), QStringLiteral("underline"));
    QVERIFY(p.object("pdfToolUnderline")->property("ticked").toBool());
    QVERIFY(!p.object("pdfToolGreen")->property("ticked").toBool());
    QCOMPARE(swatch->property("color").value<QColor>(), QColor("#ff3b30"));
    QCOMPARE(pen->property("hint").toString(), QStringLiteral("Underline text as you select it"));
    // The pen now underlines; back to a colour, it highlights in that colour.
    cycle163Drag(p, 0, "OMEGA");
    QTRY_COMPARE(p.reader->property("selectionText").toString(), QStringLiteral("OMEGA"));
    QVERIFY(cycle102Click(p.w.quick, pen));
    QVERIFY(cycle163Kinds(p).contains(QStringLiteral("underline:red:%1").arg(omega)));
    QVERIFY(cycle102Click(p.w.quick, arrow));
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(p.object("pdfToolPurple"), "click"));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QCOMPARE(marks->tool(), QStringLiteral("highlight"));
    QCOMPARE(marks->highlightColour(), QStringLiteral("purple"));
    // Another window (another PdfMarks) starts with the remembered tool.
    PdfSession otherSession;
    PdfMarks other(&otherSession, [] { return QString(); }, [](const PdfNotes::Edit &) { return false; });
    QCOMPARE(other.tool(), QStringLiteral("highlight"));
    QCOMPARE(other.highlightColour(), QStringLiteral("purple"));
    QCOMPARE(pen->property("hint").toString(), QStringLiteral("Highlight text as you select it"));
    marks->setTool(QStringLiteral("strike"));
    QCOMPARE(pen->property("hint").toString(), QStringLiteral("Strike through text as you select it"));

    // The highlighter never folds: in a narrow pane it stays in the toolbar,
    // clear of the page field and the controls on the right.
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 1);
    auto *toolbar = p.item("pdfToolbar"); QVERIFY(toolbar);
    auto *group = p.item("pdfHighlighterGroup"); QVERIFY(group);
    for (const int width : {320, 420, 600}) {
        p.w.layout->setProperty("previewWidth", width);
        QTRY_VERIFY2(qAbs(p.reader->width() - width) < 1.5, qPrintable(QString::number(p.reader->width())));
        QTest::qWait(30);
        QVERIFY(group->isVisible());
        const QRectF bounds = cycle103SceneBounds(group);
        QVERIFY2(cycle103SceneBounds(toolbar).adjusted(-0.5, -0.5, 0.5, 0.5).contains(bounds), qPrintable(QString::number(width)));
        for (const char *name : {"pdfFitWidth", "pdfPageField", "pdfPageCount", "pdfInspectorButton", "pdfFindButton", "pdfToolbarOverflow"}) {
            auto *other = p.item(name);
            if (!other || !other->isVisible()) continue;
            const QRectF overlap = bounds.intersected(cycle103SceneBounds(other));
            QVERIFY2(overlap.width() <= 0.5 || overlap.height() <= 0.5, qPrintable(QStringLiteral("%1 at %2").arg(name).arg(width)));
        }
    }
}

// The PDF menu's Highlight / Underline / Strike Through Text (Preview's
// shortcuts, free in Fomawrite) need a selection; the page's context menu
// marks a selection, and over a mark recolours (highlights) or removes it.
void FomawriteTest::pdfMarkupMenuItemsAndPageContextMenu() {
    Cycle103SettingsSnapshot settings;
    QSettings().remove("pdf/markupTool");
    QSettings().remove("pdf/highlightColour");
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    QVERIFY(p.open(f.url("geometry.pdf")));
    QTRY_VERIFY(p.page(0));
    PdfSession *pdf = p.w.backend.pdf();
    const int alpha = int(pdf->pageText(0).indexOf("ALPHA")), omega = int(pdf->pageText(0).indexOf("OMEGA"));
    const Cycle161Menus menus{p.w.window.data()};
    const QStringList names{"highlight", "underline", "strike"}; // command ids
    const QStringList texts{"Highlight Text", "Underline Text", "Strike Through Text"};
    const QStringList shortcuts{"Meta+Ctrl+H", "Meta+Ctrl+U", "Meta+Ctrl+S"};
    QList<QObject *> items;
    for (int i = 0; i < 3; ++i) {
        auto *item = menus.item(names.at(i)); QVERIFY2(item, qPrintable(names.at(i)));
        QCOMPARE(item->property("text").toString(), texts.at(i));
        if (menus.native()) QCOMPARE(QKeySequence(item->property("shortcut").toString()), QKeySequence(shortcuts.at(i)));
        QVERIFY(!item->property("enabled").toBool());
        items << item;
    }
    // No other command in the window uses these shortcuts.
    // (Windows has no shortcuts for these: Ctrl+Alt is AltGr on many layouts.)
    if (menus.native()) for (auto *object : p.w.window->findChildren<QObject *>()) {
        if (items.contains(object)) continue;
        QStringList used;
        for (const char *property : {"shortcut", "sequence"}) {
            const QVariant value = object->property(property);
            if (value.isValid()) used << QKeySequence(value.toString()).toString();
        }
        for (const QVariant &value : object->property("sequences").toList()) used << QKeySequence(value.toString()).toString();
        for (const QString &shortcut : shortcuts)
            QVERIFY2(!used.contains(QKeySequence(shortcut).toString()), qPrintable(shortcut + " is also used by " + object->objectName()));
    }
    QVERIFY(cycle163Select(p, 0, "ALPHA"));
    for (auto *item : items) QTRY_VERIFY(item->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(items.at(0), "triggered"));
    QCOMPARE(cycle163Kinds(p), QStringList{QStringLiteral("highlight:yellow:%1").arg(alpha)});
    for (auto *item : items) QTRY_VERIFY(!item->property("enabled").toBool()); // the selection is used up
    QVERIFY(cycle163Select(p, 0, "OMEGA"));
    QVERIFY(QMetaObject::invokeMethod(items.at(1), "triggered"));
    QVERIFY(cycle163Select(p, 0, "ALPHA"));
    QVERIFY(QMetaObject::invokeMethod(items.at(2), "triggered"));
    QCOMPARE(cycle163Kinds(p).size(), 3); // a strikethrough over a highlight is allowed
    QVERIFY(cycle163Kinds(p).contains(QStringLiteral("underline:red:%1").arg(omega)));
    QVERIFY(cycle163Kinds(p).contains(QStringLiteral("strike:red:%1").arg(alpha)));
    // The same kind over exactly the same text removes the mark.
    QVERIFY(cycle163Select(p, 0, "ALPHA"));
    QVERIFY(QMetaObject::invokeMethod(items.at(2), "triggered"));
    QCOMPARE(cycle163Kinds(p).size(), 2);
    QVERIFY(!cycle163Kinds(p).contains(QStringLiteral("strike:red:%1").arg(alpha)));
    QCOMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2); // marking left the layout alone

    // Context menu over a selection: Highlight, Underline, Strikethrough.
    auto *menu = p.object("pdfPageMenu"); QVERIFY(menu);
    const QRectF omegaRect = cycle163WordRect(pdf, 0, "OMEGA"), alphaRect = cycle163WordRect(pdf, 0, "ALPHA");
    p.w.editor->setProperty("text", QString());
    QTRY_VERIFY(cycle163Items(p).isEmpty());
    QVERIFY(cycle163Select(p, 0, "OMEGA"));
    p.reveal(0, omegaRect.center());
    cycle162ContextClick(p.w.quick, p.scene(0, omegaRect.center()).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    for (const char *name : {"pdfContextHighlight", "pdfContextUnderline", "pdfContextStrike"}) QVERIFY2(p.object(name)->property("visible").toBool(), name);
    QVERIFY(!p.object("pdfContextRemoveMark")->property("visible").toBool());
    QVERIFY(!p.object("pdfContextColour_yellow")->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(p.object("pdfContextStrike"), "click"));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QCOMPARE(cycle163Kinds(p), QStringList{QStringLiteral("strike:red:%1").arg(omega)});
    QVERIFY(cycle163Select(p, 0, "ALPHA"));
    p.reveal(0, alphaRect.center());
    cycle162ContextClick(p.w.quick, p.scene(0, alphaRect.center()).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(p.object("pdfContextHighlight"), "click"));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QVERIFY(cycle163Kinds(p).contains(QStringLiteral("highlight:yellow:%1").arg(alpha)));

    // Over a highlight, without a selection: the five colours (the current one ticked) and Remove Highlight.
    QCOMPARE(p.reader->property("selectionText").toString(), QString());
    cycle162ContextClick(p.w.quick, p.scene(0, alphaRect.center()).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(!p.object("pdfContextHighlight")->property("visible").toBool());
    const QStringList colours{"yellow", "green", "blue", "pink", "purple"};
    for (const QString &colour : colours) {
        auto *entry = p.object(("pdfContextColour_" + colour).toUtf8().constData());
        QVERIFY2(entry && entry->property("visible").toBool(), qPrintable(colour));
        QCOMPARE(entry->property("ticked").toBool(), colour == "yellow");
    }
    QCOMPARE(p.object("pdfContextRemoveMark")->property("text").toString(), QStringLiteral("Remove Highlight"));
    QVERIFY(QMetaObject::invokeMethod(p.object("pdfContextColour_pink"), "click"));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QTRY_VERIFY(cycle163Kinds(p).contains(QStringLiteral("highlight:pink:%1").arg(alpha)));
    QVERIFY(p.w.editor->property("text").toString().contains(QStringLiteral("highlight page=1 colour=pink text=%1+5").arg(alpha)));
    cycle162ContextClick(p.w.quick, p.scene(0, alphaRect.center()).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(p.object("pdfContextColour_pink")->property("ticked").toBool());
    QVERIFY(QMetaObject::invokeMethod(p.object("pdfContextRemoveMark"), "click"));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QCOMPARE(cycle163Kinds(p), QStringList{QStringLiteral("strike:red:%1").arg(omega)});
    // Over a strikethrough: no colours, Remove Strikethrough; over an underline, Remove Underline.
    p.reveal(0, omegaRect.center());
    cycle162ContextClick(p.w.quick, p.scene(0, omegaRect.center()).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    QVERIFY(!p.object("pdfContextColour_yellow")->property("visible").toBool());
    QCOMPARE(p.object("pdfContextRemoveMark")->property("text").toString(), QStringLiteral("Remove Strikethrough"));
    QVERIFY(QMetaObject::invokeMethod(p.object("pdfContextRemoveMark"), "click"));
    QTRY_VERIFY(!menu->property("visible").toBool());
    QVERIFY(cycle163Items(p).isEmpty());
    QVERIFY(cycle163Select(p, 0, "OMEGA"));
    QVERIFY(QMetaObject::invokeMethod(items.at(1), "triggered"));
    cycle162ContextClick(p.w.quick, p.scene(0, omegaRect.center()).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    QCOMPARE(p.object("pdfContextRemoveMark")->property("text").toString(), QStringLiteral("Remove Underline"));
    QVERIFY(QMetaObject::invokeMethod(menu, "close"));
    // Away from marks and without a selection: none of the marking entries.
    p.reveal(0, QPointF(30, 30));
    cycle162ContextClick(p.w.quick, p.scene(0, QPointF(30, 30)).toPoint());
    QTRY_VERIFY(menu->property("opened").toBool());
    for (const char *name : {"pdfContextHighlight", "pdfContextRemoveMark", "pdfContextColour_yellow"}) QVERIFY2(!p.object(name)->property("visible").toBool(), name);
    QVERIFY(QMetaObject::invokeMethod(menu, "close"));
}

// PDF only (the notes hidden): marking keeps the layout and the focus, and
// the window shows unsaved changes. With Live as the writing mode the hidden
// page does not hold the notes yet, so Undo in the pane goes to the document;
// showing the notes then brings the page the current text.
void FomawriteTest::pdfMarkingInPdfOnlyLayoutKeepsLayoutAndFocus() {
    Cycle103SettingsSnapshot settings;
    Cycle160Fixtures f; QVERIFY(f.make());
    Cycle161Pane p; QVERIFY2(p.start(), qPrintable(p.w.error));
    p.w.layout->setProperty("liveEditEnabled", true);
    // The page is ready before the PDF opens: opening goes through the window's document barrier, which waits for a loading page.
    auto *bridge = p.w.backend.editorBridge();
    QTRY_VERIFY_WITH_TIMEOUT(bridge->isReady(), 30000);
    QVERIFY(p.open(f.url("geometry.pdf")));
    QTRY_VERIFY(p.page(0));
    QCOMPARE(p.w.layout->property("layoutMode").toInt(), 2);
    QTRY_COMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);
    const QString title = p.w.quick->title();
    cycle163Drag(p, 0, "OMEGA");
    QTRY_COMPARE(p.reader->property("selectionText").toString(), QStringLiteral("OMEGA"));
    QVERIFY(cycle102Click(p.w.quick, p.item("pdfHighlighter")));
    QCOMPARE(cycle163Items(p).size(), 1);
    QVERIFY(p.w.backend.modified());
    QTRY_VERIFY(p.w.quick->title() != title); // the window shows unsaved changes
    QCOMPARE(p.w.layout->property("layoutMode").toInt(), 2);
    QCOMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);
    QVERIFY(p.list->hasActiveFocus());
    const QString marked = p.w.editor->property("text").toString();
    QVERIFY(marked.contains("> OMEGA"));
    QVERIFY(!p.w.backend.liveMirrorsDocument());
    QTest::keyClick(p.w.quick, Qt::Key_Z, Qt::ControlModifier);
    QTRY_COMPARE(p.w.editor->property("text").toString(), QString());
    QTRY_VERIFY(cycle163Items(p).isEmpty());
    QTest::keyClick(p.w.quick, Qt::Key_Z, Qt::ControlModifier | Qt::ShiftModifier);
    QTRY_COMPARE(p.w.editor->property("text").toString(), marked);
    QCOMPARE(p.w.layout->property("effectiveLayoutMode").toInt(), 2);
    QVERIFY(p.list->hasActiveFocus());
    // Showing the notes in Live: the page gets the marked text.
    p.w.layout->setProperty("layoutMode", 1);
    QTRY_COMPARE_WITH_TIMEOUT(cycle144PageText(bridge), marked, 10000);
    QVERIFY(p.w.backend.liveMirrorsDocument());
    p.w.layout->setProperty("liveEditEnabled", false);
}
