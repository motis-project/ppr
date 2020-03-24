#pragma once

#include <algorithm>

#include "ppr/common/geometry/merc.h"
#include "ppr/common/routing_graph.h"
#include "ppr/preprocessing/int_graph/int_area.h"
#include "ppr/preprocessing/int_graph/int_edge.h"
#include "ppr/preprocessing/int_graph/int_node.h"
#include "ppr/preprocessing/names.h"
#include "ppr/preprocessing/options.h"
#include "ppr/preprocessing/statistics.h"

namespace ppr::preprocessing {

struct int_graph {
  void create_in_edges() {
    for (auto const& node : nodes_) {
      for (auto& edge : node->out_edges_) {
        edge->to_->in_edges_.emplace_back(edge.get());
      }
    }
  }

  void count_edges() {
    for (auto& node : nodes_) {
      for (auto& edge : node->out_edges_) {
        if (edge->info_->type_ == edge_type::STREET) {
          node->street_edges_++;
        } else if (edge->info_->type_ == edge_type::FOOTWAY) {
          node->footway_edges_++;
        }
      }
      for (auto edge : node->in_edges_) {
        if (edge->info_->type_ == edge_type::STREET) {
          node->street_edges_++;
        } else if (edge->info_->type_ == edge_type::FOOTWAY) {
          node->footway_edges_++;
        }
      }
    }
  }

  std::vector<std::unique_ptr<int_node>> nodes_;
  data::vector<data::unique_ptr<edge_info>> edge_infos_;
  names_vector_t names_;
  names_map_t names_map_;
  std::vector<int_area> areas_;
};

struct osm_graph;
int_graph build_int_graph(osm_graph&, options const&, statistics&);

}  // namespace ppr::preprocessing
