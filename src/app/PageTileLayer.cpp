#include "app/PageTileLayer.h"

#include "app/PageTileHub.h"
#include "app/PdfDocumentAccess.h"

#include <QPainter>
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>

#include <cmath>

PageTileLayer::PageTileLayer(QQuickItem *parent) : QQuickPaintedItem(parent) {
  setRenderTarget(QQuickPaintedItem::FramebufferObject);
  setPerformanceHint(QQuickPaintedItem::FastFBOResizing, true);
}

PageTileLayer::~PageTileLayer() { PageTileHub::instance().cancelFor(this); }

void PageTileLayer::acceptTile(quint64 requestId, int page,
                               const QImage &image) {
  if (page != m_page) {
    return;
  }
  const auto it = m_inflight.constFind(requestId);
  if (it == m_inflight.cend()) {
    return;
  }
  const Inflight job = it.value();
  m_inflight.erase(it);
  if (image.isNull()) {
    return;
  }
  Tile tile;
  tile.clip = job.clip;
  tile.image = image;
  m_tiles.insert(job.key, tile);
  if (m_status != Ready) {
    setStatus(Ready);
  }
  update();
}

void PageTileLayer::setDocument(QObject *document) {
  if (m_documentObj == document) {
    return;
  }
  m_documentObj = document;
  resolveDocument();
  clearTiles();
  rebuildSourceSize();
  requestVisibleTiles();
  emit documentChanged();
}

void PageTileLayer::setPage(int page) {
  if (m_page == page) {
    return;
  }
  m_page = page;
  clearTiles();
  rebuildSourceSize();
  requestVisibleTiles();
  emit pageChanged();
}

void PageTileLayer::setRenderScale(qreal scale) {
  if (qFuzzyCompare(m_renderScale + 1.0, scale + 1.0)) {
    return;
  }
  m_renderScale = scale;
  emit renderScaleChanged();
}

void PageTileLayer::setDevicePixelRatio(qreal dpr) {
  if (dpr < 0.25) {
    dpr = 0.25;
  } else if (dpr > 4.0) {
    dpr = 4.0;
  }
  if (qFuzzyCompare(m_dpr + 1.0, dpr + 1.0)) {
    return;
  }
  m_dpr = dpr;
  emit devicePixelRatioChanged();
  if (m_paused) {
    m_pendingRescale = true;
    return;
  }
  clearTiles();
  rebuildSourceSize();
  requestVisibleTiles();
}

void PageTileLayer::setVisibleRect(const QRectF &rect) {
  if (m_visibleRect == rect) {
    return;
  }
  m_visibleRect = rect;
  if (!m_paused) {
    requestVisibleTiles();
    dropFarTiles();
  }
  emit visibleRectChanged();
}

void PageTileLayer::setPaused(bool paused) {
  if (m_paused == paused) {
    return;
  }
  m_paused = paused;
  emit pausedChanged();
  if (!m_paused) {
    if (m_pendingRescale) {
      m_pendingRescale = false;
      clearTiles();
      rebuildSourceSize();
    }
    requestVisibleTiles();
    dropFarTiles();
  }
}

void PageTileLayer::geometryChange(const QRectF &newGeometry,
                                   const QRectF &oldGeometry) {
  QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
  if (newGeometry.size() == oldGeometry.size()) {
    return;
  }
  emit paintedWidthChanged();
  emit paintedHeightChanged();
  if (m_paused) {
    m_pendingRescale = true;
    return;
  }
  clearTiles();
  rebuildSourceSize();
  requestVisibleTiles();
}

void PageTileLayer::paint(QPainter *painter) {
  if (m_tiles.isEmpty() || width() <= 0 || height() <= 0 ||
      m_sourceSize.isEmpty()) {
    return;
  }
  painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
  const qreal sx = width() / qreal(m_sourceSize.width());
  const qreal sy = height() / qreal(m_sourceSize.height());
  for (const Tile &tile : m_tiles) {
    if (tile.image.isNull() || tile.clip.isEmpty()) {
      continue;
    }
    const QRectF dest(tile.clip.x() * sx, tile.clip.y() * sy,
                      tile.clip.width() * sx, tile.clip.height() * sy);
    painter->drawImage(dest, tile.image);
  }
}

void PageTileLayer::resolveDocument() {
  if (m_statusConn) {
    QObject::disconnect(m_statusConn);
    m_statusConn = {};
  }
  m_pdf = pdfDocumentFrom(m_documentObj);
  if (m_pdf == nullptr) {
    return;
  }
  m_statusConn = QObject::connect(
      m_pdf, &QPdfDocument::statusChanged, this,
      [this](QPdfDocument::Status) {
        rebuildSourceSize();
        requestVisibleTiles();
      });
}

QSize PageTileLayer::scaledPageSize() const {
  if (width() <= 0 || height() <= 0) {
    return {};
  }
  qreal dpr = m_dpr;
  const qreal edge = width() * dpr;
  if (edge > kMaxEdge) {
    dpr *= qreal(kMaxEdge) / edge;
  }
  return QSize(qMax(1, qRound(width() * dpr)),
               qMax(1, qRound(height() * dpr)));
}

void PageTileLayer::rebuildSourceSize() {
  const QSize next = scaledPageSize();
  if (m_sourceSize == next) {
    return;
  }
  m_sourceSize = next;
  emit sourceSizeChanged();
}

void PageTileLayer::clearTiles() {
  PageTileHub::instance().cancelFor(this);
  m_tiles.clear();
  m_inflight.clear();
  setStatus(Loading);
  update();
}

void PageTileLayer::setStatus(int status) {
  if (m_status == status) {
    return;
  }
  m_status = status;
  emit statusChanged();
}

bool PageTileLayer::isInflight(const TileKey &key) const {
  for (auto it = m_inflight.cbegin(); it != m_inflight.cend(); ++it) {
    if (it.value().key == key) {
      return true;
    }
  }
  return false;
}

void PageTileLayer::requestVisibleTiles() {
  if (m_paused) {
    return;
  }
  if (m_pdf == nullptr || m_pdf->status() != QPdfDocument::Status::Ready ||
      m_page < 0 || m_page >= m_pdf->pageCount()) {
    return;
  }
  const QSize scaled = scaledPageSize();
  if (scaled.isEmpty()) {
    return;
  }

  QRectF vis = m_visibleRect;
  if (vis.isEmpty()) {
    vis = QRectF(0, 0, width(), height());
  }
  vis = vis.intersected(QRectF(0, 0, width(), height()));
  if (vis.isEmpty()) {
    return;
  }

  const qreal scaleX = qreal(scaled.width()) / width();
  const qreal scaleY = qreal(scaled.height()) / height();
  const qreal left = qMax(0.0, vis.left() * scaleX);
  const qreal top = qMax(0.0, vis.top() * scaleY);
  const qreal right = qMin(qreal(scaled.width()), vis.right() * scaleX);
  const qreal bottom = qMin(qreal(scaled.height()), vis.bottom() * scaleY);

  const int x0 = qMax(0, int(std::floor(left / kTile)) - kPrefetch);
  const int y0 = qMax(0, int(std::floor(top / kTile)) - kPrefetch);
  const int x1 = qMin((scaled.width() - 1) / kTile,
                      int(std::floor((right - 1) / kTile)) + kPrefetch);
  const int y1 = qMin((scaled.height() - 1) / kTile,
                      int(std::floor((bottom - 1) / kTile)) + kPrefetch);

  PageTileHub &hub = PageTileHub::instance();
  for (int ty = y0; ty <= y1; ++ty) {
    for (int tx = x0; tx <= x1; ++tx) {
      const TileKey key{tx, ty};
      if (m_tiles.contains(key) || isInflight(key)) {
        continue;
      }
      const int x = tx * kTile;
      const int y = ty * kTile;
      const QRect clip(x, y, qMin(kTile, scaled.width() - x),
                       qMin(kTile, scaled.height() - y));
      if (clip.isEmpty()) {
        continue;
      }
      QPdfDocumentRenderOptions opts;
      opts.setScaledSize(scaled);
      opts.setScaledClipRect(clip);
      const quint64 id = hub.request(m_pdf, m_page, clip.size(), opts, this);
      if (id == 0) {
        continue;
      }
      m_inflight.insert(id, Inflight{key, clip});
    }
  }
}

void PageTileLayer::dropFarTiles() {
  if (m_tiles.isEmpty() && m_inflight.isEmpty()) {
    return;
  }
  const QSize scaled = scaledPageSize();
  if (scaled.isEmpty() || width() <= 0 || height() <= 0) {
    return;
  }
  QRectF vis = m_visibleRect;
  if (vis.isEmpty()) {
    vis = QRectF(0, 0, width(), height());
  }
  const qreal margin =
      kTile * (kPrefetch + 1) / (qreal(scaled.width()) / width());
  const QRectF keep = vis.adjusted(-margin, -margin, margin, margin)
                          .intersected(QRectF(0, 0, width(), height()));
  const qreal scaleX = qreal(scaled.width()) / width();
  const qreal scaleY = qreal(scaled.height()) / height();
  const QRect keepPx(int(std::floor(keep.left() * scaleX)),
                     int(std::floor(keep.top() * scaleY)),
                     int(std::ceil(keep.width() * scaleX)),
                     int(std::ceil(keep.height() * scaleY)));

  QList<TileKey> drop;
  for (auto it = m_tiles.cbegin(); it != m_tiles.cend(); ++it) {
    if (!keepPx.intersects(it.value().clip)) {
      drop.append(it.key());
    }
  }
  for (const TileKey &key : drop) {
    m_tiles.remove(key);
  }
  QList<quint64> dropIds;
  for (auto it = m_inflight.cbegin(); it != m_inflight.cend(); ++it) {
    if (!keepPx.intersects(it.value().clip)) {
      dropIds.append(it.key());
    }
  }
  for (quint64 id : dropIds) {
    m_inflight.remove(id);
  }
  if (!drop.isEmpty()) {
    update();
  }
}
