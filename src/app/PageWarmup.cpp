#include "app/PageWarmup.h"

#include "app/DocumentLimits.h"

#include <QPdfPageRenderer>

PageWarmup::PageWarmup(QObject *parent) : QObject(parent) {
  m_renderer = std::make_unique<QPdfPageRenderer>();
  m_renderer->setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);
}

PageWarmup::~PageWarmup() { clearPdf(); }

void PageWarmup::setDocument(QObject *document) {
  if (m_document.source() == document) {
    return;
  }
  clearPdf();
  m_document.bind(document, this, &PageWarmup::onPdfStatus);
  m_renderer->setDocument(m_document.document());
  emit documentChanged();
  warmNeighborhood();
}

void PageWarmup::setCurrentPage(int page) {
  if (m_currentPage == page) {
    return;
  }
  m_currentPage = page;
  emit currentPageChanged();
  warmNeighborhood();
}

void PageWarmup::setTileSize(QSize size) {
  if (size.width() < 1 || size.height() < 1 || m_tileSize == size) {
    return;
  }
  m_tileSize = size;
  emit tileSizeChanged();
  warmNeighborhood();
}

void PageWarmup::setPaused(bool paused) {
  if (m_paused == paused) {
    return;
  }
  m_paused = paused;
  emit pausedChanged();
  if (!m_paused) {
    warmNeighborhood();
  }
}

void PageWarmup::clearPdf() {
  m_renderer->setDocument(nullptr);
  m_document.clear();
}

void PageWarmup::onPdfStatus(QPdfDocument::Status status) {
  if (status == QPdfDocument::Status::Ready) {
    warmNeighborhood();
    return;
  }
  clearPdf();
}

void PageWarmup::warmNeighborhood() {
  if (m_paused || !m_document.isReady() || m_currentPage < 0) {
    return;
  }
  QPdfDocument *pdf = m_document.document();
  const int count = pdf->pageCount();
  for (int delta = -omapdf::kWarmupPageRadius; delta <= omapdf::kWarmupPageRadius;
       ++delta) {
    if (delta == 0) {
      continue;
    }
    const int page = m_currentPage + delta;
    if (page < 0 || page >= count) {
      continue;
    }
    m_renderer->requestPage(page, m_tileSize);
  }
}
