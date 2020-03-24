#pragma once

#include <unordered_map>

#include "ppr/common/routing_graph.h"

namespace ppr::routing {

struct additional_edges {
  node* create_node(location const& loc) {
    auto const id = std::numeric_limits<node_id_t>::max() - nodes_.size() - 1;
    nodes_.emplace_back(std::make_unique<node>(make_node(id, 0, loc)));
    return nodes_.back().get();
  }

  void connect(node* a, node* b) {
    auto const d = distance(a->location_, b->location_);
    edges_.emplace_back(
        std::make_unique<edge>(make_edge(&edge_info_, a, b, d)));
    auto const a_to_b = edges_.back().get();
    edges_.emplace_back(
        std::make_unique<edge>(make_edge(&edge_info_, b, a, d)));
    auto const b_to_a = edges_.back().get();
    edge_map_[a].push_back(b_to_a);
    edge_map_[b].push_back(a_to_b);
  }

  std::vector<std::unique_ptr<node>> nodes_;
  std::vector<std::unique_ptr<edge>> edges_;
  std::unordered_map<node const*, std::vector<edge const*>> edge_map_;
  std::unordered_map<area const*, std::vector<node*>> area_nodes_;
  edge_info edge_info_ = make_edge_info(0, edge_type::FOOTWAY,
                                        street_type::NONE, crossing_type::NONE);
};

}  // namespace ppr::routing
