// Included after FomawriteTest's class definition; slot declarations live there.
//
// Review cycle R3 (INDEPENDENT_REVIEW.md §8): property-based fuzzing of the
// Source<->Visual mapper, the code most able to corrupt text silently.
// Random Markdown-ish sources and random visual edits are checked against the
// mapper's integrity contract: the source is retained verbatim, projection
// never alters a mapped character, and an accepted edit changes exactly the
// selected text and nothing else. Refusals (nullopt / invalid spans) are the
// bounded outcome and are counted so the run is demonstrably non-vacuous.
//
// Deterministic: a fixed QRandomGenerator seed. Override with
//   FOMAWRITE_FUZZ_SEED=<uint>  FOMAWRITE_FUZZ_ITERATIONS=<documents>
// Every failure message prints the seed, the iteration, the escaped UTF-16
// source and the edit so the case can be replayed exactly.
//
// Findings so far are pinned as expected failures at the end of this file
// (mapperBreakCursorAfterZeroWidthSpaceSplitsGrapheme,
// mapperParagraphBreakBeforeQuoteMarkerLeavesCursorUneditable). Long runs
// with other seeds, e.g. FOMAWRITE_FUZZ_SEED=4 FOMAWRITE_FUZZ_ITERATIONS=150000,
// reach the first of them; the default seed does not.

#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QTextBoundaryFinder>

namespace MapperFuzz {
using Span = SourceVisualMapping::Span;
using Mapping = SourceVisualMapping::Mapping;
using Block = SourceVisualMapping::Block;
using BlockKind = SourceVisualMapping::BlockKind;
using NavigationStatus = SourceVisualMapping::TableNavigationStatus;

constexpr quint32 kDefaultSeed = 0x52334d41u;

// Span only defines operator==.
bool operator!=(const Span &a, const Span &b) { return !(a == b); }

quint32 seed() {
    if (qEnvironmentVariableIsSet("FOMAWRITE_FUZZ_SEED")) {
        bool ok = false;
        const quint32 value = qEnvironmentVariable("FOMAWRITE_FUZZ_SEED").toUInt(&ok);
        if (ok) return value;
    }
    return kDefaultSeed;
}

int iterations(int fallback) {
    if (qEnvironmentVariableIsSet("FOMAWRITE_FUZZ_ITERATIONS")) {
        bool ok = false;
        const int value = qEnvironmentVariable("FOMAWRITE_FUZZ_ITERATIONS").toInt(&ok);
        if (ok && value > 0) return value;
    }
    return fallback;
}

// Escapes every non-printable or non-ASCII UTF-16 unit so a failing input can
// be pasted back verbatim (including lone surrogates and combining marks).
QString escape(const QString &text) {
    QString out;
    out.reserve(text.size() + 2);
    out += QLatin1Char('"');
    for (const QChar c : text) {
        const ushort u = c.unicode();
        if (u == '\n') out += QStringLiteral("\\n");
        else if (u == '\r') out += QStringLiteral("\\r");
        else if (u == '\t') out += QStringLiteral("\\t");
        else if (u == '\\') out += QStringLiteral("\\\\");
        else if (u == '"') out += QStringLiteral("\\\"");
        else if (u >= 0x20 && u < 0x7f) out += c;
        else out += QStringLiteral("\\u%1").arg(u, 4, 16, QLatin1Char('0'));
    }
    out += QLatin1Char('"');
    return out;
}

QString describe(Span span) {
    return QStringLiteral("{%1,%2}").arg(span.start).arg(span.length);
}

bool graphemeBoundary(const QString &text, int position) {
    if (position < 0 || position > text.size()) return false;
    if (position == 0 || position == text.size()) return true;
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
    finder.setPosition(position);
    return finder.isAtBoundary();
}

struct Generator {
    QRandomGenerator rng;
    explicit Generator(quint32 seedValue) : rng(seedValue) {}

    int below(int bound) { return bound <= 1 ? 0 : int(rng.bounded(quint32(bound))); }
    bool chance(int percent) { return int(rng.bounded(100u)) < percent; }
    const QString &pick(const QVector<QString> &items) { return items.at(below(items.size())); }

    QString letter() {
        const int roll = below(62);
        if (roll < 26) return QString(QChar('a' + roll));
        if (roll < 52) return QString(QChar('A' + roll - 26));
        return QString(QChar('0' + roll - 52));
    }

    // Multi-codepoint graphemes and other units that stress UTF-16 offsets.
    QString grapheme() {
        static const QVector<QString> items = {
            QString::fromUtf8("e\xCC\x81"),                      // e + U+0301 combining acute
            QString::fromUtf8("\xC3\xA9"),                        // precomposed e-acute
            QString::fromUtf8("a\xCC\x88\xCC\xA3"),               // a + two combining marks
            QString::fromUtf8("\xF0\x9F\x98\x80"),                // U+1F600, one surrogate pair
            QString::fromUtf8("\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7"), // ZWJ family
            QString::fromUtf8("\xE2\x9D\xA4\xEF\xB8\x8F"),        // U+2764 + VS16
            QString::fromUtf8("\xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD"),// thumbs up + skin tone modifier
            QString::fromUtf8("\xF0\x9F\x87\xAB\xF0\x9F\x87\xB7"),// regional indicator pair (flag)
            QString::fromUtf8("\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E"), // CJK
            QString::fromUtf8("\xED\x95\x9C\xEA\xB8\x80"),        // Hangul syllables
            QString(QChar(0x00a0)),                               // NBSP
            QString::fromUtf8("\xE2\x80\x8B"),                    // zero width space
            QString(QChar(0x0301)),                               // lone combining acute: attaches to whatever precedes it
            QString(QChar(0x200d)),                               // lone ZWJ
            QString(QChar(0xfe0f)),                               // lone VS16
        };
        return pick(items);
    }

    QString syntax() {
        static const QVector<QString> items = {
            QStringLiteral(" "), QStringLiteral(" "), QStringLiteral(" "),
            QStringLiteral("-"), QStringLiteral("-"), QStringLiteral("*"), QStringLiteral("*"),
            QStringLiteral("_"), QStringLiteral("#"), QStringLiteral("|"), QStringLiteral("|"),
            QStringLiteral("["), QStringLiteral("]"), QStringLiteral("("), QStringLiteral(")"),
            QStringLiteral("!"), QStringLiteral(">"), QStringLiteral("\n"), QStringLiteral("\t"),
            QStringLiteral("`"), QStringLiteral("+"), QStringLiteral("."), QStringLiteral(":"),
            QStringLiteral("\\"), QStringLiteral("<"),
        };
        return pick(items);
    }

    QString atom(bool allowNewline) {
        const int roll = below(100);
        QString result;
        if (roll < 45) result = letter();
        else if (roll < 75) result = syntax();
        else if (roll < 92) result = grapheme();
        else result = QStringLiteral(" ");
        if (!allowNewline && result == QStringLiteral("\n")) result = QStringLiteral(" ");
        return result;
    }

    QString soup(int minTokens, int maxTokens, bool allowNewline) {
        QString result;
        const int count = minTokens + below(maxTokens - minTokens + 1);
        for (int i = 0; i < count; ++i) result += atom(allowNewline);
        return result;
    }

    QString word() {
        QString result;
        const int count = 1 + below(6);
        for (int i = 0; i < count; ++i) result += chance(85) ? letter() : grapheme();
        return result;
    }

    QString inlineText() {
        QString result;
        const int pieces = 1 + below(4);
        for (int i = 0; i < pieces; ++i) {
            if (i > 0 && chance(70)) result += QLatin1Char(' ');
            switch (below(10)) {
            case 0: result += QLatin1Char('*') + word() + QLatin1Char('*'); break;
            case 1: result += QStringLiteral("**") + word() + QStringLiteral("**"); break;
            case 2: result += QLatin1Char('_') + word() + QLatin1Char('_'); break;
            case 3: result += QLatin1Char('`') + (chance(80) ? word() : soup(1, 3, false)) + QLatin1Char('`'); break;
            case 4: result += QLatin1Char('[') + word() + QStringLiteral("](") + word() + QStringLiteral(".md)"); break;
            case 5: result += grapheme(); break;
            case 6: result += soup(1, 4, false); break;
            case 7: result += QStringLiteral("**[") + word() + QStringLiteral("](") + word() + QStringLiteral(")**"); break;
            default: result += word(); break;
            }
        }
        return result;
    }

    QString cell() {
        switch (below(6)) {
        case 0: return QString();
        case 1: return soup(1, 4, false);
        default: return inlineText();
        }
    }

    QString tableRow(int columns, bool delimiter) {
        QString row = chance(85) ? QStringLiteral("|") : QString();
        for (int column = 0; column < columns; ++column) {
            if (column > 0) row += QLatin1Char('|');
            if (delimiter) {
                row += QLatin1Char(' ') + (chance(30) ? QStringLiteral(":") : QString())
                    + QString(3 + below(3), QLatin1Char('-')) + (chance(30) ? QStringLiteral(":") : QString()) + QLatin1Char(' ');
            } else {
                row += (chance(80) ? QStringLiteral(" ") : QString()) + cell() + (chance(80) ? QStringLiteral(" ") : QString());
            }
        }
        if (chance(85)) row += QLatin1Char('|');
        return row;
    }

    QString table() {
        const int columns = 1 + below(4);
        QStringList lines;
        lines << tableRow(columns, false) << tableRow(columns, true);
        const int rows = below(4);
        for (int row = 0; row < rows; ++row) {
            if (chance(12)) lines << tableRow(columns + 1, false);
            else if (chance(8)) lines << soup(1, 6, false);
            else lines << tableRow(columns, false);
        }
        return lines.join(QLatin1Char('\n'));
    }

    QString listMarker() {
        static const QVector<QString> markers = {
            QStringLiteral("-"), QStringLiteral("-"), QStringLiteral("*"), QStringLiteral("+"),
            QStringLiteral("1."), QStringLiteral("2)"), QStringLiteral("09."), QStringLiteral("999999999."),
        };
        return pick(markers);
    }

    QString line() {
        const int roll = below(100);
        if (roll < 22) return soup(1, 12, false);
        if (roll < 37) return inlineText();
        if (roll < 47) return QString(below(4), QLatin1Char(' ')) + QString(1 + below(6), QLatin1Char('#'))
            + (chance(90) ? QStringLiteral(" ") : QString()) + inlineText();
        if (roll < 60) {
            QString item = QString(below(4), QLatin1Char(' ')) + listMarker() + (chance(90) ? QStringLiteral(" ") : QStringLiteral("\t"));
            if (chance(30)) item += chance(50) ? QStringLiteral("[ ] ") : QStringLiteral("[x] ");
            if (chance(85)) item += inlineText();
            return item;
        }
        if (roll < 65) return QStringLiteral("> ") + inlineText();
        if (roll < 71) {
            const QString alt = chance(75) ? inlineText() : QString();
            const QString destination = chance(70) ? word() + QStringLiteral(".png")
                : QStringLiteral("<my ") + word() + QStringLiteral(".png>");
            return QStringLiteral("![") + alt + QStringLiteral("](") + destination
                + (chance(25) ? QStringLiteral(" \"title\"") : QString()) + QLatin1Char(')');
        }
        if (roll < 76) return QString();
        if (roll < 81) {
            static const QVector<QString> blocks = {
                QStringLiteral("```"), QStringLiteral("~~~"), QStringLiteral("---"), QStringLiteral("***"),
                QStringLiteral("<!-- note -->"), QStringLiteral("<!--"), QStringLiteral("-->"),
                QStringLiteral("    indented code"), QStringLiteral("[^1]: footnote"),
            };
            return pick(blocks);
        }
        return table();
    }

    // CRLF documents are generated occasionally so the mapper's line-ending
    // handling is exercised; those sources contain '\r' only before '\n'.
    QString document() {
        if (chance(10)) return soup(0, 40, true);
        const QString newline = chance(10) ? QStringLiteral("\r\n") : QStringLiteral("\n");
        QStringList lines;
        const int count = 1 + below(7);
        for (int i = 0; i < count; ++i) lines << line();
        QString result = lines.join(newline);
        if (chance(50)) result += newline;
        if (newline.size() == 2) result.replace(QLatin1Char('\n'), QStringLiteral("\r\n")).replace(QStringLiteral("\r\r\n"), QStringLiteral("\r\n"));
        return result;
    }

    QString replacement() {
        const int roll = below(100);
        if (roll < 15) return QString();
        if (roll < 25) return QStringLiteral("\n");
        if (roll < 30) return QStringLiteral("\n\n");
        if (roll < 33) return QStringLiteral("\r\n");
        if (roll < 36) return QStringLiteral("\n") + letter();
        if (roll < 38) return QString(QChar(0xd83d));            // lone high surrogate
        if (roll < 40) return letter() + QString(QChar(0xdc00)); // lone low surrogate
        if (roll < 55) return letter();
        if (roll < 65) return grapheme();
        return soup(1, 5, chance(10));
    }

    // Visual spans: inside one mapping, crossing mapping boundaries, empty, or
    // deliberately splitting a grapheme cluster.
    Span visualSpan(const SourceVisualMapping &mapping) {
        const QString &visual = mapping.visualText();
        const auto &mappings = mapping.mappings();
        const int mode = below(6);
        if (mode <= 1 && !mappings.isEmpty()) {
            const Mapping &entry = mappings.at(below(mappings.size()));
            const int from = below(entry.visual.length + 1);
            const int to = from + below(entry.visual.length - from + 1);
            return {entry.visual.start + from, to - from};
        }
        if (mode == 2) return {below(visual.size() + 1), 0};
        if (mode == 3) {
            const int start = below(visual.size() + 1);
            return {start, below(qMin(12, visual.size() - start) + 1)};
        }
        if (mode == 4) {
            QVector<int> inside;
            for (int i = 1; i < visual.size(); ++i)
                if (!graphemeBoundary(visual, i)) inside.append(i);
            if (!inside.isEmpty()) {
                const int start = inside.at(below(inside.size()));
                return {start, below(qMin(3, visual.size() - start) + 1)};
            }
        }
        if (mode == 5 && chance(50)) {
            // Out-of-range spans must be refused, never dereferenced.
            switch (below(3)) {
            case 0: return {-1, 1};
            case 1: return {int(visual.size()), 1};
            default: return {below(visual.size() + 1), -1};
            }
        }
        return {below(visual.size() + 1), 0};
    }
};

struct Stats {
    int documents = 0;
    int sourceUnits = 0;
    int mappedUnits = 0;
    int spanProbes = 0;
    int spanMapped = 0;
    int replacementsAccepted = 0;
    int replacementsRefused = 0;
    int breaksAccepted = 0;
    int breaksRefused = 0;
    int breakCursorsInsertable = 0;
    int breakCursorsRefused = 0;
    int navigateMoved = 0;
    int navigateBoundary = 0;
    int navigateOutside = 0;
    int navigateUnsupported = 0;
    int navigateCursorsInsertable = 0;
    int navigateCursorsRefused = 0;
    int navigateCursorsMidGrapheme = 0;
};

// (1) source retained, (2) every span in bounds, mappings non-overlapping and
// monotonic on both sides, (4) mapped visual text equals source text,
// (5) format/image/cell spans in bounds.
QString checkProjection(const SourceVisualMapping &mapping, const QString &original) {
    const QString &source = mapping.source();
    const QString &visual = mapping.visualText();
    if (source != original) return QStringLiteral("source() differs from the input");
    if (mapping.roundTripSource() != original) return QStringLiteral("roundTripSource() differs from the input");
    const auto &mappings = mapping.mappings();
    for (int i = 0; i < mappings.size(); ++i) {
        const Mapping &entry = mappings.at(i);
        if (!entry.source.isValid() || !entry.visual.isValid()
                || entry.source.end() > source.size() || entry.visual.end() > visual.size())
            return QStringLiteral("mapping %1 out of bounds: source %2 visual %3").arg(i).arg(describe(entry.source), describe(entry.visual));
        if (entry.source.length != entry.visual.length)
            return QStringLiteral("mapping %1 has unequal lengths: source %2 visual %3").arg(i).arg(describe(entry.source), describe(entry.visual));
        if (source.mid(entry.source.start, entry.source.length) != visual.mid(entry.visual.start, entry.visual.length))
            return QStringLiteral("mapping %1 projects altered text: source %2 visual %3").arg(i)
                .arg(escape(source.mid(entry.source.start, entry.source.length)), escape(visual.mid(entry.visual.start, entry.visual.length)));
        if (i > 0) {
            const Mapping &previous = mappings.at(i - 1);
            if (previous.source.end() > entry.source.start || previous.visual.end() > entry.visual.start)
                return QStringLiteral("mappings %1/%2 overlap or are not monotonic: %3->%4 then %5->%6").arg(i - 1).arg(i)
                    .arg(describe(previous.source), describe(previous.visual), describe(entry.source), describe(entry.visual));
        }
    }
    const auto &blocks = mapping.blocks();
    for (int i = 0; i < blocks.size(); ++i) {
        const Block &block = blocks.at(i);
        if (!block.source.isValid() || !block.visual.isValid()
                || block.source.end() > source.size() || block.visual.end() > visual.size())
            return QStringLiteral("block %1 out of bounds: source %2 visual %3").arg(i).arg(describe(block.source), describe(block.visual));
        if (i > 0) {
            const Block &previous = blocks.at(i - 1);
            if (previous.source.end() > block.source.start || previous.visual.end() > block.visual.start)
                return QStringLiteral("blocks %1/%2 overlap or are not monotonic").arg(i - 1).arg(i);
        }
        if (block.kind == BlockKind::SourceOnly) {
            if (block.editable) return QStringLiteral("block %1 is SourceOnly yet editable").arg(i);
            if (source.mid(block.source.start, block.source.length) != visual.mid(block.visual.start, block.visual.length))
                return QStringLiteral("source-only block %1 is not shown verbatim").arg(i);
        }
    }
    const auto &formats = mapping.visualFormatSpans();
    for (int i = 0; i < formats.size(); ++i) {
        const auto &format = formats.at(i);
        if (!format.visual.isValid() || format.visual.length <= 0 || format.visual.end() > visual.size())
            return QStringLiteral("format span %1 out of bounds: %2").arg(i).arg(describe(format.visual));
        const bool heading = format.kind == SourceVisualMapping::VisualFormatKind::Heading;
        if (heading ? (format.headingLevel < 1 || format.headingLevel > 6) : format.headingLevel != 0)
            return QStringLiteral("format span %1 has heading level %2").arg(i).arg(format.headingLevel);
    }
    for (int i = 0; i < mapping.imageObjects().size(); ++i) {
        const auto &image = mapping.imageObjects().at(i);
        for (const Span &span : {image.source, image.altSource, image.destinationSource})
            if (!span.isValid() || span.end() > source.size())
                return QStringLiteral("image %1 source span out of bounds: %2").arg(i).arg(describe(span));
        if (!image.visual.isValid() || image.visual.end() > visual.size())
            return QStringLiteral("image %1 visual span out of bounds").arg(i);
        if (source.mid(image.altSource.start, image.altSource.length) != image.altText)
            return QStringLiteral("image %1 altText differs from its source span").arg(i);
        if (source.mid(image.destinationSource.start, image.destinationSource.length) != image.destination)
            return QStringLiteral("image %1 destination differs from its source span").arg(i);
    }
    for (int i = 0; i < mapping.tableCellObjects().size(); ++i) {
        const auto &cell = mapping.tableCellObjects().at(i);
        if (!cell.source.isValid() || cell.source.end() > source.size()
                || !cell.visual.isValid() || cell.visual.end() > visual.size() || cell.row < 0 || cell.column < 0
                || cell.tableStart < 0 || cell.tableStart > cell.source.start)
            return QStringLiteral("table cell %1 out of bounds").arg(i);
    }
    return {};
}

// (3) Span round trips: a span wholly inside one mapping must map in both
// directions and invert exactly; any span that maps must carry identical text
// and invert exactly.
QString checkSpanRoundTrips(const SourceVisualMapping &mapping, Generator &generator, Stats &stats) {
    const QString &source = mapping.source();
    const QString &visual = mapping.visualText();
    for (const Mapping &entry : mapping.mappings()) {
        if (entry.visual.length == 0) continue;
        for (int probe = 0; probe < 2; ++probe) {
            const int from = generator.below(entry.visual.length);
            const int length = 1 + generator.below(entry.visual.length - from);
            const Span visualSpan{entry.visual.start + from, length};
            const Span expectedSource{entry.source.start + from, length};
            const Span sourceSpan = mapping.sourceSpanForVisual(visualSpan);
            ++stats.spanProbes;
            if (sourceSpan != expectedSource)
                return QStringLiteral("sourceSpanForVisual(%1) = %2, expected %3 inside mapping %4->%5")
                    .arg(describe(visualSpan), describe(sourceSpan), describe(expectedSource), describe(entry.source), describe(entry.visual));
            if (mapping.visualSpanForSource(expectedSource) != visualSpan)
                return QStringLiteral("visualSpanForSource(%1) = %2, expected %3")
                    .arg(describe(expectedSource), describe(mapping.visualSpanForSource(expectedSource)), describe(visualSpan));
            ++stats.spanMapped;
        }
    }
    for (int probe = 0; probe < 12; ++probe) {
        ++stats.spanProbes;
        if (probe % 2 == 0) {
            const int start = generator.below(visual.size() + 2) - 1;
            const Span visualSpan{start, generator.below(16) - 1};
            const Span sourceSpan = mapping.sourceSpanForVisual(visualSpan);
            if (!sourceSpan.isValid()) continue;
            ++stats.spanMapped;
            if (!visualSpan.isValid() || visualSpan.end() > visual.size() || sourceSpan.end() > source.size()
                    || sourceSpan.length != visualSpan.length)
                return QStringLiteral("sourceSpanForVisual(%1) = %2 is out of bounds").arg(describe(visualSpan), describe(sourceSpan));
            if (source.mid(sourceSpan.start, sourceSpan.length) != visual.mid(visualSpan.start, visualSpan.length))
                return QStringLiteral("sourceSpanForVisual(%1) = %2 maps to different text").arg(describe(visualSpan), describe(sourceSpan));
            if (mapping.visualSpanForSource(sourceSpan) != visualSpan)
                return QStringLiteral("visualSpanForSource(sourceSpanForVisual(%1) = %2) = %3")
                    .arg(describe(visualSpan), describe(sourceSpan), describe(mapping.visualSpanForSource(sourceSpan)));
        } else {
            const int start = generator.below(source.size() + 2) - 1;
            const Span sourceSpan{start, generator.below(16) - 1};
            const Span visualSpan = mapping.visualSpanForSource(sourceSpan);
            if (!visualSpan.isValid()) continue;
            ++stats.spanMapped;
            if (!sourceSpan.isValid() || sourceSpan.end() > source.size() || visualSpan.end() > visual.size()
                    || sourceSpan.length != visualSpan.length)
                return QStringLiteral("visualSpanForSource(%1) = %2 is out of bounds").arg(describe(sourceSpan), describe(visualSpan));
            if (source.mid(sourceSpan.start, sourceSpan.length) != visual.mid(visualSpan.start, visualSpan.length))
                return QStringLiteral("visualSpanForSource(%1) = %2 maps to different text").arg(describe(sourceSpan), describe(visualSpan));
            if (mapping.sourceSpanForVisual(visualSpan) != sourceSpan)
                return QStringLiteral("sourceSpanForVisual(visualSpanForSource(%1) = %2) = %3")
                    .arg(describe(sourceSpan), describe(visualSpan), describe(mapping.sourceSpanForVisual(visualSpan)));
        }
    }
    return {};
}

// A replacement edit, when accepted, must (a) reproject to the old visual
// text with the same replacement applied, (b) leave source outside the edit
// untouched, (c) keep the replacement mapped at the same visual position, and
// remove from source exactly the text that was selected in Visual.
QString checkReplacement(const SourceVisualMapping &mapping, Span visualSpan, const QString &replacement,
                         Stats &stats, SourceVisualMapping *next) {
    const QString &source = mapping.source();
    const QString &visual = mapping.visualText();
    const auto edit = mapping.sourceEditForVisualReplacement(visualSpan, replacement);
    const bool spanInRange = visualSpan.isValid() && visualSpan.end() <= visual.size();
    if (!edit) {
        ++stats.replacementsRefused;
        return {};
    }
    ++stats.replacementsAccepted;
    if (!spanInRange) return QStringLiteral("an out-of-range visual span was accepted");
    if (!edit->source.isValid() || edit->source.end() > source.size())
        return QStringLiteral("edit source span %1 is out of bounds").arg(describe(edit->source));
    if (edit->replacement != replacement)
        return QStringLiteral("replacement was rewritten to %1").arg(escape(edit->replacement));
    if (edit->source.length != visualSpan.length)
        return QStringLiteral("edit removes %1 source units for %2 visual units").arg(edit->source.length).arg(visualSpan.length);
    if (source.mid(edit->source.start, edit->source.length) != visual.mid(visualSpan.start, visualSpan.length))
        return QStringLiteral("edit removes source %1 but the selection shows %2")
            .arg(escape(source.mid(edit->source.start, edit->source.length)), escape(visual.mid(visualSpan.start, visualSpan.length)));
    QString candidate = source;
    candidate.replace(edit->source.start, edit->source.length, edit->replacement);
    if (candidate.left(edit->source.start) != source.left(edit->source.start)
            || candidate.mid(edit->source.start + edit->replacement.size()) != source.mid(edit->source.end()))
        return QStringLiteral("source outside the edit changed");
    const SourceVisualMapping projected = SourceVisualMapping::create(candidate);
    QString expectedVisual = visual;
    expectedVisual.replace(visualSpan.start, visualSpan.length, replacement);
    if (projected.visualText() != expectedVisual)
        return QStringLiteral("reprojected visual %1 differs from expected %2").arg(escape(projected.visualText()), escape(expectedVisual));
    if (!replacement.isEmpty() && !replacement.contains(QLatin1Char('\n'))) {
        const Span mapped = projected.visualSpanForSource({edit->source.start, int(replacement.size())});
        if (mapped != Span{visualSpan.start, int(replacement.size())})
            return QStringLiteral("replacement is no longer mapped at %1 (got %2)").arg(visualSpan.start).arg(describe(mapped));
        if (projected.sourceSpanForVisual({visualSpan.start, int(replacement.size())}) != Span{edit->source.start, int(replacement.size())})
            return QStringLiteral("replacement does not map back to its source span");
    }
    const QString structural = checkProjection(projected, candidate);
    if (!structural.isEmpty()) return QStringLiteral("after the edit: ") + structural;
    if (next) *next = projected;
    return {};
}

// Return in Visual: either inserts a line break (plus a list prefix) without
// touching any existing text, or removes exactly an empty list item's marker.
QString checkBreak(const SourceVisualMapping &mapping, int position, bool softBreak, Stats &stats) {
    const QString &source = mapping.source();
    const QString &visual = mapping.visualText();
    const auto result = mapping.sourceEditForVisualBreak(position, softBreak);
    if (!result) {
        ++stats.breaksRefused;
        return {};
    }
    ++stats.breaksAccepted;
    const auto &edit = result->edit;
    if (position < 0 || position > visual.size()) return QStringLiteral("an out-of-range break position was accepted");
    if (!edit.source.isValid() || edit.source.end() > source.size())
        return QStringLiteral("break edit %1 is out of bounds").arg(describe(edit.source));
    QString candidate = source;
    candidate.replace(edit.source.start, edit.source.length, edit.replacement);
    const SourceVisualMapping projected = SourceVisualMapping::create(candidate);
    const QString &after = projected.visualText();
    if (result->visualCursor < 0 || result->visualCursor > after.size())
        return QStringLiteral("break cursor %1 is outside the new visual text (size %2)").arg(result->visualCursor).arg(after.size());
    if (!graphemeBoundary(after, result->visualCursor))
        return QStringLiteral("break cursor %1 splits a grapheme").arg(result->visualCursor);
    if (edit.source.length == 0) {
        if (!edit.replacement.startsWith(QLatin1Char('\n')) && !edit.replacement.startsWith(QStringLiteral("\r\n")))
            return QStringLiteral("break inserted %1, which is not a line break").arg(escape(edit.replacement));
        if (edit.replacement.contains(QLatin1Char('\r')) != source.contains(QStringLiteral("\r\n")))
            return QStringLiteral("break inserted %1 into a document with a different line ending").arg(escape(edit.replacement));
        if (after.left(position) != visual.left(position))
            return QStringLiteral("text before the break changed: %1").arg(escape(after));
        if (after.mid(result->visualCursor) != visual.mid(position))
            return QStringLiteral("text after the break changed: %1 (cursor %2)").arg(escape(after)).arg(result->visualCursor);
        if (position >= after.size() || after.at(position) != QLatin1Char('\n'))
            return QStringLiteral("no visual line break at %1 in %2").arg(position).arg(escape(after));
        const QString inserted = after.mid(position, result->visualCursor - position);
        if (inserted != QStringLiteral("\n\n") && inserted.mid(1).contains(QLatin1Char('\n')))
            return QStringLiteral("break inserted more than one visual line: %1").arg(escape(inserted));
    } else {
        if (!edit.replacement.isEmpty())
            return QStringLiteral("break replaced %1 with %2").arg(describe(edit.source), escape(edit.replacement));
        const Block *item = nullptr;
        for (const Block &block : mapping.blocks())
            if (block.source == edit.source && block.kind == BlockKind::ListItem && block.editable) item = &block;
        if (!item) return QStringLiteral("break removed %1, which is not an editable list item").arg(describe(edit.source));
        static const QRegularExpression emptyItem(QStringLiteral("^ {0,3}(?:[-+*]|[0-9]{1,9}[.)])[ \\t]+(?:\\[[ xX]\\][ \\t]+)?$"));
        if (!emptyItem.match(source.mid(edit.source.start, edit.source.length)).hasMatch())
            return QStringLiteral("break removed a list item with content: %1").arg(escape(source.mid(edit.source.start, edit.source.length)));
        if (after != visual.left(item->visual.start) + visual.mid(item->visual.end()))
            return QStringLiteral("exiting the list changed other text: %1").arg(escape(after));
        if (result->visualCursor != item->visual.start)
            return QStringLiteral("exit cursor %1 is not at the former item start %2").arg(result->visualCursor).arg(item->visual.start);
    }
    const QString structural = checkProjection(projected, candidate);
    if (!structural.isEmpty()) return QStringLiteral("after the break: ") + structural;
    // Typing right after Return should normally be accepted; a refusal here is
    // bounded so it is counted rather than failed. Sampled refusals were all a
    // paragraph break whose suffix begins with ">" and so becomes a
    // source-only block (see mapperParagraphBreakBeforeQuoteMarkerLeavesCursorUneditable).
    if (projected.sourceEditForVisualReplacement({result->visualCursor, 0}, QStringLiteral("x"))) ++stats.breakCursorsInsertable;
    else ++stats.breakCursorsRefused;
    return {};
}

// Table navigation is read-only: a Moved cursor must be the start of the
// adjacent cell of the same table; nothing may ever be out of range.
QString checkNavigate(const SourceVisualMapping &mapping, Span selection, bool backwards, Stats &stats) {
    const QString &visual = mapping.visualText();
    const auto navigation = mapping.navigateTable(selection, backwards);
    const auto &cells = mapping.tableCellObjects();
    int current = -1;
    for (int i = 0; i < cells.size() && current < 0; ++i)
        if (selection.isValid() && selection.start >= cells.at(i).visual.start && selection.end() <= cells.at(i).visual.end()) current = i;
    switch (navigation.status) {
    case NavigationStatus::OutsideTable:
        ++stats.navigateOutside;
        if (current >= 0) return QStringLiteral("selection %1 inside cell %2 reported OutsideTable").arg(describe(selection)).arg(current);
        return {};
    case NavigationStatus::Boundary:
        ++stats.navigateBoundary;
        if (current < 0) return QStringLiteral("selection %1 outside any cell reported Boundary").arg(describe(selection));
        return {};
    case NavigationStatus::UnsupportedSelection:
        ++stats.navigateUnsupported;
        return {};
    case NavigationStatus::Moved:
        break;
    }
    ++stats.navigateMoved;
    if (current < 0) return QStringLiteral("selection %1 outside any cell was Moved").arg(describe(selection));
    if (navigation.cursor < 0 || navigation.cursor > visual.size())
        return QStringLiteral("navigation cursor %1 is out of range").arg(navigation.cursor);
    const int target = current + (backwards ? -1 : 1);
    if (target < 0 || target >= cells.size()) return QStringLiteral("moved past the last cell");
    if (navigation.cursor != cells.at(target).visual.start)
        return QStringLiteral("cursor %1 is not the start of the adjacent cell %2 (%3)").arg(navigation.cursor).arg(target).arg(describe(cells.at(target).visual));
    if (cells.at(target).tableStart != cells.at(current).tableStart)
        return QStringLiteral("navigation crossed into another table");
    if (!graphemeBoundary(visual, navigation.cursor)) ++stats.navigateCursorsMidGrapheme;
    if (mapping.sourceEditForVisualReplacement({navigation.cursor, 0}, QStringLiteral("x"))) ++stats.navigateCursorsInsertable;
    else ++stats.navigateCursorsRefused;
    return {};
}
} // namespace MapperFuzz

void FomawriteTest::mapperFuzzProjectionInvariantsHold() {
    using namespace MapperFuzz;
    const quint32 seedValue = seed();
    const int documents = iterations(20000);
    Generator generator(seedValue);
    Stats stats;
    QElapsedTimer timer;
    timer.start();
    for (int iteration = 0; iteration < documents; ++iteration) {
        const QString source = generator.document();
        const auto mapping = SourceVisualMapping::create(source);
        ++stats.documents;
        stats.sourceUnits += source.size();
        for (const Mapping &entry : mapping.mappings()) stats.mappedUnits += entry.source.length;
        const QString context = QStringLiteral("\nseed=%1 iteration=%2\nsource=%3\nvisual=%4")
            .arg(seedValue).arg(iteration).arg(escape(source), escape(mapping.visualText()));
        QString failure = checkProjection(mapping, source);
        QVERIFY2(failure.isEmpty(), qPrintable(failure + context));
        failure = checkSpanRoundTrips(mapping, generator, stats);
        QVERIFY2(failure.isEmpty(), qPrintable(failure + context));
    }
    qInfo("mapper fuzz projection: seed=%u documents=%d sourceUnits=%d mappedUnits=%d spanProbes=%d spanMapped=%d elapsed=%lldms",
          seedValue, stats.documents, stats.sourceUnits, stats.mappedUnits, stats.spanProbes, stats.spanMapped, timer.elapsed());
    QVERIFY(stats.mappedUnits > 0);
    QVERIFY(stats.spanMapped > 0);
}

void FomawriteTest::mapperFuzzVisualEditsNeverCorruptSource() {
    using namespace MapperFuzz;
    const quint32 seedValue = seed();
    const int documents = iterations(6000);
    Generator generator(seedValue ^ 0x9e3779b9u);
    Stats stats;
    QElapsedTimer timer;
    timer.start();
    for (int iteration = 0; iteration < documents; ++iteration) {
        const QString original = generator.document();
        SourceVisualMapping mapping = SourceVisualMapping::create(original);
        ++stats.documents;
        stats.sourceUnits += original.size();
        // Accepted edits are chained so evolved documents are exercised too.
        for (int step = 0; step < 8; ++step) {
            const Span span = generator.visualSpan(mapping);
            const QString replacement = generator.replacement();
            const QString context = QStringLiteral("\nseed=%1 iteration=%2 step=%3 visualSpan=%4 replacement=%5\nsource=%6\nvisual=%7")
                .arg(seedValue).arg(iteration).arg(step).arg(describe(span), escape(replacement), escape(mapping.source()), escape(mapping.visualText()));
            SourceVisualMapping next = mapping;
            const QString failure = checkReplacement(mapping, span, replacement, stats, &next);
            QVERIFY2(failure.isEmpty(), qPrintable(failure + context));
            mapping = next;
        }
        for (int step = 0; step < 3; ++step) {
            const int position = generator.chance(10) ? generator.below(mapping.visualText().size() + 4) - 2
                : generator.below(mapping.visualText().size() + 1);
            const bool soft = generator.chance(25);
            const QString context = QStringLiteral("\nseed=%1 iteration=%2 break=%3 position=%4 soft=%5\nsource=%6\nvisual=%7")
                .arg(seedValue).arg(iteration).arg(step).arg(position).arg(soft).arg(escape(mapping.source()), escape(mapping.visualText()));
            const QString failure = checkBreak(mapping, position, soft, stats);
            QVERIFY2(failure.isEmpty(), qPrintable(failure + context));
        }
        for (int step = 0; step < 3; ++step) {
            Span selection;
            const auto &cells = mapping.tableCellObjects();
            if (!cells.isEmpty() && generator.chance(70)) {
                const auto &cell = cells.at(generator.below(cells.size()));
                const int from = generator.below(cell.visual.length + 1);
                selection = {cell.visual.start + from, generator.below(cell.visual.length - from + 1)};
            } else {
                selection = generator.visualSpan(mapping);
            }
            const bool backwards = generator.chance(40);
            const QString context = QStringLiteral("\nseed=%1 iteration=%2 navigate=%3 selection=%4 backwards=%5\nsource=%6\nvisual=%7")
                .arg(seedValue).arg(iteration).arg(step).arg(describe(selection)).arg(backwards).arg(escape(mapping.source()), escape(mapping.visualText()));
            const QString failure = checkNavigate(mapping, selection, backwards, stats);
            QVERIFY2(failure.isEmpty(), qPrintable(failure + context));
        }
    }
    const int replacements = stats.replacementsAccepted + stats.replacementsRefused;
    const int breaks = stats.breaksAccepted + stats.breaksRefused;
    qInfo("mapper fuzz edits: seed=%u documents=%d sourceUnits=%d elapsed=%lldms", seedValue, stats.documents, stats.sourceUnits, timer.elapsed());
    qInfo("  replacements: accepted=%d refused=%d (%.1f%% accepted)", stats.replacementsAccepted, stats.replacementsRefused,
          replacements ? 100.0 * stats.replacementsAccepted / replacements : 0.0);
    qInfo("  breaks: accepted=%d refused=%d (%.1f%% accepted); typing after an accepted break: ok=%d refused=%d",
          stats.breaksAccepted, stats.breaksRefused, breaks ? 100.0 * stats.breaksAccepted / breaks : 0.0,
          stats.breakCursorsInsertable, stats.breakCursorsRefused);
    qInfo("  navigateTable: moved=%d boundary=%d outside=%d unsupported=%d; typing at a moved cursor: ok=%d refused=%d midGrapheme=%d",
          stats.navigateMoved, stats.navigateBoundary, stats.navigateOutside, stats.navigateUnsupported,
          stats.navigateCursorsInsertable, stats.navigateCursorsRefused, stats.navigateCursorsMidGrapheme);
    QVERIFY2(stats.replacementsAccepted > 0, "the replacement fuzz never produced an accepted edit");
    QVERIFY2(stats.replacementsRefused > 0, "the replacement fuzz never produced a refusal");
    QVERIFY2(stats.breaksAccepted > 0, "the break fuzz never produced an accepted edit");
    QVERIFY2(stats.navigateMoved > 0, "the table navigation fuzz never moved");
}

// Minimised inputs for what the fuzz found. Each stays an expected failure
// until src/sourcevisualmapping.cpp changes; an XPASS then marks the slot for
// deletion. Neither loses bytes: both leave the caret where Backend's next
// keystroke is refused, so typing after Return is silently dropped.

void FomawriteTest::mapperBreakCursorAfterZeroWidthSpaceSplitsGrapheme() {
    // From mapperFuzzVisualEditsNeverCorruptSource, seed=4 iteration=136844.
    // U+200B has Grapheme_Cluster_Break=Control, so the position after it is a
    // boundary (UAX#29 GB4) even though a ZWJ follows. sourceEditForVisualBreak
    // validates only that old position; after the split the ZWJ attaches to the
    // new prefix's trailing space (GB9) and the returned cursor sits inside
    // that cluster (sourcevisualmapping.cpp:567 returns without checking the
    // projected text). The same holds for any Control unit (tab, U+200B, other
    // Cf) followed by Extend/ZWJ/VS16.
    const QString source = QStringLiteral("- a") + QChar(0x200b) + QChar(0x200d) + QStringLiteral("b");
    const auto mapping = SourceVisualMapping::create(source);
    const QString &visual = mapping.visualText();
    const int position = visual.indexOf(QChar(0x200d));
    QVERIFY(position > 0);
    QVERIFY(MapperFuzz::graphemeBoundary(visual, position));
    const auto edit = mapping.sourceEditForVisualBreak(position);
    QVERIFY(edit.has_value());
    QString candidate = source;
    candidate.replace(edit->edit.source.start, edit->edit.source.length, edit->edit.replacement);
    QCOMPARE(candidate, QStringLiteral("- a") + QChar(0x200b) + QStringLiteral("\n- ") + QChar(0x200d) + QStringLiteral("b"));
    const auto projected = SourceVisualMapping::create(candidate);
    QCOMPARE(edit->visualCursor, int(projected.visualText().indexOf(QChar(0x200d))));
    QEXPECT_FAIL("", "sourceEditForVisualBreak checks the old position only; the new cursor splits space+ZWJ", Continue);
    QVERIFY(MapperFuzz::graphemeBoundary(projected.visualText(), edit->visualCursor));
    QEXPECT_FAIL("", "typing at the returned cursor is refused", Continue);
    QVERIFY(projected.sourceEditForVisualReplacement({edit->visualCursor, 0}, QStringLiteral("x")).has_value());
}

void FomawriteTest::mapperParagraphBreakBeforeQuoteMarkerLeavesCursorUneditable() {
    // From the fuzz's sampled "typing after Return refused" cases ("L>E").
    // The paragraph line-break path (sourcevisualmapping.cpp:428-435) accepts a
    // break whenever the visible text is unchanged, so a suffix beginning with
    // ">" (likewise "    ", "---", "***", "[^", "```", "<!--") becomes a
    // different, non-editable block: CommonMark now reads ">E" as a block
    // quote. The list-split path refuses exactly this via isSafeBody.
    const QString source = QStringLiteral("L>E");
    const auto mapping = SourceVisualMapping::create(source);
    QVERIFY(mapping.blocks().first().editable);
    const auto edit = mapping.sourceEditForVisualBreak(1);
    QVERIFY(edit.has_value());
    QCOMPARE(edit->edit.replacement, QStringLiteral("\n\n"));
    QString candidate = source;
    candidate.replace(edit->edit.source.start, edit->edit.source.length, edit->edit.replacement);
    QCOMPARE(candidate, QStringLiteral("L\n\n>E"));
    const auto projected = SourceVisualMapping::create(candidate);
    QCOMPARE(projected.visualText(), QStringLiteral("L\n\n>E"));
    QCOMPARE(edit->visualCursor, 3);
    QCOMPARE(projected.blocks().size(), 3);
    QEXPECT_FAIL("", "the suffix is no longer an editable paragraph", Continue);
    QVERIFY(projected.blocks().at(2).editable);
    QEXPECT_FAIL("", "typing at the returned cursor is refused", Continue);
    QVERIFY(projected.sourceEditForVisualReplacement({edit->visualCursor, 0}, QStringLiteral("x")).has_value());
}
