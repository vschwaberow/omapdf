#include "app/PageTileHub.h"

#include "app/DocumentLimits.h"
#include "app/PageTileLayer.h"

#include <QImage>
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#include <QPdfPageRenderer>

PageTileHub &PageTileHub::instance() {
  static PageTileHub hub;
  return hub;
}

PageTileHub::~PageTileHub() {
  m_jobs.clear();
  if (m_renderer) {
    m_renderer->setDocument(nullptr);
  }
  m_document.clear();
  m_renderer.reset();
}

PageTileHub::PageTileHub() : QObject(nullptr) {
  m_renderer = std::make_unique<QPdfPageRenderer>();
  m_renderer->setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);
  QObject::connect(
      m_renderer.get(), &QPdfPageRenderer::pageRendered, this,
      [this](int page, QSize, const QImage &image, QPdfDocumentRenderOptions,
             quint64 requestId) {
        const auto it = m_jobs.constFind(requestId);
        if (it == m_jobs.cend()) {
          return;
        }
        const Job job = it.value();
        m_jobs.erase(it);
        if (job.layer.isNull() || job.document.isNull()) {
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
  if (page < 0 || imageSize.width() < 1 || imageSize.height() < 1) {
    return 0;
  }
  if (imageSize.width() > omapdf::kMaxRequestEdgePx ||
      imageSize.height() > omapdf::kMaxRequestEdgePx) {
    return 0;
  }
  if (m_document.data() != document) {
    m_renderer->setDocument(document);
    m_document = document;
  }
  const quint64 id = m_renderer->requestPage(page, imageSize, options);
  m_jobs.insert(id, Job{layer, document, epoch});
  return id;
}

void PageTileHub::cancelFor(PageTileLayer *layer) {
  if (layer == nullptr || m_jobs.isEmpty()) {
    return;
  }
  eraseJobsIf([layer](const Job &job) { return job.layer.data() == layer; });
}

void PageTileHub::forgetDocument(QPdfDocument *document) {
  if (document == nullptr) {
    return;
  }
  if (m_document.data() == document) {
    m_renderer->setDocument(nullptr);
    m_document.clear();
  }
  eraseJobsIf(
      [document](const Job &job) { return job.document.data() == document; });
}
