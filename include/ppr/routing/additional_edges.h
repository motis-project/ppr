#pragma once

#include "ankerl/unordered_dense.h"

#include "ppr/common/routing_graph.h"

namespace ppr::routing {

struct additional_edges {
  node* create_node(location const& loc) {
    auto const id = std::numeric_limits<node_id_t>::max() - nodes_.size() - 1;
    nodes_.emplace_back(std::make_unique<node>(make_node(id, 0, loc)));
    return nodes_.back().get();
  }

  void connect(node* a, node* b, edge_info_idx_t const edge_info) {
    auto const d = distance(a->location_, b->location_);
    edges_.emplace_back(std::make_unique<edge>(make_edge(edge_info, a, b, d)));
    auto const* a_to_b = edges_.back().get();
    edges_.emplace_back(std::make_unique<edge>(make_edge(edge_info, b, a, d)));
    auto const* b_to_a = edges_.back().get();
    edge_map_[a].push_back(b_to_a);
    edge_map_[b].push_back(a_to_b);
  }

  void connect(node* a, node* b) { connect(a, b, default_edge_info_); }

  std::vector<std::unique_ptr<node>> nodes_;
  std::vector<std::unique_ptr<edge>> edges_;
  ankerl::unordered_dense::map<node const*, std::vector<edge const*>> edge_map_;
  ankerl::unordered_dense::map<area const*, std::vector<node*>> area_nodes_;
  edge_info_idx_t default_edge_info_{
      0};  // edge info created during preprocessing
};

}  // namespace ppr::routing
