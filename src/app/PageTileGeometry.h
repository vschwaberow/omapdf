#pragma once

#include "app/DocumentLimits.h"

#include <QtGlobal>

#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>

#include <algorithm>
#include <cmath>

namespace omapdf::page_tile {

inline constexpr qreal kMinDpr = 0.25;
inline constexpr qreal kMaxDpr = 4.0;
inline constexpr qreal kScrollDirThreshold = 2.0;

[[nodiscard]] inline qreal clampDpr(qreal dpr) {
  return std::clamp(dpr, kMinDpr, kMaxDpr);
}

[[nodiscard]] inline QSize scaledPageSize(qreal width, qreal height,
                                          qreal dpr) {
  if (width <= 0 || height <= 0 || !qIsFinite(dpr) || dpr <= 0) {
    return {};
  }
  qreal effectiveDpr = dpr;
  const qreal edge = width * effectiveDpr;
  if (!qIsFinite(edge) || edge <= 0) {
    return {};
  }
  if (edge > kMaxRenderEdgePx) {
    effectiveDpr *= qreal(kMaxRenderEdgePx) / edge;
  }
  const int w = qRound(width * effectiveDpr);
  const int h = qRound(height * effectiveDpr);
  if (w < 1 || h < 1) {
    return {};
  }
  return QSize(w, h);
}

[[nodiscard]] inline int tilePixelSize(QSize scaled) {
  const int edge = qMax(scaled.width(), scaled.height());
  if (edge <= 1200) {
    return 256;
  }
  if (edge <= 2800) {
    return 512;
  }
  return 1024;
}

[[nodiscard]] inline QRectF destRect(QSize basis, const QRect &clip,
                                     qreal itemWidth, qreal itemHeight) {
  if (basis.isEmpty() || itemWidth <= 0 || itemHeight <= 0) {
    return {};
  }
  const qreal sx = itemWidth / qreal(basis.width());
  const qreal sy = itemHeight / qreal(basis.height());
  return QRectF(clip.x() * sx, clip.y() * sy, clip.width() * sx,
                clip.height() * sy);
}

struct PrefetchBias {
  int beforeX{kPrefetchRing};
  int afterX{kPrefetchRing};
  int beforeY{kPrefetchRing};
  int afterY{kPrefetchRing};
};

[[nodiscard]] inline PrefetchBias directionalPrefetch(QPointF scrollDelta) {
  PrefetchBias bias;
  if (scrollDelta.x() > kScrollDirThreshold) {
    bias.afterX = 2;
    bias.beforeX = 0;
  } else if (scrollDelta.x() < -kScrollDirThreshold) {
    bias.beforeX = 2;
    bias.afterX = 0;
  }
  if (scrollDelta.y() > kScrollDirThreshold) {
    bias.afterY = 2;
    bias.beforeY = 0;
  } else if (scrollDelta.y() < -kScrollDirThreshold) {
    bias.beforeY = 2;
    bias.afterY = 0;
  }
  return bias;
}

} // namespace omapdf::page_tile
