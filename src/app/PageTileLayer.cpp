#include "app/PageTileLayer.h"

#include "app/PageTileHub.h"
#include "app/PdfDocumentAccess.h"

#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGNode>
#include <QSGTexture>

#include <algorithm>
#include <cmath>

namespace {

QSGImageNode *makeImageNode(QQuickWindow *window, const QImage &image,
                            const QRectF &rect) {
  if (window == nullptr || image.isNull() || rect.isEmpty()) {
    return nullptr;
  }
  QSGTexture *texture = window->createTextureFromImage(
      image, QQuickWindow::TextureCanUseAtlas);
  if (texture == nullptr) {
    return nullptr;
  }
  QSGImageNode *node = window->createImageNode();
  node->setTexture(texture);
  node->setOwnsTexture(true);
  node->setRect(rect);
  node->setFiltering(QSGTexture::Linear);
  node->setMipmapFiltering(QSGTexture::None);
  return node;
}

} // namespace

PageTileLayer::PageTileLayer(QQuickItem *parent) : QQuickItem(parent) {
  setFlag(ItemHasContents, true);
}

PageTileLayer::~PageTileLayer() {
  PageTileHub::instance().cancelFor(this);
  ++m_epoch;
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

  Tile tile;
  tile.key = job.key;
  tile.clip = job.clip;
  tile.basis = m_sourceSize;
  tile.image = image;
  tile.current = true;

  bool replaced = false;
  for (int i = 0; i < m_tiles.size(); ++i) {
    if (m_tiles[i].key == job.key && m_tiles[i].current &&
        m_tiles[i].basis == m_sourceSize) {
      m_tiles[i] = tile;
      replaced = true;
      break;
    }
  }
  if (!replaced) {
    m_tiles.append(tile);
  }
  if (m_status != Ready) {
    setStatus(Ready);
  }
  pruneStaleTiles();
  markDirty();
}

void PageTileLayer::setDocument(QObject *document) {
  if (m_documentObj == document) {
    return;
  }
  m_documentObj = document;
  resolveDocument();
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
  QQuickItem::geometryChange(newGeometry, oldGeometry);
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

QSGNode *PageTileLayer::updatePaintNode(QSGNode *oldNode,
                                        UpdatePaintNodeData *) {
  delete oldNode;
  auto *root = new QSGNode;

  QQuickWindow *win = window();
  if (win == nullptr || width() <= 0 || height() <= 0) {
    return root;
  }

  if (!m_placeholder.isNull() && !m_placeholderBasis.isEmpty()) {
    const QRectF dest(0, 0, width(), height());
    if (QSGImageNode *node = makeImageNode(win, m_placeholder, dest)) {
      root->appendChildNode(node);
    }
  }

  QVector<const Tile *> current;
  QVector<const Tile *> stale;
  current.reserve(m_tiles.size());
  stale.reserve(m_tiles.size());
  for (const Tile &tile : m_tiles) {
    if (tile.image.isNull()) {
      continue;
    }
    if (tile.current && tile.basis == m_sourceSize) {
      current.append(&tile);
    } else {
      stale.append(&tile);
    }
  }

  for (const Tile *tile : stale) {
    if (QSGImageNode *node =
            makeImageNode(win, tile->image, tileDestRect(*tile))) {
      root->appendChildNode(node);
    }
  }
  for (const Tile *tile : current) {
    if (QSGImageNode *node =
            makeImageNode(win, tile->image, tileDestRect(*tile))) {
      root->appendChildNode(node);
    }
  }
  return root;
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

int PageTileLayer::tilePixelSize() const {
  const QSize scaled = m_sourceSize.isEmpty() ? scaledPageSize() : m_sourceSize;
  const int edge = qMax(scaled.width(), scaled.height());
  if (edge <= 1200) {
    return 256;
  }
  if (edge <= 2800) {
    return 512;
  }
  return 1024;
}

void PageTileLayer::rebuildSourceSize() {
  const QSize next = scaledPageSize();
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

QRectF PageTileLayer::tileDestRect(const Tile &tile) const {
  if (tile.basis.isEmpty() || width() <= 0 || height() <= 0) {
    return {};
  }
  const qreal sx = width() / qreal(tile.basis.width());
  const qreal sy = height() / qreal(tile.basis.height());
  return QRectF(tile.clip.x() * sx, tile.clip.y() * sy, tile.clip.width() * sx,
                tile.clip.height() * sy);
}

void PageTileLayer::requestPlaceholder() {
  if (m_paused || m_placeholderInflight || m_pdf == nullptr ||
      m_pdf->status() != QPdfDocument::Status::Ready || m_page < 0 ||
      m_page >= m_pdf->pageCount()) {
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
  opts.setScaledSize(scaled);
  const quint64 id = PageTileHub::instance().request(m_pdf, m_page, small, opts,
                                                     this, m_epoch);
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
  if (m_pdf == nullptr || m_pdf->status() != QPdfDocument::Status::Ready ||
      m_page < 0 || m_page >= m_pdf->pageCount()) {
    return;
  }
  if (m_sourceSize.isEmpty()) {
    rebuildSourceSize();
  }
  const QSize scaled = m_sourceSize;
  if (scaled.isEmpty()) {
    return;
  }

  requestPlaceholder();

  QRectF vis = m_visibleRect;
  if (vis.isEmpty()) {
    vis = QRectF(0, 0, width(), height());
  }
  vis = vis.intersected(QRectF(0, 0, width(), height()));
  if (vis.isEmpty()) {
    return;
  }

  const int tile = tilePixelSize();
  const qreal scaleX = qreal(scaled.width()) / width();
  const qreal scaleY = qreal(scaled.height()) / height();
  const qreal left = qMax(0.0, vis.left() * scaleX);
  const qreal top = qMax(0.0, vis.top() * scaleY);
  const qreal right = qMin(qreal(scaled.width()), vis.right() * scaleX);
  const qreal bottom = qMin(qreal(scaled.height()), vis.bottom() * scaleY);

  int prefX0 = kPrefetchBase;
  int prefX1 = kPrefetchBase;
  int prefY0 = kPrefetchBase;
  int prefY1 = kPrefetchBase;
  constexpr qreal kDirThreshold = 2.0;
  if (m_scrollDelta.x() > kDirThreshold) {
    prefX1 = 2;
    prefX0 = 0;
  } else if (m_scrollDelta.x() < -kDirThreshold) {
    prefX0 = 2;
    prefX1 = 0;
  }
  if (m_scrollDelta.y() > kDirThreshold) {
    prefY1 = 2;
    prefY0 = 0;
  } else if (m_scrollDelta.y() < -kDirThreshold) {
    prefY0 = 2;
    prefY1 = 0;
  }

  const int x0 = qMax(0, int(std::floor(left / tile)) - prefX0);
  const int y0 = qMax(0, int(std::floor(top / tile)) - prefY0);
  const int x1 = qMin((scaled.width() - 1) / tile,
                      int(std::floor((right - 1) / tile)) + prefX1);
  const int y1 = qMin((scaled.height() - 1) / tile,
                      int(std::floor((bottom - 1) / tile)) + prefY1);

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
      bool have = false;
      for (const Tile &existing : m_tiles) {
        if (existing.current && existing.basis == scaled &&
            existing.key == key && !existing.image.isNull()) {
          have = true;
          break;
        }
      }
      if (have || isInflight(key)) {
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
  for (const Candidate &c : candidates) {
    QPdfDocumentRenderOptions opts;
    opts.setScaledSize(scaled);
    opts.setScaledClipRect(c.clip);
    const quint64 id =
        hub.request(m_pdf, m_page, c.clip.size(), opts, this, m_epoch);
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
  QRectF vis = m_visibleRect;
  if (vis.isEmpty()) {
    vis = QRectF(0, 0, width(), height());
  }
  const int tile = tilePixelSize();
  const qreal margin =
      tile * (kPrefetchBase + 2) / (qreal(scaled.width()) / width());
  const QRectF keep = vis.adjusted(-margin, -margin, margin, margin)
                          .intersected(QRectF(0, 0, width(), height()));

  QVector<Tile> kept;
  kept.reserve(m_tiles.size());
  bool removed = false;
  for (const Tile &tileImg : m_tiles) {
    const QRectF dest = tileDestRect(tileImg);
    if (dest.intersects(keep)) {
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
    const QRectF dest(
        it.value().clip.x() * width() / qreal(scaled.width()),
        it.value().clip.y() * height() / qreal(scaled.height()),
        it.value().clip.width() * width() / qreal(scaled.width()),
        it.value().clip.height() * height() / qreal(scaled.height()));
    if (!dest.intersects(keep)) {
      dropIds.append(it.key());
    }
  }
  for (quint64 id : dropIds) {
    m_inflight.remove(id);
  }
}

void PageTileLayer::pruneStaleTiles() {
  QRectF vis = m_visibleRect;
  if (vis.isEmpty()) {
    vis = QRectF(0, 0, width(), height());
  }
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
