#include "ppr/preprocessing/osm_graph/osm_edge.h"
#include "ppr/preprocessing/osm_graph/osm_node.h"

namespace ppr::preprocessing {

double osm_edge::angle(bool reverse) const {
  auto const direction = reverse ? from_->location_ - to_->location_
                                 : to_->location_ - from_->location_;
  return std::atan2(direction.y(), direction.x());
}

}  // namespace ppr::preprocessing
