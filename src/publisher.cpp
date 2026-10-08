#include "publisher.h"

#include "outputcss.h"
#include "publishinghtml.h"
#include "publishingpdf.h"
#include "publishingthemes.h"
#include "typorabase.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QPdfDocument>
#include <QPdfSelection>
#include <QRegularExpression>
#include <QSettings>
#include <QTemporaryDir>
#include <QTextDocument>

namespace {
QString comparablePdfText(QString text) {
    text.remove(QChar(0x00ad));
    return text.simplified();
}
// Heading anchors for the PDF preview, located from the rendered page text.
QVariantMap publishingAnchors(const QByteArray &bytes, const QString &html) {
    QBuffer buffer; buffer.setData(bytes); buffer.open(QIODevice::ReadOnly);
    QPdfDocument pdf; pdf.load(&buffer);
    QStringList pages;
    const bool titlePage = html.contains("<section class=\"fomawrite-title-page\">");
    for (int index = 0; index < pdf.pageCount(); ++index) {
        QString text = pdf.getAllText(index).text(); text.remove(QChar(0x00ad));
        pages << (titlePage && index == 0 ? QString() : text);
    }
    const QRegularExpression headings("<h[1-6]\\b[^>]*id=\"([^\"]+)\"[^>]*>(.*?)</h[1-6]>", QRegularExpression::DotMatchesEverythingOption);
    QList<QPair<QString, QString>> targets;
    QHash<QString, int> expected;
    auto matches = headings.globalMatch(html);
    while (matches.hasNext()) {
        const auto match = matches.next();
        QTextDocument text, name; text.setHtml(match.captured(2)); name.setHtml(match.captured(1));
        const QString heading = comparablePdfText(text.toPlainText());
        if (!heading.isEmpty()) { targets.append({name.toPlainText(), heading}); ++expected[heading]; }
    }
    QHash<QString, QList<int>> destinations;
    for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
        QStringList words;
        for (const auto &word : it.key().split(' ')) words << QRegularExpression::escape(word);
        const QRegularExpression line("(?:^|[\\r\\n])\\s*" + words.join("\\s+") + "\\s*(?=[\\r\\n]|$)");
        QList<int> candidates;
        for (int page = 0; page < pages.size(); ++page) {
            auto hits = line.globalMatch(pages[page]);
            while (hits.hasNext()) { hits.next(); candidates << page; }
        }
        // Text fallback deliberately refuses ambiguous prose matches instead
        // of silently sending the reader to a paragraph with the same words.
        if (candidates.size() == it.value()) destinations.insert(it.key(), candidates);
    }
    QVariantMap anchors;
    for (const auto &target : targets) {
        auto &candidates = destinations[target.second];
        if (!candidates.isEmpty()) anchors.insert(target.first, candidates.takeFirst());
    }
    return anchors;
}
QByteArray catalogHash(const QVariantList &catalog) {
    return QCryptographicHash::hash(QJsonDocument::fromVariant(catalog).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256);
}
}

Publisher::Publisher(PublishingSource &source, QObject *parent) : QObject(parent), m_source(source) {
    connect(this, &Publisher::cssChanged, this, &Publisher::invalidateSettings);
    m_themeId = QSettings().value("output/publishingTheme").toString();
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(200);
    connect(&m_refreshTimer, &QTimer::timeout, this, [this] { refreshThemes(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
        // In-place edits can preserve size and coarse timestamps. Re-read only
        // the file actually notified, while directory event replays stay cheap.
        m_fileHashes.remove(path);
        m_refreshTimer.start();
    });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_refreshTimer.start(); });
    m_snapshot = watchThemes();
    m_catalogSnapshot = catalogHash(themes());
    updateCss();
}

Publisher::~Publisher() = default;

QString Publisher::themeName() const {
    if (m_themeId.isEmpty()) return m_source.outputTemplateName();
    for (const auto &entry : themes()) {
        const auto theme = entry.toMap();
        if (theme["id"].toString() == m_themeId) return theme["name"].toString();
    }
    return QStringLiteral("Missing theme");
}
QVariantList Publisher::themes() const { return PublishingThemes::catalog(); }
bool Publisher::selectTheme(const QString &id) {
    QString error, advisory;
    if (!PublishingThemes::css(id, &error, &advisory)) {
        m_themeError = error;
        emit themesChanged();
        emit statusMessage(error);
        return false;
    }
    m_themeId = id;
    QSettings().setValue("output/publishingTheme", id);
    const bool changed = updateCss();
    m_themeError = advisory;
    if (changed) emit cssChanged();
    emit themesChanged();
    emit statusMessage(advisory.isEmpty() ? themeName() + " publishing theme selected." : advisory);
    return true;
}
bool Publisher::importTheme(const QUrl &file) {
    QString id, message;
    if (!PublishingThemes::importTheme(file, &id, &message)) {
        m_themeError = message;
        emit themesChanged();
        emit statusMessage(message);
        return false;
    }
    watchThemes();
    if (!selectTheme(id)) return false;
    if (!message.isEmpty()) m_themeError = message;
    emit themesChanged();
    emit statusMessage(message.isEmpty() ? themeName() + " imported for Web and PDF." : message);
    return true;
}
QString Publisher::currentCss() const {
    if (m_themeId.isEmpty()) return m_source.basicStyleCss();
    return selectedCss(nullptr).value_or(QString());
}
QMap<QString, QString> Publisher::themeVariables() const {
    const QString css = currentCss();
    if (css != m_variablesCss) {
        m_variables = TyporaBase::themeVariables(css);
        m_variablesCss = css;
    }
    return m_variables;
}
QString Publisher::themeVariable(const QString &name) const {
    return themeVariables().value(name.startsWith(QLatin1String("--")) ? name : QStringLiteral("--") + name);
}
void Publisher::clearTheme() {
    m_themeId.clear();
    m_themeError.clear();
    QSettings().remove("output/publishingTheme");
    updateCss();
}
QByteArray Publisher::watchThemes() {
    const QString folder = PublishingThemes::directory();
    QStringList watches, entries;
    if (QDir(folder).exists()) {
        const QString canonicalFolder = QDir(folder).canonicalPath();
        watches.append(canonicalFolder);
        QDirIterator iterator(canonicalFolder, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDirIterator::Subdirectories);
        while (iterator.hasNext() && entries.size() < 4096) {
            const QString path = iterator.next();
            const QFileInfo info(path);
            if (info.canonicalFilePath() != path) continue;
            entries.append(path);
            // Directories report additions, removals and atomic replacements
            // of everything inside them. CSS is the only kind of file edited
            // in place by hand; fonts and images change through their folder.
            if (info.isDir() || path.endsWith(QLatin1String(".css"), Qt::CaseInsensitive)) watches.append(path);
        }
    } else {
        // Never create the folder from a refresh: a user who moved it aside
        // must not find an empty replacement. Watch the parent for its return.
        const QString parent = QFileInfo(folder).absolutePath();
        if (QDir(parent).exists()) watches.append(parent);
    }
    // Sort before capping so the watched subset is stable between refreshes;
    // an unstable subset would churn watches and replay notifications.
    watches.sort(); entries.sort();
    if (watches.size() > 512) {
        qWarning("Fomawrite: watching 512 of %lld publishing theme entries", static_cast<long long>(watches.size()));
        watches = watches.mid(0, 512);
    }
    const QStringList watched = m_watcher.files() + m_watcher.directories();
    QStringList removed, added;
    for (const QString &path : watched) if (!watches.contains(path)) removed.append(path);
    for (const QString &path : watches) if (!watched.contains(path)) added.append(path);
    // Re-registering the whole tree replays macOS filesystem notifications.
    // Keep unchanged watches; atomic saves still re-add dropped file watches.
    if (!removed.isEmpty()) m_watcher.removePaths(removed);
    if (!added.isEmpty()) m_watcher.addPaths(added);

    for (auto it = m_fileHashes.begin(); it != m_fileHashes.end();) {
        if (!entries.contains(it.key())) it = m_fileHashes.erase(it);
        else ++it;
    }
    QCryptographicHash snapshot(QCryptographicHash::Sha256);
    for (const QString &path : entries) {
        const QFileInfo entry(path);
        snapshot.addData(path.toUtf8());
        snapshot.addData(QByteArray(1, '\0'));
        if (entry.isFile()) {
            const QByteArray metadata = QByteArray::number(entry.size()) + ':'
                + QByteArray::number(entry.lastModified().toMSecsSinceEpoch()) + ':'
                + QByteArray::number(entry.birthTime().toMSecsSinceEpoch());
            // CSS content is hashed (cached by metadata, invalidated by file
            // events) so same-size preserved-mtime edits are still noticed.
            // Fonts and images are identified by metadata alone.
            if (path.endsWith(QLatin1String(".css"), Qt::CaseInsensitive)) {
                auto cached = m_fileHashes.constFind(path);
                if (cached == m_fileHashes.cend() || cached->first != metadata) {
                    QCryptographicHash content(QCryptographicHash::Sha256);
                    QFile file(path);
                    const bool readable = entry.size() <= 128 * 1024 * 1024
                        && file.open(QIODevice::ReadOnly) && content.addData(&file);
                    m_fileHashes.insert(path, {metadata, readable ? content.result() : metadata});
                }
                snapshot.addData(m_fileHashes.value(path).second);
            } else {
                snapshot.addData(metadata);
            }
        }
        snapshot.addData(QByteArray(1, '\0'));
    }
    return snapshot.result();
}
// Recomputes the selected theme's CSS from the current folder state, memoizes
// it for html(), updates the theme error and reports whether the
// output-affecting CSS (or its error) changed since the previous computation.
bool Publisher::updateCss() {
    QString error, advisory, css;
    if (!m_themeId.isEmpty()) {
        if (auto selected = PublishingThemes::css(m_themeId, &error, &advisory)) css = *selected;
    }
    m_cssMemo = {m_themeId, m_snapshot, css, QCryptographicHash::hash(css.toUtf8(), QCryptographicHash::Sha256), error, error.isEmpty()};
    m_themeError = error.isEmpty() ? advisory : error;
    QCryptographicHash state(QCryptographicHash::Sha256);
    state.addData(m_themeId.toUtf8()); state.addData(QByteArray(1, '\0'));
    state.addData(m_cssMemo.hash); state.addData(QByteArray(1, '\0'));
    state.addData(error.toUtf8());
    const QByteArray snapshot = state.result();
    const bool changed = snapshot != m_cssSnapshot;
    m_cssSnapshot = snapshot;
    return changed;
}
std::optional<QString> Publisher::selectedCss(QString *error, QByteArray *hash) const {
    if (m_themeId.isEmpty()) { if (hash) hash->clear(); return QString(); }
    auto &memo = m_cssMemo;
    if (!memo.valid || memo.themeId != m_themeId || memo.snapshot != m_snapshot) {
        QString failure;
        const auto selected = PublishingThemes::css(m_themeId, &failure);
        const QString css = selected.value_or(QString());
        memo = {m_themeId, m_snapshot, css, QCryptographicHash::hash(css.toUtf8(), QCryptographicHash::Sha256), failure, selected.has_value()};
    }
    if (!memo.valid) { if (error) *error = memo.error; return std::nullopt; }
    if (hash) *hash = memo.hash;
    return memo.css;
}
void Publisher::refreshThemes(bool force) {
    const QByteArray snapshot = watchThemes();
    if (!force && snapshot == m_snapshot) return;
    m_snapshot = snapshot;
    const QByteArray catalog = catalogHash(themes());
    const bool catalogChanged = catalog != m_catalogSnapshot;
    m_catalogSnapshot = catalog;
    const bool changed = updateCss();
    if ((changed || force) && !m_themeError.isEmpty()) emit statusMessage(m_themeError);
    // Finder metadata, editor swap files and edits to an unselected theme
    // change the folder but not the published CSS: they must neither cancel
    // in-flight renders nor restart the preview debounce. Only output-affecting
    // changes (or an explicit Reload) reach the preview consumers.
    if (changed || force) emit cssChanged();
    if (catalogChanged || changed || force) emit themesChanged();
}
void Publisher::reloadThemes() {
    refreshThemes(true);
    emit statusMessage(m_themeError.isEmpty() ? "Publishing themes reloaded." : m_themeError);
}
bool Publisher::openThemesFolder() {
    const QString path = PublishingThemes::directory();
    if (!QDir().mkpath(path) || !QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        m_themeError = "Could not open the publishing themes folder.";
        emit themesChanged();
        emit statusMessage(m_themeError);
        return false;
    }
    watchThemes();
    return true;
}

QString Publisher::html(QString *error, bool preview, QString *warning, QByteArray *fingerprint) const {
    QString body = PublishingHtml::body(m_source.publishingMarkdown(), error);
    if (error && !error->isEmpty()) return {};
    QString css;
    QByteArray cssHash;
    if (!m_themeId.isEmpty()) {
        auto selected = selectedCss(error, &cssHash);
        if (!selected) return {};
        css = *selected;
    } else {
        css = m_source.basicStyleCss();
        cssHash = QCryptographicHash::hash(css.toUtf8(), QCryptographicHash::Sha256);
    }
    const QString foundations = "body { margin: 0; } img { max-width: 100%; height: auto; } .highlight { background: #fff2a8; } .pagebreak { height: 0; } .fomawrite-title-page { display: none; }";
    QString customCss;
    const QUrl cssFile = m_source.outputCssFile();
    if (!cssFile.isEmpty()) {
        const auto custom = OutputCss::load(cssFile, error);
        if (!custom) return {};
        customCss = *custom;
        css += '\n' + customCss;
    }
    // The theme's variables (memoised per theme) with the user's own output CSS on top, so a --highlight-color it sets counts.
    QMap<QString, QString> variables = m_themeId.isEmpty() ? TyporaBase::themeVariables(css) : themeVariables();
    if (!m_themeId.isEmpty() && !customCss.isEmpty()) variables.insert(TyporaBase::themeVariables(customCss));
    const QString styleAttribute = cssFile.isEmpty() ? QString() : " data-fomawrite-user-style";
    const QString titlePage = m_source.titlePageHtml();
    const QString title = m_source.documentTitle();
    const QString printCss = m_source.printCss();
    // A restrictive content policy also protects exported HTML opened elsewhere.
    QString html = "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<meta http-equiv=\"Content-Security-Policy\" content=\"default-src 'none'; style-src 'unsafe-inline'; img-src data:; font-src data:; base-uri 'self'; form-action 'none'\">"
        "<title>" + title.toHtmlEscaped() + "</title><style" + styleAttribute + ">" + foundations + '\n' + TyporaBase::css() + '\n' + TyporaBase::highlightCss(variables) + css + printCss
        + "</style></head><body>" + titlePage + "<article id=\"write\">" + body + "</article></body></html>";
    QByteArray assets;
    html = PublishingHtml::embedImages(html, m_source.documentBaseUrl(), preview, error, warning, &m_imageCache, &assets);
    if (fingerprint) {
        // Identify this output without hashing megabytes of embedded fonts and
        // images: every input that can change the document is hashed or
        // stat-signed instead, so a cache lookup costs the same for a theme
        // with 30 MB of fonts as for the basic style.
        QCryptographicHash identity(QCryptographicHash::Sha256);
        for (const QByteArray &part : {body.toUtf8(), cssHash, customCss.toUtf8(), printCss.toUtf8(), titlePage.toUtf8(),
                                       styleAttribute.toUtf8(), title.toUtf8(), assets,
                                       warning ? warning->toUtf8() : QByteArray(), QByteArray(preview ? "preview" : "export")}) {
            identity.addData(part);
            identity.addData(QByteArray(1, '\0'));
        }
        *fingerprint = identity.result();
    }
    return html;
}
QByteArray Publisher::pdfBytes(const QString &html, QString *error) const {
    return PublishingPdf::renderBlocking(html, m_source.documentBaseUrl(), m_source.outputPageLayout(), error);
}
QVariantMap Publisher::storeOutput(quint64 generation, const QString &format, const QByteArray &bytes, const QString &html) {
    if (!m_directory) m_directory = std::make_unique<QTemporaryDir>(QDir::tempPath() + "/fomawrite-publishing-XXXXXX");
    if (!m_directory->isValid()) return {{"ok", false}, {"error", "Could not create private publishing preview."}};
    const QString path = m_directory->filePath(QString::number(generation) + "." + format);
    QFile output(path);
    if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() || !output.flush()) {
        const QString error = output.errorString(); output.close(); output.remove(); return {{"ok", false}, {"error", error}};
    }
    output.close();
    m_files.append(path);
    const auto pinned = m_pinned.values();
    int retired = 0;
    for (auto it = m_files.end(); it != m_files.begin();) {
        --it;
        if (pinned.contains(*it) || ++retired <= 4) continue;
        QFile::remove(*it);
        it = m_files.erase(it);
    }
    return {{"ok", true}, {"url", QUrl::fromLocalFile(path)}, {"anchors", format == "pdf" ? publishingAnchors(bytes, html) : QVariantMap{}}};
}
QVariantMap Publisher::preview(const QString &format) {
    if (!m_source.hasDocument() || (format != "html" && format != "pdf")) return {{"ok", false}, {"error", "Publishing preview is unavailable."}};
    QString error, warning;
    QString html = this->html(&error, true, &warning);
    if (!error.isEmpty()) return {{"ok", false}, {"error", error}};
    const quint64 generation = ++m_generation;
    QByteArray bytes;
    if (format == "pdf") bytes = pdfBytes(html, &error);
    else { html.replace("<head>", "<head><base href=\"" + m_source.documentBaseUrl().toString(QUrl::FullyEncoded).toHtmlEscaped() + "\">"); bytes = html.toUtf8(); }
    if (!error.isEmpty()) return {{"ok", false}, {"error", error}};
    auto result = storeOutput(generation, format, bytes, html);
    if (!warning.isEmpty()) result.insert("warning", warning);
    return result;
}
void Publisher::cancelPreview(QObject *consumer) {
    // Disconnect first: cancellation can finish synchronously.
    disconnect(m_connections.take(consumer));
    m_requests.remove(consumer);
    m_pendingKeys.remove(consumer);
    if (auto *renderer = m_renderers.value(consumer, nullptr)) renderer->cancel();
}
void Publisher::invalidateDocument() {
    ++m_documentGeneration;
    for (QObject *consumer : m_requests.keys()) cancelPreview(consumer);
    emit documentIdentityChanged();
}
void Publisher::invalidateSettings() {
    ++m_settingsGeneration;
    for (QObject *consumer : m_requests.keys()) cancelPreview(consumer);
}
QVariantMap Publisher::requestPreview(const QString &format, QObject *consumer, const QString &requestIdentity) {
    const QUrl documentUrl = m_source.documentUrl();
    const QString identityNow = documentIdentity();
    const int sourceRevision = m_source.sourceRevision();
    const QByteArray sourceHash = m_source.sourceHash();
    const quint64 settingsGeneration = m_settingsGeneration;
    QVariantMap identity{{"documentIdentity", identityNow}, {"documentUrl", documentUrl},
                         {"sourceHash", QString::fromLatin1(sourceHash.toHex())}, {"sourceRevision", sourceRevision},
                         {"requestIdentity", requestIdentity}, {"format", format}};
    const auto identified = [&identity](QVariantMap result) {
        for (auto it = identity.cbegin(); it != identity.cend(); ++it) result.insert(it.key(), it.value());
        return result;
    };
    if (!consumer) return identified(preview(format));
    // Web-only consumers need the same lifetime cleanup as PDF consumers.
    if (!m_consumers.contains(consumer)) {
        m_consumers.insert(consumer);
        connect(consumer, &QObject::destroyed, this, [this, consumer] {
            cancelPreview(consumer);
            m_previewCache.remove(consumer);
            m_pinned.remove(consumer);
            m_consumers.remove(consumer);
            auto *renderer = m_renderers.take(consumer);
            if (renderer) renderer->deleteLater();
        });
    }
    if (!m_source.hasDocument() || (format != "html" && format != "pdf")) {
        cancelPreview(consumer);
        return identified({{"ok", false}, {"error", "Publishing preview is unavailable."}});
    }
    QString error, warning;
    QByteArray fingerprint;
    QString html = this->html(&error, true, &warning, &fingerprint);
    if (!error.isEmpty()) {
        cancelPreview(consumer);
        return identified({{"ok", false}, {"error", error}});
    }
    const QUrl baseUrl = m_source.documentBaseUrl();
    // Bind cached output to the canonical source and document lifetime, even
    // when two sources produce identical HTML. The fingerprint tracks the
    // theme CSS, settings and image assets without hashing the embedded bytes.
    const QByteArray key = QCryptographicHash::hash(
        (format + QChar::Null + identityNow + QChar::Null
         + documentUrl.toString(QUrl::FullyEncoded) + QChar::Null
         + QString::fromLatin1(sourceHash.toHex()) + QChar::Null
         + baseUrl.toString(QUrl::FullyEncoded) + QChar::Null
         + warning + QChar::Null + QString::fromLatin1(fingerprint.toHex())).toUtf8(), QCryptographicHash::Sha256);
    identity.insert("renderKey", QString::fromLatin1(key.toHex()));
    const auto cached = m_previewCache.constFind(consumer);
    if (cached != m_previewCache.cend() && cached->key == key
            && QFileInfo::exists(cached->result.value("url").toUrl().toLocalFile())) {
        // The output can be reused; the caller's revision token belongs to this
        // request, rather than the request that originally populated the cache.
        const auto result = identified(cached->result);
        cancelPreview(consumer);
        return result;
    }
    const QByteArray pendingKey = QCryptographicHash::hash(key + '\0' + requestIdentity.toUtf8(), QCryptographicHash::Sha256);
    if (m_pendingKeys.value(consumer) == pendingKey && m_requests.contains(consumer))
        return identified({{"ok", true}, {"pending", true}, {"requestId", m_requests.value(consumer)}});
    cancelPreview(consumer);
    const quint64 generation = ++m_generation;
    identity.insert("requestId", generation);
    if (format == "html") {
        html.replace("<head>", "<head><base href=\"" + baseUrl.toString(QUrl::FullyEncoded).toHtmlEscaped() + "\">");
        auto result = identified(storeOutput(generation, format, html.toUtf8(), html));
        if (!warning.isEmpty()) result.insert("warning", warning);
        if (result.value("ok").toBool()) {
            m_pinned[consumer] = result.value("url").toUrl().toLocalFile();
            m_previewCache[consumer] = {key, result};
        }
        return result;
    }
    auto *renderer = m_renderers.value(consumer, nullptr);
    if (!renderer) {
        renderer = new PublishingPdf(this);
        m_renderers.insert(consumer, renderer);
    }
    m_requests[consumer] = generation;
    m_pendingKeys[consumer] = pendingKey;
    m_connections[consumer] = connect(renderer, &PublishingPdf::finished, this,
        [this, consumer, generation, html, warning, key, identity, identityNow, documentUrl,
         sourceHash, settingsGeneration](quint64 id, const QByteArray &bytes, const QString &error) {
        if (id != generation || m_requests.value(consumer) != generation) return;
        disconnect(m_connections.take(consumer));
        m_requests.remove(consumer);
        m_pendingKeys.remove(consumer);
        // Reject changes that occurred while the consumer's debounce timer was
        // waiting to submit a replacement request. Never publish stale output.
        if (documentIdentity() != identityNow || m_source.documentUrl() != documentUrl
                || m_settingsGeneration != settingsGeneration || m_source.sourceHash() != sourceHash)
            return;
        auto result = error.isEmpty() ? storeOutput(generation, "pdf", bytes, html)
                                      : QVariantMap{{"ok", false}, {"error", error}};
        for (auto it = identity.cbegin(); it != identity.cend(); ++it) result.insert(it.key(), it.value());
        if (!warning.isEmpty()) result.insert("warning", warning);
        if (result.value("ok").toBool()) {
            m_pinned[consumer] = result.value("url").toUrl().toLocalFile();
            m_previewCache[consumer] = {key, result};
        }
        emit previewReady(generation, result);
    });
    renderer->render(generation, html, baseUrl, m_source.outputPageLayout());
    return identified({{"ok", true}, {"pending", true}, {"requestId", generation}});
}
