#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "cista/serialization.h"

#include "ppr/common/data.h"
#include "ppr/common/elevation.h"
#include "ppr/common/enums.h"
#include "ppr/common/location.h"
#include "ppr/common/location_geometry.h"
#include "ppr/common/node.h"
#include "ppr/common/path_geometry.h"
#include "ppr/common/tri_state.h"

namespace ppr {

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
struct edge_info {
  inline bool is_unmarked_crossing() const {
    return type_ == edge_type::CROSSING &&
           (crossing_type_ == crossing_type::UNMARKED ||
            crossing_type_ == crossing_type::GENERATED);
  }

  inline bool is_marked_crossing() const {
    return type_ == edge_type::CROSSING &&
           (crossing_type_ != crossing_type::UNMARKED &&
            crossing_type_ != crossing_type::GENERATED);
  }

  inline bool is_rail_edge() const {
    return street_type_ == street_type::RAIL ||
           street_type_ == street_type::TRAM;
  }

  template <typename Ctx>
  friend void serialize(Ctx& ctx, edge_info const* ei,
                        cista::offset_t const o) {
    cista::serialize(
        ctx, &ei->name_,
        o + static_cast<cista::offset_t>(offsetof(edge_info, name_)));
  }

  template <typename Ctx>
  friend void deserialize(Ctx const& ctx, edge_info* ei) {
    data::deserialize(ctx, &ei->name_);
  }

  std::int64_t osm_way_id_{};
  data::ptr<data::string> name_{};
  edge_type type_{edge_type::CONNECTION};
  street_type street_type_{street_type::NONE};
  crossing_type::crossing_type crossing_type_{};
  surface_type surface_type_{surface_type::UNKNOWN};
  smoothness_type smoothness_type_{smoothness_type::UNKNOWN};
  bool oneway_street_ : 1;
  bool allow_fwd_ : 1;
  bool allow_bwd_ : 1;
  bool area_ : 1;
  bool incline_up_ : 1;
  tri_state::tri_state handrail_ : 2;
  wheelchair_type::wheelchair_type wheelchair_ : 2;
  uint8_t step_count_{};
  int32_t marked_crossing_detour_{};
};

inline edge_info make_edge_info(std::int64_t osm_way_id, edge_type type,
                                street_type street,
                                crossing_type::crossing_type crossing) {
  return edge_info{osm_way_id,
                   nullptr,
                   type,
                   street,
                   crossing,
                   surface_type::UNKNOWN,
                   smoothness_type::UNKNOWN,
                   false,
                   true,
                   true,
                   false,
                   false,
                   tri_state::UNKNOWN,
                   wheelchair_type::UNKNOWN,
                   0,
                   0};
}

struct edge {
  data::ptr<edge_info const> info_{};
  data::ptr<node const> from_{};
  data::ptr<node const> to_{};
  double distance_{};
  data::vector<location> path_;
  side_type side_{side_type::CENTER};
  elevation_diff_t elevation_up_{};
  elevation_diff_t elevation_down_{};
};

inline edge make_edge(edge_info const* info, node const* from, node const* to,
                      double distance,
                      data::vector<location> path = data::vector<location>(),
                      side_type side = side_type::CENTER,
                      elevation_diff_t elevation_up = 0,
                      elevation_diff_t elevation_down = 0) {
  if (path.empty()) {
    path.reserve(2);
    path.emplace_back(from->location_);
    path.emplace_back(to->location_);
  }
  return edge{info,         from,          to, distance, std::move(path), side,
              elevation_up, elevation_down};
}

inline bool any_edge_between(node const* a, node const* b) {
  return std::any_of(begin(a->out_edges_), end(a->out_edges_),
                     [&](auto const& e) { return e->to_ == b; }) ||
         std::any_of(begin(b->out_edges_), end(b->out_edges_),
                     [&](auto const& e) { return e->to_ == a; });
}

}  // namespace ppr
