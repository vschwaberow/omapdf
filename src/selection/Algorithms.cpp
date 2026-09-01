#include "selection/Algorithms.h"

#include <algorithm>
#include <cmath>

namespace omapdf::selection {
namespace {

[[nodiscard]] bool pageInRange(int page, const PdfTextSource &source) {
  return page >= 0 && page < source.pageCount();
}

[[nodiscard]] PageRange rangeFromHit(int page, PagePoint from, PagePoint to,
                                     const TextHit &hit) {
  PageRange range;
  range.page = page;
  range.from = from;
  range.to = to;
  if (hit.valid) {
    range.text = hit.text;
    range.bounds = hit.bounds;
  }
  return range;
}

[[nodiscard]] SelectionState stateFromSingleHit(int page, const TextHit &hit) {
  SelectionState state;
  if (!hit.valid) {
    return state;
  }
  const QRectF r = hit.boundingRect;
  PageRange range;
  range.page = page;
  range.from = {r.x(), r.y()};
  range.to = {r.x() + r.width(), r.y() + r.height()};
  range.text = hit.text;
  range.bounds = hit.bounds;
  state.pages.push_back(std::move(range));
  state.text = hit.text;
  state.anchorPage = page;
  return state;
}

[[nodiscard]] TextHit wordHitAt(int page, PagePoint pt,
                                  const PdfTextSource &source) {
  if (!pageInRange(page, source)) {
    return {};
  }
  const TextHit hit = source.selection(page, pt, pt);
  if (!hit.valid) {
    return {};
  }
  const TextHit all = source.allText(page);
  if (!all.valid) {
    return {};
  }
  const QString &text = all.text;
  const int i = hit.startIndex;
  if (i < 0 || i >= text.size()) {
    return {};
  }
  int start = i;
  int end = i + 1;
  while (start > 0 && isWordChar(text.at(start - 1))) {
    --start;
  }
  while (end < text.size() && isWordChar(text.at(end))) {
    ++end;
  }
  return source.selectionAtIndex(page, start, end - start);
}

[[nodiscard]] TextHit lineHitAt(int page, PagePoint pt,
                                  const PdfTextSource &source) {
  if (!pageInRange(page, source)) {
    return {};
  }
  const TextHit hit = source.selection(page, pt, pt);
  if (!hit.valid) {
    return {};
  }
  const TextHit all = source.allText(page);
  if (!all.valid) {
    return {};
  }
  const QString &text = all.text;
  const int i = hit.startIndex;
  if (i < 0 || i >= text.size()) {
    return {};
  }
  int start = i;
  int end = i + 1;
  while (start > 0 && text.at(start - 1) != QLatin1Char('\n') &&
         text.at(start - 1) != QLatin1Char('\r')) {
    --start;
  }
  while (end < text.size() && text.at(end) != QLatin1Char('\n') &&
         text.at(end) != QLatin1Char('\r')) {
    ++end;
  }
  return source.selectionAtIndex(page, start, end - start);
}

} // namespace

bool isWordChar(QChar ch) {
  if (ch.isNull()) {
    return false;
  }
  const ushort code = ch.unicode();
  return code != 32 && code != 9 && code != 10 && code != 13 && code != 160;
}

PagePoint clamp(PagePoint pt, PageSize size) {
  const double w = size.width > 0 ? size.width : 1.0;
  const double h = size.height > 0 ? size.height : 1.0;
  return {std::clamp(pt.x, 0.0, w), std::clamp(pt.y, 0.0, h)};
}

SelectionState fromHit(int page, const TextHit &hit) {
  return stateFromSingleHit(page, hit);
}

SelectionState span(int anchorPage, PagePoint anchor, int focusPage,
                    PagePoint focus, const PdfTextSource &source) {
  SelectionState state;
  if (!pageInRange(anchorPage, source) || !pageInRange(focusPage, source)) {
    return state;
  }

  const PagePoint anchorPt = clamp(anchor, source.pageSize(anchorPage));
  const PagePoint focusPt = clamp(focus, source.pageSize(focusPage));
  const int lo = std::min(anchorPage, focusPage);
  const int hi = std::max(anchorPage, focusPage);
  const bool forward = anchorPage <= focusPage;

  QString combined;
  for (int page = lo; page <= hi; ++page) {
    const PageSize sz = source.pageSize(page);
    const PagePoint br{sz.width, sz.height};
    PagePoint fromPt;
    PagePoint toPt;
    if (lo == hi) {
      fromPt = anchorPt;
      toPt = focusPt;
    } else if (page == anchorPage) {
      fromPt = anchorPt;
      toPt = forward ? br : PagePoint{0, 0};
    } else if (page == focusPage) {
      fromPt = forward ? PagePoint{0, 0} : focusPt;
      toPt = forward ? focusPt : br;
    } else {
      fromPt = {0, 0};
      toPt = br;
    }

    const TextHit hit = source.selection(page, fromPt, toPt);
    PageRange range = rangeFromHit(page, fromPt, toPt, hit);
    if (hit.valid && !hit.text.isEmpty()) {
      if (!combined.isEmpty()) {
        combined += QLatin1Char('\n');
      }
      combined += hit.text;
    }
    state.pages.push_back(std::move(range));
  }

  state.text = std::move(combined);
  state.anchorPage = anchorPage;
  return state;
}

SelectionState wordAt(int page, PagePoint pt, const PdfTextSource &source) {
  return stateFromSingleHit(page, wordHitAt(page, pt, source));
}

SelectionState lineAt(int page, PagePoint pt, const PdfTextSource &source) {
  return stateFromSingleHit(page, lineHitAt(page, pt, source));
}

PagePoint snapWordEdge(int page, PagePoint pt, bool towardStart,
                       const PdfTextSource &source) {
  const TextHit word = wordHitAt(page, pt, source);
  if (!word.valid) {
    return pt;
  }
  const QRectF r = word.boundingRect;
  if (towardStart) {
    return {r.x(), r.y() + r.height() * 0.5};
  }
  return {r.x() + r.width(), r.y() + r.height() * 0.5};
}

SelectionState selectPageAll(int page, const PdfTextSource &source) {
  if (!pageInRange(page, source)) {
    return {};
  }
  const TextHit all = source.allText(page);
  if (!all.valid) {
    return {};
  }
  const PageSize sz = source.pageSize(page);
  SelectionState state;
  PageRange range;
  range.page = page;
  range.from = {0, 0};
  range.to = {sz.width, sz.height};
  range.text = all.text;
  range.bounds = all.bounds;
  state.pages.push_back(std::move(range));
  state.text = all.text;
  state.anchorPage = page;
  return state;
}

} // namespace omapdf::selection
