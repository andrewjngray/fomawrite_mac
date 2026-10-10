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
    // The writer's caret. A misspelled word that ends exactly at the caret is
    // the one being typed: it is not drawn until the caret leaves it, so every
    // half-typed word does not flash red (the Live page applies the same rule).
    void setCaret(int position);
    // Format properties the spelling layer leaves on marked text: the state
    // (1 drawn, 2 withheld at the caret) and the block-relative end of the word.
    static constexpr int SpellMarkProperty = QTextFormat::UserProperty + 41;
    static constexpr int SpellWordEndProperty = QTextFormat::UserProperty + 42;
    // Block-relative [start, end) ranges the layer underlines in `block`.
    QList<SpellCheck::Range> misspellingsInBlock(const QTextBlock &block);
    // Spelling and grammar findings in `block`, block-relative, document order:
    // the one list the review pane lists and the surfaces draw. Grammar comes
    // from the service (cached by prose like spelling) when it is enabled.
    QList<SpellCheck::Issue> issuesInBlock(const QTextBlock &block);
    // The service, when one is attached (null with a bare checker function).
    SpellCheck *spellCheck() const { return m_spellCheck; }
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
    void applySpellingMark(QTextCharFormat &format, bool drawn) const;
    // Re-applies the caret rule to one block's layout formats directly, without
    // a rehighlight: a rehighlight opens an edit block and bumps the document
    // revision, which the link editor and the Live sync read as an edit.
    void applyCaretRule(const QTextBlock &block);
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
    int m_caret = -1;
    int m_withheldBlock = -1;
    QHash<QString, QList<SpellCheck::Range>> m_spellCache;
    QHash<QString, QList<SpellCheck::Issue>> m_grammarCache;
    SpellCheck *m_spellCheck = nullptr;
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
