#pragma once

// Harper, the grammar and style engine, as the host sees it.
//
// Harper (harper.js, WebAssembly) runs inside the Live editor page; this object
// is the host's side of that conversation, over the editor bridge. It turns the
// asynchronous bridge into the question the writing surfaces ask:
//
//     issues(text)  ->  the findings for exactly this text, or "not known yet"
//
// A text that has not been linted is queued (once, however many blocks hold the
// same prose) and the answer arrives later as resultsReady(text); the surfaces
// keep what they are showing in the meantime. Answers are cached by text and
// the cache is dropped when the dialect or the custom words change, since both
// change what Harper says about the same text.
//
// Findings are SpellCheck::Issue values: Harper's kind (Agreement, Repetition,
// Spelling, ...) is kept in `kind` and folded into the three categories the
// surfaces draw: Spelling, Grammar and Style (see categoryForKind).

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <optional>

#include "spellcheck.h"

class EditorBridge;

class HarperEngine : public QObject {
    Q_OBJECT
public:
    explicit HarperEngine(EditorBridge *bridge, QObject *parent = nullptr);

    // The page has loaded Harper and answers requests.
    bool ready() const { return m_ready; }
    QString version() const { return m_version; }
    QString dialect() const { return m_dialect; }
    // Remembered for a later page; sent at once when the page is there. Drops
    // every cached and outstanding answer: they were given in the old dialect.
    void setDialect(const QString &dialect);
    // Load the engine again under the same dialect: the page re-reads the served
    // version and, when an update was installed (or rolled back), imports it.
    // Answers from the old engine are dropped (a newer Harper may say different
    // things), what was with the page is queued again, and `ready` goes false
    // until the page announces the engine (the surfaces use the macOS checker
    // for a moment, as for a new page). Cycle 146.
    void reload();

    // The findings for exactly `text`, or nullopt after queueing a request (the
    // answer then comes as resultsReady). Blank text has no findings.
    std::optional<QList<SpellCheck::Issue>> issues(const QString &text);
    // The writer's own words, which Harper must not flag. The list is the whole
    // dictionary (not a delta); it is sent now and again after every reload of
    // the page's engine.
    void importWords(const QStringList &words);
    QStringList words() const { return m_words; }
    // Harper's replacements for a misspelled word seen in an answer, when one
    // has been seen (the surfaces offer these instead of the system's guesses).
    QStringList suggestionsFor(const QString &word) const { return m_wordSuggestions.value(word); }

    // Harper's kind -> "Spelling", "Grammar" or "Style".
    static QString categoryForKind(const QString &kind);

    // Counters for tests and diagnostics.
    int cachedCount() const { return int(m_cache.size()); }
    int inFlightCount() const { return int(m_tokens.size()); }
    int queuedCount() const { return int(m_queue.size()); }
    int overflowCount() const { return int(m_overflow.size()); }
    static constexpr int QueueLimit = 64;     // texts waiting to be sent, newest first
    static constexpr int InFlightLimit = 16;  // texts the page is working on at once
    static constexpr int OverflowLimit = 4096;
    static constexpr int CacheLimit = 4096;

signals:
    void readyChanged();
    // `text` now has an answer in the cache.
    void resultsReady(const QString &text);
    // A request failed in the page; the text has no answer and will be asked
    // for again the next time a surface wants it.
    void failed(const QString &error);

private:
    void onBridgeReady();
    void onReadied(const QString &version);
    void onReplied(int token, const QString &lintsJson);
    void onFailed(int token, const QString &error);
    void enqueue(const QString &text);
    void pump();
    void sendWords();
    void dropOutstanding();

    EditorBridge *m_bridge;
    bool m_ready = false;
    QString m_version;
    QString m_dialect = QStringLiteral("Australian");
    QStringList m_words;
    int m_nextToken = 0;
    QHash<QString, QList<SpellCheck::Issue>> m_cache;
    QHash<QString, QStringList> m_wordSuggestions;
    // Waiting to be sent: a stack, so the newest request goes first; texts
    // beyond QueueLimit wait in m_overflow (oldest dropped past OverflowLimit)
    // and are moved up as the queue drains, so a large document's blocks are
    // all answered eventually without a flood in the page.
    QList<QString> m_queue;
    QList<QString> m_overflow;
    QSet<QString> m_wanted;            // every text in m_queue, m_overflow or in flight
    QHash<int, QString> m_tokens;      // token -> text, requests the page has
};
