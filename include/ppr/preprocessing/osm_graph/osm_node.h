#pragma once

#include "ppr/common/elevation.h"
#include "ppr/common/geometry/merc.h"
#include "ppr/preprocessing/osm_graph/osm_edge.h"

namespace ppr::preprocessing {

struct int_node;

struct osm_node {
  osm_node(std::int64_t osm_id, merc const& loc)
      : osm_id_(osm_id),
        location_(loc),
        access_allowed_(true),
        crossing_(crossing_type::NONE),
        compressed_(false),
        exit_(false),
        area_outer_(false),
        elevator_(false),
        footway_edges_(0),
        street_edges_(0),
        elevation_(NO_ELEVATION_DATA),
        int_node_(nullptr) {}

  bool can_be_compressed() const {
    return access_allowed_ && crossing_ == crossing_type::NONE &&
           in_edges_.size() == 1 && out_edges_.size() == 1 && !area_outer_ &&
           !elevator_;
  }

  std::vector<osm_edge*> all_edges() {
    std::vector<osm_edge*> edges;
    edges.reserve(out_edges_.size() + in_edges_.size());
    for (auto& e : out_edges_) {
      edges.push_back(&e);
    }
    for (auto e : in_edges_) {
      edges.push_back(e);
    }
    return edges;
  }

  merc get_merc() const { return location_; }

  bool is_exit_node() const { return exit_; }

  bool has_elevation_data() const { return elevation_ != NO_ELEVATION_DATA; }

  std::int64_t osm_id_;
  merc location_;
  bool access_allowed_ : 1;
  crossing_type::crossing_type crossing_ : 3;
  bool compressed_ : 1;
  bool exit_ : 1;  // true if connected to at least one non-area way
  bool area_outer_ : 1;
  bool elevator_ : 1;
  uint8_t footway_edges_;
  uint8_t street_edges_;
  elevation_t elevation_;
  int_node* int_node_;

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
