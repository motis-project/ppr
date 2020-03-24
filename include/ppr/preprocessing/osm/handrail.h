#pragma once

#include "osmium/osm/tag.hpp"

#include "ppr/common/tri_state.h"

namespace ppr::preprocessing::osm {

inline tri_state::tri_state get_handrail(char const* str) {
  if (str != nullptr) {
    if (strcmp(str, "yes") == 0 || strcmp(str, "both") == 0 ||
        strcmp(str, "left") == 0 || strcmp(str, "right") == 0) {
      return tri_state::YES;
    } else if (strcmp(str, "no") == 0) {
      return tri_state::NO;
    }
  }
  return tri_state::UNKNOWN;
}

inline tri_state::tri_state get_handrail(osmium::TagList const& tags) {
  auto result = get_handrail(tags["handrail"]);
  if (result == tri_state::UNKNOWN) {
    auto left = get_handrail(tags["handrail:left"]);
    auto right = get_handrail(tags["handrail:right"]);
    auto center = get_handrail(tags["handrail:center"]);
    result = tri_or(left, tri_or(right, center));
  }
  return result;
}

}  // namespace ppr::preprocessing::osm
