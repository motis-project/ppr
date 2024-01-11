#pragma once

#include "ppr/common/names.h"

namespace ppr::routing {

struct last_crossing_info {
  names_idx_t last_street_crossing_name_{};
  double last_street_crossing_distance_{};

  double last_rail_or_tram_distance_{};
};

}  // namespace ppr::routing
