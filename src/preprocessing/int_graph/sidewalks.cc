#include <algorithm>
#include <vector>

#include "ppr/preprocessing/int_graph/sidewalks.h"

namespace ppr::preprocessing {

std::pair<std::vector<merc>, std::vector<merc>> generate_sidewalk_paths(
    std::vector<merc> const& way_path, double width) {
  if (way_path.empty()) {
    return {{}, {}};
  } else if (way_path.size() == 1) {
    return {way_path, way_path};
  }
  auto const scale = scale_factor(way_path.front());
  auto side_offset = width / 2.0 / scale;

  std::vector<merc> normals;
  normals.reserve(way_path.size() - 1);
  for (std::size_t i = 0; i < way_path.size() - 1; i++) {
    auto& p0 = way_path[i];
    auto& p1 = way_path[i + 1];
    auto direction = p1 - p0;
    direction.normalize();
    normals.push_back(direction.normal());
  }

  std::vector<merc> left_path, right_path;
  for (std::size_t i = 0; i < way_path.size(); i++) {
    auto const& p = way_path[i];
    if (i == 0) {
      auto const& normal = normals.front();
      left_path.push_back(p + side_offset * normal);
      right_path.push_back(p - side_offset * normal);
    } else if (i == way_path.size() - 1) {
      auto const& normal = normals.back();
      left_path.push_back(p + side_offset * normal);
      right_path.push_back(p - side_offset * normal);
    } else {
      auto const& prev_normal = normals[i - 1];
      auto const& next_normal = normals[i];
      auto join_normal = prev_normal + next_normal;
      auto miter = false;
      auto miter_len = 1.0;
      if (join_normal.length() > 0.000000001) {
        join_normal.normalize();
        miter_len = 1.0 / join_normal.dot(next_normal);
        if (miter_len >= 0.5 && miter_len <= 2.0) {
          miter = true;
        }
      }
      if (miter) {
        left_path.push_back(p + side_offset * miter_len * join_normal);
        right_path.push_back(p - side_offset * miter_len * join_normal);
      } else {
        auto const clockwise =
            (way_path[i + 1] - way_path[i - 1]).cross(p - way_path[i - 1]) >= 0;
        if (clockwise) {
          left_path.push_back(p + side_offset * prev_normal);
          left_path.push_back(p + side_offset * next_normal);
          right_path.push_back(p - side_offset * miter_len * join_normal);
        } else {
          left_path.push_back(p + side_offset * miter_len * join_normal);
          right_path.push_back(p - side_offset * prev_normal);
          right_path.push_back(p - side_offset * next_normal);
        }
      }
    }
  }

  return {left_path, right_path};
}

}  // namespace ppr::preprocessing
