#pragma once

#include <cmath>

#include "ppr/common/routing_graph.h"

namespace ppr::routing {

inline int edge_step_count(edge const* e) {
  assert(e->info_->street_type_ == street_type::STAIRS);
  if (e->info_->step_count_ > 0) {
    return e->info_->step_count_;
  } else {
    return std::max(3, static_cast<int>(std::ceil(e->distance_ * 0.4)));
  }
}

}  // namespace ppr::routing
