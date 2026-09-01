#pragma once

#include "selection/PdfTextSource.h"

#include <QPdfDocument>
#include <QPointer>

namespace omapdf::selection {

class QtPdfTextSource final : public PdfTextSource {
public:
  explicit QtPdfTextSource(QPdfDocument *document = nullptr)
      : m_document(document) {}

  void setDocument(QPdfDocument *document) { m_document = document; }

  [[nodiscard]] QPdfDocument *document() const noexcept {
    return m_document.data();
  }

  [[nodiscard]] int pageCount() const override {
    return m_document ? m_document->pageCount() : 0;
  }

  [[nodiscard]] PageSize pageSize(int page) const override {
    if (!ready() || page < 0 || page >= m_document->pageCount()) {
      return {};
    }
    const QSizeF sz = m_document->pagePointSize(page);
    return {sz.width(), sz.height()};
  }

  [[nodiscard]] TextHit selection(int page, PagePoint a,
                                    PagePoint b) const override {
    if (!ready()) {
      return {};
    }
    return fromQt(m_document->getSelection(page, QPointF(a.x, a.y),
                                           QPointF(b.x, b.y)));
  }

  [[nodiscard]] TextHit selectionAtIndex(int page, int startIndex,
                                           int maxLength) const override {
    if (!ready()) {
      return {};
    }
    return fromQt(
        m_document->getSelectionAtIndex(page, startIndex, maxLength));
  }

  [[nodiscard]] TextHit allText(int page) const override {
    if (!ready()) {
      return {};
    }
    return fromQt(m_document->getAllText(page));
  }

private:
  [[nodiscard]] bool ready() const {
    return m_document && m_document->status() == QPdfDocument::Status::Ready;
  }

  [[nodiscard]] static TextHit fromQt(const QPdfSelection &sel) {
    TextHit hit;
    if (!sel.isValid()) {
      return hit;
    }
    hit.valid = true;
    hit.text = sel.text();
    hit.bounds = sel.bounds();
    hit.boundingRect = sel.boundingRectangle();
    hit.startIndex = sel.startIndex();
    hit.endIndex = sel.endIndex();
    return hit;
  }

  QPointer<QPdfDocument> m_document;
};

} // namespace omapdf::selection
