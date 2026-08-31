#include "app/PageWarmup.h"

#include "app/PdfDocumentAccess.h"

#include <QPdfDocument>
#include <QPdfPageRenderer>

PageWarmup::PageWarmup(QObject *parent) : QObject(parent) {
  m_renderer = new QPdfPageRenderer(this);
  m_renderer->setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);
}

PageWarmup::~PageWarmup() = default;

void PageWarmup::setDocument(QObject *document) {
  if (m_documentObj == document) {
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
  resolveDocument();
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

void PageWarmup::onDocumentStatus() {
  resolveDocument();
  warmNeighborhood();
}

void PageWarmup::resolveDocument() {
  if (m_statusConn) {
    QObject::disconnect(m_statusConn);
    m_statusConn = {};
  }
  m_pdf = pdfDocumentFrom(m_documentObj);
  m_renderer->setDocument(m_pdf);
  if (m_pdf == nullptr) {
    return;
  }
  m_statusConn = QObject::connect(m_pdf, &QPdfDocument::statusChanged, this,
                                  &PageWarmup::onDocumentStatus);
}

void PageWarmup::warmNeighborhood() {
  if (m_paused) {
    return;
  }
  if (!m_pdf || m_pdf->status() != QPdfDocument::Status::Ready) {
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
