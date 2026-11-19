#pragma once

#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QPointF>
#include <QSizeF>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

class QPdfDocument;
class QPdfPageRenderer;
class QPdfSelection;

// The one PDF a window shows (backend.pdf). Page indices are 0-based. All
// geometry is in page view points: origin at the top-left of the page as
// displayed, y down, after the page's /Rotate and relative to its CropBox,
// which is what Qt PDF returns; rectangles are normalised here because Qt
// returns negative widths or heights on rotated pages.
class PdfSession : public QObject {
    Q_OBJECT
    // A PDF is open in the window (ready, awaiting its password, or failed to reload).
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY stateChanged)
    Q_PROPERTY(QUrl url READ url NOTIFY urlChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY urlChanged)
    Q_PROPERTY(QUrl notesUrl READ notesUrl NOTIFY urlChanged)
    Q_PROPERTY(bool notesExist READ notesExist NOTIFY notesExistChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY revisionChanged)
    Q_PROPERTY(QVariantMap info READ info NOTIFY revisionChanged)
    // Bumps on every successful load or reload, so views re-render.
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    // The page a view should scroll to (0-based); set by #page=N (1-based) links.
    Q_PROPERTY(int requestedPage READ requestedPage WRITE setRequestedPage NOTIFY requestedPageChanged)
    // The widest page width and the tallest page height, in points.
    Q_PROPERTY(QSizeF largestPageSize READ largestPageSize NOTIFY revisionChanged)
    // Find in the PDF (Cycle 161): startSearch fills searchResults page by page
    // on the event loop, so a long document never blocks the window.
    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery NOTIFY searchResultsChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    // The thumbnails column beside the pages (app setting).
    Q_PROPERTY(bool thumbnailsVisible READ thumbnailsVisible WRITE setThumbnailsVisible NOTIFY thumbnailsVisibleChanged)
    // What that column shows when the PDF has an outline: "thumbnails" or "contents" (app setting).
    Q_PROPERTY(QString sidebarView READ sidebarView WRITE setSidebarView NOTIFY sidebarViewChanged)
    // How many page renders PdfPage items have asked for (tests and diagnostics).
    Q_PROPERTY(int renderRequestCount READ renderRequestCount NOTIFY renderRequestCountChanged)
public:
    explicit PdfSession(QObject *parent = nullptr);
    ~PdfSession() override;

    static bool isPdfPath(const QString &path);
    static QUrl notesUrlFor(const QUrl &pdfUrl);
    // The key a PDF's remembered view is kept under (its canonical path while it exists).
    static QString viewKey(const QUrl &url);
    // A renamed or moved PDF keeps its remembered view; a trashed one loses it.
    static void moveRememberedView(const QString &fromKey, const QUrl &to);
    static void forgetRememberedView(const QString &key);
    // "Copy as Markdown Quote": the text as a blockquote, a blank line, then
    // [Page N](<file name>#page=N), with the page label as the link text when
    // the PDF labels the page differently. Ends with a newline.
    static QString markdownQuote(const QString &text, const QString &fileName, int page, const QString &label);
    // A file name as a Markdown link destination (spaces, parentheses and the
    // like percent-encoded).
    static QString linkDestination(const QString &fileName);

    bool active() const { return !m_url.isEmpty(); }
    QString state() const { return m_state; }
    QString errorText() const { return m_errorText; }
    QUrl url() const { return m_url; }
    QString fileName() const;
    QUrl notesUrl() const { return notesUrlFor(m_url); }
    bool notesExist() const { return m_notesExist; }
    int pageCount() const;
    QVariantMap info() const { return m_info; }
    int revision() const { return m_revision; }
    int requestedPage() const { return m_requestedPage; }
    // Always announces, so asking twice for the same page scrolls twice.
    void setRequestedPage(int page);
    // Why the last load or unlock failed (also set when the previous PDF was kept).
    QString lastError() const { return m_lastError; }
    QSizeF largestPageSize() const { return m_largestPageSize; }
    QVariantList searchResults() const { return m_searchResults; }
    QString searchQuery() const { return m_searchQuery; }
    bool searching() const { return m_searching; }
    bool thumbnailsVisible() const;
    void setThumbnailsVisible(bool visible);
    QString sidebarView() const;
    void setSidebarView(const QString &view);
    int renderRequestCount() const { return m_renderRequestCount; }
    void noteRenderRequest();

    // Loads `url`. On failure a PDF that was already open stays open and
    // unchanged; with nothing open the state becomes "error". A file that
    // needs a password is opened in the "password" state (returns true).
    Q_INVOKABLE bool load(const QUrl &url, const QString &password = QString());
    Q_INVOKABLE bool unlock(const QString &password);
    Q_INVOKABLE void close();
    // The open PDF now lives at `url` (renamed or moved with its notes): it is
    // read from there without counting as a newly opened PDF, so the view keeps
    // its place. False (nothing changed) when it cannot be read there.
    bool relocate(const QUrl &url);
    Q_INVOKABLE QSizeF pageSize(int page) const;
    Q_INVOKABLE QString pageLabel(int page) const;
    Q_INVOKABLE QString pageText(int page) const;
    // {text, start, length, rects}: rects holds one normalised QRectF per line.
    Q_INVOKABLE QVariantMap selectionBetween(int page, QPointF from, QPointF to) const;
    Q_INVOKABLE QVariantMap selectionAt(int page, int start, int length) const;
    Q_INVOKABLE QVariantMap wordAt(int page, QPointF point) const;
    // The direction the page's text runs in the view, in degrees clockwise:
    // 0 left to right, 90 downwards, 180 right to left, 270 upwards (pages
    // turned by /Rotate). Marks draw their lines and padding along it.
    Q_INVOKABLE int textDirection(int page) const;
    // Case-insensitive; [{page, start, length, rects, context}], at most SearchLimit.
    Q_INVOKABLE QVariantList search(const QString &query) const;
    // Replaces searchResults with the matches for `query`, found in slices.
    Q_INVOKABLE void startSearch(const QString &query);
    Q_INVOKABLE void clearSearch();
    // [{rect, page, location, url}]: page is -1 for a URL link.
    Q_INVOKABLE QVariantList links(int page) const;
    // [{title, level, page}] in document order.
    Q_INVOKABLE QVariantList outline() const;
    Q_INVOKABLE void refreshNotesExist();
    Q_INVOKABLE void copyText(const QString &text) const;
    // markdownQuote for this PDF's page (0-based) on the clipboard; false without text.
    Q_INVOKABLE QString markdownQuoteFor(int page, const QString &text) const;
    Q_INVOKABLE bool copyMarkdownQuote(int page, const QString &text) const;
    // `File.pdf#page=N` for an internal link (page 0-based), as a Markdown link destination.
    Q_INVOKABLE QString pageLinkAddress(int page) const;
    // The last page (0-based), zoom mode ("fit" or "fixed") and zoom (1 = 100%)
    // a PDF was read at, by canonical path; {} when it has none. The newest
    // RememberedViewLimit PDFs are kept. Opening a PDF requests its remembered page.
    Q_INVOKABLE QVariantMap rememberedView(const QUrl &url = QUrl()) const;
    Q_INVOKABLE void rememberView(const QUrl &url, int page, const QString &mode, qreal zoom);

    static constexpr int SearchLimit = 1000;
    static constexpr int RememberedViewLimit = 200;

    QPdfDocument *document() const { return m_document.get(); }
    // Shared by every PdfPage of this session; replaced on each load.
    QPdfPageRenderer *renderer() const { return m_renderer.get(); }

signals:
    void activeChanged();
    void stateChanged();
    void urlChanged();
    void notesExistChanged();
    void revisionChanged();
    void requestedPageChanged();
    void searchResultsChanged();
    void searchingChanged();
    void thumbnailsVisibleChanged();
    void sidebarViewChanged();
    // relocate() moved the PDF from `from`; emitted before urlChanged.
    void relocated(const QUrl &from);
    void renderRequestCountChanged();
    // A different PDF was opened (not a reload of the same file).
    void opened();

private:
    bool ready() const;
    QVariantMap selectionMap(int page, const QPdfSelection &selection) const;
    QVariantList lineRects(int page, int start, int length) const;
    void setState(const QString &state, const QString &error = QString());
    void adopt(std::unique_ptr<QPdfDocument> document, const QUrl &url, const QString &password, const QString &state);
    void readInfo();
    void watch();
    void reloadFromDisk();
    QVariantList pageMatches(int page, const QString &query, int limit) const;
    void searchSlice();
    void resetSearch();
    std::unique_ptr<QPdfDocument> m_document;
    std::unique_ptr<QPdfPageRenderer> m_renderer;
    QUrl m_url;
    QString m_password;
    QString m_state = QStringLiteral("none");
    QString m_errorText;
    QString m_lastError;
    bool m_notesExist = false;
    QVariantMap m_info;
    int m_revision = 0;
    int m_requestedPage = 0;
    QSizeF m_largestPageSize;
    QString m_searchQuery;
    QVariantList m_searchResults;
    int m_searchPage = 0;
    bool m_searching = false;
    QTimer m_searchTimer;
    int m_renderRequestCount = 0;
    mutable QHash<int, QString> m_pageText;
    // Each character's box (view points) by page, for hit-testing points.
    const QList<QRectF> &characterBoxes(int page) const;
    int characterNear(int page, QPointF point) const;
    mutable QHash<int, QList<QRectF>> m_characterBoxes;
    mutable QHash<int, int> m_textDirections;
    QFileSystemWatcher m_watcher;
    QTimer m_reloadTimer;
};
