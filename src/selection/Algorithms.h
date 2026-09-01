#pragma once

#include "selection/PdfTextSource.h"
#include "selection/Types.h"

#include <QChar>

namespace omapdf::selection {

[[nodiscard]] bool isWordChar(QChar ch);

[[nodiscard]] PagePoint clamp(PagePoint pt, PageSize size);

[[nodiscard]] SelectionState span(int anchorPage, PagePoint anchor,
                                  int focusPage, PagePoint focus,
                                  const PdfTextSource &source);

[[nodiscard]] SelectionState wordAt(int page, PagePoint pt,
                                    const PdfTextSource &source);

[[nodiscard]] SelectionState lineAt(int page, PagePoint pt,
                                    const PdfTextSource &source);

[[nodiscard]] PagePoint snapWordEdge(int page, PagePoint pt, bool towardStart,
                                     const PdfTextSource &source);

[[nodiscard]] SelectionState selectPageAll(int page,
                                           const PdfTextSource &source);

[[nodiscard]] SelectionState fromHit(int page, const TextHit &hit);

} // namespace omapdf::selection
