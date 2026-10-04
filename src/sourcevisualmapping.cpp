#include "sourcevisualmapping.h"

#include <QRegularExpression>
#include <QTextBoundaryFinder>

namespace {
struct SourceLine { int start; QString text; bool newline; };
struct Fence { QChar marker; int length = 0; };
Fence openingFence(const QString &line) {
    static const QRegularExpression expression(QStringLiteral("^ {0,3}(`{3,}|~{3,})(.*)$"));
    const auto match = expression.match(line);
    if (!match.hasMatch()) return {};
    const QString marker = match.captured(1);
    if (marker.front() == QLatin1Char('`') && match.captured(2).contains(QLatin1Char('`'))) return {};
    return {marker.front(), int(marker.size())};
}
bool closesFence(const QString &line, const Fence &fence) {
    static const QRegularExpression expression(QStringLiteral("^ {0,3}(`{3,}|~{3,})[ \\t]*$"));
    const auto match = expression.match(line);
    return match.hasMatch() && match.captured(1).front() == fence.marker
        && match.capturedLength(1) >= fence.length;
}

// Only exact-length, single-line code spans are projected. Whitespace
// normalization and unmatched/mixed delimiters remain visible in Source.
bool hasSafeCodeSpans(const QString &line) {
    for (int cursor = 0; cursor < line.size();) {
        if (line.at(cursor) != QLatin1Char('`')) { ++cursor; continue; }
        int count = 1;
        while (cursor + count < line.size() && line.at(cursor + count) == QLatin1Char('`')) ++count;
        int search = cursor + count;
        int closing = -1;
        while (search < line.size()) {
            const int next = line.indexOf(QLatin1Char('`'), search);
            if (next < 0) break;
            int run = 1;
            while (next + run < line.size() && line.at(next + run) == QLatin1Char('`')) ++run;
            if (run == count) { closing = next; break; }
            search = next + run;
        }
        if (closing <= cursor + count) return false;
        const QString body = line.mid(cursor + count, closing - cursor - count);
        if (body.front().isSpace() || body.back().isSpace()) return false;
        cursor = closing + count;
    }
    return true;
}

bool isSourceOnlyLine(const QString &line) {
    static const QRegularExpression unsupported(
        QStringLiteral("^\\s*(?:<!--|<!|\\[\\^| {4,}|~~~|```)|\\[\\^|!\\[|\\[[^\\]]+\\]\\[[^\\]]*\\]|^\\s*\\[[^\\]]+\\]:"));
    static const QRegularExpression ambiguousInlineLink(
        QStringLiteral("\\[[^\\]]+\\]\\([^\\n)]*\\("));
    return !hasSafeCodeSpans(line) || line.contains(QChar(0x5c))
        || line.contains(QChar(0x3c)) || line.contains(QChar(0x7c))
        || ambiguousInlineLink.match(line).hasMatch() || unsupported.match(line).hasMatch();
}

bool isSafeBody(const QString &body) {
    static const QRegularExpression nestedOrBlock(
        QStringLiteral("^(?:>|[-+*]\\s|\\d+[.)]\\s|#{1,6}\\s|\\[[ xX]\\]\\s|!\\[|\\[\\^|---$|\\*\\*\\*$)"));
    return !nestedOrBlock.match(body).hasMatch();
}

QVector<SourceVisualMapping::Span> tableCells(const QString &line) {
    if (!line.contains(QLatin1Char('|')) || line.contains(QLatin1Char('\\'))
        || line.contains(QLatin1Char('`')) || line.startsWith(QStringLiteral("    "))
        || line.contains(QLatin1Char('<')) || line.contains(QStringLiteral("!["))
        || line.contains(QStringLiteral("[^"))) return {};
    int first = 0, end = line.size();
    while (first < end && line.at(first).isSpace()) ++first;
    while (end > first && line.at(end - 1).isSpace()) --end;
    if (first == end) return {};
    if (line.at(first) == QLatin1Char('|')) ++first;
    if (end > first && line.at(end - 1) == QLatin1Char('|')) --end;
    QVector<SourceVisualMapping::Span> cells;
    int start = first;
    for (int cursor = first; cursor <= end; ++cursor) {
        if (cursor != end && line.at(cursor) != QLatin1Char('|')) continue;
        int a = start, b = cursor;
        while (a < b && line.at(a).isSpace()) ++a;
        while (b > a && line.at(b - 1).isSpace()) --b;
        const QString text = line.mid(a, b - a);
        static const QRegularExpression ambiguous(QStringLiteral("\\[[^\\]]+\\]\\[[^\\]]*\\]|\\[[^\\]]+\\]\\([^)]*\\("));
        if (ambiguous.match(text).hasMatch()) return {};
        cells.append({a, b - a});
        start = cursor + 1;
    }
    return cells;
}
bool tableDelimiter(const QString &line, int columns) {
    const auto cells = tableCells(line);
    if (cells.size() != columns || columns == 0) return false;
    static const QRegularExpression delimiter(QStringLiteral("^:?-{3,}:?$"));
    for (const auto &cell : cells)
        if (!delimiter.match(line.mid(cell.start, cell.length)).hasMatch()) return false;
    return true;
}
bool graphemeBoundary(const QString &text, int position) {
    if (position < 0 || position > text.size()) return false;
    if (position == 0 || position == text.size()) return true;
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
    finder.setPosition(position);
    return finder.isAtBoundary();
}
bool validUtf16(const QString &text) {
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i).isHighSurrogate()) {
            if (++i >= text.size() || !text.at(i).isLowSurrogate()) return false;
        } else if (text.at(i).isLowSurrogate()) return false;
    }
    return true;
}
}

SourceVisualMapping SourceVisualMapping::create(const QString &source) {
    SourceVisualMapping result;
    result.m_source = source;
    QVector<SourceLine> lines;
    int offset = 0;
    while (offset < source.size()) {
        const int newline = source.indexOf(QLatin1Char('\n'), offset);
        const bool hasNewline = newline >= 0;
        const int end = hasNewline ? newline : source.size();
        int contentEnd = end;
        if (contentEnd > offset && source.at(contentEnd - 1) == QLatin1Char('\r')) --contentEnd;
        lines.append({offset, source.mid(offset, contentEnd - offset), hasNewline});
        offset = hasNewline ? newline + 1 : source.size();
    }
    Fence fence;
    bool inHtmlComment = false;
    int tableColumns = 0;
    int tableDelimiterIndex = -1;
    int tableHeaderIndex = -1;
    int tableStart = -1;
    for (int index = 0; index < lines.size(); ++index) {
        const auto &entry = lines.at(index);
        const QString &line = entry.text;
        offset = entry.start;
        const bool hasNewline = entry.newline;
        if (inHtmlComment) {
            result.appendSourceOnly(offset, line, hasNewline);
            if (line.contains(QStringLiteral("-->")))
                inHtmlComment = line.lastIndexOf(QStringLiteral("<!--")) > line.lastIndexOf(QStringLiteral("-->"));
            continue;
        }
        if (fence.length > 0) {
            result.appendSourceOnly(offset, line, hasNewline);
            if (closesFence(line, fence)) fence = {};
            continue;
        }
        if (line.contains(QStringLiteral("<!--"))) {
            inHtmlComment = line.lastIndexOf(QStringLiteral("<!--")) > line.lastIndexOf(QStringLiteral("-->"));
            tableColumns = 0;
            result.appendSourceOnly(offset, line, hasNewline);
            continue;
        }
        const Fence opening = openingFence(line);
        if (opening.length > 0) {
            fence = opening;
            tableColumns = 0;
            result.appendSourceOnly(offset, line, hasNewline);
            continue;
        }

        const auto cells = tableCells(line);
        const bool tableHeader = tableColumns == 0 && !cells.isEmpty() && index + 1 < lines.size()
            && tableDelimiter(lines.at(index + 1).text, cells.size());
        if (tableHeader) {
            tableColumns = cells.size();
            tableDelimiterIndex = index + 1;
            tableHeaderIndex = index;
            tableStart = offset;
        }
        const bool delimiter = index == tableDelimiterIndex;
        if (tableColumns > 0 && line.contains(QLatin1Char('|'))) {
            if (cells.size() != tableColumns) {
                result.appendSourceOnly(offset, line, hasNewline);
                continue;
            }
            const int visualStart = result.m_visual.size();
            for (int column = 0; column < cells.size(); ++column) {
                if (column > 0) result.m_visual += QString::fromUtf8("  │  ");
                const auto &cell = cells.at(column);
                if (delimiter) {
                    result.m_visual += QString::fromUtf8("────");
                } else {
                    const int cellVisualStart = result.m_visual.size();
                    result.appendInline(offset + cell.start, line.mid(cell.start, cell.length));
                    if (cell.length == 0)
                        result.m_mappings.append(Mapping{{offset + cell.start, 0}, {cellVisualStart, 0}});
                    result.m_tableCells.append({tableStart, index - tableHeaderIndex, column,
                        {offset + cell.start, cell.length},
                        {cellVisualStart, int(result.m_visual.size()) - cellVisualStart}});
                }
            }
            const int visualLength = result.m_visual.size() - visualStart;
            if (tableHeader) result.appendVisualFormat(VisualFormatKind::TableHeader, visualStart);
            result.appendNewline(hasNewline);
            result.m_blocks.append({delimiter ? BlockKind::TableDelimiter : BlockKind::TableRow,
                                    {offset, int(line.size())}, {visualStart, visualLength}, !delimiter});
            continue;
        }
        tableColumns = 0;

        static const QRegularExpression image(QStringLiteral(
            "^( {0,3})!\\[([^\\[\\]\\\\]*)\\]\\((?:<([^<>\\r\\n]+)>|([^\\s()<>\\\\]+))(?:[ \\t]+(?:\"[^\"\\\\]*\"|'[^'\\\\]*'))?\\)[ \\t]*$"));
        const auto imageMatch = image.match(line);
        if (imageMatch.hasMatch()) {
            const int visualStart = result.m_visual.size();
            result.m_visual += QString::fromUtf8("▧ ");
            const QString alt = imageMatch.captured(2);
            if (alt.isEmpty()) result.m_visual += QStringLiteral("Image");
            else result.appendMapped(offset + imageMatch.capturedStart(2), alt);
            const int visualLength = result.m_visual.size() - visualStart;
            const int destinationCapture = imageMatch.capturedStart(3) >= 0 ? 3 : 4;
            result.m_imageObjects.append({{offset, int(line.size())}, {visualStart, visualLength},
                {offset + int(imageMatch.capturedStart(2)), int(alt.size())},
                {offset + int(imageMatch.capturedStart(destinationCapture)), int(imageMatch.capturedLength(destinationCapture))},
                alt, imageMatch.captured(destinationCapture)});
            result.appendNewline(hasNewline);
            result.m_blocks.append({BlockKind::Image, {offset, int(line.size())}, {visualStart, visualLength}, !alt.isEmpty()});
            continue;
        }

        static const QRegularExpression thematicBreak(QStringLiteral(
            "^ {0,3}(?:(?:\\*[ \\t]*){3,}|(?:-[ \\t]*){3,}|(?:_[ \\t]*){3,})$"));
        if (isSourceOnlyLine(line) || thematicBreak.match(line).hasMatch()) {
            result.appendSourceOnly(offset, line, hasNewline);
        } else if (line.isEmpty()) {
            result.appendEditableLine(BlockKind::Paragraph, offset, line, 0, hasNewline);
        } else {
            static const QRegularExpression heading(QStringLiteral("^( {0,3}(#{1,6})[ \\t]+)"));
            static const QRegularExpression list(QStringLiteral("^(( {0,3})([-+*]|\\d+[.)])[ \\t]+)"));
            static const QRegularExpression quote(QStringLiteral("^> (.*)$"));
            static const QRegularExpression quoteLike(QStringLiteral("^\\s*>"));
            static const QRegularExpression task(QStringLiteral("^(( {0,3})(?:[-+*]|\\d+[.)])[ \\t]+\\[([ xX])\\][ \\t]+)(.*)$"));
            static const QRegularExpression taskLike(QStringLiteral("^\\s*(?:[-+*]|\\d+[.)])\\s+\\[[ xX]\\]"));
            const auto headingMatch = heading.match(line);
            const auto listMatch = list.match(line);
            const auto quoteMatch = quote.match(line);
            const auto taskMatch = task.match(line);
            if ((quoteLike.match(line).hasMatch() && !quoteMatch.hasMatch())
                    || (quoteMatch.hasMatch() && !isSafeBody(quoteMatch.captured(1)))
                    || (taskLike.match(line).hasMatch() && !taskMatch.hasMatch())
                    || (taskMatch.hasMatch() && !isSafeBody(taskMatch.captured(4)))) {
                result.appendSourceOnly(offset, line, hasNewline);
            } else if (quoteMatch.hasMatch()) {
                result.appendEditableLine(BlockKind::Paragraph, offset, line, 2, hasNewline, 0, QString::fromUtf8("❝ "));
            } else if (taskMatch.hasMatch()) {
                const bool checked = taskMatch.captured(3).compare(QStringLiteral("x"), Qt::CaseInsensitive) == 0;
                result.appendEditableLine(BlockKind::ListItem, offset, line, taskMatch.capturedLength(1), hasNewline, 0,
                    taskMatch.captured(2) + (checked ? QString::fromUtf8("☑ ") : QString::fromUtf8("☐ ")));
            } else if (headingMatch.hasMatch()) {
                result.appendEditableLine(BlockKind::Heading, offset, line, headingMatch.capturedLength(1), hasNewline,
                                          headingMatch.capturedLength(2));
            } else if (listMatch.hasMatch()) {
                const QString marker = listMatch.captured(3);
                const QString visualMarker = listMatch.captured(2) + (marker.at(0).isDigit()
                    ? marker + QLatin1Char(' ') : QString::fromUtf8("• "));
                result.appendEditableLine(BlockKind::ListItem, offset, line, listMatch.capturedLength(1), hasNewline, 0, visualMarker);
            } else {
                result.appendEditableLine(BlockKind::Paragraph, offset, line, 0, hasNewline);
            }
        }
    }
    // TextEdit has a caret after a final newline, including an empty document.
    // Give that plain paragraph an insertion anchor, except inside an unclosed fence.
    if ((source.isEmpty() || source.endsWith(QLatin1Char('\n'))) && fence.length == 0 && !inHtmlComment)
        result.appendEditableLine(BlockKind::Paragraph, source.size(), QString(), 0, false);
    return result;
}

void SourceVisualMapping::appendMapped(int sourceStart, const QString &text) {
    const int visualStart = m_visual.size();
    m_visual += text;
    if (text.isEmpty()) return;
    if (!m_mappings.isEmpty()) {
        Mapping &last = m_mappings.last();
        if (last.source.end() == sourceStart && last.visual.end() == visualStart) {
            last.source.length += int(text.size());
            last.visual.length += int(text.size());
            return;
        }
    }
    m_mappings.append(Mapping{{sourceStart, int(text.size())}, {visualStart, int(text.size())}});
}

void SourceVisualMapping::appendInline(int sourceStart, const QString &text) {
    int cursor = 0;
    while (cursor < int(text.size())) {
        const QString tail = text.mid(cursor);
        static const QRegularExpression link(QStringLiteral("^\\[([^\\]]+)\\]\\(((?:\\\\.|[^)])+)\\)"));
        static const QRegularExpression strong(QStringLiteral("^(\\*\\*|__)([^\\n]+?)\\1"));
        static const QRegularExpression emphasis(QStringLiteral("^(\\*|_)([^\\n*_]+)\\1"));
        const auto linkMatch = link.match(tail);
        const auto strongMatch = strong.match(tail);
        const auto emphasisMatch = emphasis.match(tail);
        if (tail.startsWith(QLatin1Char('`'))) {
            int delimiterLength = 1;
            while (delimiterLength < tail.size() && tail.at(delimiterLength) == QLatin1Char('`')) ++delimiterLength;
            int closing = delimiterLength;
            while (closing < tail.size()) {
                closing = tail.indexOf(QLatin1Char('`'), closing);
                if (closing < 0) break;
                int run = 1;
                while (closing + run < tail.size() && tail.at(closing + run) == QLatin1Char('`')) ++run;
                if (run == delimiterLength) break;
                closing += run;
            }
            if (closing > delimiterLength) {
                const int visualStart = m_visual.size();
                appendMapped(sourceStart + cursor + delimiterLength, tail.mid(delimiterLength, closing - delimiterLength));
                appendVisualFormat(VisualFormatKind::InlineCode, visualStart);
                cursor += closing + delimiterLength;
                continue;
            }
        }
        if (linkMatch.hasMatch()) {
            const int labelStart = cursor + linkMatch.capturedStart(1);
            const int visualStart = m_visual.size();
            appendInline(sourceStart + labelStart, linkMatch.captured(1));
            appendVisualFormat(VisualFormatKind::LinkLabel, visualStart);
            cursor += linkMatch.capturedLength(0);
        } else if (strongMatch.hasMatch()) {
            const int contentStart = cursor + strongMatch.capturedStart(2);
            const int visualStart = m_visual.size();
            appendInline(sourceStart + contentStart, strongMatch.captured(2));
            appendVisualFormat(VisualFormatKind::Strong, visualStart);
            cursor += strongMatch.capturedLength(0);
        } else if (emphasisMatch.hasMatch()) {
            const int contentStart = cursor + emphasisMatch.capturedStart(2);
            const int visualStart = m_visual.size();
            appendInline(sourceStart + contentStart, emphasisMatch.captured(2));
            appendVisualFormat(VisualFormatKind::Emphasis, visualStart);
            cursor += emphasisMatch.capturedLength(0);
        } else {
            appendMapped(sourceStart + cursor, text.mid(cursor, 1));
            ++cursor;
        }
    }
}

void SourceVisualMapping::appendVisualFormat(VisualFormatKind kind, int visualStart,
                                             int headingLevel) {
    const int length = m_visual.size() - visualStart;
    if (length > 0)
        m_visualFormatSpans.append(VisualFormatSpan{kind, {visualStart, length}, headingLevel});
}

void SourceVisualMapping::appendNewline(bool sourceHasNewline) {
    if (!sourceHasNewline) return;
    m_visual += QLatin1Char('\n');
}

void SourceVisualMapping::appendSourceOnly(int sourceStart, const QString &text, bool newline) {
    const int visualStart = m_visual.size();
    m_visual += text;
    appendNewline(newline);
    m_blocks.append(Block{BlockKind::SourceOnly, {sourceStart, int(text.size())},
                          {visualStart, int(text.size())}, false});
}

void SourceVisualMapping::appendEditableLine(BlockKind kind, int sourceStart, const QString &text,
                                             int contentOffset, bool newline, int headingLevel,
                                             const QString &visualPrefix) {
    const int visualStart = m_visual.size();
    m_visual += visualPrefix;
    appendInline(sourceStart + contentOffset, text.mid(contentOffset));
    if (contentOffset == text.size())
        m_mappings.append(Mapping{{sourceStart + contentOffset, 0}, {int(m_visual.size()), 0}});
    const int visualLength = m_visual.size() - visualStart;
    if (kind == BlockKind::Heading)
        appendVisualFormat(VisualFormatKind::Heading, visualStart, headingLevel);
    appendNewline(newline);
    m_blocks.append(Block{kind, {sourceStart, int(text.size())}, {visualStart, visualLength}, true});
}

SourceVisualMapping::Span SourceVisualMapping::visualSpanForSource(Span requested) const {
    if (!requested.isValid() || requested.end() > m_source.size()) return {};
    if (requested.length == 0) return {};
    int visualStart = -1;
    int previousSource = -1;
    int previousVisual = -1;
    for (const Mapping &mapping : m_mappings) {
        const int first = qMax(requested.start, mapping.source.start);
        const int last = qMin(requested.end(), mapping.source.end());
        if (first >= last) continue;
        if (first != (previousSource < 0 ? requested.start : previousSource)
                || (previousVisual >= 0 && mapping.visual.start + first - mapping.source.start != previousVisual)) {
            return {};
        }
        const int mappedVisual = mapping.visual.start + first - mapping.source.start;
        if (visualStart < 0) visualStart = mappedVisual;
        previousSource = last;
        previousVisual = mappedVisual + (last - first);
    }
    return previousSource == requested.end() ? Span{visualStart, previousVisual - visualStart} : Span{};
}

std::optional<SourceVisualMapping::SourceEdit> SourceVisualMapping::sourceEditForVisualReplacement(
        Span visual, const QString &replacement) const {
    if (!visual.isValid() || visual.end() > m_visual.size()
            || replacement.contains(QChar(0x0d)) || !validUtf16(replacement)
            || !graphemeBoundary(m_visual, visual.start)
            || !graphemeBoundary(m_visual, visual.end())) return std::nullopt;
    const bool lineBreak = replacement.contains(QLatin1Char('\n'));
    if (lineBreak && (visual.length != 0
            || m_source.contains(QLatin1Char('\r'))
            || (replacement != QStringLiteral("\n") && replacement != QStringLiteral("\n\n"))))
        return std::nullopt;
    for (const Mapping &mapping : m_mappings) {
        if (visual.start < mapping.visual.start || visual.end() > mapping.visual.end()) continue;
        const int offset = visual.start - mapping.visual.start;
        const SourceEdit edit{{mapping.source.start + offset, visual.length}, replacement};
        if (!graphemeBoundary(m_source, edit.source.start)
                || !graphemeBoundary(m_source, edit.source.end())) return std::nullopt;
        if (lineBreak) {
            bool plainParagraph = false;
            for (const Block &block : m_blocks) {
                if (block.kind == BlockKind::Paragraph && block.editable
                        && block.source.start <= edit.source.start
                        && edit.source.start <= block.source.end()) {
                    plainParagraph = true;
                    break;
                }
            }
            if (!plainParagraph) return std::nullopt;
        }
        QString candidate = m_source;
        candidate.replace(edit.source.start, edit.source.length, replacement);
        const SourceVisualMapping projected = create(candidate);
        QString expectedVisual = m_visual;
        expectedVisual.replace(visual.start, visual.length, replacement);
        if (projected.visualText() != expectedVisual) return std::nullopt;
        if (!lineBreak && !replacement.isEmpty()) {
            const Span newRange = projected.visualSpanForSource({edit.source.start, int(replacement.size())});
            if (!newRange.isValid() || newRange.start != visual.start) return std::nullopt;
        }
        return edit;
    }
    return std::nullopt;
}

SourceVisualMapping::Span SourceVisualMapping::sourceSpanForVisual(Span requested) const {
    if (!requested.isValid() || requested.end() > m_visual.size()) return {};
    if (requested.length == 0) return {};
    int sourceStart = -1;
    int previousVisual = -1;
    int previousSource = -1;
    for (const Mapping &mapping : m_mappings) {
        const int first = qMax(requested.start, mapping.visual.start);
        const int last = qMin(requested.end(), mapping.visual.end());
        if (first >= last) continue;
        if (first != (previousVisual < 0 ? requested.start : previousVisual)
                || (previousSource >= 0 && mapping.source.start + first - mapping.visual.start != previousSource)) {
            return {};
        }
        const int mappedSource = mapping.source.start + first - mapping.visual.start;
        if (sourceStart < 0) sourceStart = mappedSource;
        previousVisual = last;
        previousSource = mappedSource + (last - first);
    }
    return previousVisual == requested.end() ? Span{sourceStart, previousSource - sourceStart} : Span{};
}

std::optional<SourceVisualMapping::VisualBreakEdit> SourceVisualMapping::sourceEditForVisualBreak(
        int visualPosition, bool softBreak) const {
    if (!graphemeBoundary(m_visual, visualPosition)) return std::nullopt;
    const QString paragraphBreak = softBreak ? QStringLiteral("\n") : QStringLiteral("\n\n");
    if (const auto edit = sourceEditForVisualReplacement({visualPosition, 0}, paragraphBreak))
        return VisualBreakEdit{*edit, visualPosition + int(paragraphBreak.size())};
    if (softBreak) return std::nullopt;
    for (const Block &block : m_blocks) {
        if (block.kind != BlockKind::ListItem || !block.editable
                || visualPosition < block.visual.start || visualPosition > block.visual.end()) continue;
        const QString line = m_source.mid(block.source.start, block.source.length);
        static const QRegularExpression item(QStringLiteral(
            "^( {0,3})([-+*]|[0-9]{1,9}[.)])([ \\t]+)(?:\\[([ xX])\\]([ \\t]+))?(.*)$"));
        const auto match = item.match(line);
        if (!match.hasMatch()) return std::nullopt;
        const bool task = !match.captured(4).isEmpty();
        const QString oldMarker = match.captured(2);
        const QString visualPrefix = match.captured(1) + (task
            ? (match.captured(4).compare(QStringLiteral("x"), Qt::CaseInsensitive) == 0
                ? QString::fromUtf8("☑ ") : QString::fromUtf8("☐ "))
            : oldMarker.front().isDigit() ? oldMarker + QLatin1Char(' ') : QString::fromUtf8("• "));
        const int bodyVisualStart = block.visual.start + visualPrefix.size();
        const int bodySourceStart = block.source.start + match.capturedStart(6);
        if (visualPosition < bodyVisualStart) return std::nullopt;
        if (match.captured(6).isEmpty()) {
            const SourceEdit edit{block.source, QString()};
            QString candidate = m_source;
            candidate.replace(edit.source.start, edit.source.length, edit.replacement);
            const auto projected = create(candidate);
            for (const auto &nextBlock : projected.blocks()) {
                if (nextBlock.source.start == block.source.start && nextBlock.editable)
                    return VisualBreakEdit{edit, nextBlock.visual.end()};
            }
            return std::nullopt;
        }

        const bool splitting = visualPosition < block.visual.end();
        if (splitting) {
            // A following indented block or lazy continuation belongs to this
            // item. Moving its parentage would require a structural list parser.
            int next = m_source.indexOf(QLatin1Char('\n'), block.source.end());
            bool separated = false;
            while (next >= 0 && ++next < m_source.size()) {
                const int end = m_source.indexOf(QLatin1Char('\n'), next);
                const QString following = m_source.mid(next, (end < 0 ? m_source.size() : end) - next);
                if (following.trimmed().isEmpty()) { separated = true; next = end; continue; }
                int indentation = 0;
                while (indentation < following.size() && following.at(indentation) == QLatin1Char(' ')) ++indentation;
                if ((indentation < following.size() && following.at(indentation) == QLatin1Char('\t'))
                        || indentation > match.capturedLength(1)) return std::nullopt;
                if (!separated && !item.match(following).hasMatch()) return std::nullopt;
                break;
            }
            // A split cannot cut through the contents of an inline construct.
            // Complete strong/emphasis/link/code spans on either side remain
            // untouched, including their exact source delimiters.
            for (const auto &format : m_visualFormatSpans)
                if (format.visual.start < visualPosition && visualPosition < format.visual.end())
                    return std::nullopt;
        }

        QString marker = oldMarker;
        if (marker.front().isDigit()) {
            const QString digits = marker.left(marker.size() - 1);
            bool valid = false;
            const int number = digits.toInt(&valid);
            if (!valid || number >= 999999999) return std::nullopt;
            marker = QString::number(number + 1).rightJustified(digits.size(), QLatin1Char('0')) + marker.back();
        }
        QString prefix = match.captured(1) + marker + match.captured(3);
        if (task) prefix += QStringLiteral("[ ]") + match.captured(5);
        const QString nextVisualPrefix = match.captured(1) + (task ? QString::fromUtf8("☐ ")
            : marker.front().isDigit() ? marker + QLatin1Char(' ') : QString::fromUtf8("• "));
        // Keep this item's line ending convention, including a final item.
        const bool crlf = m_source.mid(block.source.end(), 2) == QStringLiteral("\r\n")
            || (block.source.end() == m_source.size() && m_source.contains(QStringLiteral("\r\n")));
        const QString newline = crlf ? QStringLiteral("\r\n") : QStringLiteral("\n");
        QVector<int> anchors;
        if (visualPosition == block.visual.end()) anchors.append(block.source.end());
        if (visualPosition == bodyVisualStart) anchors.append(bodySourceStart);
        for (const Mapping &mapping : m_mappings) {
            if (visualPosition < mapping.visual.start || visualPosition > mapping.visual.end()) continue;
            const int anchor = mapping.source.start + visualPosition - mapping.visual.start;
            if (bodySourceStart <= anchor && anchor <= block.source.end() && !anchors.contains(anchor)) anchors.append(anchor);
        }
        QString expectedVisual = m_visual;
        expectedVisual.insert(visualPosition, QLatin1Char('\n') + nextVisualPrefix);
        for (int anchor : anchors) {
            if (!graphemeBoundary(m_source, anchor)) continue;
            // Do not turn an ordinary suffix into a heading, nested list,
            // quote or other block merely because it begins a new item.
            if (splitting && !isSafeBody(m_source.mid(anchor, block.source.end() - anchor))) continue;
            const SourceEdit edit{{anchor, 0}, newline + prefix};
            QString candidate = m_source;
            candidate.insert(anchor, edit.replacement);
            const auto projected = create(candidate);
            if (projected.visualText() != expectedVisual) continue;
            const int nextSourceStart = anchor + newline.size();
            for (const auto &nextBlock : projected.blocks()) {
                if (nextBlock.source.start == nextSourceStart && nextBlock.kind == BlockKind::ListItem && nextBlock.editable)
                    return VisualBreakEdit{edit, visualPosition + 1 + int(nextVisualPrefix.size())};
            }
        }
        return std::nullopt;
    }
    return std::nullopt;
}

SourceVisualMapping::TableNavigation SourceVisualMapping::navigateTable(Span selection, bool backwards) const {
    if (!selection.isValid() || selection.end() > m_visual.size())
        return {TableNavigationStatus::UnsupportedSelection};
    int current = -1;
    for (int index = 0; index < m_tableCells.size(); ++index) {
        const auto &cell = m_tableCells.at(index);
        if (selection.start >= cell.visual.start && selection.end() <= cell.visual.end()) {
            current = index;
            break;
        }
    }
    if (current < 0) {
        for (const auto &block : m_blocks) {
            if (block.kind != BlockKind::TableRow && block.kind != BlockKind::TableDelimiter) continue;
            if (selection.start <= block.visual.end() && selection.end() >= block.visual.start)
                return {TableNavigationStatus::UnsupportedSelection};
        }
        return {};
    }
    if (!graphemeBoundary(m_visual, selection.start) || !graphemeBoundary(m_visual, selection.end()))
        return {TableNavigationStatus::UnsupportedSelection};
    const int next = current + (backwards ? -1 : 1);
    if (next < 0 || next >= m_tableCells.size()) return {TableNavigationStatus::Boundary};
    const auto &from = m_tableCells.at(current);
    const auto &to = m_tableCells.at(next);
    if (from.tableStart != to.tableStart) return {TableNavigationStatus::Boundary};
    const int rowGap = qAbs(from.row - to.row);
    // The separator is the only row we may skip. A malformed row forms a
    // deliberate navigation boundary even when later rows are editable again.
    if (rowGap > 1 && !(rowGap == 2 && qMin(from.row, to.row) == 0))
        return {TableNavigationStatus::Boundary};
    return {TableNavigationStatus::Moved, to.visual.start};
}
