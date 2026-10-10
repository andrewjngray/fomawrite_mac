#pragma once

// The one spell checker both writing surfaces share.
//
// Source (the Qt Quick text item) and Live (the CodeMirror page) each decide
// which stretches of a document are prose; this object decides which words in
// a stretch are misspelled, offers suggestions, and remembers learned and
// ignored words. On macOS it is the system checker, so the dictionary, the
// language list and the learned words are the ones every other Mac app uses.
// Elsewhere it is unavailable and reports nothing.
//
// Offsets are UTF-16 code units into the string given, the same units QString
// and the page use, so a range can be mapped back without conversion.

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>

class SpellCheck : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool grammarEnabled READ grammarEnabled WRITE setGrammarEnabled NOTIFY grammarEnabledChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

public:
    explicit SpellCheck(QObject *parent = nullptr);

    struct Range { int start; int end; };
    // One finding in a stretch of prose. Category is "Spelling" (a word the
    // dictionary does not know) or "Grammar" (a sentence-level finding with the
    // checker's own message). Suggestions are replacements for [start, end).
    struct Issue {
        int start; int end; QString category; QString message; QStringList suggestions;
        bool operator==(const Issue &o) const { return start == o.start && end == o.end && category == o.category; }
    };

    bool available() const;
    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);
    // Grammar as you type (macOS grammar pass: doubled words, capitalisation,
    // agreement). Separate setting; off means issues() returns spelling only.
    bool grammarEnabled() const { return m_grammarEnabled; }
    void setGrammarEnabled(bool enabled);
    // Empty means the system language; otherwise a tag from languages().
    QString language() const { return m_language; }
    void setLanguage(const QString &language);
    Q_INVOKABLE QStringList languages() const;

    // Misspelled words in `text`, in order, as [start, end) ranges. Empty when
    // disabled or unavailable. Text is checked as given: callers blank or omit
    // code, URLs and markup first.
    QList<Range> misspellings(const QString &text) const;
    Q_INVOKABLE QVariantList misspelledRanges(const QString &text) const;
    // Grammar findings in `text`, sentence by sentence, as [start, end) with the
    // checker's message and corrections. Empty when grammar is off.
    QList<Issue> grammarIssues(const QString &text) const;
    // [start, end) chunks of `text`, split at sentence ends (or line ends, or
    // spaces) so no chunk is longer than `limit`; public for tests.
    static QList<Range> grammarChunks(const QString &text, int limit = 1000);
    // Spelling and grammar together, in document order; the one list the
    // surfaces underline and the review pane shows.
    QList<Issue> issues(const QString &text) const;
    Q_INVOKABLE QVariantList issueList(const QString &text) const;
    // Up to eight replacements for one word, best first.
    Q_INVOKABLE QStringList suggestions(const QString &word) const;
    // Learned words persist in the system dictionary; ignored words last for
    // this run. Either emits wordsChanged so surfaces re-check.
    Q_INVOKABLE void learnWord(const QString &word);
    Q_INVOKABLE void unlearnWord(const QString &word);
    Q_INVOKABLE void ignoreWord(const QString &word);
    // Dismiss one grammar finding (the text it covers and the checker's
    // message) for this run; grammarIssues() stops reporting it.
    Q_INVOKABLE void ignoreGrammar(const QString &text, const QString &message);
    Q_INVOKABLE bool hasLearnedWord(const QString &word) const;

signals:
    void enabledChanged();
    void grammarEnabledChanged();
    void languageChanged();
    void wordsChanged();

private:
    bool m_enabled = true;
    bool m_grammarEnabled = true;
    QString m_language;
    // Guesses are the slow part of a finding and the review list asks for
    // every word's on each rebuild; keep them until the language or the
    // learned/ignored words change.
    mutable QHash<QString, QStringList> m_suggestionCache;
    // Raw grammar answers per chunk of prose (before the ignore filter). The
    // macOS pass is super-linear in the text length (0.9 s at 8,700 chars), so a
    // paragraph is checked in sentence-aligned chunks of at most ~1,000
    // characters and a keystroke only re-checks the chunk it touched.
    mutable QHash<QString, QVariantList> m_grammarChunkCache;
    QSet<QString> m_ignoredGrammar;
};
