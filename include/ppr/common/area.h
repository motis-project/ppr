#pragma once

#include <cstdint>
#include <string>

#include "boost/geometry/algorithms/for_each.hpp"
#include "boost/geometry/geometries/register/point.hpp"

#include "ppr/common/data.h"
#include "ppr/common/edge.h"
#include "ppr/common/geometry/polygon.h"
#include "ppr/common/geometry/serializable_polygon.h"
#include "ppr/common/level.h"
#include "ppr/common/matrix.h"
#include "ppr/common/names.h"
#include "ppr/common/node.h"

namespace ppr {

template <typename T>
inline T get_buffered_polygon(T const& input, double const distance = 0.5) {
  auto const distance_strategy =
      boost::geometry::strategy::buffer::distance_symmetric<double>{distance};
  auto const join_strategy = boost::geometry::strategy::buffer::join_miter{};
  auto const end_strategy = boost::geometry::strategy::buffer::end_flat{};
  auto const point_strategy = boost::geometry::strategy::buffer::point_square{};
  auto const side_strategy = boost::geometry::strategy::buffer::side_straight{};
  auto output = boost::geometry::model::multi_polygon<T>{};
  boost::geometry::buffer(input, output, distance_strategy, side_strategy,
                          join_strategy, end_strategy, point_strategy);
  return output[0];
}

struct area {
  struct point {
    merc get_merc() const { return to_merc(location_); }

    bool is_exit_node() const { return node_ != nullptr; }

    explicit operator bool() const { return true; }

    inline double lon() const { return location_.lon(); }
    inline double lat() const { return location_.lat(); }

    inline void set_lon(double lon) { location_.set_lon(lon); }
    inline void set_lat(double lat) { location_.set_lat(lat); }

    location location_{};
    data::ptr<node> node_{nullptr};
  };

  using point_type = point;
  using polygon_t = serializable_polygon<point_type>;

  std::vector<point_type> get_nodes() const {
    std::vector<point_type> all_nodes;
    boost::geometry::for_each_point(
        polygon_, [&](point_type const& p) { all_nodes.emplace_back(p); });
    return all_nodes;
  }

  static std::vector<merc> get_ring_points(
      typename polygon_t::ring_type const& nodes) {
    std::vector<merc> points;
    points.reserve(nodes.size());
    std::transform(begin(nodes), end(nodes), std::back_inserter(points),
                   [](point const& p) { return to_merc(p.location_); });
    return points;
  }

  typename polygon_t::ring_type const& outer() const {
    return polygon_.outer();
  }

  typename polygon_t::inner_container_type const& inners() const {
    return polygon_.inners();
  }

  area_polygon_t get_outer_polygon(bool const buffered = false) const {
    auto const points = get_ring_points(polygon_.outer());
    auto poly = area_polygon_t{{begin(points), end(points)}};
    return buffered ? get_buffered_polygon(poly) : poly;
  }

  std::vector<inner_area_polygon_t> get_inner_polygons(
      bool const buffered = false) const {
    std::vector<inner_area_polygon_t> obstacles;
    for (auto const& inner : polygon_.inners()) {
      auto const points = get_ring_points(inner);
      auto poly = inner_area_polygon_t{{begin(points), end(points)}};
      obstacles.emplace_back(buffered ? get_buffered_polygon(poly, -0.1)
                                      : poly);
    }
    return obstacles;
  }

  // debug code
  unsigned count_mapped_nodes() const {
    unsigned c = 0;
    boost::geometry::for_each_point(polygon_, [&](point_type const& p) {
      if (p.node_ != nullptr) {
        c++;
      }
    });
    return c;
  }

  std::uint32_t id_{0};
  edge_info_idx_t edge_info_{};
  polygon_t polygon_;
  names_idx_t name_{};
  std::int64_t osm_id_{0};
  bool from_way_{false};
  levels levels_;
  matrix<double, uint16_t> dist_matrix_;
  matrix<uint16_t, uint16_t> next_matrix_;
  data::vector<uint16_t> exit_nodes_;
  data::vector<std::uint32_t> adjacent_areas_;
  location center_{};
};

inline merc get_merc(area::point const& pt) { return to_merc(pt.location_); }
inline merc get_merc(area::point const* pt) { return to_merc(pt->location_); }

inline bool operator==(area::point const& lhs, area::point const& rhs) {
  return lhs.location_ == rhs.location_ && lhs.node_ == rhs.node_;
}

}  // namespace ppr

BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(
    ppr::area::point, double,
    boost::geometry::cs::spherical_equatorial<boost::geometry::degree>,
    ppr::area::point::lon, ppr::area::point::lat, ppr::area::point::set_lon,
    ppr::area::point::set_lat)
