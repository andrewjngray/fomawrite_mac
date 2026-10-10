#include "spellcheck.h"

#include <QSettings>
#include <algorithm>
#include <QVariantMap>
#ifdef Q_OS_MACOS
#include "macbridge.h"
#endif

SpellCheck::SpellCheck(QObject *parent) : QObject(parent) {
    QSettings settings;
    m_enabled = settings.value(QStringLiteral("writing/checkSpellingWhileTyping"), true).toBool();
    m_language = settings.value(QStringLiteral("writing/spellingLanguage")).toString().left(32);
    m_grammarEnabled = settings.value(QStringLiteral("writing/checkGrammarWhileTyping"), true).toBool();
}

void SpellCheck::setGrammarEnabled(bool enabled) {
    if (m_grammarEnabled == enabled) return;
    m_grammarEnabled = enabled;
    QSettings().setValue(QStringLiteral("writing/checkGrammarWhileTyping"), enabled);
    emit grammarEnabledChanged();
}

bool SpellCheck::available() const {
#ifdef Q_OS_MACOS
    return true;
#else
    return false;
#endif
}

void SpellCheck::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    QSettings().setValue(QStringLiteral("writing/checkSpellingWhileTyping"), enabled);
    emit enabledChanged();
}

void SpellCheck::setLanguage(const QString &language) {
    const QString chosen = language.left(32);
    if (m_language == chosen) return;
    m_language = chosen;
    m_suggestionCache.clear();
    QSettings().setValue(QStringLiteral("writing/spellingLanguage"), chosen);
    emit languageChanged();
}

QStringList SpellCheck::languages() const {
#ifdef Q_OS_MACOS
    return macWritingLanguages();
#else
    return {};
#endif
}

QList<SpellCheck::Range> SpellCheck::misspellings(const QString &text) const {
    QList<Range> ranges;
    if (!m_enabled || !available() || text.isEmpty()) return ranges;
#ifdef Q_OS_MACOS
    const QVariantList found = macMisspelledRanges(text, m_language);
    ranges.reserve(found.size());
    for (const QVariant &entry : found) {
        const QVariantMap map = entry.toMap();
        const int start = map.value(QStringLiteral("start")).toInt(), end = map.value(QStringLiteral("end")).toInt();
        if (start < 0 || end <= start || end > text.size()) continue;
        ranges.append({start, end});
    }
#endif
    return ranges;
}

QVariantList SpellCheck::misspelledRanges(const QString &text) const {
    QVariantList list;
    for (const Range &range : misspellings(text))
        list.append(QVariantMap{{QStringLiteral("start"), range.start}, {QStringLiteral("end"), range.end}});
    return list;
}

QList<SpellCheck::Range> SpellCheck::grammarChunks(const QString &text, int limit) {
    QList<Range> chunks;
    const int size = text.size();
    int start = 0;
    while (start < size) {
        if (size - start <= limit) { chunks.append({start, size}); break; }
        // Prefer the last sentence end inside the window, then a line end, then a space.
        int cut = -1;
        for (int i = start + limit - 1; i > start + limit / 4; --i) {
            const QChar c = text.at(i);
            // Cut after the space that follows the sentence end, so the next chunk starts on its own sentence.
            if ((c == QLatin1Char('.') || c == QLatin1Char('!') || c == QLatin1Char('?')) && i + 1 < size && text.at(i + 1).isSpace()) { cut = i + 2; break; }
        }
        if (cut < 0) for (int i = start + limit - 1; i > start + limit / 4; --i) if (text.at(i) == QLatin1Char('\n')) { cut = i + 1; break; }
        if (cut < 0) for (int i = start + limit - 1; i > start + limit / 4; --i) if (text.at(i).isSpace()) { cut = i + 1; break; }
        if (cut < 0) cut = start + limit;
        chunks.append({start, cut});
        start = cut;
    }
    return chunks;
}

QList<SpellCheck::Issue> SpellCheck::grammarIssues(const QString &text) const {
    QList<Issue> issues;
    if (!m_enabled || !m_grammarEnabled || !available() || text.trimmed().isEmpty()) return issues;
#ifdef Q_OS_MACOS
    for (const Range &chunk : grammarChunks(text)) {
        const QString piece = text.mid(chunk.start, chunk.end - chunk.start);
        if (piece.trimmed().isEmpty()) continue;
        auto cached = m_grammarChunkCache.constFind(piece);
        if (cached == m_grammarChunkCache.constEnd()) {
            if (m_grammarChunkCache.size() > 4096) m_grammarChunkCache.clear();
            cached = m_grammarChunkCache.insert(piece, macGrammarIssues(piece, m_language));
        }
        for (const QVariant &entry : *cached) {
            const QVariantMap map = entry.toMap();
            const int start = chunk.start + map.value(QStringLiteral("start")).toInt(), end = chunk.start + map.value(QStringLiteral("end")).toInt();
            if (start < chunk.start || end <= start || end > chunk.end) continue;
            if (!m_ignoredGrammar.isEmpty()
                && m_ignoredGrammar.contains(text.mid(start, end - start) + QLatin1Char('\n') + map.value(QStringLiteral("message")).toString())) continue;
            issues.append({start, end, QStringLiteral("Grammar"), map.value(QStringLiteral("message")).toString(),
                           map.value(QStringLiteral("suggestions")).toStringList()});
        }
    }
#endif
    return issues;
}

QList<SpellCheck::Issue> SpellCheck::issues(const QString &text) const {
    QList<Issue> all;
    for (const Range &range : misspellings(text))
        all.append({range.start, range.end, QStringLiteral("Spelling"), QString(), suggestions(text.mid(range.start, range.end - range.start))});
    all.append(grammarIssues(text));
    std::stable_sort(all.begin(), all.end(), [](const Issue &a, const Issue &b) { return a.start < b.start || (a.start == b.start && a.end < b.end); });
    return all;
}

QVariantList SpellCheck::issueList(const QString &text) const {
    QVariantList list;
    for (const Issue &issue : issues(text))
        list.append(QVariantMap{{QStringLiteral("start"), issue.start}, {QStringLiteral("end"), issue.end},
                                {QStringLiteral("category"), issue.category}, {QStringLiteral("message"), issue.message},
                                {QStringLiteral("suggestions"), issue.suggestions}});
    return list;
}

QStringList SpellCheck::suggestions(const QString &word) const {
    if (!available() || word.trimmed().isEmpty() || word.size() > 100) return {};
#ifdef Q_OS_MACOS
    const auto cached = m_suggestionCache.constFind(word);
    if (cached != m_suggestionCache.constEnd()) return *cached;
    if (m_suggestionCache.size() >= 4096) m_suggestionCache.clear();
    return m_suggestionCache.insert(word, macSpellingGuesses(word, m_language)).value();
#else
    return {};
#endif
}

void SpellCheck::learnWord(const QString &word) {
    if (!available() || word.trimmed().isEmpty() || word.size() > 100) return;
#ifdef Q_OS_MACOS
    macLearnWord(word);
#endif
    m_suggestionCache.clear();
    emit wordsChanged();
}

void SpellCheck::unlearnWord(const QString &word) {
    if (!available() || word.trimmed().isEmpty() || word.size() > 100) return;
#ifdef Q_OS_MACOS
    macUnlearnWord(word);
#endif
    m_suggestionCache.clear();
    emit wordsChanged();
}

void SpellCheck::ignoreWord(const QString &word) {
    if (!available() || word.trimmed().isEmpty() || word.size() > 100) return;
#ifdef Q_OS_MACOS
    macIgnoreWord(word);
#endif
    m_suggestionCache.clear();
    emit wordsChanged();
}

void SpellCheck::ignoreGrammar(const QString &text, const QString &message) {
    if (text.trimmed().isEmpty() || text.size() > 1000) return;
    m_ignoredGrammar.insert(text + QLatin1Char('\n') + message);
    emit wordsChanged();
}

bool SpellCheck::hasLearnedWord(const QString &word) const {
#ifdef Q_OS_MACOS
    return macHasLearnedWord(word);
#else
    Q_UNUSED(word); return false;
#endif
}
