#include "app/PageWarmup.h"

#include "app/PdfDocumentAccess.h"

#include <QPdfPageRenderer>

PageWarmup::PageWarmup(QObject *parent) : QObject(parent) {
  m_renderer = std::make_unique<QPdfPageRenderer>();
  m_renderer->setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);
}

PageWarmup::~PageWarmup() { clearPdf(); }

void PageWarmup::setDocument(QObject *document) {
  if (m_documentObj.data() == document) {
    return;
  }
  m_documentObj = document;
  resolveDocument();
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
  m_statusConn.reset();
  m_renderer->setDocument(nullptr);
  m_pdf.clear();
}

void PageWarmup::onPdfStatus(QPdfDocument::Status status) {
  if (status == QPdfDocument::Status::Ready) {
    warmNeighborhood();
    return;
  }
  clearPdf();
}

void PageWarmup::resolveDocument() {
  clearPdf();
  if (m_documentObj.isNull()) {
    return;
  }
  m_pdf = pdfDocumentPtr(m_documentObj.data());
  m_renderer->setDocument(m_pdf.data());
  if (m_pdf.isNull()) {
    return;
  }
  m_statusConn.reset(QObject::connect(m_pdf.data(), &QPdfDocument::statusChanged,
                                      this, &PageWarmup::onPdfStatus));
}

void PageWarmup::warmNeighborhood() {
  if (m_paused) {
    return;
  }
  if (m_pdf.isNull() || m_pdf->status() != QPdfDocument::Status::Ready) {
    return;
  }
  if (m_currentPage < 0) {
    return;
  }
  const int count = m_pdf->pageCount();
  for (int delta = -2; delta <= 2; ++delta) {
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
