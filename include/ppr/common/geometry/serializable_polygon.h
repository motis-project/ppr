#pragma once

#include "cista/serialization.h"

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/register/ring.hpp"

#include "ppr/common/data.h"
#include "ppr/common/math.h"

namespace ppr {

template <typename Point>
struct ring : public data::vector<Point> {};

template <typename Point>
cista::hash_t type_hash(ring<Point> const&, cista::hash_t h,
                        std::map<cista::hash_t, unsigned> done) {
  h = cista::hash_combine(h, cista::hash("ring"));
  return cista::type_hash(Point{}, h, done);
}

template <typename Ctx, typename Point>
void serialize(Ctx& ctx, ring<Point> const* r, cista::offset_t const o) {
  cista::serialize(ctx, static_cast<data::vector<Point> const*>(r), o);
}

template <typename Ctx, typename Point>
void deserialize(Ctx const& ctx, ring<Point>* r) {
  data::deserialize(ctx, static_cast<data::vector<Point>*>(r));
}

template <typename Point>
struct serializable_polygon {
  using point_type = Point;
  using ring_type = ring<Point>;
  using inner_container_type = data::vector<ring<Point>>;

  ring_type const& outer() const { return outer_; }
  ring_type& outer() { return outer_; }
  inner_container_type const& inners() const { return inner_; }
  inner_container_type& inners() { return inner_; }

  ring_type outer_;
  inner_container_type inner_;
};

template <typename Location, typename Point>
inline double distance(Location const& loc,
                       serializable_polygon<Point> const& poly) {
  return boost::geometry::distance(loc, poly) * AVG_EARTH_RADIUS;
}

}  // namespace ppr

BOOST_GEOMETRY_REGISTER_RING_TEMPLATED(ppr::ring)

namespace boost {
namespace geometry {
namespace traits {

template <typename Point>
struct tag<ppr::serializable_polygon<Point>> {
  using type = polygon_tag;
};

template <typename Point>
struct ring_const_type<ppr::serializable_polygon<Point>> {
  using type = typename ppr::serializable_polygon<Point>::ring_type const&;
};

template <typename Point>
struct ring_mutable_type<ppr::serializable_polygon<Point>> {
  using type = typename ppr::serializable_polygon<Point>::ring_type&;
};

template <typename Point>
struct interior_const_type<ppr::serializable_polygon<Point>> {
  using type =
      typename ppr::serializable_polygon<Point>::inner_container_type const&;
};

template <typename Point>
struct interior_mutable_type<ppr::serializable_polygon<Point>> {
  using type = typename ppr::serializable_polygon<Point>::inner_container_type&;
};

template <typename Point>
struct exterior_ring<ppr::serializable_polygon<Point>> {
  using polygon_type = ppr::serializable_polygon<Point>;
  static inline typename polygon_type::ring_type& get(polygon_type& p) {
    return p.outer();
  }
  static inline typename polygon_type::ring_type const& get(
      polygon_type const& p) {
    return p.outer();
  }
};

template <typename Point>
struct interior_rings<ppr::serializable_polygon<Point>> {
  using polygon_type = ppr::serializable_polygon<Point>;
  static inline typename polygon_type::inner_container_type& get(
      polygon_type& p) {
    return p.inners();
  }
  static inline typename polygon_type::inner_container_type const& get(
      polygon_type const& p) {
    return p.inners();
  }
};

template <typename Point>
struct point_order<ppr::data::vector<Point>> {
  static const order_selector value = counterclockwise;
};

template <typename Point>
struct closure<ppr::data::vector<Point>> {
  static const closure_selector value = closed;
};

}  // namespace traits
}  // namespace geometry
}  // namespace boost
