#pragma once

#include <limits>
#include <vector>

#include "boost/geometry/algorithms/comparable_distance.hpp"

#include "ppr/common/geometry/merc.h"

namespace ppr::preprocessing {

inline double side_of(merc const& line_dir, merc const& point_diff) {
  return (line_dir.x() * point_diff.y() - line_dir.y() * point_diff.x());
}

inline double side_of(merc const& ref_from, merc const& ref_to,
                      merc const& ref_dir, merc const& other_from,
                      merc const& other_to, merc const& other_dir,
                      double angle) {
  auto const side_p0 = side_of(ref_dir, other_from - ref_from);
  auto const side_p1 = side_of(ref_dir, other_to - ref_from);
  if (same_sign(side_p0, side_p1)) {
    return side_p0;
  } else {
    auto const other_side = side_of(other_from, other_to, other_dir, ref_from,
                                    ref_to, ref_dir, angle);
    if (angle < 1 || angle > 5) {
      return -other_side;
    } else {
      return other_side;
    }
  }
}

inline merc closest_point(std::vector<merc> const& points, merc const& target) {
  merc closest;

  if (!points.empty()) {
    double closest_dist = std::numeric_limits<double>::max();
    for (auto const& pt : points) {
      auto const dist = boost::geometry::comparable_distance(pt, target);
      if (dist < closest_dist) {
        closest = pt;
        closest_dist = dist;
      }
    }
  }

  return closest;
}

}  // namespace ppr::preprocessing
