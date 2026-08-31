#include "app/PageTileLayer.h"

#include "app/PageTileGeometry.h"
#include "app/PageTileHub.h"

#include <QPainter>
#include <QPdfDocumentRenderOptions>

#include <algorithm>
#include <cmath>

namespace {

using omapdf::kPrefetchRing;
using omapdf::page_tile::clampDpr;
using omapdf::page_tile::destRect;
using omapdf::page_tile::directionalPrefetch;
using omapdf::page_tile::scaledPageSize;
using omapdf::page_tile::tilePixelSize;

} // namespace

PageTileLayer::PageTileLayer(QQuickItem *parent) : QQuickPaintedItem(parent) {
  setRenderTarget(QQuickPaintedItem::FramebufferObject);
  setPerformanceHint(QQuickPaintedItem::FastFBOResizing, true);
}

PageTileLayer::~PageTileLayer() {
  PageTileHub::instance().cancelFor(this);
  clearPdf();
  ++m_epoch;
}

void PageTileLayer::upsertTile(const Tile &tile) {
  for (Tile &existing : m_tiles) {
    if (existing.key == tile.key && existing.current &&
        existing.basis == tile.basis) {
      existing = tile;
      return;
    }
  }
  m_tiles.append(tile);
}

void PageTileLayer::acceptTile(quint64 requestId, int page, const QImage &image,
                               quint64 epoch) {
  if (epoch != m_epoch || page != m_page) {
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
  if (job.placeholder) {
    m_placeholderInflight = false;
    m_placeholder = image;
    m_placeholderBasis = m_sourceSize;
    if (m_status != Ready) {
      setStatus(Ready);
    }
    markDirty();
    return;
  }

  upsertTile(Tile{job.key, job.clip, m_sourceSize, image, true});
  if (m_status != Ready) {
    setStatus(Ready);
  }
  pruneStaleTiles();
  markDirty();
}

void PageTileLayer::setDocument(QObject *document) {
  if (m_document.source() == document) {
    return;
  }
  clearPdf();
  m_document.bind(document, this, &PageTileLayer::onPdfStatus);
  clearAllTiles();
  rebuildSourceSize();
  requestVisibleTiles();
  emit documentChanged();
}

void PageTileLayer::setPage(int page) {
  if (m_page == page) {
    return;
  }
  m_page = page;
  clearAllTiles();
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
  dpr = clampDpr(dpr);
  if (qFuzzyCompare(m_dpr + 1.0, dpr + 1.0)) {
    return;
  }
  m_dpr = dpr;
  emit devicePixelRatioChanged();
  if (m_paused) {
    m_pendingRescale = true;
    return;
  }
  beginRescale();
}

void PageTileLayer::setVisibleRect(const QRectF &rect) {
  if (m_visibleRect == rect) {
    return;
  }
  if (!m_visibleRect.isEmpty()) {
    m_scrollDelta =
        QPointF(rect.center().x() - m_visibleRect.center().x(),
                rect.center().y() - m_visibleRect.center().y());
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
      beginRescale();
    } else {
      requestVisibleTiles();
      dropFarTiles();
    }
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
  beginRescale();
}

void PageTileLayer::paintTilePass(QPainter *painter, bool currentGeneration) {
  for (const Tile &tile : m_tiles) {
    if (tile.image.isNull()) {
      continue;
    }
    const bool isCurrent = tile.current && tile.basis == m_sourceSize;
    if (isCurrent != currentGeneration) {
      continue;
    }
    const QRectF dest = tileDestRect(tile);
    if (!dest.isEmpty()) {
      painter->drawImage(dest, tile.image);
    }
  }
}

void PageTileLayer::paint(QPainter *painter) {
  if (width() <= 0 || height() <= 0) {
    return;
  }
  painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

  if (!m_placeholder.isNull()) {
    painter->drawImage(QRectF(0, 0, width(), height()), m_placeholder);
  }

  paintTilePass(painter, false);
  paintTilePass(painter, true);
}

void PageTileLayer::clearPdf() {
  if (QPdfDocument *pdf = m_document.document()) {
    PageTileHub::instance().forgetDocument(pdf);
  }
  m_document.clear();
}

void PageTileLayer::onPdfStatus(QPdfDocument::Status status) {
  if (status == QPdfDocument::Status::Ready) {
    rebuildSourceSize();
    requestVisibleTiles();
    return;
  }
  PageTileHub::instance().cancelFor(this);
  clearPdf();
}

void PageTileLayer::rebuildSourceSize() {
  const QSize next = scaledPageSize(width(), height(), m_dpr);
  if (m_sourceSize == next) {
    return;
  }
  m_sourceSize = next;
  emit sourceSizeChanged();
}

void PageTileLayer::invalidateGeneration() {
  PageTileHub::instance().cancelFor(this);
  ++m_epoch;
  m_inflight.clear();
  m_placeholderInflight = false;
}

void PageTileLayer::beginRescale() {
  invalidateGeneration();
  for (Tile &tile : m_tiles) {
    tile.current = false;
  }
  rebuildSourceSize();
  requestVisibleTiles();
  markDirty();
}

void PageTileLayer::clearAllTiles() {
  invalidateGeneration();
  m_tiles.clear();
  m_placeholder = QImage();
  m_placeholderBasis = {};
  setStatus(Loading);
  markDirty();
}

void PageTileLayer::setStatus(int status) {
  if (m_status == status) {
    return;
  }
  m_status = status;
  emit statusChanged();
}

void PageTileLayer::markDirty() { update(); }

bool PageTileLayer::isInflight(const TileKey &key) const {
  for (auto it = m_inflight.cbegin(); it != m_inflight.cend(); ++it) {
    if (!it.value().placeholder && it.value().key == key) {
      return true;
    }
  }
  return false;
}

bool PageTileLayer::hasCurrentTile(const TileKey &key, QSize basis) const {
  for (const Tile &existing : m_tiles) {
    if (existing.current && existing.basis == basis && existing.key == key &&
        !existing.image.isNull()) {
      return true;
    }
  }
  return false;
}

QRectF PageTileLayer::tileDestRect(const Tile &tile) const {
  return destRect(tile.basis, tile.clip, width(), height());
}

QRectF PageTileLayer::viewportOrFull() const {
  if (!m_visibleRect.isEmpty()) {
    return m_visibleRect;
  }
  return QRectF(0, 0, width(), height());
}

void PageTileLayer::requestPlaceholder() {
  if (m_paused || m_placeholderInflight || !m_document.isReady() ||
      m_page < 0 || m_page >= m_document.document()->pageCount()) {
    return;
  }
  if (!m_placeholder.isNull() && m_placeholderBasis == m_sourceSize) {
    return;
  }
  const QSize scaled = m_sourceSize;
  if (scaled.isEmpty()) {
    return;
  }
  const QSize small(qMax(1, scaled.width() / 4), qMax(1, scaled.height() / 4));
  QPdfDocumentRenderOptions opts;
  opts.setScaledSize(small);
  const quint64 id = PageTileHub::instance().request(
      m_document.document(), m_page, small, opts, this, m_epoch);
  if (id == 0) {
    return;
  }
  m_placeholderInflight = true;
  m_inflight.insert(id, Inflight{{}, {}, true});
}

void PageTileLayer::requestVisibleTiles() {
  if (m_paused) {
    return;
  }
  requestPlaceholder();
  if (!m_document.isReady() || m_page < 0 ||
      m_page >= m_document.document()->pageCount()) {
    return;
  }
  const QSize scaled = m_sourceSize;
  if (scaled.isEmpty() || width() <= 0 || height() <= 0) {
    return;
  }

  const QRectF vis = viewportOrFull();
  const int tile = tilePixelSize(scaled);
  const qreal scaleX = qreal(scaled.width()) / width();
  const qreal scaleY = qreal(scaled.height()) / height();
  const qreal left = qMax(0.0, vis.left() * scaleX);
  const qreal top = qMax(0.0, vis.top() * scaleY);
  const qreal right = qMin(qreal(scaled.width()), vis.right() * scaleX);
  const qreal bottom = qMin(qreal(scaled.height()), vis.bottom() * scaleY);

  const auto bias = directionalPrefetch(m_scrollDelta);
  const int x0 = qMax(0, int(std::floor(left / tile)) - bias.beforeX);
  const int y0 = qMax(0, int(std::floor(top / tile)) - bias.beforeY);
  const int x1 = qMin((scaled.width() - 1) / tile,
                      int(std::floor((right - 1) / tile)) + bias.afterX);
  const int y1 = qMin((scaled.height() - 1) / tile,
                      int(std::floor((bottom - 1) / tile)) + bias.afterY);

  const qreal cx = (left + right) * 0.5;
  const qreal cy = (top + bottom) * 0.5;

  struct Candidate {
    TileKey key;
    QRect clip;
    qreal dist2;
  };
  QVector<Candidate> candidates;
  candidates.reserve((x1 - x0 + 1) * (y1 - y0 + 1));

  for (int ty = y0; ty <= y1; ++ty) {
    for (int tx = x0; tx <= x1; ++tx) {
      const TileKey key{tx, ty};
      if (hasCurrentTile(key, scaled) || isInflight(key)) {
        continue;
      }
      const int x = tx * tile;
      const int y = ty * tile;
      const QRect clip(x, y, qMin(tile, scaled.width() - x),
                       qMin(tile, scaled.height() - y));
      if (clip.isEmpty()) {
        continue;
      }
      const qreal dx = clip.center().x() - cx;
      const qreal dy = clip.center().y() - cy;
      candidates.append(Candidate{key, clip, dx * dx + dy * dy});
    }
  }

  std::sort(candidates.begin(), candidates.end(),
            [](const Candidate &a, const Candidate &b) {
              return a.dist2 < b.dist2;
            });

  PageTileHub &hub = PageTileHub::instance();
  QPdfDocument *pdf = m_document.document();
  for (const Candidate &c : candidates) {
    QPdfDocumentRenderOptions opts;
    opts.setScaledSize(scaled);
    opts.setScaledClipRect(c.clip);
    const quint64 id =
        hub.request(pdf, m_page, c.clip.size(), opts, this, m_epoch);
    if (id == 0) {
      continue;
    }
    m_inflight.insert(id, Inflight{c.key, c.clip, false});
  }
}

void PageTileLayer::dropFarTiles() {
  if (m_tiles.isEmpty() && m_inflight.isEmpty()) {
    return;
  }
  const QSize scaled = m_sourceSize;
  if (scaled.isEmpty() || width() <= 0 || height() <= 0) {
    return;
  }
  const QRectF vis = viewportOrFull();
  const int tile = tilePixelSize(scaled);
  const qreal margin =
      tile * (kPrefetchRing + 2) / (qreal(scaled.width()) / width());
  const QRectF keep = vis.adjusted(-margin, -margin, margin, margin)
                          .intersected(QRectF(0, 0, width(), height()));

  QVector<Tile> kept;
  kept.reserve(m_tiles.size());
  bool removed = false;
  for (const Tile &tileImg : m_tiles) {
    if (tileDestRect(tileImg).intersects(keep)) {
      kept.append(tileImg);
    } else {
      removed = true;
    }
  }
  if (removed) {
    m_tiles = kept;
    markDirty();
  }

  QList<quint64> dropIds;
  for (auto it = m_inflight.cbegin(); it != m_inflight.cend(); ++it) {
    if (it.value().placeholder) {
      continue;
    }
    const QRectF dest = destRect(scaled, it.value().clip, width(), height());
    if (!dest.intersects(keep)) {
      dropIds.append(it.key());
    }
  }
  for (quint64 id : dropIds) {
    m_inflight.remove(id);
  }
}

void PageTileLayer::pruneStaleTiles() {
  const QRectF vis = viewportOrFull();
  bool haveCurrentVisible = false;
  for (const Tile &tile : m_tiles) {
    if (!tile.current || tile.basis != m_sourceSize || tile.image.isNull()) {
      continue;
    }
    if (tileDestRect(tile).intersects(vis)) {
      haveCurrentVisible = true;
      break;
    }
  }
  if (!haveCurrentVisible) {
    return;
  }
  QVector<Tile> kept;
  kept.reserve(m_tiles.size());
  bool removed = false;
  for (const Tile &tile : m_tiles) {
    if (tile.current && tile.basis == m_sourceSize) {
      kept.append(tile);
    } else {
      removed = true;
    }
  }
  if (!removed) {
    return;
  }
  m_tiles = kept;
  if (m_placeholderBasis != m_sourceSize) {
    m_placeholder = QImage();
    m_placeholderBasis = {};
  }
}
