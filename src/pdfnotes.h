#pragma once

#include <QList>
#include <QPointF>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <functional>

// The PDF notes file (docs/pdf-plan.md §4.2): plain Markdown beside the PDF
// whose marks are one-line HTML comments,
//   <!-- fomawrite:pdf KIND key=value … -->
// each followed by its block (for text markup a blockquote of the marked text,
// then the writer's own comment). Pure functions on the notes text: nothing
// here reads files or the PDF. Fomawrite changes the notes only through the
// edits these functions return, so every byte outside an edit's range is kept.
namespace PdfNotes {

constexpr int FormatVersion = 1;
// "<!-- fomawrite:pdf-notes 1 -->"
QString headerLine();
// yellow, green, blue, pink, purple (highlights); underline and strike default to red.
QStringList highlightColours();
QString defaultColour(const QString &kind);
// highlight, underline and strike: marks over a range of the page's text.
bool isTextMarkup(const QString &kind);

// One replacement in the text the edit was computed on.
struct Change {
    int position = 0;
    int removed = 0;
    QString inserted;
    bool operator==(const Change &other) const {
        return position == other.position && removed == other.removed && inserted == other.inserted;
    }
};
// Ascending, non-overlapping changes, all in the coordinates of the original
// text (as a CodeMirror change set is); empty when there is nothing to do.
using Edit = QList<Change>;
QString apply(const QString &text, const Edit &edit);

struct Key {
    QString name;
    QString value;
    int valueStart = 0; // the value's range in the text
    int valueEnd = 0;
};

struct Item {
    QString kind;
    int page = 0;          // 1-based, as in the marker; 0 when missing or invalid
    QString colour;
    int start = -1;        // text=START+LENGTH; -1 when absent or invalid
    int length = 0;
    bool hasQuote = false; // a blockquote follows the marker (text markup only)
    QString quote;         // its lines without "> ", joined by '\n'
    QList<Key> keys;       // every key=value in order, unknown ones included
    int markerStart = 0;   // the marker line, without its line break
    int markerEnd = 0;
    int markerLineEnd = 0; // after the line break
    int keysEnd = 0;       // after the kind and the last key=value (where a new key goes)
    int quoteStart = -1;   // the blockquote's lines; quoteEnd is after the last line break
    int quoteEnd = -1;
    int end = 0;           // the item ends at the next marker, heading or the end of the text
    bool hasAt = false;    // at=X,Y (integers, page view points) is present and valid
    int atX = 0;
    int atY = 0;
    int bodyStart = 0;     // the quote's start, else the first non-blank line after the marker, else markerLineEnd
    int contentEnd = 0;    // after the last non-blank character of the item (at least markerLineEnd)
    bool hasBody = false;  // there is text after the marker (for a note: the note's text)
    QString value(const QString &name) const;
};

// A page heading (any heading linking to "#page=N", or reading "Page N").
struct Section {
    int page = 0;          // 1-based
    int level = 2;
    int headingStart = 0;
    int headingEnd = 0;    // without the line break
    int headingLineEnd = 0;
    int end = 0;           // the next heading of the same or a higher level, or the end of the text
    QString heading;       // the line as written
};

struct Document {
    bool hasHeader = false;
    int headerStart = -1;
    int headerLineEnd = -1;
    QList<Item> items;       // in document order
    QList<Section> sections; // in document order
};

// Tolerant of anything a person or a tool may have done to the file: Windows
// line breaks, indentation of up to three spaces, extra spaces or tabs in a
// marker, reordered items and pages, edited or deleted headings, a deleted
// header line. Markers in fenced code blocks are text, not markers.
Document parse(const QString &text);

struct Mark {
    QString kind;   // highlight, underline or strike
    QString colour; // empty: the kind's default
    int page = 1;   // 1-based
    int start = 0;  // in the page's text (PdfSession::pageText)
    int length = 0;
    QString text;   // the marked text, as the PDF has it
};
QString markerLine(const Mark &mark);
// The text as Markdown blockquote lines, tidied as Copy as Markdown Quote does
// (whitespace runs collapsed, blank lines kept as ">"); no final line break.
QString quoteBlock(const QString &text);
// A file name as a Markdown link destination (spaces, parentheses and the
// like percent-encoded).
QString linkDestination(const QString &fileName);
// "[Page N](File.pdf#page=N)", with the label as link text when it differs from N.
QString pageLink(const QString &fileName, int page, const QString &label);
// "## [Page N](File.pdf#page=N)"
QString pageHeading(const QString &fileName, int page, const QString &label);
// Whitespace runs as one space, trimmed; lone ">" lines of a quote vanish.
QString normalised(const QString &text);

// The edit that adds `mark`: the header line at the very top when the notes
// hold no marks yet, a "# Notes on <file>" title when the notes are empty, the
// page's heading when the page has no section yet (pages in ascending order),
// and the item in the page's section in text order, separated by blank lines.
Edit insertMark(const QString &text, const Mark &mark, const QString &pdfFileName, const QString &pageLabel);
// The edit that takes `item` away. Its marker and quote go when the quote is
// still `writtenQuote` (whitespace-normalised: the page's text at the mark),
// otherwise only the marker; the writer's comment always stays. A page
// heading still in its generated form goes with the page's last item; the
// header line (and an untouched title) go with the notes' last mark.
// Applying insertMark and then removeMark to the result restores the original
// text byte for byte, except for a text that already held the header line but
// no marks, which loses the header line.
Edit removeMark(const QString &text, const Item &item, const QString &writtenQuote);
// The edit that sets the item's colour= value (adding the key when absent).
Edit recolourMark(const QString &text, const Item &item, const QString &colour);

// A note pinned on the page (Cycle 164): "<!-- fomawrite:pdf note page=N at=X,Y -->"
// on its own line, then the note's text. X,Y are integers in page view points.
struct Note {
    int page = 1; // 1-based
    int x = 0;
    int y = 0;
};
QString noteMarkerLine(const Note &note);
struct NoteInsert {
    Edit edit;
    int caret = -1; // in the edited text: the line after the marker, where the note's text goes
};
// The edit that adds a note after the existing items of its page (page heading
// created and ordered as for marks): the marker and an empty text line.
NoteInsert insertNote(const QString &text, const Note &note, const QString &pdfFileName, const QString &pageLabel);
// The edit that sets the note's at= value (adding the key when absent); only
// those bytes change.
Edit moveNote(const QString &text, const Item &item, int x, int y);

struct Placement {
    bool placed = false;
    int page = 0;   // 1-based
    int start = -1;
    int length = 0;
};
// Where a text mark is now. Its stored range when the page's text there still
// equals the quote (whitespace-normalised); otherwise the occurrence of the
// quote on the same page nearest the stored start; otherwise the first on the
// pages 1, 2 and 3 after and before it; otherwise unplaced. A mark without a
// quote is placed at its stored range when that range exists. `pageText`
// receives a 1-based page and returns an empty string for a page that does
// not exist.
Placement place(const Item &item, const std::function<QString(int)> &pageText);

// A point on a page of view size `size` after the page is turned clockwise by
// `degrees` (0, 90, 180, 270): the one transform for drawing and for saving.
QPointF turnedPoint(QPointF point, QSizeF size, int degrees);

// What saving page edits did to one baseline page (index = baseline page - 1).
struct PageRemap {
    int newPage = 0;   // 1-based; 0 when the page was removed
    int rotation = 0;  // clockwise, relative to the baseline
    QSizeF size;       // the baseline page's view size
};
// The edit that renumbers the notes after the PDF was saved: marker page=
// values (at= and rect= turned with their page), links "File.pdf#page=N" to
// this PDF and generated page headings. Nothing moves and every other byte is
// kept. Items on a removed page get page=0 (never placed); a generated heading
// of a removed page becomes "## Deleted page N". `oldLabel` / `newLabel` give a
// 1-based page's label before and after (a link whose text is the generated
// "Page N" or "Page <label>" follows; one the writer changed is left alone).
// An identity map gives an empty edit.
Edit renumber(const QString &text, const QString &pdfFileName, const QList<PageRemap> &pages,
              const std::function<QString(int)> &oldLabel, const std::function<QString(int)> &newLabel);

}
