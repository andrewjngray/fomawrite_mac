#include "visualtexthighlighter.h"
#include <QTextDocument>
VisualTextHighlighter::VisualTextHighlighter(QTextDocument *document) : QSyntaxHighlighter(document) {}
void VisualTextHighlighter::setMapping(const SourceVisualMapping &mapping) { m_spans = mapping.visualFormatSpans(); m_blocks = mapping.blocks(); rehighlight(); }
void VisualTextHighlighter::setFormatSpans(const QVector<SourceVisualMapping::VisualFormatSpan> &spans) { m_spans = spans; rehighlight(); }
void VisualTextHighlighter::setBlocks(const QVector<SourceVisualMapping::Block> &blocks) { m_blocks = blocks; rehighlight(); }
void VisualTextHighlighter::setStyle(const Style &style) { m_style = style; rehighlight(); }
QTextCharFormat VisualTextHighlighter::baseFormat() const {
    QTextCharFormat format;
    if (!m_style.font.family().isEmpty()) format.setFontFamilies(m_style.font.families());
    if (m_style.font.pointSizeF() > 0) format.setFontPointSize(m_style.font.pointSizeF());
    if (m_style.textColor.isValid()) format.setForeground(m_style.textColor);
    return format;
}
void VisualTextHighlighter::applyGlobalFormat(int blockStart, int blockLength, const SourceVisualMapping::Span &span, const QTextCharFormat &format) {
    const int first = qMax(blockStart, span.start);
    const int last = qMin(blockStart + blockLength, span.end());
    if (first < last) setFormat(first - blockStart, last - first, format);
}
void VisualTextHighlighter::highlightBlock(const QString &text) {
    const int blockStart = currentBlock().position();
    const int blockLength = text.size();
    if (blockLength == 0) return;
    setFormat(0, blockLength, baseFormat());
    for (const SourceVisualMapping::Block &block : m_blocks) {
        if (block.kind != SourceVisualMapping::BlockKind::SourceOnly) continue;
        QTextCharFormat format = baseFormat();
        if (m_style.sourceOnlyColor.isValid()) format.setForeground(m_style.sourceOnlyColor);
        if (m_style.sourceOnlyBackground.isValid()) format.setBackground(m_style.sourceOnlyBackground);
        applyGlobalFormat(blockStart, blockLength, block.visual, format);
    }
    for (const SourceVisualMapping::VisualFormatSpan &span : m_spans) {
        QTextCharFormat format = baseFormat();
        switch (span.kind) {
        case SourceVisualMapping::VisualFormatKind::Heading: {
            const qreal baseSize = m_style.font.pointSizeF() > 0 ? m_style.font.pointSizeF() : 12.0;
            const int level = qBound(1, span.headingLevel, 6);
            format.setFontPointSize(baseSize * (1.60 - (level - 1) * 0.10));
            format.setFontWeight(QFont::DemiBold);
            break;
        }
        case SourceVisualMapping::VisualFormatKind::Strong: format.setFontWeight(QFont::Bold); break;
        case SourceVisualMapping::VisualFormatKind::Emphasis: format.setFontItalic(true); break;
        case SourceVisualMapping::VisualFormatKind::LinkLabel:
            format.setFontUnderline(true);
            if (m_style.linkColor.isValid()) format.setForeground(m_style.linkColor);
            break;
        }
        applyGlobalFormat(blockStart, blockLength, span.visual, format);
    }
}
