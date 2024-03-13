#pragma once

#include "ppr/common/elevation.h"
#include "ppr/common/enums.h"
#include "ppr/common/geometry/merc.h"
#include "ppr/common/level.h"
#include "ppr/common/tri_state.h"

#include "ppr/preprocessing/osm_graph/osm_edge.h"

namespace ppr::preprocessing {

struct int_node;

struct osm_node {
  osm_node(std::int64_t osm_id, merc const& loc)
      : osm_id_(osm_id), location_(loc) {}

  bool can_be_compressed() const {
    return access_allowed_ && crossing_ == crossing_type::NONE &&
           in_edges_.size() == 1 && out_edges_.size() == 1 && !area_outer_ &&
           !elevator_ && !entrance_ && !cycle_barrier_;
  }

  std::vector<osm_edge*> all_edges() {
    std::vector<osm_edge*> edges;
    edges.reserve(out_edges_.size() + in_edges_.size());
    for (auto& e : out_edges_) {
      edges.push_back(&e);
    }
    for (auto* e : in_edges_) {
      edges.push_back(e);
    }
    return edges;
  }

  merc get_merc() const { return location_; }

  bool is_exit_node() const { return exit_; }

  bool has_elevation_data() const { return elevation_ != NO_ELEVATION_DATA; }

  std::int64_t osm_id_;
  merc location_;
  bool access_allowed_ : 1 {true};
  crossing_type crossing_ : 3 {crossing_type::NONE};
  bool compressed_ : 1 {};
  bool exit_ : 1 {};  // true if connected to at least one non-area way
  bool area_outer_ : 1 {};
  bool elevator_ : 1 {};
  bool entrance_ : 1 {};
  bool cycle_barrier_ : 1 {};
  door_type door_type_ : 4 {door_type::UNKNOWN};
  automatic_door_type automatic_door_type_ : 3 {automatic_door_type::UNKNOWN};
  tri_state traffic_signals_sound_ : 2 {tri_state::UNKNOWN};
  tri_state traffic_signals_vibration_ : 2 {tri_state::UNKNOWN};
  std::uint8_t max_width_{};  // centimeters
  std::uint8_t footway_edges_{};
  std::uint8_t street_edges_{};
  elevation_t elevation_{NO_ELEVATION_DATA};
  levels levels_;
  edge_info_idx_t crossing_edge_info_{NO_EDGE_INFO};

  int_node* int_node_{};

  std::vector<osm_edge> out_edges_;
  std::vector<osm_edge*> in_edges_;
};

inline bool any_edge_between(osm_node const* a, osm_node const* b) {
  return std::any_of(begin(a->out_edges_), end(a->out_edges_),
                     [&](osm_edge const& e) { return e.to_ == b; }) ||
         std::any_of(begin(b->out_edges_), end(b->out_edges_),
                     [&](osm_edge const& e) { return e.to_ == a; });
}

}  // namespace ppr::preprocessing
