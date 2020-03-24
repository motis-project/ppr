#pragma once

#include <cinttypes>
#include <algorithm>

namespace ppr::preprocessing {

template <typename T>
inline uint8_t clamp_uint8(T x) {
  return static_cast<uint8_t>(std::min(std::max(x, 0), 255));
}

}  // namespace ppr::preprocessing
