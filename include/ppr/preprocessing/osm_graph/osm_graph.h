#pragma once

#include <unordered_map>

#include "ppr/common/geometry/merc.h"
#include "ppr/preprocessing/names.h"
#include "ppr/preprocessing/osm_graph/osm_area.h"
#include "ppr/preprocessing/osm_graph/osm_edge.h"
#include "ppr/preprocessing/osm_graph/osm_node.h"

namespace ppr::preprocessing {

struct osm_graph {
  void create_in_edges() {
    for (auto const& node : nodes_) {
      for (auto& edge : node->out_edges_) {
        edge.to_->in_edges_.emplace_back(&edge);
      }
    }
  }

  void count_edges() {
    for (auto& node : nodes_) {
      for (auto& edge : node->out_edges_) {
        if (edge.info_->type_ == edge_type::STREET) {
          node->street_edges_++;
        } else if (edge.info_->type_ == edge_type::FOOTWAY) {
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

  std::vector<std::unique_ptr<osm_node>> nodes_;
  data::vector<data::unique_ptr<edge_info>> edge_infos_;
  std::vector<std::unique_ptr<osm_area>> areas_;
  names_vector_t names_;
  names_map_t names_map_;
};

}  // namespace ppr::preprocessing
