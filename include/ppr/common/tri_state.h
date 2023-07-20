#pragma once

#include <cstdint>

namespace ppr {

enum class tri_state : std::uint8_t { UNKNOWN, NO, YES };

inline tri_state tri_or(tri_state a, tri_state b) {
  if (a == tri_state::YES || b == tri_state::YES) {
    return tri_state::YES;
  } else if (a == tri_state::NO || b == tri_state::NO) {
    return tri_state::NO;
  } else {
    return tri_state::UNKNOWN;
  }
}

}  // namespace ppr
