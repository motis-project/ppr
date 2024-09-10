#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "cista/serialization.h"

#include "ppr/common/data.h"
#include "ppr/common/elevation.h"
#include "ppr/common/enums.h"
#include "ppr/common/level.h"
#include "ppr/common/location.h"
#include "ppr/common/location_geometry.h"
#include "ppr/common/names.h"
#include "ppr/common/node.h"
#include "ppr/common/path_geometry.h"
#include "ppr/common/tri_state.h"

namespace ppr {

using edge_info_idx_t = std::uint32_t;

struct routing_graph_data;
struct routing_graph;

constexpr auto const UNKNOWN_INCLINE = std::numeric_limits<std::int8_t>::min();
constexpr auto const NO_EDGE_INFO = std::numeric_limits<edge_info_idx_t>::max();

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

  inline bool is_signals_crossing_with_sound_or_vibration() const {
    return crossing_type_ == crossing_type::SIGNALS &&
           (traffic_signals_sound_ == tri_state::YES ||
            traffic_signals_vibration_ == tri_state::YES);
  }

  template <typename Ctx>
  friend void serialize(Ctx& /*ctx*/, edge_info const* /*ei*/,
                        cista::offset_t const /*o*/) {}

  template <typename Ctx>
  friend void deserialize(Ctx const& /*ctx*/, edge_info* /*ei*/) {}

  std::int64_t osm_way_id_{};
  names_idx_t name_{};
  edge_type type_{edge_type::CONNECTION};
  street_type street_type_{street_type::NONE};
  crossing_type crossing_type_{};
  surface_type surface_type_{surface_type::UNKNOWN};
  smoothness_type smoothness_type_{smoothness_type::UNKNOWN};
  bool oneway_street_ : 1 {false};
  bool allow_fwd_ : 1 {true};
  bool allow_bwd_ : 1 {true};
  bool area_ : 1 {false};
  bool incline_up_ : 1 {false};
  tri_state handrail_ : 2 {tri_state::UNKNOWN};
  wheelchair_type wheelchair_ : 2 {wheelchair_type::UNKNOWN};
  wheelchair_type stroller_ : 2 {wheelchair_type::UNKNOWN};
  tri_state traffic_signals_sound_ : 2 {tri_state::UNKNOWN};
  tri_state traffic_signals_vibration_ : 2 {tri_state::UNKNOWN};
  std::uint8_t step_count_{};
  std::int32_t marked_crossing_detour_{};
  levels levels_{};
  std::uint8_t max_width_{};  // centimeters
  std::int8_t incline_{UNKNOWN_INCLINE};  // percent, UNKNOWN_INCLINE = unknown
  door_type door_type_ : 4 {door_type::UNKNOWN};
  automatic_door_type automatic_door_type_ : 3 {automatic_door_type::UNKNOWN};
};

inline edge_info make_edge_info(std::int64_t osm_way_id, edge_type type,
                                street_type street, crossing_type crossing) {
  return edge_info{.osm_way_id_ = osm_way_id,
                   .type_ = type,
                   .street_type_ = street,
                   .crossing_type_ = crossing};
}

inline std::pair<edge_info_idx_t, edge_info*> make_edge_info(
    data::vector_map<edge_info_idx_t, edge_info>& edge_infos,
    std::int64_t osm_way_id, edge_type type, street_type street,
    crossing_type crossing) {
  auto const idx = edge_infos.size();
  auto& info = edge_infos.emplace_back(
      make_edge_info(osm_way_id, type, street, crossing));
  return {idx, &info};
}

struct edge {
  edge_info* info(routing_graph_data& rg) const;
  edge_info* info(routing_graph& rg) const;
  edge_info const* info(routing_graph_data const& rg) const;
  edge_info const* info(routing_graph const& rg) const;
  edge_info const* info(
      data::vector_map<edge_info_idx_t, edge_info> const& edge_infos) const;

  node const* from(routing_graph_data const& rg) const;
  node const* to(routing_graph_data const& rg) const;

  edge_info_idx_t info_{};
  data::ptr<node const> from_{};
  data::ptr<node const> to_{};
  double distance_{};
  data::vector<location> path_;
  side_type side_{side_type::CENTER};
  elevation_diff_t elevation_up_{};
  elevation_diff_t elevation_down_{};
};

inline edge make_edge(edge_info_idx_t const info, node const* from,
                      node const* to, double distance,
                      data::vector<location> path = data::vector<location>(),
                      side_type side = side_type::CENTER,
                      elevation_diff_t elevation_up = 0,
                      elevation_diff_t elevation_down = 0) {
  if (path.empty()) {
    path.reserve(2);
    path.emplace_back(from->location_);
    path.emplace_back(to->location_);
  }
  return edge{
      .info_ = info,
      .from_ = from,
      .to_ = to,
      .distance_ = distance,
      .path_ = std::move(path),
      .side_ = side,
      .elevation_up_ = elevation_up,
      .elevation_down_ = elevation_down,
  };
}

inline bool any_edge_between(node const* a, node const* b) {
  return std::any_of(begin(a->out_edges_), end(a->out_edges_),
                     [&](auto const& e) { return e->to_ == b; }) ||
         std::any_of(begin(b->out_edges_), end(b->out_edges_),
                     [&](auto const& e) { return e->to_ == a; });
}

}  // namespace ppr
