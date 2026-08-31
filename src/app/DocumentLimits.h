#pragma once

#include <cstdint>

namespace omapdf {

inline constexpr int kMaxPageCount = 10000;
inline constexpr std::int64_t kMaxFileBytes = 512LL * 1024 * 1024;
inline constexpr int kMaxRenderEdgePx = 4096;
inline constexpr int kMaxRequestEdgePx = 8192;
inline constexpr int kPrefetchRing = 1;
inline constexpr int kWarmupPageRadius = 2;

} // namespace omapdf
