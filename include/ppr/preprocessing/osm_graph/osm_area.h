#pragma once

#include <algorithm>

#include "ankerl/unordered_dense.h"

#include "ppr/common/geometry/polygon.h"
#include "ppr/common/matrix.h"
#include "ppr/common/names.h"
#include "ppr/preprocessing/osm_graph/osm_node.h"

namespace ppr::preprocessing {

struct osm_area {
  using point_type = osm_node*;

  osm_area() = default;

  osm_area(std::size_t id, std::vector<osm_node*> outer,
           std::vector<std::vector<osm_node*>> inner)
      : id_(static_cast<uint32_t>(id)),
        outer_(std::move(outer)),
        inner_(std::move(inner)) {}

  std::vector<point_type> get_nodes() const {
    std::vector<point_type> all_nodes;
    std::copy(begin(outer_), end(outer_), std::back_inserter(all_nodes));
    for (auto const& r : inner_) {
      std::copy(begin(r), end(r), std::back_inserter(all_nodes));
    }
    return all_nodes;
  }

  static std::vector<merc> get_ring_points(
      std::vector<osm_node*> const& nodes) {
    std::vector<merc> points;
    points.reserve(nodes.size());
    std::transform(begin(nodes), end(nodes), std::back_inserter(points),
                   [](osm_node const* p) { return p->location_; });
    return points;
  }

  typename std::vector<osm_node*> const& outer() const { return outer_; }

  typename std::vector<std::vector<osm_node*>> const& inners() const {
    return inner_;
  }

  area_polygon_t get_outer_polygon() const {
    auto const points = get_ring_points(outer_);
    return {{begin(points), end(points)}};
  }

  std::vector<inner_area_polygon_t> get_inner_polygons() const {
    std::vector<inner_area_polygon_t> obstacles;
    for (auto const& inner : inner_) {
      auto const points = get_ring_points(inner);
      obstacles.emplace_back(
          inner_area_polygon_t{{begin(points), end(points)}});
    }
    return obstacles;
  }

  std::uint32_t id_{0};
  std::vector<osm_node*> outer_;
  std::vector<std::vector<osm_node*>> inner_;
  names_idx_t name_{};
  std::int64_t osm_id_{0};
  bool from_way_{false};
  matrix<double, uint16_t> dist_matrix_;
  matrix<uint16_t, uint16_t> next_matrix_;
  data::vector<uint16_t> exit_nodes_;
  ankerl::unordered_dense::set<std::uint32_t> adjacent_areas_;
};

inline merc get_merc(osm_node const* n) { return n->location_; }

}  // namespace ppr::preprocessing
