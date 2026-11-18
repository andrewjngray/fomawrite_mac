#include "pdfpageitem.h"
#include "pdfsession.h"

#include <QCoreApplication>
#include <QPdfDocumentRenderOptions>
#include <QPdfPageRenderer>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGRectangleNode>
#include <QtQml/qqml.h>
#include <cmath>

// Registered as the application object is created, so the app, the test
// binary and the in-bundle view checks all know the type before Main.qml loads.
static void registerPdfQmlTypes() {
    qmlRegisterType<PdfPageItem>("Fomawrite", 1, 0, "PdfPage");
}
Q_COREAPP_STARTUP_FUNCTION(registerPdfQmlTypes)

PdfPageItem::PdfPageItem(QQuickItem *parent) : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
    m_renderTimer.setSingleShot(true);
    connect(&m_renderTimer, &QTimer::timeout, this, &PdfPageItem::requestRender);
}

PdfPageItem::~PdfPageItem() {
    disconnect(m_rendererConnection);
    disconnect(m_revisionConnection);
}

QObject *PdfPageItem::session() const { return m_session.data(); }

void PdfPageItem::setSession(QObject *session) {
    auto *pdf = qobject_cast<PdfSession *>(session);
    if (pdf == m_session) return;
    disconnect(m_revisionConnection);
    m_session = pdf;
    if (pdf) m_revisionConnection = connect(pdf, &PdfSession::revisionChanged, this, [this] { scheduleRender(RequestDelay); });
    clearImage();
    emit sessionChanged();
    scheduleRender(RequestDelay);
}

void PdfPageItem::setPage(int page) {
    if (page == m_page) return;
    m_page = page;
    clearImage();
    emit pageChanged();
    scheduleRender(RequestDelay);
}

void PdfPageItem::setWanted(bool wanted) {
    if (wanted == m_wanted) return;
    m_wanted = wanted;
    emit wantedChanged();
    scheduleRender(RequestDelay);
}

void PdfPageItem::setRendered(bool rendered) {
    if (m_rendered == rendered) return;
    m_rendered = rendered;
    emit renderedChanged();
}

PdfPageItem::Key PdfPageItem::currentKey() const {
    Key key;
    if (!m_session) return key;
    key.revision = m_session->revision();
    key.page = m_page;
    const qreal ratio = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    key.pixels = QSize(qRound(width() * ratio), qRound(height() * ratio));
    const qint64 pixels = qint64(key.pixels.width()) * key.pixels.height();
    if (pixels > MaximumPixels) {
        const qreal shrink = std::sqrt(qreal(MaximumPixels) / pixels);
        key.pixels = QSize(qMax(1, int(key.pixels.width() * shrink)), qMax(1, int(key.pixels.height() * shrink)));
    }
    return key;
}

void PdfPageItem::scheduleRender(int delay) {
    if (m_wanted) m_renderTimer.start(delay);
    else m_renderTimer.stop();
}

void PdfPageItem::clearImage() {
    m_image = QImage();
    m_imageChanged = false;
    m_clear = true;
    m_shown = Key();
    m_pending = Key();
    setRendered(false);
    update();
}

void PdfPageItem::requestRender() {
    if (!window() || !isVisible() || !m_wanted) return;
    QPdfPageRenderer *renderer = m_session ? m_session->renderer() : nullptr;
    if (!renderer || m_page < 0 || m_page >= m_session->pageCount()) {
        if (m_shown.page >= 0) clearImage();
        return;
    }
    const Key key = currentKey();
    if (key.pixels.isEmpty() || key == m_shown || key == m_pending) return;
    if (renderer != m_renderer) {
        disconnect(m_rendererConnection);
        m_renderer = renderer;
        m_rendererConnection = connect(renderer, &QPdfPageRenderer::pageRendered, this, &PdfPageItem::pageRendered);
    }
    m_pending = key;
    m_session->noteRenderRequest();
    QPdfDocumentRenderOptions options;
    options.setRenderFlags(QPdfDocumentRenderOptions::RenderFlag::Annotations);
    m_requestId = renderer->requestPage(key.page, key.pixels, options);
}

void PdfPageItem::pageRendered(int, QSize, const QImage &image, const QPdfDocumentRenderOptions &, quint64 requestId) {
    if (requestId != m_requestId || m_pending.page < 0 || sender() != m_renderer.data()) return;
    m_image = image;
    m_imageChanged = true;
    m_shown = m_pending;
    m_pending = Key();
    setRendered(!image.isNull());
    update();
    // A resize while this was rendering asks again for the size shown now.
    if (!(currentKey() == m_shown)) scheduleRender(RequestDelay);
}

void PdfPageItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() == oldGeometry.size()) return;
    update();
    // A live resize asks once it settles; the current image stretches meanwhile.
    scheduleRender(m_shown.page >= 0 ? 120 : RequestDelay);
}

void PdfPageItem::itemChange(ItemChange change, const ItemChangeData &value) {
    QQuickItem::itemChange(change, value);
    if (change == ItemSceneChange) {
        // The old window's scene graph took the node and its texture with it.
        m_shown = Key();
        m_pending = Key();
        setRendered(false);
        if (value.window) scheduleRender(RequestDelay);
    } else if (change == ItemVisibleHasChanged || change == ItemDevicePixelRatioHasChanged) {
        scheduleRender(RequestDelay);
    }
}

QSGNode *PdfPageItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    auto *root = static_cast<QSGRectangleNode *>(oldNode);
    if (!root) {
        root = window()->createRectangleNode();
        root->setColor(Qt::white);
    }
    root->setRect(boundingRect());
    auto *page = static_cast<QSGImageNode *>(root->firstChild());
    if (page && (m_clear || m_imageChanged)) {
        root->removeChildNode(page);
        delete page; // owns its texture
        page = nullptr;
    }
    m_clear = false;
    if (m_imageChanged) {
        m_imageChanged = false;
        if (!m_image.isNull()) {
            page = window()->createImageNode();
            page->setOwnsTexture(true);
            page->setTexture(window()->createTextureFromImage(m_image));
            page->setFiltering(QSGTexture::Linear);
            root->appendChildNode(page);
        }
        m_image = QImage(); // the texture is now the one copy
    }
    if (page) page->setRect(boundingRect());
    return root;
}
