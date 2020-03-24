#pragma once

#include <cmath>
#include <ostream>

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/register/point.hpp"

#include "ppr/common/location.h"
#include "ppr/common/math.h"

namespace ppr {

struct merc {
  merc() : x_(0), y_(0) {}
  merc(double x, double y) : x_(x), y_(y) {}

  inline double x() const { return x_; }
  inline double y() const { return y_; }

  inline double length() const { return std::sqrt(x_ * x_ + y_ * y_); }

  inline void normalize() {
    auto len = length();
    x_ /= len;
    y_ /= len;
  }

  inline merc normal(bool left = true) const {
    if (left) {
      return {-y_, x_};
    } else {
      return {y_, -x_};
    }
  }

  inline double dot(merc const& o) const { return x_ * o.x_ + y_ * o.y_; }

  inline double cross(merc const& o) const { return x_ * o.y_ - o.x_ * y_; }

  inline void operator+=(merc const& rhs) {
    x_ += rhs.x_;
    y_ += rhs.y_;
  }

  inline void operator-=(merc const& rhs) {
    x_ -= rhs.x_;
    y_ -= rhs.y_;
  }

  inline bool isnan() const { return std::isnan(x_) || std::isnan(y_); }

  double x_;
  double y_;
};

}  // namespace ppr

BOOST_GEOMETRY_REGISTER_POINT_2D(ppr::merc, double,
                                 boost::geometry::cs::cartesian, x_, y_)

namespace ppr {

inline merc operator+(merc const& lhs, merc const& rhs) {
  return {lhs.x() + rhs.x(), lhs.y() + rhs.y()};
}

inline merc operator-(merc const& lhs, merc const& rhs) {
  return {lhs.x() - rhs.x(), lhs.y() - rhs.y()};
}

inline merc operator*(merc const& v, double s) {
  return {v.x() * s, v.y() * s};
}

inline merc operator*(double s, merc const& v) { return v * s; }

inline void operator*=(merc& v, double s) {
  v.x_ *= s;
  v.y_ *= s;
}

inline merc operator/(merc const& v, double s) {
  return {v.x() / s, v.y() / s};
}

inline void operator/=(merc& v, double s) {
  v.x_ /= s;
  v.y_ /= s;
}

inline bool operator==(merc const& lhs, merc const& rhs) {
  return std::fabs(lhs.x_ - rhs.x_) < 0.000000001 &&
         std::fabs(lhs.y_ - rhs.y_) < 0.000000001;
}

inline bool operator!=(merc const& lhs, merc const& rhs) {
  return !(lhs == rhs);
}

template <typename Location>
inline merc to_merc(Location const& loc) {
  auto const lat = std::max(std::min(MERC_MAX_LAT, loc.lat()), -MERC_MAX_LAT);
  auto const sin = std::sin(to_rad(lat));
  return {EQUATOR_EARTH_RADIUS * to_rad(loc.lon()),
          EQUATOR_EARTH_RADIUS * std::log((1.0 + sin) / (1.0 - sin)) / 2.0};
}

inline location to_location(merc const& mc) {
  return make_location(
      to_deg(mc.x()) / EQUATOR_EARTH_RADIUS,
      to_deg(2.0 * std::atan(std::exp(mc.y() / EQUATOR_EARTH_RADIUS)) -
             (PI / 2)));
}

inline double scale_factor(merc const& mc) {
  auto const lat_rad =
      2.0 * std::atan(std::exp(mc.y() / EQUATOR_EARTH_RADIUS)) - (PI / 2);
  return std::cos(lat_rad);
}

inline double distance(merc const& a, merc const& b) {
  return boost::geometry::distance(a, b) * scale_factor(a);
}

inline std::ostream& operator<<(std::ostream& os, merc const& mc) {
  auto const loc = to_location(mc);
  auto const precision = os.precision(12);
  os << "merc{x=" << mc.x() << ", y=" << mc.y() << " => lon=" << loc.lon()
     << ", lat=" << loc.lat() << "}";
  os.precision(precision);
  return os;
}

}  // namespace ppr
