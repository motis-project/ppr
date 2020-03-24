#pragma once

#include <cmath>
#include <functional>

namespace ppr {

constexpr auto PI = 3.14159265358979323846;
constexpr auto AVG_EARTH_RADIUS = 6371000.0;
constexpr auto EQUATOR_EARTH_RADIUS = 6378137.0;
constexpr auto MERC_MAX_LAT = 85.0511287798;

constexpr double to_rad(double deg) { return deg * PI / 180.0; }
constexpr double to_deg(double rad) { return rad * 180.0 / PI; }

template <typename T>
inline bool same_sign(T const a, T const b) {
  return ((a < 0) == (b < 0)) || std::equal_to<>()(a, 0) ||
         std::equal_to<>()(b, 0);
}

constexpr double normalized_angle(double angle) {
  return angle < 0 ? angle + 2 * PI : angle;
}

template <typename T>
inline double get_normalized_angle(T const& vec) {
  auto angle = std::atan2(vec.y(), vec.x());
  return normalized_angle(angle);
}

template <typename T>
inline double get_angle_between(T const& v1, T const& v2) {
  auto angle = get_normalized_angle(v2) - get_normalized_angle(v1);
  return normalized_angle(angle);
}

constexpr bool is_angle_between(double angle, double lo_deg, double hi_deg) {
  return angle >= to_rad(lo_deg) && angle <= to_rad(hi_deg);
}

constexpr bool is_angle_around(double angle, double ref_deg,
                               double margin_deg) {
  auto const lo = normalized_angle(to_rad(ref_deg - margin_deg));
  auto const hi = normalized_angle(to_rad(ref_deg + margin_deg));
  if (lo < hi) {
    return angle >= lo && angle <= hi;
  } else {
    return angle >= lo || angle <= hi;
  }
}

constexpr bool is_parallel(double angle, double margin_deg) {
  return is_angle_around(angle, 0, margin_deg) ||
         is_angle_around(angle, 180, margin_deg);
}

constexpr bool is_right_angle(double angle, double margin_deg) {
  return is_angle_around(angle, 90, margin_deg) ||
         is_angle_around(angle, 270, margin_deg);
}

}  // namespace ppr
