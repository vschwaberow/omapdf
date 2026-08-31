#pragma once

#include <QtQml/qqmlregistration.h>

#include <QHash>
#include <QImage>
#include <QMetaObject>
#include <QObject>
#include <QPair>
#include <QPdfDocument>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QVector>

class PageTileLayer : public QQuickPaintedItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QObject *document READ document WRITE setDocument NOTIFY
                 documentChanged)
  Q_PROPERTY(int page READ page WRITE setPage NOTIFY pageChanged)
  Q_PROPERTY(int currentFrame READ page WRITE setPage NOTIFY pageChanged)
  Q_PROPERTY(qreal renderScale READ renderScale WRITE setRenderScale NOTIFY
                 renderScaleChanged)
  Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio WRITE
                 setDevicePixelRatio NOTIFY devicePixelRatioChanged)
  Q_PROPERTY(QRectF visibleRect READ visibleRect WRITE setVisibleRect NOTIFY
                 visibleRectChanged)
  Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
  Q_PROPERTY(int status READ status NOTIFY statusChanged)
  Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)
  Q_PROPERTY(qreal paintedWidth READ paintedWidth NOTIFY paintedWidthChanged)
  Q_PROPERTY(
      qreal paintedHeight READ paintedHeight NOTIFY paintedHeightChanged)

public:
  static constexpr int Ready = 1;
  static constexpr int Loading = 2;

  explicit PageTileLayer(QQuickItem *parent = nullptr);
  ~PageTileLayer() override;

  [[nodiscard]] QObject *document() const { return m_documentObj; }
  void setDocument(QObject *document);

  [[nodiscard]] int page() const { return m_page; }
  void setPage(int page);

  [[nodiscard]] qreal renderScale() const { return m_renderScale; }
  void setRenderScale(qreal scale);

  [[nodiscard]] qreal devicePixelRatio() const { return m_dpr; }
  void setDevicePixelRatio(qreal dpr);

  [[nodiscard]] QRectF visibleRect() const { return m_visibleRect; }
  void setVisibleRect(const QRectF &rect);

  [[nodiscard]] bool paused() const { return m_paused; }
  void setPaused(bool paused);

  [[nodiscard]] int status() const { return m_status; }
  [[nodiscard]] QSize sourceSize() const { return m_sourceSize; }
  [[nodiscard]] qreal paintedWidth() const { return width(); }
  [[nodiscard]] qreal paintedHeight() const { return height(); }

  void acceptTile(quint64 requestId, int page, const QImage &image,
                  quint64 epoch);

  void paint(QPainter *painter) override;

signals:
  void documentChanged();
  void pageChanged();
  void renderScaleChanged();
  void devicePixelRatioChanged();
  void visibleRectChanged();
  void pausedChanged();
  void statusChanged();
  void sourceSizeChanged();
  void paintedWidthChanged();
  void paintedHeightChanged();

protected:
  void geometryChange(const QRectF &newGeometry,
                      const QRectF &oldGeometry) override;

private:
  using TileKey = QPair<int, int>;

  struct Tile {
    TileKey key;
    QRect clip;
    QSize basis;
    QImage image;
    bool current{true};
  };

  struct Inflight {
    TileKey key;
    QRect clip;
    bool placeholder{false};
  };

  void resolveDocument();
  void rebuildSourceSize();
  void invalidateGeneration();
  void beginRescale();
  void clearAllTiles();
  void clearPdf();
  void onPdfStatus(QPdfDocument::Status status);
  void requestVisibleTiles();
  void requestPlaceholder();
  void dropFarTiles();
  void pruneStaleTiles();
  void setStatus(int status);
  void markDirty();
  [[nodiscard]] bool isInflight(const TileKey &key) const;
  [[nodiscard]] QSize scaledPageSize() const;
  [[nodiscard]] int tilePixelSize() const;
  [[nodiscard]] QRectF tileDestRect(const Tile &tile) const;

  QObject *m_documentObj{nullptr};
  QPointer<QPdfDocument> m_pdf;
  QMetaObject::Connection m_statusConn;
  int m_page{-1};
  qreal m_renderScale{1.0};
  qreal m_dpr{1.0};
  QRectF m_visibleRect;
  QPointF m_scrollDelta;
  bool m_paused{false};
  bool m_pendingRescale{false};
  bool m_placeholderInflight{false};
  int m_status{Loading};
  QSize m_sourceSize;
  quint64 m_epoch{1};
  QVector<Tile> m_tiles;
  QImage m_placeholder;
  QSize m_placeholderBasis;
  QHash<quint64, Inflight> m_inflight;
  static constexpr int kMaxEdge = 4096;
  static constexpr int kPrefetchBase = 1;
};
