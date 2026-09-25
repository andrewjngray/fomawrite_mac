#pragma once
#include "sourcevisualmapping.h"
#include <QColor>
#include <QFont>
#include <QSyntaxHighlighter>
class VisualTextHighlighter final : public QSyntaxHighlighter {
public:
    struct Style { QFont font; QColor textColor; QColor linkColor; QColor sourceOnlyColor = QColor(112, 112, 112); QColor sourceOnlyBackground = QColor(112, 112, 112, 30); };
    explicit VisualTextHighlighter(QTextDocument *document = nullptr);
    void setMapping(const SourceVisualMapping &mapping);
    void setFormatSpans(const QVector<SourceVisualMapping::VisualFormatSpan> &spans);
    void setBlocks(const QVector<SourceVisualMapping::Block> &blocks);
    void setStyle(const Style &style);
protected:
    void highlightBlock(const QString &text) override;
private:
    QTextCharFormat baseFormat() const;
    void applyGlobalFormat(int blockStart, int blockLength, const SourceVisualMapping::Span &span, const QTextCharFormat &format);
    QVector<SourceVisualMapping::VisualFormatSpan> m_spans;
    QVector<SourceVisualMapping::Block> m_blocks;
    Style m_style;
};
