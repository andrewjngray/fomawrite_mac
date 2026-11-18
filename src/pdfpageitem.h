#pragma once

#include <QImage>
#include <QPointer>
#include <QQuickItem>
#include <QTimer>

class PdfSession;
class QPdfPageRenderer;
class QPdfDocumentRenderOptions;

// One PDF page (QML: Fomawrite 1.0 PdfPage). Renders the page with its
// annotations at the item's size times the window's pixel ratio on the
// session's render thread, shows a white page until the image arrives and
// holds exactly one image (as a texture once uploaded), released with the
// item. Nothing else caches pages: Qt's PdfPageImage and Image providers
// retain every page a list has shown (research/cycle-159, spike 5).
// Qt's renderer cannot cancel a queued page, so a request waits RequestDelay
// and is not made while `wanted` is false (a view sets it from what is on
// screen): a fast flick through a hundred pages queues only the pages it stops on.
class PdfPageItem : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject *session READ session WRITE setSession NOTIFY sessionChanged)
    Q_PROPERTY(int page READ page WRITE setPage NOTIFY pageChanged)
    Q_PROPERTY(bool rendered READ rendered NOTIFY renderedChanged)
    Q_PROPERTY(bool wanted READ wanted WRITE setWanted NOTIFY wantedChanged)
public:
    static constexpr int RequestDelay = 50;
    // Larger pages render at this many pixels and are drawn scaled up.
    static constexpr qint64 MaximumPixels = 12'000'000;
    explicit PdfPageItem(QQuickItem *parent = nullptr);
    ~PdfPageItem() override;

    QObject *session() const;
    void setSession(QObject *session);
    int page() const { return m_page; }
    void setPage(int page);
    bool rendered() const { return m_rendered; }
    bool wanted() const { return m_wanted; }
    void setWanted(bool wanted);

signals:
    void sessionChanged();
    void pageChanged();
    void renderedChanged();
    void wantedChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
    struct Key {
        int revision = -1, page = -1;
        QSize pixels;
        bool operator==(const Key &other) const { return revision == other.revision && page == other.page && pixels == other.pixels; }
    };
    Key currentKey() const;
    void scheduleRender(int delay);
    void requestRender();
    void clearImage();
    void setRendered(bool rendered);
    void pageRendered(int page, QSize size, const QImage &image, const QPdfDocumentRenderOptions &, quint64 requestId);
    QPointer<PdfSession> m_session;
    QMetaObject::Connection m_revisionConnection;
    QPointer<QPdfPageRenderer> m_renderer;
    QMetaObject::Connection m_rendererConnection;
    int m_page = 0;
    bool m_rendered = false;
    bool m_wanted = true;
    QImage m_image;          // waiting to become the texture; dropped once uploaded
    bool m_imageChanged = false;
    bool m_clear = false;    // drop the texture (another page, or no document)
    Key m_shown, m_pending;
    quint64 m_requestId = 0;
    QTimer m_renderTimer;
};
