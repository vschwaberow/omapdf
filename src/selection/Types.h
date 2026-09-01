#pragma once

#include <QList>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <vector>

namespace omapdf::selection {

struct PagePoint {
  double x{0};
  double y{0};
};

struct PageSize {
  double width{0};
  double height{0};
};

struct TextHit {
  bool valid{false};
  QString text;
  QList<QPolygonF> bounds;
  QRectF boundingRect;
  int startIndex{-1};
  int endIndex{-1};
};

struct PageRange {
  int page{-1};
  PagePoint from{};
  PagePoint to{};
  QString text;
  QList<QPolygonF> bounds;
};

struct SelectionState {
  QString text;
  std::vector<PageRange> pages;
  int anchorPage{-1};

  [[nodiscard]] bool empty() const {
    return text.isEmpty() && pages.empty();
  }

  [[nodiscard]] bool multiPage() const { return pages.size() > 1; }
};

} // namespace omapdf::selection
