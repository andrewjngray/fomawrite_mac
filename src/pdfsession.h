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
public:
    explicit PdfSession(QObject *parent = nullptr);
    ~PdfSession() override;

    static bool isPdfPath(const QString &path);
    static QUrl notesUrlFor(const QUrl &pdfUrl);

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

    // Loads `url`. On failure a PDF that was already open stays open and
    // unchanged; with nothing open the state becomes "error". A file that
    // needs a password is opened in the "password" state (returns true).
    Q_INVOKABLE bool load(const QUrl &url, const QString &password = QString());
    Q_INVOKABLE bool unlock(const QString &password);
    Q_INVOKABLE void close();
    Q_INVOKABLE QSizeF pageSize(int page) const;
    Q_INVOKABLE QString pageLabel(int page) const;
    Q_INVOKABLE QString pageText(int page) const;
    // {text, start, length, rects}: rects holds one normalised QRectF per line.
    Q_INVOKABLE QVariantMap selectionBetween(int page, QPointF from, QPointF to) const;
    Q_INVOKABLE QVariantMap selectionAt(int page, int start, int length) const;
    Q_INVOKABLE QVariantMap wordAt(int page, QPointF point) const;
    // Case-insensitive; [{page, start, length, rects, context}], at most SearchLimit.
    Q_INVOKABLE QVariantList search(const QString &query) const;
    // [{rect, page, location, url}]: page is -1 for a URL link.
    Q_INVOKABLE QVariantList links(int page) const;
    // [{title, level, page}] in document order.
    Q_INVOKABLE QVariantList outline() const;
    Q_INVOKABLE void refreshNotesExist();

    static constexpr int SearchLimit = 1000;

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
    // A different PDF was opened (not a reload of the same file).
    void opened();

private:
    bool ready() const;
    QVariantMap selectionMap(const QPdfSelection &selection) const;
    void setState(const QString &state, const QString &error = QString());
    void adopt(std::unique_ptr<QPdfDocument> document, const QUrl &url, const QString &password, const QString &state);
    void readInfo();
    void watch();
    void reloadFromDisk();
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
    mutable QHash<int, QString> m_pageText;
    // Each character's box (view points) by page, for hit-testing points.
    const QList<QRectF> &characterBoxes(int page) const;
    int characterNear(int page, QPointF point) const;
    mutable QHash<int, QList<QRectF>> m_characterBoxes;
    QFileSystemWatcher m_watcher;
    QTimer m_reloadTimer;
};
