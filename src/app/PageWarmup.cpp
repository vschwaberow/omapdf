#include "app/PageWarmup.h"

#include <QPdfDocument>
#include <QPdfPageRenderer>
#include <QUrl>

PageWarmup::PageWarmup(QObject *parent) : QObject(parent) {
  m_ownDoc = new QPdfDocument(this);
  m_renderer = new QPdfPageRenderer(this);
  m_renderer->setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);
  m_renderer->setDocument(m_ownDoc);
}

PageWarmup::~PageWarmup() = default;

void PageWarmup::setDocument(QObject *document) {
  if (m_documentObj == document) {
    return;
  }
  m_documentObj = document;
  m_ownPath.clear();
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

void PageWarmup::onDocumentStatus() {
  resolveDocument();
  warmNeighborhood();
}

void PageWarmup::resolveDocument() {
  m_pdf = nullptr;
  if (!m_documentObj) {
    m_ownDoc->close();
    m_ownPath.clear();
    return;
  }
  if (auto *doc = qobject_cast<QPdfDocument *>(m_documentObj)) {
    m_pdf = doc;
    m_renderer->setDocument(m_pdf);
    return;
  }
  const QVariant source = m_documentObj->property("source");
  if (!source.isValid()) {
    return;
  }
  const QUrl url = source.toUrl();
  const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
  if (path.isEmpty()) {
    return;
  }
  if (m_ownDoc->status() == QPdfDocument::Status::Ready && m_ownPath == path) {
    m_pdf = m_ownDoc;
    return;
  }
  m_ownPath = path;
  if (m_ownDoc->load(path) != QPdfDocument::Error::None) {
    m_pdf = nullptr;
    return;
  }
  m_pdf = m_ownDoc;
  m_renderer->setDocument(m_ownDoc);
}

void PageWarmup::warmNeighborhood() {
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
