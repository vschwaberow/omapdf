#pragma once

#include "selection/Types.h"

namespace omapdf::selection {

class PdfTextSource {
public:
  virtual ~PdfTextSource() = default;

  [[nodiscard]] virtual int pageCount() const = 0;
  [[nodiscard]] virtual PageSize pageSize(int page) const = 0;
  [[nodiscard]] virtual TextHit selection(int page, PagePoint a,
                                           PagePoint b) const = 0;
  [[nodiscard]] virtual TextHit selectionAtIndex(int page, int startIndex,
                                                  int maxLength) const = 0;
  [[nodiscard]] virtual TextHit allText(int page) const = 0;
};

} // namespace omapdf::selection
