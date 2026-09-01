#include "selection/TextSelectionModel.h"

#include "selection/Algorithms.h"

#include <QPointF>
#include <QPolygonF>

TextSelectionModel::TextSelectionModel(QObject *parent) : QObject(parent) {}

void TextSelectionModel::setDocument(QObject *document) {
  if (m_binding.source() == document) {
    return;
  }
  m_binding.bind(document, this, &TextSelectionModel::onPdfStatus);
  refreshSource();
  clear();
  emit documentChanged();
}

void TextSelectionModel::setRenderScale(qreal scale) {
  if (qFuzzyCompare(m_renderScale, scale)) {
    return;
  }
  m_renderScale = scale;
  emit renderScaleChanged();
  emit selectionChanged();
}

QVariantMap TextSelectionModel::spanPages() const {
  QVariantMap map;
  const qreal scale = m_renderScale > 0 ? m_renderScale : 1.0;
  for (const auto &range : m_state.pages) {
    QVariantMap entry;
    entry.insert(QStringLiteral("fromPt"),
                 QPointF(range.from.x * scale, range.from.y * scale));
    entry.insert(QStringLiteral("toPt"),
                 QPointF(range.to.x * scale, range.to.y * scale));
    map.insert(QString::number(range.page), entry);
  }
  return map;
}

QVariantList TextSelectionModel::selectionGeometry() const {
  if (m_state.pages.size() != 1) {
    return {};
  }
  return boundsToVariant(m_state.pages.front().bounds);
}

void TextSelectionModel::clear() {
  if (m_state.empty() && !m_anchorValid) {
    return;
  }
  m_state = {};
  m_anchorValid = false;
  m_anchorPage = -1;
  m_anchorPoint = {};
  emit selectionChanged();
  emit anchorChanged();
}

void TextSelectionModel::setAnchor(int page, qreal x, qreal y) {
  m_anchorPage = page;
  m_anchorPoint = pagePoint(x, y);
  m_anchorValid = page >= 0;
  emit anchorChanged();
}

void TextSelectionModel::setAnchorSnapped(int page, qreal x, qreal y) {
  const auto pt = pagePoint(x, y);
  const auto snapped =
      omapdf::selection::snapWordEdge(page, pt, true, m_source);
  m_anchorPage = page;
  m_anchorPoint = snapped;
  m_anchorValid = page >= 0;
  emit anchorChanged();
}

bool TextSelectionModel::updateTo(int page, qreal x, qreal y) {
  if (!m_anchorValid) {
    return false;
  }
  applyState(omapdf::selection::span(m_anchorPage, m_anchorPoint, page,
                                     pagePoint(x, y), m_source));
  return !m_state.empty() || !m_state.pages.empty();
}

bool TextSelectionModel::selectWordAt(int page, qreal x, qreal y) {
  const auto pt = pagePoint(x, y);
  auto state = omapdf::selection::wordAt(page, pt, m_source);
  if (state.empty()) {
    clear();
    return false;
  }
  m_anchorPage = page;
  m_anchorPoint = pt;
  m_anchorValid = true;
  emit anchorChanged();
  applyState(std::move(state));
  return true;
}

bool TextSelectionModel::selectLineAt(int page, qreal x, qreal y) {
  const auto pt = pagePoint(x, y);
  auto state = omapdf::selection::lineAt(page, pt, m_source);
  if (state.empty()) {
    clear();
    return false;
  }
  m_anchorPage = page;
  m_anchorPoint = pt;
  m_anchorValid = true;
  emit anchorChanged();
  applyState(std::move(state));
  return true;
}

bool TextSelectionModel::selectAllOnPage(int page) {
  auto state = omapdf::selection::selectPageAll(page, m_source);
  if (state.empty()) {
    clear();
    return false;
  }
  m_anchorPage = page;
  m_anchorPoint = {0, 0};
  m_anchorValid = true;
  emit anchorChanged();
  applyState(std::move(state));
  return true;
}

QVariant TextSelectionModel::captureForHighlight() const {
  if (m_state.text.isEmpty() || m_state.pages.empty()) {
    return {};
  }
  if (m_state.pages.size() == 1) {
    const auto &range = m_state.pages.front();
    if (range.bounds.isEmpty()) {
      return {};
    }
    QVariantMap part;
    part.insert(QStringLiteral("page"), range.page);
    part.insert(QStringLiteral("text"), range.text);
    part.insert(QStringLiteral("geometry"), boundsToVariant(range.bounds));
    return part;
  }

  QVariantList parts;
  for (const auto &range : m_state.pages) {
    const auto hit =
        m_source.selection(range.page, range.from, range.to);
    if (!hit.valid || hit.text.isEmpty() || hit.bounds.isEmpty()) {
      continue;
    }
    QVariantMap part;
    part.insert(QStringLiteral("page"), range.page);
    part.insert(QStringLiteral("text"), hit.text);
    part.insert(QStringLiteral("geometry"), boundsToVariant(hit.bounds));
    parts.push_back(part);
  }
  return parts.isEmpty() ? QVariant{} : QVariant{parts};
}

QPointF TextSelectionModel::snapWordEdge(int page, qreal x, qreal y,
                                         bool towardStart) const {
  const auto pt =
      omapdf::selection::snapWordEdge(page, pagePoint(x, y), towardStart,
                                      m_source);
  return {pt.x, pt.y};
}

void TextSelectionModel::onPdfStatus(QPdfDocument::Status) {
  refreshSource();
  if (!m_source.document() ||
      m_source.document()->status() != QPdfDocument::Status::Ready) {
    clear();
  }
}

void TextSelectionModel::applyState(omapdf::selection::SelectionState state) {
  m_state = std::move(state);
  emit selectionChanged();
}

void TextSelectionModel::refreshSource() {
  m_source.setDocument(m_binding.document());
}

omapdf::selection::PagePoint TextSelectionModel::pagePoint(qreal x,
                                                            qreal y) const {
  return {x, y};
}

QVariantList TextSelectionModel::boundsToVariant(
    const QList<QPolygonF> &bounds) {
  QVariantList list;
  list.reserve(bounds.size());
  for (const QPolygonF &poly : bounds) {
    list.push_back(QVariant::fromValue(poly));
  }
  return list;
}
