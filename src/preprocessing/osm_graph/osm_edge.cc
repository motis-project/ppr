#include "ppr/preprocessing/osm_graph/osm_graph.h"

namespace ppr::preprocessing {

edge_info* osm_edge::info(osm_graph& og) const {
  return &og.edge_infos_[info_];
}

edge_info const* osm_edge::info(osm_graph const& og) const {
  return &og.edge_infos_[info_];
}

double osm_edge::angle(bool reverse) const {
  auto const direction = reverse ? from_->location_ - to_->location_
                                 : to_->location_ - from_->location_;
  return std::atan2(direction.y(), direction.x());
}

bool osm_edge::generate_sidewalks(osm_graph const& og) const {
  return info(og)->type_ == edge_type::STREET;
}

bool osm_edge::calculate_elevation(osm_graph const& og) const {
  auto const i = info(og);
  return i->type_ != edge_type::ELEVATOR &&
         i->street_type_ != street_type::ESCALATOR &&
         i->street_type_ != street_type::MOVING_WALKWAY;
}

}  // namespace ppr::preprocessing
