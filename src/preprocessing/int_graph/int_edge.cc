#include "ppr/preprocessing/int_graph/int_graph.h"

namespace ppr::preprocessing {

edge_info* int_edge::info(int_graph& ig) const {
  return &ig.edge_infos_[info_];
}

edge_info const* int_edge::info(int_graph const& ig) const {
  return &ig.edge_infos_[info_];
}

bool int_edge::generate_sidewalks(int_graph const& ig) const {
  return info(ig)->type_ == edge_type::STREET;
}

}  // namespace ppr::preprocessing
