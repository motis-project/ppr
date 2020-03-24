#pragma once

#include <vector>

#include "boost/geometry.hpp"
#include "boost/geometry/algorithms/length.hpp"
#include "boost/geometry/geometries/register/point.hpp"

#include "ppr/common/data.h"
#include "ppr/common/location.h"
#include "ppr/common/math.h"

BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(
    ppr::location, double,
    boost::geometry::cs::spherical_equatorial<boost::geometry::degree>,
    ppr::location::lon, ppr::location::lat, ppr::location::set_lon,
    ppr::location::set_lat)

namespace ppr {

template <typename Location>
double distance(Location const& a, Location const& b) {
  return boost::geometry::distance(a, b) * AVG_EARTH_RADIUS;
}

template <typename Location>
double distance(Location const& a, std::vector<Location> const& b) {
  return boost::geometry::distance(a, b) * AVG_EARTH_RADIUS;
}

template <typename Location>
double distance(Location const& a, data::vector<Location> const& b) {
  return boost::geometry::distance(a, b) * AVG_EARTH_RADIUS;
}

template <typename Location>
double length(std::vector<Location> const& path) {
  return boost::geometry::length(path) * AVG_EARTH_RADIUS;
}

template <typename Location>
double length(data::vector<Location> const& path) {
  return static_cast<double>(boost::geometry::length(path)) * AVG_EARTH_RADIUS;
}

}  // namespace ppr
