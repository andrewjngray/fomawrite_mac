#include "harperengine.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

#include "editorbridge.h"

HarperEngine::HarperEngine(EditorBridge *bridge, QObject *parent) : QObject(parent), m_bridge(bridge) {
    connect(bridge, &EditorBridge::readyChanged, this, &HarperEngine::onBridgeReady);
    connect(bridge, &EditorBridge::harperReadied, this, &HarperEngine::onReadied);
    connect(bridge, &EditorBridge::harperReplied, this, &HarperEngine::onReplied);
    connect(bridge, &EditorBridge::harperLintFailed, this, &HarperEngine::onFailed);
    if (bridge->isReady()) onBridgeReady();
}

QString HarperEngine::categoryForKind(const QString &kind) {
    static const QSet<QString> grammar{
        QStringLiteral("Agreement"), QStringLiteral("BoundaryError"), QStringLiteral("Capitalization"), QStringLiteral("Eggcorn"),
        QStringLiteral("Grammar"), QStringLiteral("Malapropism"), QStringLiteral("Nonstandard"), QStringLiteral("Punctuation"),
        QStringLiteral("Repetition"), QStringLiteral("Typo"), QStringLiteral("Usage"), QStringLiteral("WordOrder")};
    if (kind == QLatin1String("Spelling")) return QStringLiteral("Spelling");
    if (grammar.contains(kind)) return QStringLiteral("Grammar");
    // Enhancement, Formatting, Miscellaneous, Readability, Redundancy,
    // Regionalism, Style, WordChoice ("very unique" is WordChoice in Harper, and
    // reads as a suggestion, not an error) and any kind a later Harper adds.
    return QStringLiteral("Style");
}

// A new page (or a reload) means a new engine: whatever the old one was doing is
// lost. Texts it had are asked for again once the new one reports ready.
void HarperEngine::onBridgeReady() {
    if (!m_bridge->isReady()) return;
    const bool wasReady = m_ready;
    m_ready = false;
    for (auto it = m_tokens.cbegin(); it != m_tokens.cend(); ++it) m_queue.append(it.value());
    m_tokens.clear();
    if (wasReady) emit readyChanged();
    emit m_bridge->harperLoad(m_dialect);
}

void HarperEngine::onReadied(const QString &version) {
    const bool changed = !m_ready || m_version != version;
    m_ready = true;
    m_version = version;
    sendWords();
    pump();
    if (changed) emit readyChanged();
}

void HarperEngine::setDialect(const QString &dialect) {
    if (dialect.isEmpty() || dialect == m_dialect) return;
    m_dialect = dialect;
    // Answers from the old dialect are wrong now (colour/color); late replies
    // for them find no token and are ignored.
    m_cache.clear();
    m_wordSuggestions.clear();
    dropOutstanding();
    if (m_bridge->isReady()) emit m_bridge->harperLoad(m_dialect);
}

void HarperEngine::reload() {
    // A newer (or older) engine may answer differently, so nothing cached stands.
    m_cache.clear();
    m_wordSuggestions.clear();
    // Same steps as a new page: not ready, what the old engine held is asked again.
    const bool wasReady = m_ready;
    m_ready = false;
    for (auto it = m_tokens.cbegin(); it != m_tokens.cend(); ++it) m_queue.append(it.value());
    m_tokens.clear();
    if (wasReady) emit readyChanged();
    if (m_bridge->isReady()) emit m_bridge->harperLoad(m_dialect);
}

void HarperEngine::dropOutstanding() {
    m_queue.clear();
    m_overflow.clear();
    m_wanted.clear();
    m_tokens.clear();
}

void HarperEngine::importWords(const QStringList &words) {
    if (words == m_words) return;
    m_words = words;
    m_cache.clear();
    m_wordSuggestions.clear();
    dropOutstanding();
    sendWords();
}

void HarperEngine::sendWords() {
    if (!m_ready || m_words.isEmpty()) return;
    emit m_bridge->harperImportWords(QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(m_words)).toJson(QJsonDocument::Compact)));
}

std::optional<QList<SpellCheck::Issue>> HarperEngine::issues(const QString &text) {
    if (text.trimmed().isEmpty()) return QList<SpellCheck::Issue>();
    const auto cached = m_cache.constFind(text);
    if (cached != m_cache.constEnd()) return *cached;
    enqueue(text);
    return std::nullopt;
}

void HarperEngine::enqueue(const QString &text) {
    if (m_wanted.contains(text)) return; // already waiting or with the page
    m_wanted.insert(text);
    m_queue.append(text);
    if (m_queue.size() > QueueLimit) {
        // Newest first: the oldest waiting text steps aside (it is asked for
        // again as the queue drains).
        m_overflow.append(m_queue.takeFirst());
        if (m_overflow.size() > OverflowLimit) m_wanted.remove(m_overflow.takeFirst());
    }
    pump();
}

void HarperEngine::pump() {
    if (!m_ready) return;
    while (m_tokens.size() < InFlightLimit && !m_queue.isEmpty()) {
        const QString text = m_queue.takeLast();
        const int token = ++m_nextToken;
        m_tokens.insert(token, text);
        emit m_bridge->harperLint(token, text);
        // Refill from the overflow (the newest of the older ones first).
        while (m_queue.size() < QueueLimit && !m_overflow.isEmpty()) m_queue.prepend(m_overflow.takeLast());
    }
}

void HarperEngine::onReplied(int token, const QString &lintsJson) {
    const auto it = m_tokens.find(token);
    if (it == m_tokens.end()) return; // an answer to a question already dropped
    const QString text = it.value();
    m_tokens.erase(it);
    m_wanted.remove(text);
    QList<SpellCheck::Issue> found;
    const QJsonDocument document = QJsonDocument::fromJson(lintsJson.toUtf8());
    const QJsonArray lints = document.isArray() ? document.array() : QJsonArray();
    for (const QJsonValue &value : lints) {
        const QJsonObject lint = value.toObject();
        const int start = lint.value(QStringLiteral("start")).toInt(-1), end = lint.value(QStringLiteral("end")).toInt(-1);
        if (start < 0 || end <= start || end > text.size()) continue;
        const QString kind = lint.value(QStringLiteral("kind")).toString();
        QStringList suggestions;
        for (const QJsonValue &suggestion : lint.value(QStringLiteral("suggestions")).toArray())
            if (suggestion.isString() && suggestions.size() < 8) suggestions.append(suggestion.toString());
        SpellCheck::Issue issue{start, end, categoryForKind(kind), lint.value(QStringLiteral("message")).toString(), suggestions, kind};
        if (issue.category == QLatin1String("Spelling") && !suggestions.isEmpty()) {
            if (m_wordSuggestions.size() >= CacheLimit) m_wordSuggestions.clear();
            m_wordSuggestions.insert(text.mid(start, end - start), suggestions);
        }
        found.append(issue);
    }
    std::stable_sort(found.begin(), found.end(), [](const SpellCheck::Issue &a, const SpellCheck::Issue &b) {
        return a.start < b.start || (a.start == b.start && a.end < b.end); });
    if (m_cache.size() >= CacheLimit) m_cache.clear();
    m_cache.insert(text, found);
    pump();
    emit resultsReady(text);
}

void HarperEngine::onFailed(int token, const QString &error) {
    const auto it = m_tokens.find(token);
    if (it == m_tokens.end()) return;
    m_wanted.remove(it.value());
    m_tokens.erase(it);
    pump();
    emit failed(error);
}
