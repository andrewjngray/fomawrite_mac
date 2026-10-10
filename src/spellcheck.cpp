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

QList<SpellCheck::Issue> SpellCheck::grammarIssues(const QString &text) const {
    QList<Issue> issues;
    if (!m_enabled || !m_grammarEnabled || !available() || text.trimmed().isEmpty()) return issues;
#ifdef Q_OS_MACOS
    for (const QVariant &entry : macGrammarIssues(text, m_language)) {
        const QVariantMap map = entry.toMap();
        const int start = map.value(QStringLiteral("start")).toInt(), end = map.value(QStringLiteral("end")).toInt();
        if (start < 0 || end <= start || end > text.size()) continue;
        issues.append({start, end, QStringLiteral("Grammar"), map.value(QStringLiteral("message")).toString(),
                       map.value(QStringLiteral("suggestions")).toStringList()});
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
    return macSpellingGuesses(word, m_language);
#else
    return {};
#endif
}

void SpellCheck::learnWord(const QString &word) {
    if (!available() || word.trimmed().isEmpty() || word.size() > 100) return;
#ifdef Q_OS_MACOS
    macLearnWord(word);
#endif
    emit wordsChanged();
}

void SpellCheck::unlearnWord(const QString &word) {
    if (!available() || word.trimmed().isEmpty() || word.size() > 100) return;
#ifdef Q_OS_MACOS
    macUnlearnWord(word);
#endif
    emit wordsChanged();
}

void SpellCheck::ignoreWord(const QString &word) {
    if (!available() || word.trimmed().isEmpty() || word.size() > 100) return;
#ifdef Q_OS_MACOS
    macIgnoreWord(word);
#endif
    emit wordsChanged();
}

bool SpellCheck::hasLearnedWord(const QString &word) const {
#ifdef Q_OS_MACOS
    return macHasLearnedWord(word);
#else
    Q_UNUSED(word); return false;
#endif
}
