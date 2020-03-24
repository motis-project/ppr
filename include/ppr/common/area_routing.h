#pragma once

#include <cmath>
#include <cstring>
#include <algorithm>
#include <functional>
#include <iostream>
#include <set>

#include "boost/geometry/algorithms/intersects.hpp"
#include "boost/geometry/geometries/geometries.hpp"

#include "ppr/common/area.h"
#include "ppr/common/edge.h"
#include "ppr/common/geometry/merc.h"
#include "ppr/common/geometry/polygon.h"
#include "ppr/common/matrix.h"

namespace ppr {

using merc_segment_t = boost::geometry::model::segment<merc>;

template <typename Area>
struct visibility_graph {
  explicit visibility_graph(Area* area)
      : nodes_(area->get_nodes()),
        n_(static_cast<uint16_t>(nodes_.size())),
        base_size_(n_),
        dist_matrix_(make_matrix<double, uint16_t>(n_, n_)),
        next_matrix_(make_matrix<uint16_t, uint16_t>(n_, n_)) {
    assert(nodes_.size() < std::numeric_limits<uint16_t>::max());
    assert(nodes_.size() == n_);
    dist_matrix_.init(0, std::numeric_limits<double>::max());
    next_matrix_.init(std::numeric_limits<uint16_t>::max(),
                      std::numeric_limits<uint16_t>::max());
    for (uint16_t i = 0; i < n_; i++) {
      if (nodes_[i] && nodes_[i]->is_exit_node()) {
        exit_nodes_.push_back(i);
      }
    }
  }

  visibility_graph(Area const* area,
                   std::vector<typename Area::point_type>& additional_points)
      : nodes_(area->get_nodes()),
        n_(static_cast<uint16_t>(nodes_.size() + additional_points.size())),
        base_size_(static_cast<uint16_t>(nodes_.size())),
        dist_matrix_(make_matrix<double, uint16_t>(
            area->dist_matrix_, n_, n_, 0, std::numeric_limits<double>::max())),
        next_matrix_(make_matrix<uint16_t, uint16_t>(
            area->next_matrix_, n_, n_, std::numeric_limits<uint16_t>::max(),
            std::numeric_limits<uint16_t>::max())),
        exit_nodes_(area->exit_nodes_) {
    std::copy(begin(additional_points), end(additional_points),
              std::back_inserter(nodes_));
    for (auto i = 0UL; i < additional_points.size(); i++) {
      exit_nodes_.push_back(static_cast<uint16_t>(base_size_ + i));
    }
    assert(nodes_.size() == n_);
  }

  bool extended() const { return base_size_ < n_; }

  std::vector<typename Area::point_type> nodes_;
  uint16_t n_;
  uint16_t base_size_;
  matrix<double, uint16_t> dist_matrix_;
  matrix<uint16_t, uint16_t> next_matrix_;
  data::vector<uint16_t> exit_nodes_;
};

inline void shorten_segment(merc_segment_t& seg, double len) {
  if (distance(seg.first, seg.second) <= len * 2) {
    return;
  }
  auto dir = seg.second - seg.first;
  dir.normalize();
  auto offset = len * scale_factor(seg.first);
  dir *= offset;
  seg.first += dir;
  seg.second -= dir;
}

template <typename Area>
void init_polygons(Area* area, visibility_graph<Area>& vg) {
  auto const outer_points = area->get_ring_points(area->outer());
  for (uint16_t i = 0; i < static_cast<uint16_t>(outer_points.size() - 1);
       i++) {
    auto const d = distance(outer_points[i], outer_points[i + 1]);
    vg.dist_matrix_.at(i, i + 1) = d;
    vg.dist_matrix_.at(i + 1, i) = d;
  }
  auto idx = static_cast<uint16_t>(outer_points.size());
  for (auto const& inner : area->inners()) {
    for (uint16_t i = 0; i < static_cast<uint16_t>(inner.size() - 1); i++) {
      auto const d = distance(get_merc(inner[i]), get_merc(inner[i + 1]));
      vg.dist_matrix_.at(idx, idx + 1) = d;
      vg.dist_matrix_.at(idx + 1, idx) = d;
      ++idx;
    }
    ++idx;
  }
}

template <typename Area>
void calc_visiblity(visibility_graph<Area>& vg,
                    area_polygon_t const& outer_polygon,
                    std::vector<inner_area_polygon_t> const& obstacles,
                    uint16_t i, uint16_t start_at) {
  auto& a = vg.nodes_[i];
  if (!a) {
    return;
  }
  for (uint16_t j = start_at; j < vg.n_; j++) {
    auto& b = vg.nodes_[j];
    if (i == j || a == b || !b) {
      continue;
    }
    auto const a_loc = get_merc(a);
    auto const b_loc = get_merc(b);
    if (a_loc == b_loc) {
      continue;
    }
    auto seg = merc_segment_t{a_loc, b_loc};
    shorten_segment(seg, 0.5);
    area_polygon_t seg_poly{{seg.first, seg.second, seg.first}};
    if (!boost::geometry::within(seg_poly,
                                 outer_polygon)) {  // does not support segments
      continue;
    }
    auto visible = true;
    for (auto const& obstacle : obstacles) {
      if (boost::geometry::intersects(seg, obstacle)) {
        visible = false;
        break;
      }
    }
    if (visible) {
      auto const dist = distance(a_loc, b_loc);
      vg.dist_matrix_.at(i, j) = dist;
      vg.dist_matrix_.at(j, i) = dist;
    }
  }
}

template <typename Area>
visibility_graph<Area> build_visibility_graph(Area* area) {
  visibility_graph<Area> vg(area);
  auto const outer_polygon = area->get_outer_polygon();
  auto const obstacles = area->get_inner_polygons();

  init_polygons(area, vg);

  for (uint16_t i = 0; i < vg.n_; i++) {
    calc_visiblity(vg, outer_polygon, obstacles, i, i + 1);
  }

  return vg;
}

template <typename Area>
visibility_graph<Area> extend_visibility_graph(
    Area const* area,
    std::vector<typename Area::point_type>& additional_points) {
  visibility_graph<Area> vg(area, additional_points);
  auto const outer_polygon = area->get_outer_polygon();
  auto const obstacles = area->get_inner_polygons();

  for (auto i = vg.base_size_; i < vg.n_; i++) {
    calc_visiblity(vg, outer_polygon, obstacles, i, 0);
  }

  return vg;
}

template <typename Node, typename MakeEdgeFn>
void make_vg_edges(visibility_graph<Node> const& vg, MakeEdgeFn make_edge) {
  for (auto const& i : vg.exit_nodes_) {
    for (auto const& j : vg.exit_nodes_) {
      if (vg.next_matrix_.at(i, j) == std::numeric_limits<uint16_t>::max()) {
        continue;
      }
      auto u = i;
      while (u != j) {
        auto const next_node = vg.next_matrix_.at(u, j);
        assert(u < vg.nodes_.size() && vg.nodes_[u]);
        assert(next_node < vg.nodes_.size() && vg.nodes_[next_node]);
        if (next_node == std::numeric_limits<uint16_t>::max()) {
          break;
        }
        make_edge(u, next_node);
        u = next_node;
      }
    }
  }
}

template <typename Area>
void reduce_visibility_graph(visibility_graph<Area>& vg,
                             uint16_t start_at = 1) {
  auto const infinity = std::numeric_limits<double>::max();
  auto const n = vg.n_;
  auto& dist = vg.dist_matrix_;
  auto& next = vg.next_matrix_;

  // direct edges from visibility graph
  for (uint16_t i = 0; i < n; i++) {
    for (uint16_t j = start_at; j < n; j++) {
      auto const d = dist.at(i, j);
      assert(std::equal_to<>()(dist.at(j, i), d));
      if (!std::equal_to<>()(d, infinity) && !std::equal_to<>()(d, 0)) {
        next.at(i, j) = j;
        next.at(j, i) = i;
      }
    }
  }

  // floyd warshall
  for (uint16_t k = 0; k < n; k++) {
    for (uint16_t i = 0; i < n; i++) {
      for (uint16_t j = start_at; j < n; j++) {
        auto const dist_to_k = dist.at(i, k);
        auto const dist_from_k = dist.at(k, j);
        if (std::equal_to<>()(dist_to_k, infinity) ||
            std::equal_to<>()(dist_from_k, infinity)) {
          continue;
        }
        auto const dist_via_k = dist_to_k + dist_from_k;
        assert(!std::equal_to<>()(dist.at(j, k), infinity) &&
               !std::equal_to<>()(dist.at(k, i), infinity));
        assert(std::fabs(dist.at(j, k) + dist.at(k, i) - dist_via_k) < 0.0001);
        if (dist.at(i, j) > dist_via_k) {
          dist.at(i, j) = dist_via_k;
          next.at(i, j) = next.at(i, k);
          dist.at(j, i) = dist_via_k;
          next.at(j, i) = next.at(j, k);
        }
      }
    }
  }
}

template <typename Area>
void reduce_extended_visibility_graph(visibility_graph<Area>& vg) {
  assert(vg.extended());
  reduce_visibility_graph(vg, vg.base_size_);
}

}  // namespace ppr
