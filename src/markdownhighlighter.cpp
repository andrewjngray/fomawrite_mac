#include "markdownhighlighter.h"

#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextLayout>
#include <QAbstractTextDocumentLayout>
#include <QBrush>
#include <algorithm>

namespace {
enum CodeLanguage { UnknownLanguage = 0, JavaScriptLanguage = 1, PythonLanguage = 2,
                    JsonLanguage = 3, CppLanguage = 4, ShellLanguage = 5,
                    CssLanguage = 6, HtmlLanguage = 7 };

QString normalizedLanguage(QString language) {
    language = language.trimmed().toLower();
    if (language.startsWith(QLatin1Char('.'))) language.remove(0, 1);
    const int space = language.indexOf(QRegularExpression(QStringLiteral("[\\s,{]")));
    if (space >= 0) language.truncate(space);
    if (language == QLatin1String("mjs") || language == QLatin1String("cjs")
        || language == QLatin1String("js") || language == QLatin1String("javascript")
        || language == QLatin1String("jsx") || language == QLatin1String("ts")
        || language == QLatin1String("typescript") || language == QLatin1String("tsx"))
        return QStringLiteral("js");
    if (language == QLatin1String("py") || language == QLatin1String("python")) return QStringLiteral("py");
    if (language == QLatin1String("json") || language == QLatin1String("jsonc")) return QStringLiteral("json");
    if (language == QLatin1String("c") || language == QLatin1String("h")
        || language == QLatin1String("cc") || language == QLatin1String("hh")
        || language == QLatin1String("c++") || language == QLatin1String("cpp") || language == QLatin1String("hpp")
        || language == QLatin1String("cxx") || language == QLatin1String("hxx")) return QStringLiteral("cpp");
    if (language == QLatin1String("sh") || language == QLatin1String("bash")
        || language == QLatin1String("zsh") || language == QLatin1String("shell")) return QStringLiteral("sh");
    if (language == QLatin1String("css") || language == QLatin1String("scss")
        || language == QLatin1String("less")) return QStringLiteral("css");
    if (language == QLatin1String("html") || language == QLatin1String("htm")
        || language == QLatin1String("xml") || language == QLatin1String("svg")) return QStringLiteral("html");
    return {};
}

int codeLanguageId(const QString &language) {
    if (language == QLatin1String("js")) return JavaScriptLanguage;
    if (language == QLatin1String("py")) return PythonLanguage;
    if (language == QLatin1String("json")) return JsonLanguage;
    if (language == QLatin1String("cpp")) return CppLanguage;
    if (language == QLatin1String("sh")) return ShellLanguage;
    if (language == QLatin1String("css")) return CssLanguage;
    if (language == QLatin1String("html")) return HtmlLanguage;
    return UnknownLanguage;
}

bool escapedMarker(const QString &text, int start) {
    int backslashes = 0;
    while (start > 0 && text.at(--start) == QLatin1Char('\\')) ++backslashes;
    return backslashes % 2 != 0;
}

QList<MarkdownHighlighter::Span> inlineCodeSpans(const QString &text) {
    QList<MarkdownHighlighter::Span> spans;
    for (int start = 0; start < text.size();) {
        if (text.at(start) != QLatin1Char('`') || escapedMarker(text, start)) { ++start; continue; }
        int openingEnd = start;
        while (openingEnd < text.size() && text.at(openingEnd) == QLatin1Char('`')) ++openingEnd;
        const int count = openingEnd - start;
        int after = openingEnd;
        bool matched = false;
        while (after < text.size()) {
            const int closing = text.indexOf(QLatin1Char('`'), after);
            if (closing < 0) break;
            int closingEnd = closing;
            while (closingEnd < text.size() && text.at(closingEnd) == QLatin1Char('`')) ++closingEnd;
            if (closingEnd - closing == count) {
                spans.append({start, closingEnd - start});
                start = closingEnd;
                matched = true;
                break;
            }
            after = closingEnd;
        }
        if (!matched) start = openingEnd;
    }
    return spans;
}

bool overlapsSpan(const QList<MarkdownHighlighter::Span> &spans, const MarkdownHighlighter::Span &candidate) {
    const auto first = std::lower_bound(spans.cbegin(), spans.cend(), candidate.start,
        [](const MarkdownHighlighter::Span &span, int start) { return span.start + span.length <= start; });
    return first != spans.cend() && first->start < candidate.start + candidate.length;
}
} // namespace

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document) {
    rebuildFormats();
}

void MarkdownHighlighter::setDarkMode(bool darkMode) {
    if (m_darkMode == darkMode)
        return;

    m_darkMode = darkMode;
    rebuildFormats();
    rehighlight();
}

void MarkdownHighlighter::setColors(const QString &background, const QString &foreground,
                                    const QString &accent) {
    if (m_customBackground == background && m_customForeground == foreground
            && m_customAccent == accent)
        return;

    m_customBackground = background;
    m_customForeground = foreground;
    m_customAccent = accent;
    rebuildFormats();
    rehighlight();
}

void MarkdownHighlighter::setSearch(const QString &query, int currentMatchStart) {
    if (m_searchQuery == query && m_currentMatchStart == currentMatchStart)
        return;
    m_searchQuery = query;
    m_currentMatchStart = currentMatchStart;
    rehighlight();
}

void MarkdownHighlighter::setCodeStyle(bool enabled) {
    if (m_codeStyle == enabled) return;
    m_codeStyle = enabled;
    rehighlight();
}

void MarkdownHighlighter::setCodeLanguage(const QString &language) {
    const QString normalized = normalizedLanguage(language);
    if (m_codeLanguage == normalized) return;
    m_codeLanguage = normalized;
    rehighlight();
}

void MarkdownHighlighter::setReviewSpans(const QList<Span> &spans) {
    QList<Span> normalized;
    const int documentLength = document() ? qMax(0, document()->characterCount() - 1) : 0;
    for (const Span &span : spans) {
        const int start = qBound(0, span.start, documentLength);
        const qint64 requestedEnd = qint64(span.start) + qMax(0, span.length);
        const int end = int(qBound(qint64(start), requestedEnd, qint64(documentLength)));
        if (end > start) normalized.append({start, end - start});
    }
    std::sort(normalized.begin(), normalized.end(), [](const Span &left, const Span &right) {
        return left.start == right.start ? left.length < right.length : left.start < right.start;
    });
    if (normalized.size() > 1000) normalized = normalized.mid(0, 1000);
    if (m_reviewSpans == normalized) return;
    m_reviewSpans = normalized;
    rehighlight();
}

void MarkdownHighlighter::rebuildFormats() {
    const QColor marker = m_darkMode ? QColor(QStringLiteral("#A4ABB7"))
                                     : QColor(QStringLiteral("#68707B"));
    const QColor background = !m_customBackground.isEmpty() ? QColor(m_customBackground)
        : (m_darkMode ? QColor(QStringLiteral("#101010")) : QColor(QStringLiteral("#ffffff")));
    const QColor text = !m_customForeground.isEmpty() ? QColor(m_customForeground)
        : (m_darkMode ? QColor(QStringLiteral("#eeeeee")) : QColor(QStringLiteral("#222324")));
    const QColor link = !m_customAccent.isEmpty() ? QColor(m_customAccent)
        : (m_darkMode ? QColor(QStringLiteral("#5584aa")) : QColor(QStringLiteral("#2077b2")));
    const QColor quote = marker;
    const QColor codeBackground = m_darkMode ? QColor(QStringLiteral("#252A32"))
                                             : QColor(QStringLiteral("#EAECF0"));

    m_markerFormat = QTextCharFormat();
    m_markerFormat.setForeground(marker);

    // A sub-pixel font size combined with a stretch factor used to make these
    // markers occupy (close to) zero space, but that combination deadlocks Qt's
    // font metrics engine on some platforms. Instead, use a normal font size and
    // cancel out its advance width with negative absolute letter-spacing.
    m_hiddenMarkerFormat = QTextCharFormat();
    m_hiddenMarkerFormat.setForeground(background);
    m_hiddenMarkerFormat.setFontPointSize(1.0);

    QFont hiddenFont = document() ? document()->defaultFont() : QFont();
    hiddenFont.setPointSizeF(1.0);
    const qreal charWidth = QFontMetricsF(hiddenFont).horizontalAdvance(QLatin1Char('['));

    m_hiddenMarkerFormat.setFontLetterSpacingType(QFont::AbsoluteSpacing);
    m_hiddenMarkerFormat.setFontLetterSpacing(-charWidth);

    m_headingFormat = QTextCharFormat();
    m_headingFormat.setForeground(text);
    m_headingFormat.setFontWeight(QFont::Bold);

    m_boldFormat = QTextCharFormat();
    m_boldFormat.setFontWeight(QFont::Bold);
    m_boldFormat.setForeground(text);

    m_italicFormat = QTextCharFormat();
    m_italicFormat.setFontItalic(true);
    m_italicFormat.setForeground(text);

    m_boldItalicFormat = m_boldFormat;
    m_boldItalicFormat.setFontItalic(true);

    m_codeFormat = QTextCharFormat();
    m_codeFormat.setForeground(text);
    m_codeFormat.setBackground(codeBackground);

    m_quoteFormat = QTextCharFormat();
    m_quoteFormat.setForeground(quote);
    m_quoteFormat.setFontItalic(true);

    m_linkFormat = QTextCharFormat();
    m_linkFormat.setForeground(link);
    m_linkFormat.setFontUnderline(true);

    m_reviewFormat = QTextCharFormat();
    m_reviewFormat.setBackground(m_darkMode ? QColor(QStringLiteral("#554315"))
                                            : QColor(QStringLiteral("#fff1b8")));

    m_spellBackground = m_darkMode ? QColor(QStringLiteral("#5c2b2b")) : QColor(QStringLiteral("#fbd5d1"));
    m_grammarBackground = m_darkMode ? QColor(QStringLiteral("#243a5c")) : QColor(QStringLiteral("#d9e7fb"));

    m_searchFormat = QTextCharFormat();
    m_searchFormat.setBackground(m_darkMode ? QColor(QStringLiteral("#725b18"))
                                            : QColor(QStringLiteral("#ffe58a")));
    m_currentSearchFormat = QTextCharFormat();
    m_currentSearchFormat.setBackground(m_darkMode ? QColor(QStringLiteral("#b36b20"))
                                                   : QColor(QStringLiteral("#ffad42")));

    const auto syntaxFormat = [](const QColor &color) {
        QTextCharFormat format;
        format.setForeground(color);
        return format;
    };
    m_codeKeywordFormat = syntaxFormat(m_darkMode ? QColor(QStringLiteral("#C792EA")) : QColor(QStringLiteral("#7B2CBF")));
    m_codeStringFormat = syntaxFormat(m_darkMode ? QColor(QStringLiteral("#C3E88D")) : QColor(QStringLiteral("#287A35")));
    m_codeNumberFormat = syntaxFormat(m_darkMode ? QColor(QStringLiteral("#F78C6C")) : QColor(QStringLiteral("#B54708")));
    m_codeCommentFormat = syntaxFormat(m_darkMode ? QColor(QStringLiteral("#7F9F7F")) : QColor(QStringLiteral("#64775B")));
    m_codeTypeFormat = syntaxFormat(m_darkMode ? QColor(QStringLiteral("#FFCB6B")) : QColor(QStringLiteral("#8A5A00")));
    m_codeFunctionFormat = syntaxFormat(m_darkMode ? QColor(QStringLiteral("#82AAFF")) : QColor(QStringLiteral("#2459A6")));
}

void MarkdownHighlighter::setShowMarkup(bool show) {
    if (m_showMarkup == show) return;
    m_showMarkup = show;
    rehighlight();
}

void MarkdownHighlighter::setFocusRange(int start, int end) {
    if (m_focusStart == start && m_focusEnd == end) return;
    m_focusStart = start;
    m_focusEnd = end;
    rehighlight();
}

void MarkdownHighlighter::setFocusBlock(int block) {
    m_focusBlock = block;
    const auto current = document()->findBlockByNumber(block);
    setFocusRange(block < 0 ? -1 : current.position(),
                  block < 0 ? -1 : current.position() + current.length());
}

void MarkdownHighlighter::highlightBlock(const QString &text) {
    // Fenced source is literal: never style headings or hide emphasis markers in it.
    static const QRegularExpression fenceRe(QStringLiteral("^ {0,3}(`{3,}|~{3,})(.*)$"));
    const auto fence = fenceRe.match(text);
    const bool rawCode = m_codeStyle && !m_codeLanguage.isEmpty();
    int state = rawCode ? previousBlockState() : (previousBlockState() > 0 ? previousBlockState() : 0);
    bool literal = state > 0;
    const bool encoded = m_codeStyle && state >= 3000;
    int languageId = encoded ? (state / 10) % 10 : UnknownLanguage;
    int lexicalState = rawCode ? state : (encoded ? state % 10 : 0);
    if (!rawCode && fence.hasMatch()) {
        const QString marker = fence.captured(1);
        const int kind = marker.startsWith('`') ? 1 : 2;
        if (!state) {
            if (m_codeStyle) {
                languageId = codeLanguageId(normalizedLanguage(fence.captured(2)));
                lexicalState = 0;
                state = marker.size() * 1000 + kind * 100 + languageId * 10;
            } else {
                state = marker.size() * 10 + kind;
            }
            literal = true;
        } else {
            const bool encodedState = m_codeStyle && state >= 3000;
            const int openLength = encodedState ? state / 1000 : state / 10;
            const int openKind = encodedState ? (state / 100) % 10 : state % 10;
            if (openKind == kind && marker.size() >= openLength
                && fence.captured(2).trimmed().isEmpty()) {
                state = 0;
                literal = true;
            }
            if (encodedState) {
                languageId = (previousBlockState() / 10) % 10;
                lexicalState = previousBlockState() % 10;
            }
        }
    }
    if (rawCode) {
        literal = true;
        int nextLexicalState = lexicalState;
        if (!text.isEmpty()) highlightCode(text, m_codeLanguage, lexicalState, &nextLexicalState);
        state = nextLexicalState;
    } else if (literal) {
        // Fenced backgrounds are painted across the full block width by the editor.
        if (m_codeStyle && state > 0 && !fence.hasMatch() && languageId != UnknownLanguage) {
            int nextLexicalState = lexicalState;
            highlightCode(text, languageId == JavaScriptLanguage ? QStringLiteral("js")
                : languageId == PythonLanguage ? QStringLiteral("py")
                : languageId == JsonLanguage ? QStringLiteral("json")
                : languageId == CppLanguage ? QStringLiteral("cpp")
                : languageId == ShellLanguage ? QStringLiteral("sh")
                : languageId == CssLanguage ? QStringLiteral("css") : QStringLiteral("html"),
                lexicalState, &nextLexicalState);
            state = (state / 10) * 10 + nextLexicalState;
        }
    }
    setCurrentBlockState(state);
    if (!literal && !text.isEmpty()) {
        highlightMarkers(text);
        if (text.contains(QLatin1Char('`')) || text.contains(QLatin1Char('*'))
            || text.contains(QLatin1Char('_')) || text.contains(QLatin1Char('['))) {
            highlightInline(text);
        }
    }
    if (m_focusStart >= 0) {
        for (int i = 0; i < text.size(); ++i) {
            const int offset = currentBlock().position() + i;
            if (offset >= m_focusStart && offset < m_focusEnd) continue;
            QTextCharFormat dimmed = format(i);
            if (dimmed.fontPointSize() == 1.0) continue;
            dimmed.setForeground(m_darkMode ? QColor("#777c84") : QColor("#a1a6ad"));
            setFormat(i, 1, dimmed);
        }
    }
    if (!literal) highlightReviewSpans(text);
    if (!literal && !text.isEmpty()) highlightSpelling(text);
    highlightSearch(text);
}

void MarkdownHighlighter::setSpellChecker(const SpellChecker &checker) {
    m_spellChecker = checker;
    refreshSpelling();
}

void MarkdownHighlighter::setSpellingEnabled(bool enabled) {
    if (m_spellingEnabled == enabled) return;
    m_spellingEnabled = enabled;
    rehighlight();
}

void MarkdownHighlighter::setSpellCheck(SpellCheck *spellCheck) {
    m_spellCheck = spellCheck;
    if (!spellCheck) {
        m_spellingEnabled = false;
        setSpellChecker(nullptr);
        return;
    }
    // A guarded pointer keeps a late repaint from reaching a destroyed checker.
    const QPointer<SpellCheck> guarded(spellCheck);
    m_spellingEnabled = spellCheck->enabled();
    m_spellChecker = [guarded](const QString &prose) -> QList<SpellCheck::Range> {
        return guarded ? guarded->misspellings(prose) : QList<SpellCheck::Range>();
    };
    connect(spellCheck, &SpellCheck::enabledChanged, this, [this, guarded] {
        if (guarded) m_spellingEnabled = guarded->enabled();
        refreshSpelling();
    });
    connect(spellCheck, &SpellCheck::languageChanged, this, &MarkdownHighlighter::refreshSpelling);
    connect(spellCheck, &SpellCheck::grammarEnabledChanged, this, &MarkdownHighlighter::refreshSpelling);
    connect(spellCheck, &SpellCheck::wordsChanged, this, &MarkdownHighlighter::refreshSpelling);
    refreshSpelling();
}

void MarkdownHighlighter::setCaret(int position) {
    if (m_caret == position) return;
    m_caret = position;
    if (!m_spellingEnabled || !document()) return;
    const QTextBlock current = document()->findBlock(position);
    const int withheld = m_withheldBlock; m_withheldBlock = -1;
    if (withheld >= 0 && (!current.isValid() || withheld != current.blockNumber()))
        applyCaretRule(document()->findBlockByNumber(withheld));
    if (current.isValid()) applyCaretRule(current);
}

void MarkdownHighlighter::applySpellingMark(QTextCharFormat &format, bool drawn, int category) const {
    const QColor wash = category == 2 ? m_grammarBackground : m_spellBackground;
    if (drawn) {
        // Measured in the rendered window (Qt 6.11, tests/cycle143-source-
        // spelling.inc): Qt Quick never draws SpellCheckUnderline (it tests
        // fontUnderline), and a plain underline takes the text colour unless
        // the run also has a background, when the underline colour is used.
        // So the mark is a single underline (red for spelling, blue for
        // grammar) plus a pale wash of the same hue.
        format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        format.setUnderlineColor(QColor(category == 2 ? QStringLiteral("#1a73e8") : QStringLiteral("#d93025")));
        if (!format.hasProperty(QTextFormat::BackgroundBrush) || format.background().style() == Qt::NoBrush)
            format.setBackground(wash);
        format.setProperty(SpellMarkProperty, 1);
    } else {
        format.setUnderlineStyle(QTextCharFormat::NoUnderline);
        if (format.background() == QBrush(wash)) format.clearBackground();
        format.setProperty(SpellMarkProperty, 2);
    }
    format.setProperty(SpellCategoryProperty, category);
}

void MarkdownHighlighter::commitLayoutFormats(const QTextBlock &block, const QList<QTextLayout::FormatRange> &ranges) {
    block.layout()->setFormats(ranges);
    document()->markContentsDirty(block.position(), block.length());
    // Views repaint a block on the layout's updateBlock signal (the Qt Quick
    // text item connects it to its own invalidateBlock). A format change
    // outside an edit block has to announce itself.
    if (auto *layout = document()->documentLayout()) emit layout->updateBlock(block);
}

void MarkdownHighlighter::applyCaretRule(const QTextBlock &block) {
    if (!block.isValid() || !block.layout()) return;
    QList<QTextLayout::FormatRange> ranges = block.layout()->formats();
    bool changed = false;
    for (QTextLayout::FormatRange &range : ranges) {
        const int state = range.format.intProperty(SpellMarkProperty);
        if (!state) continue;
        const bool atCaret = m_caret >= 0 && block.position() + range.format.intProperty(SpellWordEndProperty) == m_caret;
        if (atCaret == (state == 2)) continue;
        const int category = range.format.intProperty(SpellCategoryProperty) == 2 ? 2 : 1;
        applySpellingMark(range.format, !atCaret, category);
        if (atCaret) m_withheldBlock = block.blockNumber();
        changed = true;
    }
    if (changed) commitLayoutFormats(block, ranges);
}

QString MarkdownHighlighter::issueKey(const QTextBlock &block, int start, int end) const {
    return QStringLiteral("%1:%2:%3:%4").arg(block.blockNumber()).arg(start).arg(end)
        .arg(block.text().mid(start, end - start));
}

void MarkdownHighlighter::clearIgnoredIssues() {
    if (m_ignoredIssues.isEmpty()) return;
    m_ignoredIssues.clear();
    rehighlight();
}

// A user action, but still the direct layout path: the marks of the ignored
// finding are stripped from the block's formats in place (nothing is
// rehighlighted, so the document revision does not move and the link editor or
// the Live sync never reads the restyle as an edit).
void MarkdownHighlighter::ignoreIssue(int blockNumber, int start, int end) {
    if (!document()) return;
    const QTextBlock block = document()->findBlockByNumber(blockNumber);
    if (!block.isValid() || start < 0 || end > block.text().size() || end <= start) return;
    m_ignoredIssues.insert(issueKey(block, start, end));
    if (!block.layout()) return;
    QList<QTextLayout::FormatRange> ranges = block.layout()->formats();
    bool changed = false;
    for (QTextLayout::FormatRange &range : ranges) {
        if (range.format.intProperty(SpellCategoryProperty) != 2
            || range.format.intProperty(SpellWordEndProperty) != end
            || range.start + range.length <= start || range.start >= end) continue;
        range.format.setUnderlineStyle(QTextCharFormat::NoUnderline);
        if (range.format.background() == QBrush(m_grammarBackground)) range.format.clearBackground();
        range.format.clearProperty(SpellMarkProperty);
        range.format.clearProperty(SpellWordEndProperty);
        range.format.clearProperty(SpellCategoryProperty);
        changed = true;
    }
    if (m_withheldBlock == blockNumber) m_withheldBlock = -1;
    if (changed) commitLayoutFormats(block, ranges);
}

void MarkdownHighlighter::refreshSpelling() {
    m_spellCache.clear();
    m_grammarCache.clear();
    rehighlight();
}

// Blank what is syntax rather than prose, keeping every UTF-16 offset.
QString MarkdownHighlighter::spellingProse(const QString &blockText) {
    QString prose = blockText;
    const auto blank = [&prose](int start, int length) {
        for (int i = start; i < start + length && i < prose.size(); ++i) prose[i] = QLatin1Char(' ');
    };
    if (prose.contains(QLatin1Char('`')))
        for (const auto &code : inlineCodeSpans(prose)) blank(code.start, code.length);
    // Link destinations, raw tags, raw URLs and inline math are syntax.
    static const QRegularExpression syntax(QStringLiteral(
        "\\]\\([^\\n]*?\\)|<[^>\\n]*>|(?:https?|ftp|file)://[^\\s)>\\]]+|www\\.[^\\s)>\\]]+"
        "|(?<![\\\\$\\w])\\$(?=\\S)[^$\\n]*?\\S\\$(?![\\d\\w])"));
    auto matches = syntax.globalMatch(prose);
    while (matches.hasNext()) {
        const auto match = matches.next();
        blank(int(match.capturedStart()), int(match.capturedLength()));
    }
    return prose;
}

int MarkdownHighlighter::frontMatterEndBlock() const {
    const QTextDocument *doc = document();
    if (!doc || doc->firstBlock().text().trimmed() != QLatin1String("---")) return -1;
    int number = 0;
    for (QTextBlock block = doc->firstBlock().next(); block.isValid() && number < 500;
         block = block.next(), ++number) {
        const QString trimmed = block.text().trimmed();
        if (trimmed == QLatin1String("---") || trimmed == QLatin1String("...")) return number + 1;
    }
    return -1;
}

// The misspelled words in one block, as block-relative [start, end) ranges. The
// highlighter underlines exactly these, and the right-click menu offers exactly
// these, so what is drawn and what can be corrected never disagree.
QList<SpellCheck::Range> MarkdownHighlighter::misspellingsInBlock(const QTextBlock &block) {
    QList<SpellCheck::Range> result;
    if (!m_spellingEnabled || !m_spellChecker || !block.isValid()) return result;
    const QString text = block.text();
    if (text.isEmpty()) return result;
    // Fenced and raw code are literal (the highlighter's block state says so).
    if (m_codeStyle && !m_codeLanguage.isEmpty()) return result;
    const int previousState = block.previous().isValid() ? block.previous().userState() : 0;
    static const QRegularExpression fenceRe(QStringLiteral("^ {0,3}(`{3,}|~{3,})(.*)$"));
    if (previousState > 0 || fenceRe.match(text).hasMatch()) return result;
    if (block.blockNumber() <= 500 && block.blockNumber() <= frontMatterEndBlock()) return result;
    // An indented run that follows a blank line is a code block, not prose; a
    // nested list item or continuation line follows text and is checked.
    static const QRegularExpression indentedRe(QStringLiteral("^(?: {4,}|\\t)"));
    static const QRegularExpression listItemRe(QStringLiteral("^\\s*(?:[-+*]|\\d+[.)])\\s"));
    if (indentedRe.match(text).hasMatch() && !listItemRe.match(text).hasMatch()) {
        const QTextBlock previous = block.previous();
        if (!previous.isValid() || previous.text().trimmed().isEmpty()
            || indentedRe.match(previous.text()).hasMatch()) return result;
    }
    const QString prose = spellingProse(text);
    if (prose.trimmed().isEmpty()) return result;
    auto cached = m_spellCache.constFind(prose);
    if (cached == m_spellCache.constEnd()) {
        // One entry per distinct block text. 4096 was a cliff: a long document
        // above it re-checked everything on every focus-mode caret move.
        if (m_spellCache.size() > 65536) m_spellCache.clear();
        cached = m_spellCache.insert(prose, m_spellChecker(prose));
    }
    static const QRegularExpression glueRe(QStringLiteral("[\\p{L}\\p{N}][_@/\\\\][\\p{L}\\p{N}]"));
    for (const SpellCheck::Range &range : *cached) {
        if (range.start < 0 || range.end > text.size() || range.end <= range.start) continue;
        // snake_case, paths, e-mail addresses and the like are tokens, not words.
        int tokenStart = range.start, tokenEnd = range.end;
        // Measured on the blanked prose, not the raw text: a link destination
        // has already been blanked, so the last word of link text is a word.
        while (tokenStart > 0 && !prose.at(tokenStart - 1).isSpace()) --tokenStart;
        while (tokenEnd < prose.size() && !prose.at(tokenEnd).isSpace()) ++tokenEnd;
        if (glueRe.match(prose.mid(tokenStart, tokenEnd - tokenStart)).hasMatch()) continue;
        result.append(range);
    }
    return result;
}

QList<SpellCheck::Issue> MarkdownHighlighter::grammarIssuesInBlock(const QTextBlock &block) {
    QList<SpellCheck::Issue> issues;
    const bool grammarOn = m_grammarChecker || (m_spellCheck && m_spellCheck->grammarEnabled());
    if (!grammarOn || !m_spellingEnabled || !block.isValid()) return issues;
    const QString text = block.text();
    // Grammar runs on the same prose as spelling, under the same prose
    // rules (code, fences, front matter and syntax are never grammar).
    const QString prose = spellingProse(text);
    if (prose.trimmed().isEmpty()) return issues;
    // The same literal/code/front-matter gate misspellingsInBlock applies.
    if (m_codeStyle && !m_codeLanguage.isEmpty()) return issues;
    const int previousState = block.previous().isValid() ? block.previous().userState() : 0;
    static const QRegularExpression fenceRe(QStringLiteral("^ {0,3}(`{3,}|~{3,})(.*)$"));
    if (previousState > 0 || fenceRe.match(text).hasMatch()) return issues;
    if (block.blockNumber() <= 500 && block.blockNumber() <= frontMatterEndBlock()) return issues;
    // An indented run after a blank line is code, as in misspellingsInBlock.
    static const QRegularExpression indentedRe(QStringLiteral("^(?: {4,}|\\t)"));
    static const QRegularExpression listItemRe(QStringLiteral("^\\s*(?:[-+*]|\\d+[.)])\\s"));
    if (indentedRe.match(text).hasMatch() && !listItemRe.match(text).hasMatch()) {
        const QTextBlock previous = block.previous();
        if (!previous.isValid() || previous.text().trimmed().isEmpty()
            || indentedRe.match(previous.text()).hasMatch()) return issues;
    }
    auto cached = m_grammarCache.constFind(prose);
    if (cached == m_grammarCache.constEnd()) {
        if (m_grammarCache.size() > 65536) m_grammarCache.clear();
        cached = m_grammarCache.insert(prose, m_grammarChecker ? m_grammarChecker(prose) : m_spellCheck->grammarIssues(prose));
    }
    for (const SpellCheck::Issue &issue : *cached) {
        if (issue.start < 0 || issue.end > text.size() || issue.end <= issue.start) continue;
        if (!m_ignoredIssues.isEmpty() && m_ignoredIssues.contains(issueKey(block, issue.start, issue.end))) continue;
        issues.append(issue);
    }
    return issues;
}

QList<SpellCheck::Issue> MarkdownHighlighter::issuesInBlock(const QTextBlock &block) {
    QList<SpellCheck::Issue> issues;
    const QString text = block.text();
    for (const SpellCheck::Range &range : misspellingsInBlock(block)) {
        const QString word = text.mid(range.start, range.end - range.start);
        issues.append({range.start, range.end, QStringLiteral("Spelling"), QString(),
                       m_spellCheck ? m_spellCheck->suggestions(word) : QStringList()});
    }
    issues.append(grammarIssuesInBlock(block));
    std::stable_sort(issues.begin(), issues.end(), [](const SpellCheck::Issue &a, const SpellCheck::Issue &b) {
        return a.start < b.start || (a.start == b.start && a.end < b.end); });
    return issues;
}

void MarkdownHighlighter::highlightSpelling(const QString &text) {
    const QTextBlock block = currentBlock();
    const int blockStart = block.position(), blockEnd = blockStart + block.length();
    // In focus mode the dimmed text outside the focus is left alone: a pink
    // wash under dimmed grey text reads worse than the dimming itself.
    if (m_focusStart >= 0 && (blockEnd <= m_focusStart || blockStart >= m_focusEnd)) return;
    const QList<SpellCheck::Range> spelling = misspellingsInBlock(block);
    // Grammar first, minus the characters spelling claims: where the two
    // overlap, spelling wins for that character.
    QVector<bool> spelled(text.size(), false);
    for (const SpellCheck::Range &range : spelling)
        for (int i = range.start; i < range.end && i < text.size(); ++i) spelled[i] = true;
    for (const SpellCheck::Issue &issue : grammarIssuesInBlock(block)) {
        const bool atCaret = m_caret >= 0 && blockStart + issue.end == m_caret;
        if (atCaret) m_withheldBlock = block.blockNumber();
        for (int i = issue.start; i < issue.end; ++i) {
            if (spelled.at(i)) continue;
            // Partly dimmed focus text carries no wash either.
            const int offset = blockStart + i;
            if (m_focusStart >= 0 && (offset < m_focusStart || offset >= m_focusEnd)) continue;
            QTextCharFormat composed = format(i);
            if (composed.fontPointSize() == 1.0) continue; // a hidden marker, not a letter
            composed.setProperty(SpellWordEndProperty, issue.end);
            applySpellingMark(composed, !atCaret, 2);
            setFormat(i, 1, composed);
        }
    }
    for (const SpellCheck::Range &range : spelling) {
        const bool atCaret = m_caret >= 0 && blockStart + range.end == m_caret;
        if (atCaret) m_withheldBlock = block.blockNumber();
        for (int i = range.start; i < range.end; ++i) {
            QTextCharFormat composed = format(i);
            if (composed.fontPointSize() == 1.0) continue; // a hidden marker, not a letter
            composed.setProperty(SpellWordEndProperty, range.end);
            applySpellingMark(composed, !atCaret, 1);
            setFormat(i, 1, composed);
        }
    }
}

void MarkdownHighlighter::highlightCode(const QString &text, const QString &language,
                                       int lexicalState, int *nextLexicalState, int offset) {
    const int id = codeLanguageId(language);
    if (nextLexicalState) *nextLexicalState = 0;
    if (id == UnknownLanguage || text.isEmpty()) return;

    int scanStart = 0;
    if (lexicalState == 1 || lexicalState == 2) {
        int close = -1;
        if (lexicalState == 1) {
            close = text.indexOf(QStringLiteral("*/"));
            if (close >= 0) close += 2;
        } else {
            for (int i = 0; i < text.size(); ++i) {
                if (text.at(i) != QLatin1Char('`')) continue;
                int slashes = 0;
                for (int j = i - 1; j >= 0 && text.at(j) == QLatin1Char('\\'); --j) ++slashes;
                if (slashes % 2 == 0) { close = i + 1; break; }
            }
        }
        if (close < 0) {
            setFormat(offset, text.size(), lexicalState == 1 ? m_codeCommentFormat : m_codeStringFormat);
            if (nextLexicalState) *nextLexicalState = lexicalState;
            return;
        }
        setFormat(offset, close, lexicalState == 1 ? m_codeCommentFormat : m_codeStringFormat);
        scanStart = close;
    }

    const bool hashComments = id == PythonLanguage || id == ShellLanguage;
    const QRegularExpression tokenRe(QStringLiteral("//[^\\n]*|/\\*.*?(?:\\*/|$)|")
        + (hashComments ? QStringLiteral("#[^\\n]*|") : QString())
        + QStringLiteral(
            "(?:u8|u|U|L)?(?:\"(?:\\\\.|[^\"\\\\])*\"|'(?:\\\\.|[^'\\\\])*'|`(?:\\\\.|[^`\\\\])*`|`[^`]*$)|"
            "(?:0[xX][\\da-fA-F]+|\\d+(?:\\.\\d+)?(?:[eE][+-]?\\d+)?[fFuUlL]*)|"
            "[\\p{L}_$][\\p{L}\\p{N}_$]*"));
    static const QRegularExpression htmlNameRe(QStringLiteral(
        "</?([A-Za-z][\\w:-]*)|([A-Za-z_:][\\w:.-]*)(?=\\s*=)"));

    QStringList keywords;
    QStringList types;
    switch (id) {
    case JavaScriptLanguage:
        keywords = {"async","await","break","case","catch","class","const","continue","debugger","default","delete","do","else","export","extends","false","finally","for","from","function","if","import","in","instanceof","let","new","null","of","return","static","super","switch","this","throw","true","try","typeof","undefined","var","void","while","yield","interface","type","implements","enum","readonly","public","private","protected"};
        types = {"Array","BigInt","Boolean","Date","Error","Function","Map","Number","Object","Promise","RegExp","Set","String","Symbol","WeakMap","WeakSet"};
        break;
    case PythonLanguage:
        keywords = {"and","as","assert","async","await","break","class","continue","def","del","elif","else","except","False","finally","for","from","global","if","import","in","is","lambda","None","nonlocal","not","or","pass","raise","return","True","try","while","with","yield"};
        types = {"bool","bytes","dict","float","int","list","object","set","str","tuple","type"};
        break;
    case JsonLanguage:
        keywords = {"false","null","true"};
        break;
    case CppLanguage:
        keywords = {"alignas","auto","bool","break","case","catch","char","class","const","constexpr","continue","default","delete","do","double","else","enum","explicit","extern","false","float","for","friend","if","inline","int","long","namespace","new","noexcept","nullptr","operator","private","protected","public","return","short","signed","sizeof","static","struct","switch","template","this","throw","true","try","typedef","typename","union","unsigned","using","virtual","void","volatile","while","include","define","ifdef","ifndef","endif"};
        types = {"char","double","float","int","long","short","size_t","string","uint8_t","uint32_t","uint64_t","void"};
        break;
    case ShellLanguage:
        keywords = {"case","do","done","elif","else","esac","fi","for","function","if","in","select","then","time","until","while","export","local","readonly","return","set","shift","source","unset"};
        break;
    case CssLanguage:
        keywords = {"important","inherit","initial","none","unset"};
        break;
    case HtmlLanguage:
        break;
    }

    auto matches = tokenRe.globalMatch(text.mid(scanStart));
    while (matches.hasNext()) {
        const auto match = matches.next();
        const QString token = match.captured();
        const int start = offset + scanStart + match.capturedStart();
        const int length = match.capturedLength();
        QTextCharFormat format;
        bool apply = true;
        if (token.startsWith(QStringLiteral("//")) || token.startsWith(QStringLiteral("/*"))
            || (hashComments && token.startsWith(QLatin1Char('#')))) {
            format = m_codeCommentFormat;
            if (token.startsWith(QStringLiteral("/*")) && !token.endsWith(QStringLiteral("*/"))
                && nextLexicalState) *nextLexicalState = 1;
        } else if (token.startsWith(QLatin1Char('\"')) || token.startsWith(QLatin1Char('\''))
                   || token.startsWith(QLatin1Char('`')) || token.startsWith(QStringLiteral("u8\""))
                   || token.startsWith(QStringLiteral("L\"")) || token.startsWith(QStringLiteral("u\""))
                   || token.startsWith(QStringLiteral("U\""))) {
            format = m_codeStringFormat;
            if (token.startsWith(QLatin1Char('`')) && !token.endsWith(QLatin1Char('`'))
                && nextLexicalState) *nextLexicalState = 2;
        } else if (token.at(0).isDigit()) {
            format = m_codeNumberFormat;
        } else if (id == HtmlLanguage) {
            apply = false;
        } else if (keywords.contains(token)) {
            format = m_codeKeywordFormat;
        } else if (types.contains(token) || (id == CppLanguage && token.size() > 1 && token.at(0).isUpper())) {
            format = m_codeTypeFormat;
        } else if (id == JsonLanguage) {
            const int next = text.indexOf(QRegularExpression(QStringLiteral("\\S")),
                                          scanStart + match.capturedEnd());
            if (next >= 0 && text.at(next) == QLatin1Char(':')) format = m_codeTypeFormat;
            else apply = false;
        } else {
            int next = scanStart + match.capturedEnd();
            while (next < text.size() && text.at(next).isSpace()) ++next;
            if (next < text.size() && text.at(next) == QLatin1Char('(')) format = m_codeFunctionFormat;
            else apply = false;
        }
        if (apply) setFormat(start, length, format);
        if (nextLexicalState && *nextLexicalState != 0) break;
    }
    if (id == HtmlLanguage) {
        auto names = htmlNameRe.globalMatch(text);
        while (names.hasNext()) {
            const auto match = names.next();
            setFormat(offset + match.capturedStart(), match.capturedLength(),
                      match.capturedStart(1) >= 0 ? m_codeTypeFormat : m_codeFunctionFormat);
        }
    }
}

void MarkdownHighlighter::highlightReviewSpans(const QString &text) {
    if (m_reviewSpans.isEmpty() || text.isEmpty()) return;
    const int blockStart = currentBlock().position();
    const int blockEnd = blockStart + text.size();
    for (const Span &span : m_reviewSpans) {
        const int spanEnd = span.start + span.length;
        if (spanEnd <= blockStart) continue;
        if (span.start >= blockEnd) break;
        const int first = qMax(span.start, blockStart) - blockStart;
        const int last = qMin(spanEnd, blockEnd) - blockStart;
        for (int i = first; i < last; ++i) {
            QTextCharFormat composed = format(i);
            // Backend spans exclude code and markers. These guards keep the
            // setter conservative if a caller supplies an overlapping span.
            if (composed.fontPointSize() == 1.0
                || composed.background() == m_codeFormat.background()) continue;
            composed.setBackground(m_reviewFormat.background());
            setFormat(i, 1, composed);
        }
    }
}

void MarkdownHighlighter::highlightSearch(const QString &text) {
    if (m_searchQuery.isEmpty())
        return;

    int from = 0;
    while ((from = text.indexOf(m_searchQuery, from, Qt::CaseInsensitive)) >= 0) {
        const int documentStart = currentBlock().position() + from;
        QTextCharFormat format = this->format(from);
        format.setBackground(documentStart == m_currentMatchStart
                                 ? m_currentSearchFormat.background()
                                 : m_searchFormat.background());
        setFormat(from, m_searchQuery.length(), format);
        from += qMax(1, m_searchQuery.length());
    }
}

void MarkdownHighlighter::highlightMarkers(const QString &text) {
    int first = 0;
    while (first < text.length() && text.at(first).isSpace())
        ++first;
    if (first >= text.length())
        return;

    const QChar firstChar = text.at(first);
    if (first <= 3 && firstChar == QLatin1Char('#')) {
        static const QRegularExpression headingRe(QStringLiteral("^ {0,3}(#{1,6})([ \\t]+)(.*)$"));
        const QRegularExpressionMatch heading = headingRe.match(text);
        if (heading.hasMatch()) {
            setFormat(0, heading.capturedStart(3),
                      m_markerFormat);
            setFormat(heading.capturedStart(3), heading.capturedLength(3),
                      m_headingFormat);
            return;
        }
    }

    if (firstChar == QLatin1Char('>')) {
        static const QRegularExpression quoteRe(QStringLiteral("^(\\s*>+\\s?)(.*)$"));
        const QRegularExpressionMatch quote = quoteRe.match(text);
        if (quote.hasMatch()) {
            setFormat(0, quote.capturedLength(1), m_markerFormat);
            setFormat(quote.capturedStart(2), quote.capturedLength(2), m_quoteFormat);
        }
    }

    if (firstChar == QLatin1Char('-') || firstChar == QLatin1Char('+')
            || firstChar == QLatin1Char('*') || firstChar.isDigit()) {
        static const QRegularExpression listRe(
            QStringLiteral("^(\\s*(?:[-+*]|\\d+[.)])\\s+)(.*)$"));
        const QRegularExpressionMatch list = listRe.match(text);
        if (list.hasMatch())
            setFormat(0, list.capturedLength(1), m_markerFormat);
    }

    if (firstChar == QLatin1Char('-') || firstChar == QLatin1Char('*')
            || firstChar == QLatin1Char('_')) {
        static const QRegularExpression ruleRe(QStringLiteral("^\\s{0,3}([-*_])(?:\\s*\\1){2,}\\s*$"));
        const QRegularExpressionMatch rule = ruleRe.match(text);
        if (rule.hasMatch())
            setFormat(0, text.length(), m_markerFormat);
    }
}

void MarkdownHighlighter::highlightInline(const QString &text) {
    for (const Span &code : inlineCodeSpans(text))
        setFormat(code.start, code.length, m_codeFormat);

    const QList<InlineMarkup> markup = inlineMarkup(text);
    for (const InlineMarkup &item : markup) {
        const QTextCharFormat &contentFormat =
            item.kind == InlineKind::Bold ? m_boldFormat
            : item.kind == InlineKind::Italic ? m_italicFormat
            : item.kind == InlineKind::BoldItalic ? m_boldItalicFormat
                                                 : m_linkFormat;
        setFormat(item.content.start, item.content.length, contentFormat);
        for (const Span &marker : item.markers)
            setFormat(marker.start, marker.length, m_showMarkup ? m_markerFormat : m_hiddenMarkerFormat);
    }
}

QList<MarkdownHighlighter::InlineMarkup> MarkdownHighlighter::inlineMarkup(const QString &text) {
    QList<InlineMarkup> markup;
    if (!text.contains(QLatin1Char('*')) && !text.contains(QLatin1Char('_'))
            && !text.contains(QLatin1Char('['))) {
        return markup;
    }

    const auto span = [](const QRegularExpressionMatch &match, int group) {
        return Span{int(match.capturedStart(group)), int(match.capturedLength(group))};
    };

    const auto literal = inlineCodeSpans(text);
    QList<Span> combined;
    static const QRegularExpression combinedRe(QStringLiteral(
        "(?<!\\*)(\\*{3})(\\S(?:.*?\\S)?)(\\*{3})(?!\\*)"
        "|(?<![\\p{L}\\p{N}_])(_{3})(\\S(?:.*?\\S)?)(_{3})(?![\\p{L}\\p{N}_])"));
    auto combinedMatches = combinedRe.globalMatch(text);
    while (combinedMatches.hasNext()) {
        const auto match = combinedMatches.next();
        const Span whole = span(match, 0);
        const int marker = match.capturedStart(1) >= 0 ? 1 : 4;
        if (overlapsSpan(literal, whole) || escapedMarker(text, whole.start)
            || escapedMarker(text, match.capturedStart(marker + 2))) continue;
        combined.append(whole);
        markup.append({InlineKind::BoldItalic, span(match, marker + 1),
                       {span(match, marker), span(match, marker + 2)}});
    }

    static const QRegularExpression boldRe(QStringLiteral("(\\*\\*|__)(.+?)(\\1)"));
    QRegularExpressionMatchIterator boldMatches = boldRe.globalMatch(text);
    while (boldMatches.hasNext()) {
        const QRegularExpressionMatch match = boldMatches.next();
        const Span whole = span(match, 0);
        if (overlapsSpan(literal, whole) || overlapsSpan(combined, whole)
            || escapedMarker(text, whole.start) || escapedMarker(text, match.capturedStart(3))) continue;
        markup.append({InlineKind::Bold, span(match, 2),
                       {span(match, 1), span(match, 3)}});
    }

    static const QRegularExpression italicRe(
        QStringLiteral("(?<!\\*)\\*([^*\\n]+)\\*(?!\\*)|(?<!_)_([^_\\n]+)_(?!_)"));
    QRegularExpressionMatchIterator italicMatches = italicRe.globalMatch(text);
    while (italicMatches.hasNext()) {
        const QRegularExpressionMatch match = italicMatches.next();
        const Span whole = span(match, 0);
        if (overlapsSpan(literal, whole) || overlapsSpan(combined, whole)
            || escapedMarker(text, whole.start) || escapedMarker(text, whole.start + whole.length - 1)) continue;
        const int contentIndex = match.capturedStart(1) >= 0 ? 1 : 2;
        markup.append({InlineKind::Italic, span(match, contentIndex),
                       {{whole.start, 1}, {whole.start + whole.length - 1, 1}}});
    }

    static const QRegularExpression linkRe(
        QStringLiteral("\\[([^\\]]+)\\]\\(((?:\\\\.|[^)])+)\\)"));
    QRegularExpressionMatchIterator linkMatches = linkRe.globalMatch(text);
    while (linkMatches.hasNext()) {
        const QRegularExpressionMatch match = linkMatches.next();
        const Span whole = span(match, 0);
        if (overlapsSpan(literal, whole) || escapedMarker(text, whole.start)) continue;
        const Span content = span(match, 1);
        const int contentEnd = content.start + content.length;
        markup.append({InlineKind::Link, content,
                       {{whole.start, 1},
                        {contentEnd, whole.start + whole.length - contentEnd}}});
    }

    return markup;
}
