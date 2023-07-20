#pragma once

#include "osmium/osm/tag.hpp"

#include "ppr/common/tri_state.h"

namespace ppr::preprocessing::osm {

inline tri_state::tri_state get_ramp(char const* str) {
  if (str != nullptr) {
    if (strcmp(str, "yes") == 0 || strcmp(str, "automatic") == 0 ||
        strcmp(str, "manual") == 0) {
      // automatic/manual = luggage ramps
      return tri_state::YES;
    } else if (strcmp(str, "no") == 0) {
      return tri_state::NO;
    }
  }
  return tri_state::UNKNOWN;
}

// enum class ramp_type { ANY, STROLLER, BICYCLE, WHEELCHAIR, LUGGAGE };

inline tri_state::tri_state get_wheelchair_ramp(osmium::TagList const& tags) {
  return get_ramp(tags["ramp:wheelchair"]);
}

inline tri_state::tri_state get_stroller_ramp(osmium::TagList const& tags) {
  return get_ramp(tags["ramp:stroller"]);
}

}  // namespace ppr::preprocessing::osm
