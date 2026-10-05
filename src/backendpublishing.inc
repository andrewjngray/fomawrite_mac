// Shared semantic HTML and browser pagination for publishing, export and preview.
namespace {
QString cssString(QString value) {
    value.replace('\\', "\\\\").replace('"', "\\\"").replace('\n', "\\a ").replace('\r', "");
    value.replace('<', "\\3c "); // Never let CSS text terminate its HTML style element.
    return '"' + value + '"';
}
QString pageContent(QString pattern, const QString &title) {
    pattern.replace("{title}", title);
    QStringList parts;
    const QRegularExpression token("\\{(page|pages)\\}");
    int start = 0;
    auto matches = token.globalMatch(pattern);
    while (matches.hasNext()) {
        const auto match = matches.next();
        parts << cssString(pattern.mid(start, match.capturedStart() - start));
        parts << (match.captured(1) == "page" ? "counter(page)" : "counter(pages)");
        start = match.capturedEnd();
    }
    parts << cssString(pattern.mid(start));
    return parts.join(' ');
}
QString comparablePdfText(QString text) {
    text.remove(QChar(0x00ad));
    return text.simplified();
}
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
}

QString Backend::publishingThemeName() const {
    if (m_publishingThemeId.isEmpty()) return outputTemplateName();
    for (const auto &entry : publishingThemes()) {
        const auto theme = entry.toMap();
        if (theme["id"].toString() == m_publishingThemeId) return theme["name"].toString();
    }
    return QStringLiteral("Missing theme");
}
QVariantList Backend::publishingThemes() const { return PublishingThemes::catalog(); }
bool Backend::selectPublishingTheme(const QString &id) {
    QString error, advisory;
    if (!PublishingThemes::css(id, &error, &advisory)) {
        m_publishingThemeError = error;
        emit publishingThemesChanged();
        setStatus(error);
        return false;
    }
    m_publishingThemeId = id;
    m_publishingThemeError = advisory;
    QSettings().setValue("output/publishingTheme", id);
    emit publishingThemesChanged();
    setStatus(advisory.isEmpty() ? publishingThemeName() + " publishing theme selected." : advisory);
    return true;
}
bool Backend::importPublishingTheme(const QUrl &file) {
    QString id, message;
    if (!PublishingThemes::importTheme(file, &id, &message)) {
        m_publishingThemeError = message;
        emit publishingThemesChanged();
        setStatus(message);
        return false;
    }
    watchPublishingThemes();
    if (!selectPublishingTheme(id)) return false;
    if (!message.isEmpty()) m_publishingThemeError = message;
    emit publishingThemesChanged();
    setStatus(message.isEmpty() ? publishingThemeName() + " imported for Web and PDF." : message);
    return true;
}
QByteArray Backend::watchPublishingThemes() {
    const QString folder = PublishingThemes::directory();
    QStringList paths;
    if (QDir(folder).exists() || QDir().mkpath(folder)) {
        const QString canonicalFolder = QDir(folder).canonicalPath();
        paths.append(canonicalFolder);
        QDirIterator entries(canonicalFolder, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDirIterator::Subdirectories);
        while (entries.hasNext() && paths.size() < 512) {
            const QString path = entries.next();
            if (QFileInfo(path).canonicalFilePath() == path) paths.append(path);
        }
    } else {
        const QString parent = QFileInfo(folder).absolutePath();
        if (QDir(parent).exists()) paths.append(parent);
    }
    paths.sort();
    const QStringList watched = m_publishingThemeWatcher.files() + m_publishingThemeWatcher.directories();
    QStringList removed, added;
    for (const QString &path : watched) if (!paths.contains(path)) removed.append(path);
    for (const QString &path : paths) if (!watched.contains(path)) added.append(path);
    // Re-registering the whole tree replays macOS filesystem notifications.
    // Keep unchanged watches; atomic saves still re-add dropped file watches.
    if (!removed.isEmpty()) m_publishingThemeWatcher.removePaths(removed);
    if (!added.isEmpty()) m_publishingThemeWatcher.addPaths(added);

    for (auto it = m_publishingThemeFileHashes.begin(); it != m_publishingThemeFileHashes.end();) {
        if (!paths.contains(it.key())) it = m_publishingThemeFileHashes.erase(it);
        else ++it;
    }
    QCryptographicHash snapshot(QCryptographicHash::Sha256);
    for (const QString &path : paths) {
        const QFileInfo entry(path);
        snapshot.addData(path.toUtf8());
        snapshot.addData(QByteArray(1, '\0'));
        // Directory timestamp/access notifications are not theme edits. Hash
        // file contents, caching unchanged files so font trees are not re-read
        // for each directory notification. File events invalidate this cache.
        if (entry.isFile()) {
            const QByteArray metadata = QByteArray::number(entry.size()) + ':'
                + QByteArray::number(entry.lastModified().toMSecsSinceEpoch()) + ':'
                + QByteArray::number(entry.birthTime().toMSecsSinceEpoch());
            auto cached = m_publishingThemeFileHashes.constFind(path);
            if (cached == m_publishingThemeFileHashes.cend() || cached->first != metadata) {
                QCryptographicHash content(QCryptographicHash::Sha256);
                QFile file(path);
                // The CSS importer accepts at most 128 MiB per expanded file;
                // unsupported oversized files need only discovery metadata.
                const bool readable = entry.size() <= 128 * 1024 * 1024
                    && file.open(QIODevice::ReadOnly) && content.addData(&file);
                m_publishingThemeFileHashes.insert(path, {metadata, readable ? content.result() : metadata});
            }
            snapshot.addData(m_publishingThemeFileHashes.value(path).second);
        }
        snapshot.addData(QByteArray(1, '\0'));
    }
    return snapshot.result();
}
void Backend::refreshPublishingThemes(bool force) {
    const QByteArray snapshot = watchPublishingThemes();
    if (!force && snapshot == m_publishingThemeSnapshot) return;
    m_publishingThemeSnapshot = snapshot;
    if (!m_publishingThemeId.isEmpty()) {
        QString error, advisory;
        PublishingThemes::css(m_publishingThemeId, &error, &advisory);
        m_publishingThemeError = error.isEmpty() ? advisory : error;
        if (!m_publishingThemeError.isEmpty()) setStatus(m_publishingThemeError);
    }
    // Preview consumers already listen to this signal, so selected CSS and
    // imported resources are rendered again without changing document state.
    emit publishingThemesChanged();
}
void Backend::reloadPublishingThemes() {
    refreshPublishingThemes(true);
    setStatus(m_publishingThemeError.isEmpty() ? "Publishing themes reloaded." : m_publishingThemeError);
}
bool Backend::openPublishingThemesFolder() {
    const QString path = PublishingThemes::directory();
    if (!QDir().mkpath(path) || !QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        m_publishingThemeError = "Could not open the publishing themes folder.";
        emit publishingThemesChanged();
        setStatus(m_publishingThemeError);
        return false;
    }
    watchPublishingThemes();
    return true;
}

QString Backend::publishingPrintCss() const {
    const auto layout = effectiveOutputPageLayout();
    const QSizeF size = layout.fullRect(QPageLayout::Millimeter).size();
    const auto margins = layout.margins(QPageLayout::Millimeter);
    const bool decorated = m_outputStyle != 0 && (m_outputStyle != 3 || m_customPageFurniture);
    const QString header = decorated ? pageContent(m_outputStyle == 3 ? m_outputHeader : "{title}", fileName()) : "none";
    const QString footer = decorated ? pageContent(m_outputStyle == 3 ? m_outputFooter : "{page} / {pages}", fileName()) : "none";
    return QStringLiteral(R"CSS(
@page { size: %1mm %2mm; margin: %3mm %4mm %5mm %6mm;
 @top-left { content: %7; font: 8pt Helvetica,Arial,sans-serif; color: #555; }
 @bottom-center { content: %8; font: 8pt Helvetica,Arial,sans-serif; color: #555; }
}
@media print {
 html, body { -webkit-print-color-adjust: exact; print-color-adjust: exact; }
 html body #write { max-width: none; width: auto; margin: 0; padding: 0; }
 .pagebreak { break-before: page; page-break-before: always; }
 h1,h2,h3,h4,h5,h6 { break-after: avoid; }
 p,li { orphans: 2; widows: 2; }
 tr,figure { break-inside: avoid; } thead { display: table-header-group; }
 .fomawrite-title-page { display: flex; align-items: center; justify-content: center; min-height: 80vh; break-after: page; }
}
)CSS").arg(size.width()).arg(size.height()).arg(margins.top()).arg(margins.right()).arg(margins.bottom()).arg(margins.left()).arg(header, footer);
}

QString Backend::publishingHtml(QString *error, bool preview, QString *warning) const {
    QString body = PublishingHtml::body(previewMarkdown(currentDocumentText()), error);
    if (error && !error->isEmpty()) return {};
    QString css;
    if (!m_publishingThemeId.isEmpty()) {
        auto selected = PublishingThemes::css(m_publishingThemeId, error);
        if (!selected) return {};
        css = *selected;
    } else {
        css = QStringLiteral(R"CSS(
html { font-size: %1pt; background: white; color: #24292f; }
body { margin: 0; font-family: %2, serif; line-height: %3; }
#write { max-width: 800px; padding: 32px; margin: auto; overflow-wrap: anywhere; }
h1,h2,h3,h4,h5,h6 { line-height: 1.25; margin: 1.3em 0 .6em; font-weight: 700; }
h1 { font-size: 2em; } h2 { font-size: 1.5em; } h3 { font-size: 1.25em; }
p,ul,ol,blockquote,pre,table { margin: .8em 0; }
ul,ol { padding-left: 1.7em; } li > p { margin: .3em 0; }
blockquote { border-left: 3px solid #d0d7de; color: #57606a; padding: .2em 1em; margin-left: 0; }
pre { background: #f3f4f6; padding: 1em; border-radius: 5px; white-space: pre-wrap; }
code { font-family: Menlo,monospace; background: #f3f4f6; padding: .1em .2em; }
pre code { background: transparent; padding: 0; }
table { border-collapse: collapse; width: 100%%; } th,td { border: 1px solid #d0d7de; padding: .5em; text-align: left; }
th { font-weight: bold; } tr:nth-child(even) { background: #f8f9fa; }
a { color: #245da8; } hr { border: 0; border-top: 1px solid #d0d7de; margin: 1.5em 0; }
)CSS").arg(outputPointSize()).arg(cssString(outputFont())).arg(m_outputStyle == 2 || m_outputStyle == 7 ? "2" : "1.5");
        css.replace("100%%", "100%");
        if (m_outputStyle == 7) css += "#write > p { text-indent: 3em; } h1 { text-align: center; font-size: 1em; font-weight: normal; }";
    }
    const QString foundations = "body { margin: 0; } img { max-width: 100%; height: auto; } .highlight { background: #fff2a8; } .pagebreak { height: 0; } .fomawrite-title-page { display: none; }";
    if (!m_outputCssFile.isEmpty()) {
        const auto custom = OutputCss::load(m_outputCssFile, error);
        if (!custom) return {};
        css += '\n' + *custom;
    }
    const QString styleAttribute = m_outputCssFile.isEmpty() ? QString() : " data-fomawrite-user-style";
    QString titlePage;
    if (m_outputStyle == 3 && m_outputTitlePage)
        titlePage = "<section class=\"fomawrite-title-page\"><h1>" + fileName().toHtmlEscaped() + "</h1></section>";
    // A restrictive content policy also protects exported HTML opened elsewhere.
    QString html = "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<meta http-equiv=\"Content-Security-Policy\" content=\"default-src 'none'; style-src 'unsafe-inline'; img-src data:; font-src data:; base-uri 'self'; form-action 'none'\">"
        "<title>" + fileName().toHtmlEscaped() + "</title><style" + styleAttribute + ">" + foundations + '\n' + css + publishingPrintCss()
        + "</style></head><body>" + titlePage + "<article id=\"write\">" + body + "</article></body></html>";
    return PublishingHtml::embedImages(html, documentBaseUrl(), preview, error, warning);
}
QString Backend::outputHtml(QTextDocument &, QString *error) const { return publishingHtml(error); }
QByteArray Backend::publishingPdfBytes(const QString &html, QString *error) const {
    return PublishingPdf::renderBlocking(html, documentBaseUrl(), effectiveOutputPageLayout(), error);
}
QVariantMap Backend::storePublishingOutput(quint64 generation, const QString &format, const QByteArray &bytes, const QString &html) {
    if (!m_publishingDirectory) m_publishingDirectory = std::make_unique<QTemporaryDir>(QDir::tempPath()+"/fomawrite-publishing-XXXXXX");
    if (!m_publishingDirectory->isValid()) return {{"ok",false},{"error","Could not create private publishing preview."}};
    const QString path = m_publishingDirectory->filePath(QString::number(generation)+"."+format);
    QFile output(path);
    if (!output.open(QIODevice::WriteOnly) || output.write(bytes)!=bytes.size() || !output.flush()) {
        const QString error=output.errorString(); output.close(); output.remove(); return {{"ok",false},{"error",error}};
    }
    output.close();
    m_publishingFiles.append(path);
    const auto pinned = m_publishingPinned.values();
    int retired = 0;
    for (auto it = m_publishingFiles.end(); it != m_publishingFiles.begin();) {
        --it;
        if (pinned.contains(*it) || ++retired <= 4) continue;
        QFile::remove(*it);
        it = m_publishingFiles.erase(it);
    }
    return {{"ok",true},{"url",QUrl::fromLocalFile(path)},{"anchors",format=="pdf"?publishingAnchors(bytes,html):QVariantMap{}}};
}
QVariantMap Backend::publishingPreview(const QString &format) {
    if (!m_document || (format!="html" && format!="pdf")) return {{"ok",false},{"error","Publishing preview is unavailable."}};
    QString error;
    QString warning;
    QString html=publishingHtml(&error, true, &warning);
    if (!error.isEmpty()) return {{"ok",false},{"error",error}};
    const quint64 generation=++m_publishingGeneration;
    QByteArray bytes;
    if (format=="pdf") bytes=publishingPdfBytes(html,&error);
    else { html.replace("<head>","<head><base href=\""+documentBaseUrl().toString(QUrl::FullyEncoded).toHtmlEscaped()+"\">"); bytes=html.toUtf8(); }
    if (!error.isEmpty()) return {{"ok",false},{"error",error}};
    auto result = storePublishingOutput(generation,format,bytes,html);
    if (!warning.isEmpty()) result.insert("warning", warning);
    return result;
}
void Backend::cancelPublishingPreview(QObject *consumer) {
    // Disconnect first: cancellation can finish synchronously.
    disconnect(m_publishingConnections.take(consumer));
    m_publishingRequests.remove(consumer);
    m_publishingPendingKeys.remove(consumer);
    if (auto *renderer = m_publishingRenderers.value(consumer, nullptr)) renderer->cancel();
}

void Backend::invalidatePublishingDocument() {
    ++m_publishingDocumentGeneration;
    for (QObject *consumer : m_publishingRequests.keys()) cancelPublishingPreview(consumer);
    emit publishingDocumentIdentityChanged();
}

QVariantMap Backend::requestPublishingPreview(const QString &format, QObject *consumer, const QString &requestIdentity) {
    const QUrl documentUrl = m_fileUrl;
    const QString documentIdentity = publishingDocumentIdentity();
    const int sourceRevision = documentRevision();
    const QByteArray sourceHash = QCryptographicHash::hash(currentDocumentText().toUtf8(), QCryptographicHash::Sha256);
    const quint64 settingsGeneration = m_publishingSettingsGeneration;
    QVariantMap identity{{"documentIdentity", documentIdentity}, {"documentUrl", documentUrl},
                         {"sourceHash", QString::fromLatin1(sourceHash.toHex())}, {"sourceRevision", sourceRevision},
                         {"requestIdentity", requestIdentity}, {"format", format}};
    const auto identified = [&identity](QVariantMap result) {
        for (auto it = identity.cbegin(); it != identity.cend(); ++it) result.insert(it.key(), it.value());
        return result;
    };
    if (!consumer) return identified(publishingPreview(format));
    // Web-only consumers need the same lifetime cleanup as PDF consumers.
    if (!m_publishingConsumers.contains(consumer)) {
        m_publishingConsumers.insert(consumer);
        connect(consumer, &QObject::destroyed, this, [this, consumer] {
            cancelPublishingPreview(consumer);
            m_publishingPreviewCache.remove(consumer);
            m_publishingPinned.remove(consumer);
            m_publishingConsumers.remove(consumer);
            auto *renderer = m_publishingRenderers.take(consumer);
            if (renderer) renderer->deleteLater();
        });
    }
    if (!m_document || (format != "html" && format != "pdf")) {
        cancelPublishingPreview(consumer);
        return identified({{"ok", false}, {"error", "Publishing preview is unavailable."}});
    }
    QString error, warning;
    QString html = publishingHtml(&error, true, &warning);
    if (!error.isEmpty()) {
        cancelPublishingPreview(consumer);
        return identified({{"ok", false}, {"error", error}});
    }
    const QUrl baseUrl = documentBaseUrl();
    // Bind cached output to the canonical source and document lifetime, even
    // when two sources produce identical HTML. Embedded bytes track assets.
    const QByteArray key = QCryptographicHash::hash(
        (format + QChar::Null + documentIdentity + QChar::Null
         + documentUrl.toString(QUrl::FullyEncoded) + QChar::Null
         + QString::fromLatin1(sourceHash.toHex()) + QChar::Null
         + baseUrl.toString(QUrl::FullyEncoded) + QChar::Null
         + warning + QChar::Null + html).toUtf8(), QCryptographicHash::Sha256);
    identity.insert("renderKey", QString::fromLatin1(key.toHex()));
    const auto cached = m_publishingPreviewCache.constFind(consumer);
    if (cached != m_publishingPreviewCache.cend() && cached->key == key
            && QFileInfo::exists(cached->result.value("url").toUrl().toLocalFile())) {
        // The output can be reused; the caller's revision token belongs to this
        // request, rather than the request that originally populated the cache.
        const auto result = identified(cached->result);
        cancelPublishingPreview(consumer);
        return result;
    }
    const QByteArray pendingKey = QCryptographicHash::hash(key + '\0' + requestIdentity.toUtf8(), QCryptographicHash::Sha256);
    if (m_publishingPendingKeys.value(consumer) == pendingKey && m_publishingRequests.contains(consumer))
        return identified({{"ok", true}, {"pending", true}, {"requestId", m_publishingRequests.value(consumer)}});
    cancelPublishingPreview(consumer);
    const quint64 generation = ++m_publishingGeneration;
    identity.insert("requestId", generation);
    if (format == "html") {
        html.replace("<head>", "<head><base href=\"" + baseUrl.toString(QUrl::FullyEncoded).toHtmlEscaped() + "\">");
        auto result = identified(storePublishingOutput(generation, format, html.toUtf8(), html));
        if (!warning.isEmpty()) result.insert("warning", warning);
        if (result.value("ok").toBool()) {
            m_publishingPinned[consumer] = result.value("url").toUrl().toLocalFile();
            m_publishingPreviewCache[consumer] = {key, result};
        }
        return result;
    }
    auto *renderer = m_publishingRenderers.value(consumer, nullptr);
    if (!renderer) {
        renderer = new PublishingPdf(this);
        m_publishingRenderers.insert(consumer, renderer);
    }
    m_publishingRequests[consumer] = generation;
    m_publishingPendingKeys[consumer] = pendingKey;
    m_publishingConnections[consumer] = connect(renderer, &PublishingPdf::finished, this,
        [this, consumer, generation, html, warning, key, identity, documentIdentity, documentUrl,
         sourceHash, settingsGeneration](quint64 id, const QByteArray &bytes, const QString &error) {
        if (id != generation || m_publishingRequests.value(consumer) != generation) return;
        disconnect(m_publishingConnections.take(consumer));
        m_publishingRequests.remove(consumer);
        m_publishingPendingKeys.remove(consumer);
        // Reject changes that occurred while the consumer's debounce timer was
        // waiting to submit a replacement request. Never publish stale output.
        if (publishingDocumentIdentity() != documentIdentity || m_fileUrl != documentUrl
                || m_publishingSettingsGeneration != settingsGeneration
                || QCryptographicHash::hash(currentDocumentText().toUtf8(), QCryptographicHash::Sha256) != sourceHash)
            return;
        auto result = error.isEmpty() ? storePublishingOutput(generation, "pdf", bytes, html)
                                      : QVariantMap{{"ok", false}, {"error", error}};
        for (auto it = identity.cbegin(); it != identity.cend(); ++it) result.insert(it.key(), it.value());
        if (!warning.isEmpty()) result.insert("warning", warning);
        if (result.value("ok").toBool()) {
            m_publishingPinned[consumer] = result.value("url").toUrl().toLocalFile();
            m_publishingPreviewCache[consumer] = {key, result};
        }
        emit publishingPreviewReady(generation, result);
    });
    renderer->render(generation, html, baseUrl, effectiveOutputPageLayout());
    return identified({{"ok", true}, {"pending", true}, {"requestId", generation}});
}

void Backend::paintPublishingOutput(QPagedPaintDevice &device) {
    const QPageLayout previous = m_pageLayout;
    m_pageLayout = device.pageLayout();
    const auto restore = qScopeGuard([&] { m_pageLayout = previous; });
    QString error;
    const QString html = publishingHtml(&error);
    const QByteArray bytes = error.isEmpty() ? publishingPdfBytes(html, &error) : QByteArray{};
    if (!error.isEmpty()) { setStatus(error); return; }
    QBuffer buffer; buffer.setData(bytes); buffer.open(QIODevice::ReadOnly);
    QPdfDocument pdf; pdf.load(&buffer);
    QPainter painter(&device);
    // Native printer painting uses the actual browser PDF pages. PDF export
    // itself retains Chromium's vector text, links and embedded fonts.
    const QRect pixels = device.pageLayout().paintRectPixels(device.logicalDpiX());
    const QRect fullPage = device.pageLayout().fullRectPixels(device.logicalDpiX());
    for (int page = 0; page < pdf.pageCount(); ++page) {
        if (page) device.newPage();
        const QSizeF points = pdf.pagePointSize(page);
        const QImage image = pdf.render(page, (points * (300.0 / 72.0)).toSize());
        const QRectF target(-pixels.x(), -pixels.y(), fullPage.width(), fullPage.height());
        painter.drawImage(target, image);
    }
}
