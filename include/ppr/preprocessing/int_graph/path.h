#pragma once

#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"

#include "ppr/common/geometry/merc.h"
#include "ppr/common/path_geometry.h"
#include "ppr/preprocessing/options.h"

namespace ppr::preprocessing {

using segment_t = boost::geometry::model::segment<merc>;

double path_length(std::vector<merc> const& path);

struct point_at_result {
  point_at_result() = default;
  point_at_result(std::size_t seg_from_idx, std::size_t seg_to_idx,
                  segment_t seg, double offset_from_start, merc point, merc dir,
                  merc normal)
      : seg_from_idx_(seg_from_idx),
        seg_to_idx_(seg_to_idx),
        seg_(std::move(seg)),
        offset_from_start_(offset_from_start),
        point_(point),
        dir_(dir),
        normal_(normal) {}

  bool valid() const { return seg_from_idx_ != 0 || seg_to_idx_ != 0; }
  explicit operator bool() const { return valid(); }

  std::size_t seg_from_idx_{0};
  std::size_t seg_to_idx_{0};
  segment_t seg_;
  double offset_from_start_{0};
  merc point_;
  merc dir_;
  merc normal_;
};

point_at_result point_at(std::vector<merc> const& path, double dist,
                         bool reverse);

point_at_result intersect_path(std::vector<merc> const& path,
                               segment_t const& cutting_seg);

void shorten_path(std::vector<merc>& path, merc const& intersection,
                  bool reverse, std::vector<merc>& other_path);

void add_point_to_path(std::vector<merc>& path, merc const& pt, bool at_end);

void extend_path(std::vector<merc>& path, bool reverse, double distance);

std::pair<merc, bool> join_paths(std::vector<merc>& path1, bool path1_reverse,
                                 std::vector<merc>& path2, bool path2_reverse,
                                 merc const& center);

std::pair<merc, bool> join_single_path(std::vector<merc>& in_path,
                                       merc const& center);

merc join_footpath_with_street(std::vector<merc>& foot_path, bool foot_reverse,
                               std::vector<merc> const& street_path,
                               bool street_reverse);

inline merc& first_point(std::vector<merc>& path, bool reverse) {
  return reverse ? path.back() : path.front();
}

inline merc& last_point(std::vector<merc>& path, bool reverse) {
  return reverse ? path.front() : path.back();
}

}  // namespace ppr::preprocessing
