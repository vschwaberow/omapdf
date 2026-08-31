#pragma once

#include <QtQml/qqmlregistration.h>

#include <QMetaObject>
#include <QObject>
#include <QPdfDocument>
#include <QPointer>
#include <QSize>

class QPdfPageRenderer;

class PageWarmup : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QObject *document READ document WRITE setDocument NOTIFY
                 documentChanged)
  Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY
                 currentPageChanged)
  Q_PROPERTY(QSize tileSize READ tileSize WRITE setTileSize NOTIFY tileSizeChanged)
  Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)

public:
  explicit PageWarmup(QObject *parent = nullptr);
  ~PageWarmup() override;

  [[nodiscard]] QObject *document() const { return m_documentObj; }
  void setDocument(QObject *document);

  [[nodiscard]] int currentPage() const { return m_currentPage; }
  void setCurrentPage(int page);

  [[nodiscard]] QSize tileSize() const { return m_tileSize; }
  void setTileSize(QSize size);

  [[nodiscard]] bool paused() const { return m_paused; }
  void setPaused(bool paused);

signals:
  void documentChanged();
  void currentPageChanged();
  void tileSizeChanged();
  void pausedChanged();

private:
  void resolveDocument();
  void warmNeighborhood();
  void clearPdf();
  void onPdfStatus(QPdfDocument::Status status);

  QObject *m_documentObj{nullptr};
  QPointer<QPdfDocument> m_pdf;
  QMetaObject::Connection m_statusConn;
  QPdfPageRenderer *m_renderer{nullptr};
  int m_currentPage{-1};
  QSize m_tileSize{720, 960};
  bool m_paused{false};
};
