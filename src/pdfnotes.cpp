#include "pdfnotes.h"

#include <QPair>
#include <QRegularExpression>
#include <QUrl>
#include <algorithm>
#include <limits>

namespace PdfNotes {

namespace {

enum class LineKind { Text, Blank, Header, Marker, Heading, Code };

struct Line {
    int start = 0;
    int end = 0;  // before the line break
    int next = 0; // after it
    LineKind kind = LineKind::Text;
};

QString eolOf(const QString &text) {
    return text.contains(QStringLiteral("\r\n")) ? QStringLiteral("\r\n") : QStringLiteral("\n");
}

bool isBlank(QStringView text) {
    for (const QChar c : text)
        if (!c.isSpace()) return false;
    return true;
}

QList<Line> splitLines(const QString &text) {
    QList<Line> lines;
    int start = 0;
    const int length = int(text.size());
    while (start < length) {
        int newline = int(text.indexOf(QLatin1Char('\n'), start));
        Line line;
        line.start = start;
        if (newline < 0) {
            line.end = line.next = length;
        } else {
            line.end = newline > start && text.at(newline - 1) == QLatin1Char('\r') ? newline - 1 : newline;
            line.next = newline + 1;
        }
        lines.append(line);
        start = line.next;
    }
    return lines;
}

const QRegularExpression &headerPattern() {
    static const QRegularExpression pattern(QStringLiteral("^ {0,3}<!--[ \\t]*fomawrite:pdf-notes(?:[ \\t]+\\d+)?[ \\t]*-->[ \\t]*$"));
    return pattern;
}
const QRegularExpression &markerPattern() {
    static const QRegularExpression pattern(QStringLiteral(
        "^ {0,3}<!--[ \\t]*fomawrite:pdf[ \\t]+([A-Za-z][A-Za-z0-9_-]*)((?:[ \\t]+[^\\s=]+=\\S*)*)[ \\t]*-->[ \\t]*$"));
    return pattern;
}
const QRegularExpression &keyPattern() {
    static const QRegularExpression pattern(QStringLiteral("([^\\s=]+)=(\\S*)"));
    return pattern;
}
const QRegularExpression &headingPattern() {
    static const QRegularExpression pattern(QStringLiteral("^ {0,3}(#{1,6})(?:[ \\t]+(.*?))?[ \\t]*$"));
    return pattern;
}
const QRegularExpression &fenceOpenPattern() {
    static const QRegularExpression pattern(QStringLiteral("^ {0,3}(`{3,}|~{3,})(.*)$"));
    return pattern;
}
const QRegularExpression &quoteLinePattern() {
    static const QRegularExpression pattern(QStringLiteral("^ {0,3}>[ \\t]?"));
    return pattern;
}

// The page a heading's text names: a link to "#page=N", else "Page N" at its start.
int headingPage(const QString &heading) {
    static const QRegularExpression link(QStringLiteral("\\]\\(<?[^)\\s>]*#page=(\\d+)>?\\)"));
    static const QRegularExpression plain(QStringLiteral("^\\[?Page[ \\t]+(\\d+)(?![\\w])"));
    QRegularExpressionMatch match = link.match(heading);
    if (!match.hasMatch()) match = plain.match(heading);
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}

bool isGeneratedHeading(const QString &line, int page) {
    static const QRegularExpression generated(QStringLiteral("^## \\[Page (?:[^\\]\\\\]|\\\\.)+\\]\\([^)\\s]+#page=(\\d+)\\)$"));
    const QRegularExpressionMatch match = generated.match(line);
    return match.hasMatch() && match.captured(1).toInt() == page;
}

// The line ending at `position` (a line start) is blank.
bool lineBeforeIsBlank(const QString &text, int position) {
    if (position <= 0) return false;
    const int from = int(text.lastIndexOf(QLatin1Char('\n'), position - 2)) + 1;
    return isBlank(QStringView(text).mid(from, position - from));
}

// The line ending at `position` is empty (just its line break).
bool lineBeforeIsEmpty(const QString &text, int position, const QString &eol) {
    if (position < eol.size() || QStringView(text).mid(position - eol.size(), eol.size()) != eol) return false;
    const int before = position - int(eol.size());
    return before == 0 || text.at(before - 1) == QLatin1Char('\n');
}

// Inserts `body` (complete lines) at the line start `position`, with one blank
// line between it and the text before or after it, so that removeRange can
// find that blank line again.
Change insertion(const QString &text, int position, const QString &body, const QString &eol) {
    const int length = int(text.size());
    if (position == length && length > 0 && !text.endsWith(QLatin1Char('\n')))
        return {position, 0, eol + eol + body.chopped(eol.size())};
    const bool blankBefore = position == 0 || lineBeforeIsBlank(text, position);
    return {position, 0, blankBefore ? body + eol : eol + body};
}

// The range that removes the lines [from, to) and the blank line insertion added.
QPair<int, int> removalRange(const QString &text, int from, int to, const QString &eol) {
    const int length = int(text.size());
    const QString doubled = eol + eol;
    if (to == length && (to == from || !text.endsWith(QLatin1Char('\n'))) && from >= doubled.size()
            && QStringView(text).mid(from - doubled.size(), doubled.size()) == doubled)
        return {from - int(doubled.size()), to};
    if (lineBeforeIsEmpty(text, from, eol)) return {from - int(eol.size()), to};
    if (QStringView(text).mid(to, eol.size()) == eol) return {from, to + int(eol.size())};
    return {from, to};
}

Edit merged(QList<QPair<int, int>> ranges) {
    std::sort(ranges.begin(), ranges.end());
    Edit edit;
    for (const auto &range : ranges) {
        if (range.second <= range.first) continue;
        if (!edit.isEmpty() && range.first <= edit.last().position + edit.last().removed) {
            Change &last = edit.last();
            last.removed = qMax(last.position + last.removed, range.second) - last.position;
        } else {
            edit.append({range.first, range.second - range.first, QString()});
        }
    }
    return edit;
}

QList<QPair<int, int>> occurrences(const QString &pageText, const QString &quote) {
    QList<QPair<int, int>> found;
    const QStringList words = quote.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (words.isEmpty()) return found;
    QStringList escaped;
    for (const QString &word : words) escaped.append(QRegularExpression::escape(word));
    const QRegularExpression pattern(escaped.join(QStringLiteral("\\s+")), QRegularExpression::UseUnicodePropertiesOption);
    for (auto it = pattern.globalMatch(pageText); it.hasNext();) {
        const QRegularExpressionMatch match = it.next();
        found.append({int(match.capturedStart()), int(match.capturedLength())});
    }
    return found;
}

}

QString headerLine() { return QStringLiteral("<!-- fomawrite:pdf-notes %1 -->").arg(FormatVersion); }

QStringList highlightColours() {
    return {QStringLiteral("yellow"), QStringLiteral("green"), QStringLiteral("blue"), QStringLiteral("pink"), QStringLiteral("purple")};
}

QString defaultColour(const QString &kind) {
    return kind == QLatin1String("highlight") ? QStringLiteral("yellow") : QStringLiteral("red");
}

bool isTextMarkup(const QString &kind) {
    return kind == QLatin1String("highlight") || kind == QLatin1String("underline") || kind == QLatin1String("strike");
}

QString apply(const QString &text, const Edit &edit) {
    QString result = text;
    for (auto it = edit.crbegin(); it != edit.crend(); ++it) result.replace(it->position, it->removed, it->inserted);
    return result;
}

QString Item::value(const QString &name) const {
    for (const Key &key : keys)
        if (key.name == name) return key.value;
    return {};
}

Document parse(const QString &text) {
    Document document;
    QList<Line> lines = splitLines(text);
    QChar fenceChar;
    int fenceLength = 0;
    struct Heading { int line; int level; int page; };
    QList<Heading> headings;
    for (int i = 0; i < lines.size(); ++i) {
        Line &line = lines[i];
        const QString content = text.mid(line.start, line.end - line.start);
        if (fenceLength > 0) {
            line.kind = LineKind::Code;
            const QString trimmed = content.trimmed();
            int run = 0;
            while (run < trimmed.size() && trimmed.at(run) == fenceChar) ++run;
            if (run >= fenceLength && run == trimmed.size() && content.indexOf(fenceChar) <= 3) fenceLength = 0;
            continue;
        }
        if (const auto fence = fenceOpenPattern().match(content); fence.hasMatch()) {
            const QString run = fence.captured(1);
            if (!(run.at(0) == QLatin1Char('`') && fence.captured(2).contains(QLatin1Char('`')))) {
                fenceChar = run.at(0);
                fenceLength = int(run.size());
                line.kind = LineKind::Code;
                continue;
            }
        }
        if (isBlank(content)) { line.kind = LineKind::Blank; continue; }
        if (headerPattern().match(content).hasMatch()) {
            line.kind = LineKind::Header;
            if (!document.items.isEmpty() && document.items.last().end == text.size()) document.items.last().end = line.start;
            if (!document.hasHeader) {
                document.hasHeader = true;
                document.headerStart = line.start;
                document.headerLineEnd = line.next;
            }
            continue;
        }
        if (const auto marker = markerPattern().match(content); marker.hasMatch()) {
            line.kind = LineKind::Marker;
            Item item;
            item.kind = marker.captured(1);
            item.markerStart = line.start;
            item.markerEnd = line.end;
            item.markerLineEnd = line.next;
            item.end = int(text.size());
            item.keysEnd = line.start + int(marker.capturedEnd(1));
            const int keysOffset = int(marker.capturedStart(2));
            for (auto it = keyPattern().globalMatch(marker.captured(2)); it.hasNext();) {
                const auto key = it.next();
                Key entry{key.captured(1), key.captured(2), line.start + keysOffset + int(key.capturedStart(2)),
                          line.start + keysOffset + int(key.capturedEnd(2))};
                item.keysEnd = entry.valueEnd;
                item.keys.append(entry);
            }
            bool ok = false;
            const int page = item.value(QStringLiteral("page")).toInt(&ok);
            item.page = ok && page > 0 ? page : 0;
            item.colour = item.value(QStringLiteral("colour"));
            static const QRegularExpression range(QStringLiteral("^(\\d+)\\+(\\d+)$"));
            if (const auto match = range.match(item.value(QStringLiteral("text"))); match.hasMatch()) {
                item.start = match.captured(1).toInt();
                item.length = match.captured(2).toInt();
            }
            static const QRegularExpression atPattern(QStringLiteral("^(-?\\d{1,9}),(-?\\d{1,9})$"));
            if (const auto match = atPattern.match(item.value(QStringLiteral("at"))); match.hasMatch()) {
                item.hasAt = true;
                item.atX = match.captured(1).toInt();
                item.atY = match.captured(2).toInt();
            }
            if (!document.items.isEmpty() && document.items.last().end == text.size()) document.items.last().end = line.start;
            document.items.append(item);
            continue;
        }
        if (const auto heading = headingPattern().match(content); heading.hasMatch()) {
            line.kind = LineKind::Heading;
            if (!document.items.isEmpty() && document.items.last().end == text.size()) document.items.last().end = line.start;
            QString title = heading.captured(2);
            static const QRegularExpression closing(QStringLiteral("[ \\t]+#+$"));
            title.remove(closing);
            headings.append({i, int(heading.captured(1).size()), headingPage(title)});
        }
    }
    // Sections: a page heading up to the next heading of its level or above, or the next page heading.
    for (int h = 0; h < headings.size(); ++h) {
        if (headings.at(h).page <= 0) continue;
        const Line &line = lines.at(headings.at(h).line);
        Section section;
        section.page = headings.at(h).page;
        section.level = headings.at(h).level;
        section.headingStart = line.start;
        section.headingEnd = line.end;
        section.headingLineEnd = line.next;
        section.heading = text.mid(line.start, line.end - line.start);
        section.end = int(text.size());
        for (int next = h + 1; next < headings.size(); ++next) {
            if (headings.at(next).level <= section.level || headings.at(next).page > 0) {
                section.end = lines.at(headings.at(next).line).start;
                break;
            }
        }
        document.sections.append(section);
    }
    // The blockquote after a text mark's marker: blank lines, then ">" lines, inside the item.
    int lineIndex = 0;
    for (Item &item : document.items) {
        while (lineIndex < lines.size() && lines.at(lineIndex).start < item.markerLineEnd) ++lineIndex;
        // The body: from the first non-blank line after the marker to the last non-blank character.
        item.bodyStart = item.markerLineEnd;
        item.contentEnd = item.markerLineEnd;
        for (int b = lineIndex; b < lines.size() && lines.at(b).start < item.end; ++b) {
            if (lines.at(b).kind == LineKind::Blank) continue;
            if (!item.hasBody) { item.hasBody = true; item.bodyStart = lines.at(b).start; }
            item.contentEnd = lines.at(b).end;
        }
        if (!isTextMarkup(item.kind)) continue;
        int i = lineIndex;
        while (i < lines.size() && lines.at(i).start < item.end && lines.at(i).kind == LineKind::Blank) ++i;
        QStringList quoted;
        int quoteStart = -1, quoteEnd = -1;
        for (; i < lines.size() && lines.at(i).start < item.end && lines.at(i).kind == LineKind::Text; ++i) {
            const QString content = text.mid(lines.at(i).start, lines.at(i).end - lines.at(i).start);
            const auto prefix = quoteLinePattern().match(content);
            if (!prefix.hasMatch()) break;
            if (quoteStart < 0) quoteStart = lines.at(i).start;
            quoteEnd = lines.at(i).next;
            quoted.append(content.mid(prefix.capturedLength()));
        }
        if (quoteStart >= 0) {
            item.hasQuote = true;
            item.quote = quoted.join(QLatin1Char('\n'));
            item.quoteStart = quoteStart;
            item.quoteEnd = quoteEnd;
            item.bodyStart = quoteStart;
        }
    }
    return document;
}

QString markerLine(const Mark &mark) {
    const QString colour = mark.colour.isEmpty() ? defaultColour(mark.kind) : mark.colour;
    return QStringLiteral("<!-- fomawrite:pdf %1 page=%2 colour=%3 text=%4+%5 -->")
        .arg(mark.kind).arg(mark.page).arg(colour).arg(mark.start).arg(mark.length);
}

QString quoteBlock(const QString &text) {
    QString normalisedText = text;
    normalisedText.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    for (const QChar separator : {QChar(u'\r'), QChar(QChar::LineSeparator), QChar(QChar::ParagraphSeparator)})
        normalisedText.replace(separator, QLatin1Char('\n'));
    static const QRegularExpression runs(QStringLiteral("[\\t\\f\\v \\x{00A0}]+"));
    QStringList lines;
    bool blank = false;
    for (QString line : normalisedText.split(QLatin1Char('\n'))) {
        line = line.replace(runs, QStringLiteral(" ")).trimmed();
        if (line.isEmpty()) { blank = !lines.isEmpty(); continue; }
        if (blank) lines.append(QStringLiteral(">"));
        blank = false;
        lines.append(QStringLiteral("> ") + line);
    }
    return lines.join(QLatin1Char('\n'));
}

QString linkDestination(const QString &fileName) {
    QString result;
    for (const QChar c : fileName) {
        if (c.isSpace() || QStringLiteral("%()<>#?[]\\").contains(c)) {
            for (const char byte : QString(c).toUtf8())
                result += QStringLiteral("%%1").arg(uchar(byte), 2, 16, QLatin1Char('0')).toUpper();
        } else {
            result += c;
        }
    }
    return result;
}

QString pageLink(const QString &fileName, int page, const QString &label) {
    const QString number = QString::number(page);
    QString name = label.isEmpty() || label == number ? number : label;
    name.replace(QLatin1Char('\\'), QStringLiteral("\\\\")).replace(QLatin1Char('['), QStringLiteral("\\[")).replace(QLatin1Char(']'), QStringLiteral("\\]"));
    return QStringLiteral("[Page %1](%2#page=%3)").arg(name, linkDestination(fileName), number);
}

QString pageHeading(const QString &fileName, int page, const QString &label) {
    return QStringLiteral("## ") + pageLink(fileName, page, label);
}

QString normalised(const QString &text) { return text.simplified(); }

namespace {
// Adds `block` (complete lines, ending in a line break) as an item of `page`:
// in the page's section before the first item whose text start is later than
// `order` (a note passes INT_MAX and goes after every item), or under a new
// page heading placed in ascending page order.
Edit insertItem(const QString &text, int page, int order, const QString &block, const QString &pdfFileName,
                const QString &pageLabel, const QString &eol) {
    const QString heading = pageHeading(pdfFileName, page, pageLabel);
    if (text.isEmpty()) {
        const QString lead = headerLine() + eol + QStringLiteral("# Notes on ") + pdfFileName + eol + eol + heading + eol + eol;
        return {{0, 0, lead + block}};
    }
    const Document document = parse(text);
    int position = -1;
    QString body;
    const Section *section = nullptr;
    for (const Section &candidate : document.sections)
        if (candidate.page == page) { section = &candidate; break; }
    if (section) {
        body = block;
        position = section->end;
        for (const Item &item : document.items) {
            if (item.markerStart < section->headingLineEnd || item.markerStart >= section->end || item.page != page) continue;
            if (item.start > order) { position = item.markerStart; break; }
        }
    } else {
        body = heading + eol + eol + block;
        int lastEnd = -1;
        for (const Section &candidate : document.sections) {
            if (position < 0 && candidate.page > page) position = candidate.headingStart;
            lastEnd = qMax(lastEnd, candidate.end);
        }
        if (position < 0) position = lastEnd >= 0 ? lastEnd : int(text.size());
    }
    Change change = insertion(text, position, body, eol);
    if (document.hasHeader || !document.items.isEmpty()) return {change};
    const QString header = headerLine() + eol + eol;
    if (change.position == 0) {
        change.inserted.prepend(header);
        return {change};
    }
    return {{0, 0, header}, change};
}
}

Edit insertMark(const QString &text, const Mark &mark, const QString &pdfFileName, const QString &pageLabel) {
    const QString quote = quoteBlock(mark.text);
    if (!isTextMarkup(mark.kind) || mark.page < 1 || mark.start < 0 || mark.length <= 0 || quote.isEmpty()) return {};
    const QString eol = eolOf(text);
    QString quoteLines = quote;
    quoteLines.replace(QLatin1Char('\n'), eol);
    return insertItem(text, mark.page, mark.start, markerLine(mark) + eol + quoteLines + eol, pdfFileName, pageLabel, eol);
}

QString noteMarkerLine(const Note &note) {
    return QStringLiteral("<!-- fomawrite:pdf note page=%1 at=%2,%3 -->").arg(note.page).arg(note.x).arg(note.y);
}

NoteInsert insertNote(const QString &text, const Note &note, const QString &pdfFileName, const QString &pageLabel) {
    if (note.page < 1) return {};
    const QString eol = eolOf(text);
    const QString marker = noteMarkerLine(note);
    NoteInsert result;
    result.edit = insertItem(text, note.page, std::numeric_limits<int>::max(), marker + eol + eol, pdfFileName, pageLabel, eol);
    int delta = 0;
    for (const Change &change : std::as_const(result.edit)) {
        const int at = int(change.inserted.indexOf(marker));
        if (at >= 0) { result.caret = change.position + delta + at + int(marker.size() + eol.size()); break; }
        delta += int(change.inserted.size()) - change.removed;
    }
    return result;
}

Edit moveNote(const QString &text, const Item &item, int x, int y) {
    if (item.markerEnd > text.size()) return {};
    const QString value = QStringLiteral("%1,%2").arg(x).arg(y);
    for (const Key &key : item.keys) {
        if (key.name != QLatin1String("at")) continue;
        if (key.value == value) return {};
        return {{key.valueStart, key.valueEnd - key.valueStart, value}};
    }
    return {{item.keysEnd, 0, QStringLiteral(" at=") + value}};
}

Edit removeMark(const QString &text, const Item &item, const QString &writtenQuote) {
    const int length = int(text.size());
    if (item.markerStart < 0 || item.markerLineEnd > length || item.markerLineEnd <= item.markerStart) return {};
    static const QRegularExpression stillAMarker(QStringLiteral("^ {0,3}<!--[ \\t]*fomawrite:pdf[ \\t]"));
    if (!stillAMarker.match(text.mid(item.markerStart, item.markerEnd - item.markerStart)).hasMatch()) return {};
    const QString eol = eolOf(text);
    const Document document = parse(text);
    const bool isNote = item.kind == QLatin1String("note");
    // After the marker's line and the one blank line that may follow it.
    int afterBlank = item.markerLineEnd;
    if (isNote && item.markerLineEnd < length) {
        const int nl = int(text.indexOf(QLatin1Char('\n'), item.markerLineEnd));
        const QStringView line = QStringView(text).mid(item.markerLineEnd, nl < 0 ? length - item.markerLineEnd : nl - item.markerLineEnd);
        if (isBlank(line)) afterBlank = nl < 0 ? length : nl + 1;
    }
    // A note's own text is the writer's: only an empty note goes whole.
    const bool whole = isNote ? !item.hasBody
        : isTextMarkup(item.kind) && item.hasQuote && item.quoteEnd <= length
            && !normalised(writtenQuote).isEmpty() && normalised(item.quote) == normalised(writtenQuote);
    QPair<int, int> range{item.markerStart, isNote ? afterBlank : item.markerLineEnd};
    if (whole) {
        int from = item.markerStart;
        const int to = isNote ? afterBlank : item.quoteEnd;
        for (const Section &section : document.sections) {
            if (section.headingLineEnd > item.markerStart || item.markerStart >= section.end || section.page != item.page) continue;
            if (isGeneratedHeading(section.heading, item.page)
                    && isBlank(QStringView(text).mid(section.headingLineEnd, item.markerStart - section.headingLineEnd))
                    && isBlank(QStringView(text).mid(to, section.end - to)))
                from = section.headingStart;
            break;
        }
        range = removalRange(text, from, to, eol);
    }
    QList<QPair<int, int>> ranges{range};
    // The notes' last mark takes the header line with it, as insertMark added it.
    const bool lastMark = document.items.size() == 1;
    const QString header = headerLine() + eol;
    if (lastMark && document.hasHeader && document.headerStart == 0 && text.startsWith(header) && range.first >= header.size()) {
        const QString rest = apply(text, {{range.first, range.second - range.first, QString()}});
        static const QRegularExpression title(QStringLiteral("^# Notes on [^\\r\\n]+\\r?\\n$"));
        const int headerEnd = int(header.size());
        if (title.match(rest.mid(headerEnd)).hasMatch()) return {{0, length, QString()}};
        if (QStringView(rest).mid(headerEnd, eol.size()) == eol) {
            const int end = headerEnd + int(eol.size()); // in `rest`; map back over the removed range
            if (end <= range.first) ranges.append({0, end});
            else ranges.append({0, end + range.second - range.first});
        }
    }
    return merged(ranges);
}

Edit recolourMark(const QString &text, const Item &item, const QString &colour) {
    if (colour.isEmpty() || item.markerEnd > text.size()) return {};
    for (const Key &key : item.keys) {
        if (key.name != QLatin1String("colour")) continue;
        if (key.value == colour) return {};
        return {{key.valueStart, key.valueEnd - key.valueStart, colour}};
    }
    return {{item.keysEnd, 0, QStringLiteral(" colour=") + colour}};
}

Placement place(const Item &item, const std::function<QString(int)> &pageText) {
    if (item.page < 1) return {};
    const QString text = pageText(item.page);
    const bool inRange = item.start >= 0 && item.length > 0 && item.start + item.length <= text.size();
    const QString wanted = normalised(item.quote);
    if (!item.hasQuote || wanted.isEmpty()) {
        if (inRange) return {true, item.page, item.start, item.length};
        return {};
    }
    if (inRange && normalised(text.mid(item.start, item.length)) == wanted) return {true, item.page, item.start, item.length};
    const auto here = occurrences(text, wanted);
    if (!here.isEmpty()) {
        auto best = here.first();
        for (const auto &found : here)
            if (qAbs(found.first - item.start) < qAbs(best.first - item.start)) best = found;
        return {true, item.page, best.first, best.second};
    }
    for (int distance = 1; distance <= 3; ++distance) {
        for (const int page : {item.page + distance, item.page - distance}) {
            if (page < 1) continue;
            const auto found = occurrences(pageText(page), wanted);
            if (!found.isEmpty()) return {true, page, found.first().first, found.first().second};
        }
    }
    return {};
}

QPointF turnedPoint(QPointF p, QSizeF size, int degrees) {
    switch (((degrees % 360) + 360) % 360) {
    case 90: return {size.height() - p.y(), p.x()};
    case 180: return {size.width() - p.x(), size.height() - p.y()};
    case 270: return {p.y(), size.width() - p.x()};
    default: return p;
    }
}

namespace {
QString escapedLabel(QString label) {
    return label.replace(QLatin1Char('\\'), QStringLiteral("\\\\")).replace(QLatin1Char('['), QStringLiteral("\\["))
        .replace(QLatin1Char(']'), QStringLiteral("\\]"));
}
QString unescapedLabel(const QString &label) {
    QString out;
    for (int i = 0; i < label.size(); ++i) {
        if (label.at(i) == QLatin1Char('\\') && i + 1 < label.size()) ++i;
        out += label.at(i);
    }
    return out;
}
bool pointInPage(double x, double y, QSizeF size) { return x >= 0 && y >= 0 && x <= size.width() && y <= size.height(); }
}

Edit renumber(const QString &text, const QString &pdfFileName, const QList<PageRemap> &pages,
              const std::function<QString(int)> &oldLabel, const std::function<QString(int)> &newLabel) {
    const auto remapOf = [&](int page) -> const PageRemap * {
        return page >= 1 && page <= pages.size() ? &pages.at(page - 1) : nullptr;
    };
    Edit changes;
    const Document document = parse(text);
    // Markers.
    for (const Item &item : document.items) {
        const PageRemap *remap = remapOf(item.page);
        if (!remap) continue;
        const Key *pageKey = nullptr, *atKey = nullptr, *rectKey = nullptr;
        for (const Key &key : item.keys) {
            if (key.name == QLatin1String("page") && !pageKey) pageKey = &key;
            else if (key.name == QLatin1String("at") && !atKey) atKey = &key;
            else if (key.name == QLatin1String("rect") && !rectKey) rectKey = &key;
        }
        if (!pageKey || pageKey->value.toInt() != item.page) continue;
        if (remap->newPage != item.page)
            changes.append({pageKey->valueStart, pageKey->valueEnd - pageKey->valueStart, QString::number(remap->newPage)});
        if (remap->newPage < 1 || remap->rotation % 360 == 0) continue;
        if (atKey) {
            const QStringList xy = atKey->value.split(QLatin1Char(','));
            bool okX = false, okY = false;
            const int x = xy.size() == 2 ? xy.at(0).toInt(&okX) : 0, y = xy.size() == 2 ? xy.at(1).toInt(&okY) : 0;
            if (okX && okY && pointInPage(x, y, remap->size)) {
                const QPoint to = turnedPoint(QPointF(x, y), remap->size, remap->rotation).toPoint();
                changes.append({atKey->valueStart, atKey->valueEnd - atKey->valueStart, QStringLiteral("%1,%2").arg(to.x()).arg(to.y())});
            }
        }
        if (rectKey) {
            const QStringList v = rectKey->value.split(QLatin1Char(','));
            bool ok[4] = {false, false, false, false};
            double n[4] = {0, 0, 0, 0};
            if (v.size() == 4)
                for (int i = 0; i < 4; ++i) n[i] = v.at(i).toDouble(&ok[i]);
            if (v.size() == 4 && ok[0] && ok[1] && ok[2] && ok[3]) {
                const QPointF a = turnedPoint(QPointF(n[0], n[1]), remap->size, remap->rotation);
                const QPointF b = turnedPoint(QPointF(n[2], n[3]), remap->size, remap->rotation);
                changes.append({rectKey->valueStart, rectKey->valueEnd - rectKey->valueStart,
                                QStringLiteral("%1,%2,%3,%4").arg(qRound(qMin(a.x(), b.x()))).arg(qRound(qMin(a.y(), b.y())))
                                    .arg(qRound(qMax(a.x(), b.x()))).arg(qRound(qMax(a.y(), b.y())))});
            }
        }
    }
    // Links to this PDF and page headings, outside code fences and inline code.
    static const QRegularExpression link(QStringLiteral("\\[((?:[^\\]\\\\]|\\\\.)*)\\]\\(<?([^)\\s>]*)#page=(\\d+)>?\\)"));
    static const QRegularExpression plainHeading(QStringLiteral("^( {0,3}#{1,6}[ \\t]+)(?:Page[ \\t]+)(\\d+)([ \\t]*)$"));
    static const QRegularExpression labelText(QStringLiteral("^Page (.+)$"));
    QChar fence;
    int fenceLength = 0;
    for (const Line &line : splitLines(text)) {
        const QString content = text.mid(line.start, line.end - line.start);
        const auto open = fenceOpenPattern().match(content);
        if (fenceLength > 0) {
            const QString trimmed = content.trimmed();
            if (!trimmed.isEmpty() && trimmed.count(fence) == trimmed.size() && trimmed.size() >= fenceLength) fenceLength = 0;
            continue;
        }
        if (open.hasMatch() && !(open.captured(1).startsWith(QLatin1Char('`')) && open.captured(2).contains(QLatin1Char('`')))) {
            fence = open.captured(1).at(0);
            fenceLength = int(open.captured(1).size());
            continue;
        }
        const bool heading = headingPattern().match(content).hasMatch();
        bool anyLink = false, lineReplaced = false;
        for (auto it = link.globalMatch(content); it.hasNext() && !lineReplaced;) {
            const QRegularExpressionMatch match = it.next();
            QString destination = QUrl::fromPercentEncoding(match.captured(2).toUtf8());
            if (destination.startsWith(QLatin1String("./"))) destination.remove(0, 2);
            if (destination != pdfFileName) continue;
            anyLink = true;
            if (content.left(match.capturedStart()).count(QLatin1Char('`')) % 2) continue;
            const int old = match.captured(3).toInt();
            const PageRemap *remap = remapOf(old);
            if (!remap) continue;
            const int numberStart = line.start + match.capturedStart(3);
            if (remap->newPage == 0) {
                if (heading && isGeneratedHeading(content, old)) {
                    changes.append({line.start, int(content.size()), QStringLiteral("## Deleted page %1").arg(old)});
                    lineReplaced = true;
                } else if (heading) {
                    changes.append({numberStart, int(match.capturedLength(3)), QStringLiteral("0")});
                }
                continue;
            }
            if (remap->newPage == old) continue;
            changes.append({numberStart, int(match.capturedLength(3)), QString::number(remap->newPage)});
            const auto lt = labelText.match(match.captured(1));
            if (!lt.hasMatch()) continue;
            const QString shown = unescapedLabel(lt.captured(1));
            if (shown != QString::number(old) && (shown.isEmpty() || shown != oldLabel(old))) continue;
            const QString label = newLabel(remap->newPage);
            const QString now = escapedLabel(label.isEmpty() || label == QString::number(remap->newPage)
                                                 ? QString::number(remap->newPage) : label);
            if (now != lt.captured(1))
                changes.append({line.start + int(match.capturedStart(1)) + 5, int(lt.captured(1).size()), now});
        }
        if (!anyLink && heading && !content.contains(QLatin1String("#page="))) {
            const auto plain = plainHeading.match(content);
            const PageRemap *remap = plain.hasMatch() ? remapOf(plain.captured(2).toInt()) : nullptr;
            if (!remap) continue;
            const int old = plain.captured(2).toInt();
            if (remap->newPage == 0)
                changes.append({line.start + int(plain.captured(1).size()), int(content.size() - plain.captured(1).size()),
                                QStringLiteral("Deleted page %1").arg(old)});
            else if (remap->newPage != old)
                changes.append({line.start + int(plain.capturedStart(2)), int(plain.capturedLength(2)), QString::number(remap->newPage)});
        }
    }
    changes.removeIf([&](const Change &change) {
        return QStringView(text).mid(change.position, change.removed) == change.inserted;
    });
    std::sort(changes.begin(), changes.end(), [](const Change &a, const Change &b) { return a.position < b.position; });
    return changes;
}

}
