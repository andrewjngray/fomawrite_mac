#include "sourcevisualmapping.h"

#include <QRegularExpression>

namespace {
bool isSourceOnlyLine(const QString &line, bool inFence) {
    if (inFence) return true;
    static const QRegularExpression unsupported(
        QStringLiteral("^\\s*(?:<!--|<!|\\[\\^|!\\[|\\|| {4,}|~~~|```)|\\[\\^|\\[[^\\]]+\\]\\[[^\\]]*\\]|^\\s*\\[[^\\]]+\\]:"));
    static const QRegularExpression ambiguousInlineLink(
        QStringLiteral("\\[[^\\]]+\\]\\([^\\n)]*\\("));
    // Markdown tables need not have a leading pipe. Keep every pipe-bearing
    // line source-only until the visual editor has a table-aware parser.
    return line.contains(QChar(0x60)) || line.contains(QChar(0x5c))
        || line.contains(QChar(0x3c)) || line.contains(QChar(0x7c))
        || ambiguousInlineLink.match(line).hasMatch() || unsupported.match(line).hasMatch();
}
bool isFence(const QString &line) {
    static const QRegularExpression fence(QStringLiteral("^\\s*(?:```|~~~)"));
    return fence.match(line).hasMatch();
}

// Quote and task markers are deliberately generated and unmapped. The body
// can then be edited without ever serializing or changing Markdown syntax.
// Nested and mixed block structures remain source-only for now.
bool isSafeQuoteBody(const QString &body) {
    static const QRegularExpression nestedOrBlock(
        QStringLiteral("^(?:>|[-+*]\\s|\\d+[.)]\\s|#{1,6}\\s|\\[[ xX]\\]\\s|!\\[|\\[\\^|---$|\\*\\*\\*$)"));
    return !body.isEmpty() && !nestedOrBlock.match(body).hasMatch();
}

bool isSafeTaskBody(const QString &body) {
    static const QRegularExpression nestedOrBlock(
        QStringLiteral("^(?:>|[-+*]\\s|\\d+[.)]\\s|#{1,6}\\s|\\[[ xX]\\]\\s|!\\[|\\[\\^|---$|\\*\\*\\*$)"));
    return !body.isEmpty() && !nestedOrBlock.match(body).hasMatch();
}
}

SourceVisualMapping SourceVisualMapping::create(const QString &source) {
    SourceVisualMapping result;
    result.m_source = source;

    int offset = 0;
    bool inFence = false;
    while (offset < source.size()) {
        const int newline = source.indexOf(QLatin1Char('\n'), offset);
        const bool hasNewline = newline >= 0;
        const int end = hasNewline ? newline : source.size();
        int contentEnd = end;
        if (contentEnd > offset && source.at(contentEnd - 1) == QLatin1Char('\r')) --contentEnd;
        const QString line = source.mid(offset, contentEnd - offset);
        const bool fenceLine = isFence(line);

        if (isSourceOnlyLine(line, inFence)) {
            result.appendSourceOnly(offset, line, hasNewline);
        } else if (line.isEmpty()) {
            result.appendEditableLine(BlockKind::Paragraph, offset, line, 0, hasNewline);
        } else {
            static const QRegularExpression heading(QStringLiteral("^( {0,3}(#{1,6})[ \t]+)"));
            static const QRegularExpression list(QStringLiteral("^(\\s*(?:[-+*]|\\d+[.)])\\s+)"));
            static const QRegularExpression quote(QStringLiteral("^> (.*)$"));
            static const QRegularExpression quoteLike(QStringLiteral("^\\s*>"));
            static const QRegularExpression task(
                QStringLiteral("^((?:[-+*]|\\d+[.)])\\s+\\[([ xX])\\]\\s+)(.*)$"));
            static const QRegularExpression taskLike(
                QStringLiteral("^\\s*(?:[-+*]|\\d+[.)])\\s+\\[[ xX]\\]"));
            const auto headingMatch = heading.match(line);
            const auto listMatch = list.match(line);
            const auto quoteMatch = quote.match(line);
            const auto taskMatch = task.match(line);
            if (quoteLike.match(line).hasMatch() && !quoteMatch.hasMatch()) {
                result.appendSourceOnly(offset, line, hasNewline);
            } else if (quoteMatch.hasMatch() && !isSafeQuoteBody(quoteMatch.captured(1))) {
                result.appendSourceOnly(offset, line, hasNewline);
            } else if (taskLike.match(line).hasMatch() && !taskMatch.hasMatch()) {
                result.appendSourceOnly(offset, line, hasNewline);
            } else if (taskMatch.hasMatch() && !isSafeTaskBody(taskMatch.captured(3))) {
                result.appendSourceOnly(offset, line, hasNewline);
            } else if (quoteMatch.hasMatch()) {
                result.appendEditableLine(BlockKind::Paragraph, offset, line, 2, hasNewline, 0,
                                          QString::fromUtf8("❝ "));
            } else if (taskMatch.hasMatch()) {
                const bool checked = taskMatch.captured(2).compare(QStringLiteral("x"), Qt::CaseInsensitive) == 0;
                result.appendEditableLine(BlockKind::ListItem, offset, line,
                                          taskMatch.capturedLength(1), hasNewline, 0,
                                          checked ? QString::fromUtf8("☑ ") : QString::fromUtf8("☐ "));
            } else if (headingMatch.hasMatch()) {
                result.appendEditableLine(BlockKind::Heading, offset, line,
                                          headingMatch.capturedLength(1), hasNewline,
                                          headingMatch.capturedLength(2));
            } else if (listMatch.hasMatch()) {
                // Show the list marker in Visual Edit without mapping it back
                // to source. Editing the body can never rewrite the marker.
                const QString marker = listMatch.captured(1).trimmed();
                const QString visualMarker = marker.at(0).isDigit()
                    ? marker + QLatin1Char(' ') : QString::fromUtf8("• ");
                result.appendEditableLine(BlockKind::ListItem, offset, line,
                                          listMatch.capturedLength(1), hasNewline, 0, visualMarker);
            } else {
                result.appendEditableLine(BlockKind::Paragraph, offset, line, 0, hasNewline);
            }
        }
        if (fenceLine) inFence = !inFence;
        offset = hasNewline ? newline + 1 : source.size();
    }
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
            || replacement.contains(QChar(0x0a))
            || replacement.contains(QChar(0x0d))) return std::nullopt;
    for (const Mapping &mapping : m_mappings) {
        if (visual.start < mapping.visual.start || visual.end() > mapping.visual.end()) continue;
        const int offset = visual.start - mapping.visual.start;
        return SourceEdit{{mapping.source.start + offset, visual.length}, replacement};
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
