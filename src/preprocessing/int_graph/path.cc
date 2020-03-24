#include <algorithm>
#include <limits>

#include "ppr/preprocessing/geo_util.h"
#include "ppr/preprocessing/int_graph/path.h"

namespace bg = boost::geometry;

namespace ppr::preprocessing {

static constexpr double EXT_JOIN_DISTANCE = 30.0;

static constexpr double SIMPLE_EXTEND_DISTANCE = 5.0;
static constexpr double SIMPLE_SHORTEN_DISTANCE = 5.0;

double path_length(std::vector<merc> const& path) {
  double len = 0.0;
  for (std::size_t i = 0; i < path.size() - 1; i++) {
    len += distance(path[i], path[i + 1]);
  }
  return len;
}

namespace {
template <typename Handler>
void for_path_segments(std::vector<merc> const& path, bool reverse,
                       Handler handler) {
  if (path.empty()) {
    return;
  }
  if (reverse) {
    for (std::size_t i = path.size() - 1; i > 0; i--) {
      if (handler(i - 1, i)) {
        break;
      }
    }
  } else {
    for (std::size_t i = 0; i < path.size() - 1; i++) {
      if (handler(i, i + 1)) {
        break;
      }
    }
  }
}
}  // namespace

point_at_result point_at(std::vector<merc> const& path, double dist,
                         bool reverse) {
  point_at_result result;
  double offset = 0.0;
  for_path_segments(
      path, reverse, [&](std::size_t seg_from_idx, std::size_t seg_to_idx) {
        auto const& seg_from = path[seg_from_idx];
        auto const& seg_to = path[seg_to_idx];
        auto const seg_len = distance(seg_from, seg_to);
        if (offset + seg_len > dist) {
          auto const seg = segment_t{seg_from, seg_to};
          auto dir = seg_to - seg_from;
          dir.normalize();
          auto const normal = dir.normal();
          auto const point = seg_from + (dist - offset) * dir;
          auto const offset_from_start =
              reverse ? path_length(path) - dist : dist;
          result = {seg_from_idx, seg_to_idx, seg,   offset_from_start,
                    point,        dir,        normal};
          return true;
        }
        offset += seg_len;
        return false;
      });
  return result;
}

point_at_result intersect_path(std::vector<merc> const& path,
                               segment_t const& cutting_seg) {
  point_at_result result;
  double offset = 0.0;
  for_path_segments(
      path, false, [&](std::size_t seg_from_idx, std::size_t seg_to_idx) {
        auto const& seg_from = path[seg_from_idx];
        auto const& seg_to = path[seg_to_idx];
        auto const seg_len = distance(seg_from, seg_to);
        auto const seg = segment_t{seg_from, seg_to};
        std::vector<merc> intersections;
        bg::intersection(seg, cutting_seg, intersections);
        if (!intersections.empty()) {
          assert(intersections.size() == 1);
          auto dir = seg_to - seg_from;
          dir.normalize();
          auto const normal = dir.normal();
          auto const point = intersections.front();
          auto const dist = offset + distance(seg_from, point);
          result = {seg_from_idx, seg_to_idx, seg, dist, point, dir, normal};
          return true;
        }
        offset += seg_len;
        return false;
      });
  return result;
}

void shorten_path(std::vector<merc>& path, merc const& intersection,
                  bool reverse, std::vector<merc>& other_path) {
  if (reverse) {
    while (path.size() >= 2) {
      auto const seg = segment_t{path.back(), path[path.size() - 2]};
      path.erase(end(path) - 1);
      if (bg::intersects(seg, other_path)) {
        path.push_back(intersection);
        break;
      }
    }
  } else {
    while (path.size() >= 2) {
      auto const seg = segment_t{path[0], path[1]};
      path.erase(begin(path));
      if (bg::intersects(seg, other_path)) {
        path.insert(begin(path), intersection);
        break;
      }
    }
  }
}

void shorten_path(std::vector<merc>& path, bool reverse, double shorten_by) {
  auto remaining = path_length(path);
  auto target = remaining - shorten_by;
  if (remaining <= 1.0) {
    return;
  } else if (remaining - shorten_by < 1.0) {
    target = 1.0;
  }
  if (reverse) {
    while (path.size() >= 2 && remaining > target) {
      auto const p0 = path[path.size() - 1];
      auto const p1 = path[path.size() - 2];
      auto const end_seg_len = distance(p0, p1);
      auto const without_seg = remaining - end_seg_len;
      if (without_seg >= target) {
        path.erase(end(path) - 1);
        remaining = without_seg;
      } else {
        auto dir = p0 - p1;
        dir.normalize();
        path.back() = p1 + dir * (target - without_seg) / scale_factor(p1);
        break;
      }
    }
  } else {
    while (path.size() >= 2 && remaining > target) {
      auto const p0 = path[0];
      auto const p1 = path[1];
      auto const end_seg_len = distance(p0, p1);
      auto const without_seg = remaining - end_seg_len;
      if (without_seg >= target) {
        path.erase(begin(path));
        remaining = without_seg;
      } else {
        auto dir = p0 - p1;
        dir.normalize();
        path[0] = p1 + dir * (target - without_seg) / scale_factor(p1);
        break;
      }
    }
  }
}

void add_point_to_path(std::vector<merc>& path, merc const& pt, bool at_end) {
  if (at_end) {
    path.push_back(pt);
  } else {
    path.insert(begin(path), pt);
  }
}

segment_t get_incoming_segment(std::vector<merc>& path, bool reverse,
                               double extension) {
  auto const size = path.size();
  if (size < 2) {
    auto const nan = std::numeric_limits<double>::quiet_NaN();
    return {{nan, nan}, {nan, nan}};
  }

  auto p0 = reverse ? path[size - 2] : path[1];
  auto p1 = reverse ? path[size - 1] : path[0];

  auto dir = p1 - p0;
  dir.normalize();
  p1 += (extension / scale_factor(p1)) * dir;

  return segment_t{p0, p1};
}

void extend_path(std::vector<merc>& path, bool reverse, double distance) {
  auto seg = get_incoming_segment(path, reverse, distance);
  if (std::get<1>(seg).isnan()) {
    return;
  }
  add_point_to_path(path, {bg::get<1, 0>(seg), bg::get<1, 1>(seg)}, reverse);
}

std::pair<merc, bool> join_paths(std::vector<merc>& path1, bool path1_reverse,
                                 std::vector<merc>& path2, bool path2_reverse,
                                 merc const& center) {
  merc intersection;

  auto path1_orig = path1;
  auto path2_orig = path2;
  extend_path(path1, path1_reverse, SIMPLE_EXTEND_DISTANCE);
  extend_path(path2, path2_reverse, SIMPLE_EXTEND_DISTANCE);

  std::vector<merc> intersections;
  bg::intersection(path1, path2, intersections);
  auto const intersection_count = intersections.size();
  if (intersection_count >= 1) {
    if (intersection_count == 1) {
      intersection = intersections.front();
    } else {
      intersection = closest_point(intersections, center);
    }
    auto path1_copy = path1;
    shorten_path(path1, intersection, path1_reverse, path2);
    shorten_path(path2, intersection, path2_reverse, path1_copy);
  } else {
    path1 = path1_orig;
    path2 = path2_orig;
    shorten_path(path1, path1_reverse, SIMPLE_SHORTEN_DISTANCE);
    shorten_path(path2, path2_reverse, SIMPLE_SHORTEN_DISTANCE);
    auto& p1_pt = first_point(path1, path1_reverse);
    auto& p2_pt = first_point(path2, path2_reverse);
    intersection = p1_pt + (p2_pt - p1_pt) / 2;
    add_point_to_path(path1, intersection, path1_reverse);
    add_point_to_path(path2, intersection, path2_reverse);
  }

  return {intersection, true};
}

std::pair<merc, bool> join_single_path(std::vector<merc>& in_path,
                                       merc const& center) {
  merc intersection = center;

  auto check_intersect = [&](std::vector<merc>& path) {
    std::vector<merc> intersections;
    auto const size = path.size();
    for (std::size_t start_idx = 0; start_idx < size - 2; start_idx++) {
      auto const start_seg = segment_t{path[start_idx], path[start_idx + 1]};
      for (std::size_t end_idx = size - 2; end_idx > start_idx + 1; end_idx--) {
        auto const end_seg = segment_t{path[end_idx], path[end_idx + 1]};
        bg::intersection(start_seg, end_seg, intersections);
        if (!intersections.empty()) {
          assert(intersections.size() == 1);
          intersection = intersections.front();

          path.erase(
              std::next(begin(path), static_cast<std::ptrdiff_t>(end_idx + 1)),
              end(path));
          path.push_back(intersection);

          path.erase(begin(path),
                     std::next(begin(path),
                               static_cast<std::ptrdiff_t>(start_idx + 1)));
          path.insert(begin(path), intersection);

          return true;
        }
      }
    }
    return false;
  };

  if (check_intersect(in_path)) {
    return {intersection, true};
  }

  auto extended_path = in_path;
  extend_path(extended_path, false, EXT_JOIN_DISTANCE);
  extend_path(extended_path, true, EXT_JOIN_DISTANCE);
  if (check_intersect(extended_path)) {
    in_path = extended_path;
    return {intersection, true};
  }

  if (distance(in_path.front(), in_path.back()) <= 1.0) {
    intersection = in_path.front();
    in_path.push_back(intersection);
    return {intersection, true};
  }

  return {intersection, false};
}

merc join_footpath_with_street(std::vector<merc>& foot_path, bool foot_reverse,
                               std::vector<merc> const& street_path,
                               bool street_reverse) {
  auto const& street_end =
      street_reverse ? street_path.back() : street_path.front();
  merc intersection = street_end;

  auto foot_path_orig = foot_path;
  auto street_path_ext = street_path;
  extend_path(foot_path, foot_reverse, EXT_JOIN_DISTANCE);
  extend_path(street_path_ext, street_reverse, EXT_JOIN_DISTANCE);

  std::vector<merc> intersections;
  bg::intersection(foot_path, street_path_ext, intersections);
  auto const intersection_count = intersections.size();
  if (intersection_count >= 1) {
    if (intersection_count == 1) {
      intersection = intersections.front();
    } else {
      intersection = closest_point(intersections, street_end);
    }
    shorten_path(foot_path, intersection, foot_reverse, street_path_ext);
  } else {
    foot_path = foot_path_orig;
  }
  auto const& foot_end = foot_reverse ? foot_path.back() : foot_path.front();
  if (foot_end != street_end) {
    add_point_to_path(foot_path, street_end, foot_reverse);
  }

  return intersection;
}

}  // namespace ppr::preprocessing
