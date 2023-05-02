#include "ppr/common/routing_graph.h"

namespace ppr {

edge_info* edge::info(routing_graph_data& rg) const {
  return &rg.edge_infos_[info_];
}

edge_info* edge::info(routing_graph& rg) const {
  return &rg.data_->edge_infos_[info_];
}

edge_info const* edge::info(routing_graph_data const& rg) const {
  return &rg.edge_infos_[info_];
}

edge_info const* edge::info(routing_graph const& rg) const {
  return &rg.data_->edge_infos_[info_];
}

edge_info const* edge::info(
    data::vector_map<edge_info_idx_t, edge_info> const& edge_infos) const {
  return &edge_infos[info_];
}

node const* edge::from(routing_graph_data const& /*rg*/) const { return from_; }

node const* edge::to(routing_graph_data const& /*rg*/) const { return to_; }

}  // namespace ppr
