#include "spellcheck.h"

#include <QSettings>
#include "harperengine.h"
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
    const QString engine = settings.value(QStringLiteral("writing/engine"), QStringLiteral("harper")).toString();
    m_engine = engine == QLatin1String("macos") ? engine : QStringLiteral("harper");
    const QString dialect = settings.value(QStringLiteral("writing/dialect"), QStringLiteral("Australian")).toString();
    m_dialect = dialects().contains(dialect) ? dialect : QStringLiteral("Australian");
    m_styleEnabled = settings.value(QStringLiteral("writing/checkStyleWhileTyping"), false).toBool();
}

QStringList SpellCheck::dialects() const {
    return {QStringLiteral("Australian"), QStringLiteral("British"), QStringLiteral("American"), QStringLiteral("Canadian")};
}

void SpellCheck::setHarper(HarperEngine *harper) {
    if (m_harper == harper) return;
    const bool wasHarper = usesHarper();
    if (m_harper) disconnect(m_harper, nullptr, this, nullptr);
    m_harper = harper;
    if (harper) {
        harper->setDialect(m_dialect);
        harper->importWords(learnedWords());
        connect(harper, &HarperEngine::readyChanged, this, [this] {
            emit harperReadyChanged();
            emit engineChanged(); // effectiveEngine moved between harper and macos
        });
        connect(harper, &HarperEngine::resultsReady, this, &SpellCheck::resultsReady);
    }
    emit harperReadyChanged();
    if (wasHarper != usesHarper()) emit engineChanged();
}

bool SpellCheck::harperReady() const { return m_harper && m_harper->ready(); }
QString SpellCheck::harperVersion() const { return m_harper && m_harper->ready() ? m_harper->version() : QString(); }

QString SpellCheck::effectiveEngine() const {
    return m_engine == QLatin1String("harper") && harperReady() ? QStringLiteral("harper") : QStringLiteral("macos");
}

void SpellCheck::setEngine(const QString &engine) {
    const QString chosen = engine == QLatin1String("macos") ? engine : engine == QLatin1String("harper") ? engine : m_engine;
    if (m_engine == chosen) return;
    m_engine = chosen;
    m_suggestionCache.clear();
    QSettings().setValue(QStringLiteral("writing/engine"), chosen);
    emit engineChanged();
}

void SpellCheck::setDialect(const QString &dialect) {
    if (!dialects().contains(dialect) || m_dialect == dialect) return;
    m_dialect = dialect;
    QSettings().setValue(QStringLiteral("writing/dialect"), dialect);
    if (m_harper) m_harper->setDialect(dialect);
    emit dialectChanged();
}

void SpellCheck::setStyleEnabled(bool enabled) {
    if (m_styleEnabled == enabled) return;
    m_styleEnabled = enabled;
    QSettings().setValue(QStringLiteral("writing/checkStyleWhileTyping"), enabled);
    emit styleEnabledChanged();
}

QStringList SpellCheck::learnedWords() const {
    return QSettings().value(QStringLiteral("writing/learnedWords")).toStringList();
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
    const auto known = issuesIfKnown(text);
    return known ? *known : QList<Issue>();
}

std::optional<QList<SpellCheck::Issue>> SpellCheck::issuesIfKnown(const QString &text) const {
    if (!usesHarper()) return macosIssues(text);
    if (!m_enabled || text.trimmed().isEmpty()) return QList<Issue>();
    const auto answer = m_harper->issues(text);
    if (!answer) return std::nullopt;
    // Harper answers for the text as it stands; this setting and the one list of
    // ignored findings (word, or text and message) are applied here, so ignoring
    // or switching grammar off needs no new lint.
    QList<Issue> found;
    for (const Issue &issue : *answer) {
        const QString covered = text.mid(issue.start, issue.end - issue.start);
        if (issue.category == QLatin1String("Spelling")) {
            if (m_ignoredWords.contains(covered)) continue;
        } else {
            if (issue.category == QLatin1String("Grammar") && !m_grammarEnabled) continue;
            if (!m_ignoredGrammar.isEmpty() && m_ignoredGrammar.contains(covered + QLatin1Char('\n') + issue.message)) continue;
        }
        found.append(issue);
    }
    return found;
}

QList<SpellCheck::Issue> SpellCheck::macosIssues(const QString &text) const {
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
    if (usesHarper()) {
        const QStringList harper = m_harper->suggestionsFor(word);
        if (!harper.isEmpty()) return harper;
    }
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

// Learned words are the system dictionary's and, for Harper, the app's own list
// (writing/learnedWords), which Harper imports whole whenever it changes and
// again whenever the engine reloads.
void SpellCheck::learnWord(const QString &word) {
    if (!available() || word.trimmed().isEmpty() || word.size() > 100) return;
#ifdef Q_OS_MACOS
    macLearnWord(word);
#endif
    QStringList words = learnedWords();
    if (!words.contains(word)) {
        words.append(word);
        QSettings().setValue(QStringLiteral("writing/learnedWords"), words);
    }
    if (m_harper) m_harper->importWords(words);
    m_suggestionCache.clear();
    emit wordsChanged();
}

void SpellCheck::unlearnWord(const QString &word) {
    if (!available() || word.trimmed().isEmpty() || word.size() > 100) return;
#ifdef Q_OS_MACOS
    macUnlearnWord(word);
#endif
    QStringList words = learnedWords();
    if (words.removeAll(word) > 0) QSettings().setValue(QStringLiteral("writing/learnedWords"), words);
    if (m_harper) m_harper->importWords(words);
    m_suggestionCache.clear();
    emit wordsChanged();
}

void SpellCheck::ignoreWord(const QString &word) {
    if (!available() || word.trimmed().isEmpty() || word.size() > 100) return;
#ifdef Q_OS_MACOS
    macIgnoreWord(word);
#endif
    m_ignoredWords.insert(word);
    m_suggestionCache.clear();
    emit wordsChanged();
}

void SpellCheck::ignoreGrammar(const QString &text, const QString &message) {
    if (text.trimmed().isEmpty() || text.size() > 1000) return;
    m_ignoredGrammar.insert(text + QLatin1Char('\n') + message);
    emit wordsChanged();
}

bool SpellCheck::hasLearnedWord(const QString &word) const {
    if (learnedWords().contains(word)) return true;
#ifdef Q_OS_MACOS
    return macHasLearnedWord(word);
#else
    Q_UNUSED(word); return false;
#endif
}
