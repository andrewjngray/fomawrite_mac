#include "publishingpdf.h"

#include <QEventLoop>
#include <QFile>
#include <QGuiApplication>
#include <QPointer>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QVariant>
#include <QWebEngineDownloadRequest>
#include <QWebEnginePage>
#include <QWebEnginePermission>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineSettings>
#include <QWebEngineUrlRequestInfo>
#include <QWebEngineUrlRequestInterceptor>
#include <optional>
#include <utility>

namespace {
constexpr int RenderTimeoutMs = 30000;

class OfflineResources final : public QWebEngineUrlRequestInterceptor
{
public:
    OfflineResources(const QUrl &documentUrl, QObject *parent)
        : QWebEngineUrlRequestInterceptor(parent), m_documentUrl(documentUrl) {}

    void interceptRequest(QWebEngineUrlRequestInfo &info) override
    {
        const QUrl url = info.requestUrl().adjusted(QUrl::RemoveFragment);
        const bool document = info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame
                && url == m_documentUrl && info.requestMethod() == QByteArrayLiteral("GET");
        const bool asset = url.scheme() == QStringLiteral("data")
                && (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeImage
                    || info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeFontResource
                    || info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeStylesheet);
        info.block(!document && !asset);
    }
private:
    const QUrl m_documentUrl;
};

class PdfPage final : public QWebEnginePage
{
public:
    PdfPage(QWebEngineProfile *profile, const QUrl &documentUrl)
        : QWebEnginePage(profile), m_documentUrl(documentUrl) {}
protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override
    {
        if (!m_loaded && isMainFrame && url == m_documentUrl
                && (type == NavigationTypeTyped || type == NavigationTypeOther)) {
            m_loaded = true;
            return true;
        }
        return false;
    }
    QWebEnginePage *createWindow(WebWindowType) override { return nullptr; }
    QStringList chooseFiles(FileSelectionMode, const QStringList &, const QStringList &) override
    { return {}; }
private:
    const QUrl m_documentUrl;
    bool m_loaded = false;
};

struct Request {
    quint64 id;
    QString html;
    QUrl baseUrl;
    QPageLayout layout;
};

// Each job owns its page and private HTML directory; the off-the-record
// profile (no cache, no cookies, no storage) is shared across jobs so every
// render does not create a new browser context. Destruction order matters:
// page before the private HTML directory, and all pages before the profile.
struct Job final : QObject {
    explicit Job(QObject *parent) : QObject(parent) {}
    QTemporaryDir directory;
    std::unique_ptr<PdfPage> page;
    QTimer timeout;
    Request request;
    bool printing = false;
};

QString offlineHtml(const QString &html, const QUrl &baseUrl)
{
    const QString policy = QStringLiteral(
        "<meta http-equiv=\"Content-Security-Policy\" content=\"default-src 'none'; "
        "script-src 'none'; style-src 'unsafe-inline' data:; img-src data:; font-src data:; "
        "connect-src 'none'; frame-src 'none'; object-src 'none'; form-action 'none'\">");
    const QString base = baseUrl.isEmpty() ? QString() : QStringLiteral("<base href=\"%1\">")
            .arg(baseUrl.toString(QUrl::FullyEncoded).toHtmlEscaped());
    const QRegularExpression head(QStringLiteral("<head\\b[^>]*>"),
                                 QRegularExpression::CaseInsensitiveOption);
    const auto match = head.match(html);
    QString result = html;
    if (match.hasMatch())
        result.insert(match.capturedEnd(), policy + base);
    else
        result = QStringLiteral("<!doctype html><html><head><meta charset=\"utf-8\">")
                + policy + base + QStringLiteral("</head><body>") + html
                + QStringLiteral("</body></html>");
    return result;
}
}

struct PublishingPdf::Private {
    std::optional<Request> pending;
    Job *job = nullptr;
    quint64 generation = 0;
    std::unique_ptr<QWebEngineProfile> profile; // destroyed after the jobs (d outlives the destructor body)
};

PublishingPdf::PublishingPdf(QObject *parent) : QObject(parent), d(std::make_unique<Private>()) {}

PublishingPdf::~PublishingPdf()
{
    ++d->generation;
    // JS/PDF callbacks can be invoked during page teardown. Invalidate the
    // active job before destroying every retired job while d still exists.
    d->job = nullptr;
    for (QObject *child : children()) {
        if (auto *job = dynamic_cast<Job *>(child)) {
            if (job->page)
                job->page->disconnect(this);
            delete job;
        }
    }
}

void PublishingPdf::render(quint64 requestId, const QString &html, const QUrl &baseUrl,
                           const QPageLayout &layout)
{
    if (d->pending) {
        const quint64 superseded = d->pending->id;
        QTimer::singleShot(0, this, [this, superseded] {
            emit finished(superseded, {}, QStringLiteral("PDF request superseded."));
        });
    }
    d->pending = Request{requestId, html, baseUrl, layout};
    if (!d->job)
        QTimer::singleShot(0, this, &PublishingPdf::startPending);
}

void PublishingPdf::startPending()
{
    if (d->job || !d->pending)
        return;
    auto *job = new Job(this);
    job->request = std::move(*d->pending);
    d->pending.reset();
    d->job = job;
    const quint64 generation = ++d->generation;
    if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance())
            || QThread::currentThread() != QCoreApplication::instance()->thread()) {
        finish(generation, {}, QStringLiteral("Browser PDF rendering requires the GUI thread."));
        return;
    }
    if (!job->request.layout.isValid() || !job->directory.isValid()) {
        finish(generation, {}, QStringLiteral("Cannot prepare the PDF page or temporary document."));
        return;
    }
    const QString path = job->directory.filePath(QStringLiteral("publishing.html"));
    QFile file(path);
    const QByteArray bytes = offlineHtml(job->request.html, job->request.baseUrl).toUtf8();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.flush()) {
        finish(generation, {}, QStringLiteral("Cannot write the temporary publishing document."));
        return;
    }
    file.close();
    const QUrl documentUrl = QUrl::fromLocalFile(path);
    if (!d->profile) {
        d->profile = std::make_unique<QWebEngineProfile>();
        d->profile->setHttpCacheType(QWebEngineProfile::NoCache);
        d->profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
        d->profile->setPersistentPermissionsPolicy(QWebEngineProfile::PersistentPermissionsPolicy::AskEveryTime);
        connect(d->profile.get(), &QWebEngineProfile::downloadRequested, this,
                [](QWebEngineDownloadRequest *download) { download->cancel(); });
    }
    job->page = std::make_unique<PdfPage>(d->profile.get(), documentUrl);
    job->page->setUrlRequestInterceptor(new OfflineResources(documentUrl, job->page.get()));
    auto *settings = job->page->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
    settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);
    settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, false);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, false);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
    settings->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled, false);
    settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, false);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    settings->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, false);
    settings->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);
    settings->setAttribute(QWebEngineSettings::PrintElementBackgrounds, true);
    settings->setAttribute(QWebEngineSettings::PrintHeaderAndFooter, false);
    settings->setAttribute(QWebEngineSettings::PreferCSSMarginsForPrinting, true);
    settings->setUnknownUrlSchemePolicy(QWebEngineSettings::DisallowUnknownUrlSchemes);
    settings->setImageAnimationPolicy(QWebEngineSettings::ImageAnimationPolicy::Disallow);
    connect(job->page.get(), &QWebEnginePage::permissionRequested, this,
            [](QWebEnginePermission permission) { permission.deny(); });
    connect(job->page.get(), &QWebEnginePage::renderProcessTerminated, this,
            [this, generation](QWebEnginePage::RenderProcessTerminationStatus, int) {
        finish(generation, {}, QStringLiteral("The browser PDF renderer stopped unexpectedly."));
    });
    connect(job->page.get(), &QWebEnginePage::loadFinished, this, [this, generation](bool ok) {
        if (!ok)
            finish(generation, {}, QStringLiteral("Cannot load the publishing document."));
        else
            pollReady(generation);
    });
    job->timeout.setSingleShot(true);
    connect(&job->timeout, &QTimer::timeout, this, [this, generation] {
        finish(generation, {}, QStringLiteral("Browser PDF rendering timed out."));
    });
    job->timeout.start(RenderTimeoutMs);
    job->page->load(documentUrl);
}

void PublishingPdf::pollReady(quint64 generation)
{
    if (generation != d->generation || !d->job || d->job->printing)
        return;
    // Isolated-world application code still runs when document JavaScript is
    // disabled. Promise results cannot be returned through runJavaScript, so
    // remember document.fonts.ready in that world and poll its settled state.
    const QString script = QStringLiteral(R"JS((() => {
        if (!globalThis.__fomawritePdfReady) {
            globalThis.__fomawritePdfReady = { fonts: !document.fonts };
            if (document.fonts) document.fonts.ready.then(
                () => { globalThis.__fomawritePdfReady.fonts = true; },
                () => { globalThis.__fomawritePdfReady.failed = true; });
        }
        return { fonts: globalThis.__fomawritePdfReady.fonts,
                 failed: !!globalThis.__fomawritePdfReady.failed,
                 images: Array.from(document.images).every(image => image.complete) };
    })())JS");
    QPointer<PublishingPdf> guard(this);
    d->job->page->runJavaScript(script, QWebEngineScript::ApplicationWorld,
                               [guard, generation](const QVariant &result) {
        if (!guard || generation != guard->d->generation || !guard->d->job)
            return;
        const QVariantMap state = result.toMap();
        if (state.isEmpty() || state.value(QStringLiteral("failed")).toBool()) {
            guard->finish(generation, {}, QStringLiteral("Cannot prepare browser PDF fonts and images."));
            return;
        }
        if (!state.value(QStringLiteral("fonts")).toBool()
                || !state.value(QStringLiteral("images")).toBool()) {
            QTimer::singleShot(40, guard, [guard, generation] {
                if (guard)
                    guard->pollReady(generation);
            });
            return;
        }
        guard->d->job->printing = true;
        guard->d->job->page->printToPdf([guard, generation](const QByteArray &pdf) {
            if (guard)
                guard->finish(generation, pdf, pdf.startsWith("%PDF-") ? QString()
                    : QStringLiteral("The browser could not produce a PDF."));
        }, guard->d->job->request.layout);
    });
}

void PublishingPdf::finish(quint64 generation, const QByteArray &pdf, const QString &error)
{
    if (generation != d->generation || !d->job)
        return;
    Job *job = d->job;
    const quint64 id = job->request.id;
    d->job = nullptr;
    ++d->generation;
    job->timeout.stop();
    if (job->page)
        job->page->disconnect(this);
    job->deleteLater();
    QPointer<PublishingPdf> guard(this);
    emit finished(id, error.isEmpty() ? pdf : QByteArray(), error);
    if (guard)
        QTimer::singleShot(0, this, &PublishingPdf::startPending);
}

void PublishingPdf::cancel()
{
    const std::optional<Request> pending = std::move(d->pending);
    d->pending.reset();
    QPointer<PublishingPdf> guard(this);
    if (d->job)
        finish(d->generation, {}, QStringLiteral("PDF rendering cancelled."));
    if (guard && pending)
        emit finished(pending->id, {}, QStringLiteral("PDF rendering cancelled."));
}

QByteArray PublishingPdf::renderBlocking(const QString &html, const QUrl &baseUrl,
                                        const QPageLayout &layout, QString *error, int timeoutMs)
{
    static thread_local bool waiting = false;
    if (error)
        error->clear();
    if (waiting || !qobject_cast<QGuiApplication *>(QCoreApplication::instance())
            || QThread::currentThread() != QCoreApplication::instance()->thread()) {
        if (error)
            *error = QStringLiteral("Browser PDF export requires the GUI thread and cannot be nested.");
        return {};
    }
    struct WaitingGuard {
        bool &value;
        explicit WaitingGuard(bool &flag) : value(flag) { value = true; }
        ~WaitingGuard() { value = false; }
    } waitingGuard(waiting);
    PublishingPdf renderer;
    QEventLoop loop;
    QTimer deadline;
    deadline.setSingleShot(true);
    QByteArray pdf;
    QString failure;
    connect(&renderer, &PublishingPdf::finished, &loop,
            [&](quint64, const QByteArray &bytes, const QString &message) {
        pdf = bytes;
        failure = message;
        loop.quit();
    });
    connect(&deadline, &QTimer::timeout, &loop, [&] {
        failure = QStringLiteral("Browser PDF export timed out.");
        loop.quit();
    });
    deadline.start(qBound(1, timeoutMs, RenderTimeoutMs));
    renderer.render(1, html, baseUrl, layout);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    if (error)
        *error = failure;
    return pdf;
}
