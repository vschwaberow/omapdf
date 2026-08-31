#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSize>
#include <memory>

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
                                PageTileLayer *layer, quint64 epoch);
  void cancelFor(PageTileLayer *layer);
  void forgetDocument(QPdfDocument *document);

private:
  explicit PageTileHub();
  ~PageTileHub() override;

  struct Job {
    QPointer<PageTileLayer> layer;
    QPointer<QPdfDocument> document;
    quint64 epoch{0};
  };

  template <typename Predicate>
  void eraseJobsIf(Predicate &&shouldErase) {
    QList<quint64> drop;
    for (auto it = m_jobs.cbegin(); it != m_jobs.cend(); ++it) {
      if (shouldErase(it.value())) {
        drop.append(it.key());
      }
    }
    for (quint64 id : drop) {
      m_jobs.remove(id);
    }
  }

  std::unique_ptr<QPdfPageRenderer> m_renderer;
  QPointer<QPdfDocument> m_document;
  QHash<quint64, Job> m_jobs;
};
