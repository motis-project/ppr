#pragma once

#include "ppr/common/geometry/merc.h"
#include "ppr/common/routing_graph.h"

namespace ppr::preprocessing {

struct int_edge;
struct int_area_point;

struct int_node {
  int_node(std::int64_t osm_id, merc const& loc, crossing_type const crossing)
      : osm_id_(osm_id), location_(loc), crossing_(crossing) {}

  void remove_incoming_edge(int_edge const* e) {
    in_edges_.erase(std::remove(begin(in_edges_), end(in_edges_), e),
                    end(in_edges_));
  }

  template <typename... Args>
  void emplace_out_edge(Args&&... args) {
    out_edges_.emplace_back(
        std::make_unique<int_edge>(std::forward<Args>(args)...));
  }

  inline bool is_special_node() const {
    return elevator_ || entrance_ || cycle_barrier_;
  }

  std::int64_t osm_id_;
  merc location_;
  crossing_type crossing_ : 3;
  bool generated_crossing_edges_ : 1 {};
  bool access_allowed_ : 1 {true};
  bool elevator_ : 1 {};
  bool entrance_ : 1 {};
  bool cycle_barrier_ : 1 {};
  door_type door_type_ : 4 {door_type::UNKNOWN};
  automatic_door_type automatic_door_type_ : 3 {automatic_door_type::UNKNOWN};
  std::uint8_t max_width_{};  // centimeters
  uint8_t footway_edges_{};
  uint8_t street_edges_{};
  node* rg_foot_node_{};

  std::vector<std::unique_ptr<int_edge>> out_edges_;
  std::vector<int_edge*> in_edges_;
};

}  // namespace ppr::preprocessing
