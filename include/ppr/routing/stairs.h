#pragma once

#include <cmath>

#include "ppr/common/routing_graph.h"

namespace ppr::routing {

inline int edge_step_count(routing_graph_data const& rg, edge const* e) {
  auto const info = e->info(rg);
  assert(info->street_type_ == street_type::STAIRS);
  if (info->step_count_ > 0) {
    return info->step_count_;
  } else {
    return std::max(3, static_cast<int>(std::ceil(e->distance_ * 0.4)));
  }
}

}  // namespace ppr::routing
