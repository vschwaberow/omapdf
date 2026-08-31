#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSize>

class QImage;
class QPdfDocument;
class QPdfDocumentRenderOptions;
class QPdfPageRenderer;
class PageTileLayer;

class PageTileHub : public QObject {
  Q_OBJECT

public:
  static PageTileHub &instance();

  [[nodiscard]] quint64 request(QPdfDocument *document, int page,
                                QSize imageSize,
                                const QPdfDocumentRenderOptions &options,
                                PageTileLayer *layer);
  void cancelFor(PageTileLayer *layer);

private:
  explicit PageTileHub();

  struct Job {
    QPointer<PageTileLayer> layer;
  };

  QPdfPageRenderer *m_renderer{nullptr};
  QPdfDocument *m_document{nullptr};
  QHash<quint64, Job> m_jobs;
};
