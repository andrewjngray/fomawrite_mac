#pragma once

#include <QString>
#include <QVector>
#include <optional>

// A deliberately conservative, UTF-16-offset mapping between canonical
// Markdown source and text that the visual editor may edit.  This is not
// a Markdown serializer: source() is retained exactly, and only text that can
// be mapped without touching surrounding syntax is marked editable.
class SourceVisualMapping {
public:
    struct Span {
        int start = -1;
        int length = 0;

        int end() const { return start < 0 ? -1 : start + length; }
        bool isValid() const { return start >= 0 && length >= 0; }
        bool operator==(const Span &other) const {
            return start == other.start && length == other.length;
        }
    };

    enum class BlockKind { Paragraph, Heading, ListItem, Image, TableRow, TableDelimiter, SourceOnly };

    struct Block {
        BlockKind kind = BlockKind::SourceOnly;
        Span source;
        Span visual;
        bool editable = false;
    };

    struct Mapping {
        Span source;
        Span visual;
    };

    // Formatting stays separate from edit mappings, so a visual client can
    // style the projection without inferring Markdown source from it.
    enum class VisualFormatKind { Heading, Strong, Emphasis, LinkLabel, InlineCode, TableHeader };

    struct VisualFormatSpan {
        VisualFormatKind kind = VisualFormatKind::Emphasis;
        Span visual;
        // Heading levels are 1--6. It is zero for inline formats.
        int headingLevel = 0;
    };

    struct ImageObject {
        Span source;
        Span visual;
        Span altSource;
        Span destinationSource;
        QString altText;
        QString destination;
    };

    struct TableCell {
        int tableStart = -1;
        int row = 0; // Physical row: header 0, delimiter 1, first body row 2.
        int column = 0;
        Span source;
        Span visual;
    };

    enum class TableNavigationStatus { OutsideTable, Moved, Boundary, UnsupportedSelection };
    struct TableNavigation {
        TableNavigationStatus status = TableNavigationStatus::OutsideTable;
        int cursor = -1;
    };

    struct SourceEdit {
        Span source;
        QString replacement;
    };

    struct VisualBreakEdit {
        SourceEdit edit;
        int visualCursor = 0;
    };

    static SourceVisualMapping create(const QString &source);

    // The original source is the round-trip result. Mode changes must use this
    // value rather than serializing visualText().
    const QString &source() const { return m_source; }
    const QString &visualText() const { return m_visual; }
    QString roundTripSource() const { return m_source; }

    const QVector<Block> &blocks() const { return m_blocks; }
    const QVector<Mapping> &mappings() const { return m_mappings; }
    const QVector<VisualFormatSpan> &visualFormatSpans() const { return m_visualFormatSpans; }

    const QVector<ImageObject> &imageObjects() const { return m_imageObjects; }
    const QVector<TableCell> &tableCellObjects() const { return m_tableCells; }

    // Navigation is a read-only query. It never jumps over an unsupported row,
    // crosses a table boundary, or converts a multi-cell selection into input.
    TableNavigation navigateTable(Span selection, bool backwards) const;

    // A selection maps only when every selected UTF-16 unit is represented and
    // its counterpart is contiguous. Syntax markers, block prefixes and
    // source-only blocks deliberately return an invalid span.
    Span visualSpanForSource(Span source) const;
    Span sourceSpanForVisual(Span visual) const;

    // Produces one bounded source replacement only when the visual range is
    // wholly inside a single editable mapping. The candidate must reproject
    // exactly and remain mapped; grapheme boundaries cannot be split. A line
    // break is allowed only inside a paragraph with the same visible result.
    std::optional<SourceEdit> sourceEditForVisualReplacement(
        Span visual, const QString &replacement) const;

    // Return splits a simple single-line list item at a safe mapped boundary,
    // or continues it at the end, preserving its exact marker style. An empty
    // item exits the list. Ambiguous continuations/inline splits stay in Source.
    std::optional<VisualBreakEdit> sourceEditForVisualBreak(
        int visualPosition, bool softBreak = false) const;

private:
    void appendMapped(int sourceStart, const QString &text);
    void appendInline(int sourceStart, const QString &text);
    void appendVisualFormat(VisualFormatKind kind, int visualStart, int headingLevel = 0);
    void appendSourceOnly(int sourceStart, const QString &text, bool newline);
    void appendEditableLine(BlockKind kind, int sourceStart, const QString &text,
                            int contentOffset, bool newline, int headingLevel = 0,
                            const QString &visualPrefix = QString());
    void appendNewline(bool sourceHasNewline);

    QString m_source;
    QString m_visual;
    QVector<Block> m_blocks;
    QVector<Mapping> m_mappings;
    QVector<VisualFormatSpan> m_visualFormatSpans;
    QVector<ImageObject> m_imageObjects;
    QVector<TableCell> m_tableCells;
};
