#include "app/PageTileHub.h"

#include "app/PageTileLayer.h"

#include <QImage>
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#include <QPdfPageRenderer>

PageTileHub &PageTileHub::instance() {
  static PageTileHub hub;
  return hub;
}

PageTileHub::PageTileHub() : QObject(nullptr) {
  m_renderer = new QPdfPageRenderer(this);
  m_renderer->setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);
  QObject::connect(
      m_renderer, &QPdfPageRenderer::pageRendered, this,
      [this](int page, QSize, const QImage &image, QPdfDocumentRenderOptions,
             quint64 requestId) {
        const auto it = m_jobs.constFind(requestId);
        if (it == m_jobs.cend()) {
          return;
        }
        const Job job = it.value();
        m_jobs.erase(it);
        if (job.layer.isNull()) {
          return;
        }
        job.layer->acceptTile(requestId, page, image, job.epoch);
      });
}

quint64 PageTileHub::request(QPdfDocument *document, int page, QSize imageSize,
                             const QPdfDocumentRenderOptions &options,
                             PageTileLayer *layer, quint64 epoch) {
  if (document == nullptr || layer == nullptr) {
    return 0;
  }
  if (m_document != document) {
    m_renderer->setDocument(document);
    m_document = document;
  }
  const quint64 id = m_renderer->requestPage(page, imageSize, options);
  m_jobs.insert(id, Job{layer, epoch});
  return id;
}

void PageTileHub::cancelFor(PageTileLayer *layer) {
  if (layer == nullptr || m_jobs.isEmpty()) {
    return;
  }
  QList<quint64> drop;
  for (auto it = m_jobs.cbegin(); it != m_jobs.cend(); ++it) {
    if (it.value().layer.data() == layer) {
      drop.append(it.key());
    }
  }
  for (quint64 id : drop) {
    m_jobs.remove(id);
  }
}
