#include "spellcheck.h"

#include <QSettings>
#include <QVariantMap>
#ifdef Q_OS_MACOS
#include "macbridge.h"
#endif

SpellCheck::SpellCheck(QObject *parent) : QObject(parent) {
    QSettings settings;
    m_enabled = settings.value(QStringLiteral("writing/checkSpellingWhileTyping"), true).toBool();
    m_language = settings.value(QStringLiteral("writing/spellingLanguage")).toString().left(32);
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
