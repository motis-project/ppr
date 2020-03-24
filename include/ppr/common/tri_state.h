#pragma once

#include <cstdint>

namespace ppr {

namespace tri_state {
// enum tri_state : uint8_t { UNKNOWN, NO, YES };
using tri_state = uint8_t;
constexpr tri_state UNKNOWN = 0;
constexpr tri_state NO = 1;
constexpr tri_state YES = 2;
}  // namespace tri_state

inline tri_state::tri_state tri_or(tri_state::tri_state a,
                                   tri_state::tri_state b) {
  if (a == tri_state::YES || b == tri_state::YES) {
    return tri_state::YES;
  } else if (a == tri_state::NO || b == tri_state::NO) {
    return tri_state::NO;
  } else {
    return tri_state::UNKNOWN;
  }
}

}  // namespace ppr
