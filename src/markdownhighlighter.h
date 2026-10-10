#pragma once

#include <QHash>
#include <QPointer>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextBlock>
#include <QTextCharFormat>
#include <functional>

#include "spellcheck.h"

class MarkdownHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit MarkdownHighlighter(QTextDocument *document);

    void setDarkMode(bool darkMode);
    void setShowMarkup(bool show);
    void setFocusBlock(int block);
    void setFocusRange(int start, int end);
    void setColors(const QString &background, const QString &foreground, const QString &accent);
    void setSearch(const QString &query, int currentMatchStart);
    void setCodeStyle(bool enabled);
    void setCodeLanguage(const QString &language);

    struct Span {
        int start;
        int length;
        bool operator==(const Span &other) const {
            return start == other.start && length == other.length;
        }
    };

    void setReviewSpans(const QList<Span> &spans);

    // Spelling layer. The checker receives the prose of one block (code, link
    // destinations, URLs and tags blanked to spaces so UTF-16 offsets still line
    // up) and returns [start, end) ranges of misspelled words; those get a red
    // spell-check underline composed onto the syntax format. Results are cached
    // by prose text so typing in one block never re-checks the others.
    using SpellChecker = std::function<QList<SpellCheck::Range>(const QString &)>;
    void setSpellChecker(const SpellChecker &checker);
    void setSpellingEnabled(bool enabled);
    // Follows the shared service: its setting, language and word lists.
    void setSpellCheck(SpellCheck *spellCheck);
    // Forget cached results and restyle every block (words or language changed).
    void refreshSpelling();
    // Block-relative [start, end) ranges the layer underlines in `block`.
    QList<SpellCheck::Range> misspellingsInBlock(const QTextBlock &block);
    // The prose of one block as the checker sees it (public for tests).
    static QString spellingProse(const QString &blockText);

    enum class InlineKind { Bold, Italic, BoldItalic, Link };

    struct InlineMarkup {
        InlineKind kind;
        Span content;
        Span markers[2];
    };

    // Single source of truth for inline markdown spans: the highlighter uses it
    // to style content and hide markers, and the editor uses it (via
    // Backend::hiddenRangesAt) to skip the caret over the hidden markers.
    static QList<InlineMarkup> inlineMarkup(const QString &text);

protected:
    void highlightBlock(const QString &text) override;

private:
    void rebuildFormats();
    void highlightMarkers(const QString &text);
    void highlightInline(const QString &text);
    void highlightReviewSpans(const QString &text);
    void highlightSearch(const QString &text);
    void highlightSpelling(const QString &text);
    int frontMatterEndBlock() const;
    void highlightCode(const QString &text, const QString &language, int lexicalState = 0,
                       int *nextLexicalState = nullptr, int offset = 0);

    int m_focusBlock = -1;
    int m_focusStart = -1;
    int m_focusEnd = -1;
    bool m_showMarkup = false;
    bool m_darkMode = true;
    bool m_codeStyle = false;
    QString m_codeLanguage;
    QString m_customBackground;
    QString m_customForeground;
    QString m_customAccent;
    QTextCharFormat m_markerFormat;
    QTextCharFormat m_hiddenMarkerFormat;
    QTextCharFormat m_headingFormat;
    QTextCharFormat m_boldFormat;
    QTextCharFormat m_italicFormat;
    QTextCharFormat m_boldItalicFormat;
    QTextCharFormat m_codeFormat;
    QTextCharFormat m_quoteFormat;
    QTextCharFormat m_linkFormat;
    QList<Span> m_reviewSpans;
    SpellChecker m_spellChecker;
    QColor m_spellBackground;
    bool m_spellingEnabled = false;
    QHash<QString, QList<SpellCheck::Range>> m_spellCache;
    QTextCharFormat m_reviewFormat;
    QString m_searchQuery;
    int m_currentMatchStart = -1;
    QTextCharFormat m_searchFormat;
    QTextCharFormat m_currentSearchFormat;
    QTextCharFormat m_codeKeywordFormat;
    QTextCharFormat m_codeStringFormat;
    QTextCharFormat m_codeNumberFormat;
    QTextCharFormat m_codeCommentFormat;
    QTextCharFormat m_codeTypeFormat;
    QTextCharFormat m_codeFunctionFormat;
};
