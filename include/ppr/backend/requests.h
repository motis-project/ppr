#pragma once

#include <vector>

#include "ppr/common/location.h"
#include "ppr/routing/search_profile.h"

namespace ppr::backend {

struct route_request {
  location start_;
  location destination_;
  ppr::routing::search_profile profile_;
  bool include_infos_{};
  bool include_full_path_{};
  bool include_steps_{};
  bool include_steps_path_{};
  bool include_edges_{};
  bool include_statistics_{};
};

struct graph_request {
  std::vector<location> waypoints_;
  bool include_areas_{};
  bool include_visibility_graphs_{};
};

}  // namespace ppr::backend
