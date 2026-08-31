#pragma once

#include <QtQml/qqmlregistration.h>

#include <QObject>
#include <QSize>
#include <QString>

class QPdfDocument;
class QPdfPageRenderer;

class PageWarmup : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QObject *document READ document WRITE setDocument NOTIFY
                 documentChanged)
  Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY
                 currentPageChanged)
  Q_PROPERTY(QSize tileSize READ tileSize WRITE setTileSize NOTIFY tileSizeChanged)

public:
  explicit PageWarmup(QObject *parent = nullptr);
  ~PageWarmup() override;

  [[nodiscard]] QObject *document() const { return m_documentObj; }
  void setDocument(QObject *document);

  [[nodiscard]] int currentPage() const { return m_currentPage; }
  void setCurrentPage(int page);

  [[nodiscard]] QSize tileSize() const { return m_tileSize; }
  void setTileSize(QSize size);

signals:
  void documentChanged();
  void currentPageChanged();
  void tileSizeChanged();

private slots:
  void onDocumentStatus();

private:
  void resolveDocument();
  void warmNeighborhood();

  QObject *m_documentObj{nullptr};
  QPdfDocument *m_pdf{nullptr};
  QPdfDocument *m_ownDoc{nullptr};
  QPdfPageRenderer *m_renderer{nullptr};
  QString m_ownPath;
  int m_currentPage{-1};
  QSize m_tileSize{720, 960};
};
