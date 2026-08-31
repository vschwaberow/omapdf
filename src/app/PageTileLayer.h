#pragma once

#include <QtQml/qqmlregistration.h>

#include <QHash>
#include <QImage>
#include <QObject>
#include <QPair>
#include <QQuickPaintedItem>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QString>

class QPdfDocument;
class QPdfPageRenderer;

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
    QRect clip;
    QImage image;
  };

  struct Inflight {
    TileKey key;
    QRect clip;
  };

  void resolveDocument();
  void rebuildSourceSize();
  void clearTiles();
  void requestVisibleTiles();
  void dropFarTiles();
  void setStatus(int status);
  [[nodiscard]] bool isInflight(const TileKey &key) const;
  [[nodiscard]] QSize scaledPageSize() const;

  QObject *m_documentObj{nullptr};
  QPdfDocument *m_pdf{nullptr};
  QPdfDocument *m_ownDoc{nullptr};
  QPdfPageRenderer *m_renderer{nullptr};
  QString m_ownPath;
  int m_page{-1};
  qreal m_renderScale{1.0};
  qreal m_dpr{1.0};
  QRectF m_visibleRect;
  bool m_paused{false};
  bool m_pendingRescale{false};
  int m_status{Loading};
  QSize m_sourceSize;
  QHash<TileKey, Tile> m_tiles;
  QHash<quint64, Inflight> m_inflight;
  static constexpr int kTile = 512;
  static constexpr int kPrefetch = 1;
  static constexpr int kMaxEdge = 4096;
};
