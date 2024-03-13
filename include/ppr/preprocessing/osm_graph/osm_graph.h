#pragma once

#include <unordered_map>

#include "ppr/common/edge.h"
#include "ppr/common/geometry/merc.h"
#include "ppr/common/level.h"

#include "ppr/preprocessing/names.h"
#include "ppr/preprocessing/osm_graph/osm_area.h"
#include "ppr/preprocessing/osm_graph/osm_edge.h"
#include "ppr/preprocessing/osm_graph/osm_node.h"

namespace ppr::preprocessing {

struct osm_graph {
  osm_graph() {
    // names[0] = empty string
    names_.emplace_back(std::string_view{});
    names_map_[""] = 0;

    // edge_infos[0] = edge info for additional edges during routing
    make_edge_info(edge_infos_, 0, edge_type::FOOTWAY, street_type::NONE,
                   crossing_type::NONE);
  }

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
        auto const type = edge.info(*this)->type_;
        if (type == edge_type::STREET) {
          node->street_edges_++;
        } else if (type == edge_type::FOOTWAY || type == edge_type::CROSSING) {
          node->footway_edges_++;
        }
      }
      for (auto* edge : node->in_edges_) {
        auto const type = edge->info(*this)->type_;
        if (type == edge_type::STREET) {
          node->street_edges_++;
        } else if (type == edge_type::FOOTWAY || type == edge_type::CROSSING) {
          node->footway_edges_++;
        }
      }
    }
  }

  std::vector<std::unique_ptr<osm_node>> nodes_;
  data::vector_map<edge_info_idx_t, edge_info> edge_infos_;
  std::vector<std::unique_ptr<osm_area>> areas_;
  names_vector_t names_;
  names_map_t names_map_;
  levels_vector_t levels_;
};

}  // namespace ppr::preprocessing
