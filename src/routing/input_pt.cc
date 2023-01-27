#include <cmath>
#include <algorithm>
#include <iostream>
#include <optional>

#include "boost/geometry/algorithms/comparable_distance.hpp"
#include "boost/geometry/algorithms/for_each.hpp"
#include "boost/geometry/geometries/geometries.hpp"

#include "utl/erase_if.h"

#include "ppr/common/geometry/path_conversion.h"
#include "ppr/routing/input_pt.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace ppr::routing {

location nearest_pt_on_segment(location const& loc, location const& seg_from,
                               location const& seg_to) {
  auto const merc_loc = to_merc(loc);
  auto const merc_seg_from = to_merc(seg_from);
  auto const merc_seg_to = to_merc(seg_to);
  auto seg_dir = merc_seg_to - merc_seg_from;
  auto const seg_len = seg_dir.length();
  if (seg_len < 0.000000001) {
    return seg_from;
  }
  auto const start_vec = merc_loc - merc_seg_from;
  auto const end_vec = merc_loc - merc_seg_to;
  auto const start_angle =
      std::acos(seg_dir.dot(start_vec) / (seg_len * start_vec.length()));
  auto const end_angle =
      std::acos(seg_dir.dot(end_vec) / (seg_len * end_vec.length()));

  if (start_angle >= to_rad(90)) {
    return seg_from;
  } else if (end_angle <= to_rad(90)) {
    return seg_to;
  } else {
    // law of sines
    auto const beta = to_rad(90) - start_angle;
    auto seg_offset = start_vec.length() * std::sin(beta);
    seg_dir.normalize();
    auto const point = to_location(merc_seg_from + seg_offset * seg_dir);
    return point;
  }
}

input_pt nearest_pt_on_edge(edge const* e, location const& loc) {
  using loc_segment_t = boost::geometry::model::segment<location>;
  if (e == nullptr) {
    return {};
  }
  assert(!e->path_.empty());
  double min_dist = std::numeric_limits<double>::max();
  std::size_t nearest_segment = 0;
  auto const& path = e->path_;
  for (std::size_t i = 0; i < path.size() - 1; i++) {
    auto seg = loc_segment_t{path[i], path[i + 1]};
    auto const dist = bg::comparable_distance(loc, seg);
    if (dist < min_dist) {
      min_dist = dist;
      nearest_segment = i;
    }
  }
  location point = nearest_pt_on_segment(loc, path[nearest_segment],
                                         path[nearest_segment + 1]);
  data::vector<location> from_path, to_path;
  std::copy(begin(path), begin(path) + nearest_segment + 1,
            std::back_inserter(from_path));
  from_path.push_back(point);
  std::reverse(begin(from_path), end(from_path));
  to_path.push_back(point);
  std::copy(begin(path) + nearest_segment + 1, end(path),
            std::back_inserter(to_path));

  return {loc, point, e, std::move(from_path), std::move(to_path)};
}

std::vector<edge const*> nearest_edges(
    routing_graph const& g, location const& loc,
    std::optional<std::int16_t> const& opt_level, routing_options const& opt,
    unsigned max_query, unsigned max_count, double max_dist) {
  auto const level = opt_level.value_or(0);
  auto const check_level = opt_level.has_value();
  auto const level_penalty = [&](edge const* e) {
    return opt.level_dist_penalty_ * (std::abs(level - e->info_->level_));
  };

  std::vector<routing_graph::edge_rtree_value_type> results;
  g.edge_rtree_->query(bgi::nearest(loc, max_query),
                       std::back_inserter(results));
  if (results.empty()) {
    std::clog << "nearest_edges not found!" << std::endl;
  }
  std::sort(begin(results), end(results), [&](auto const& a, auto const& b) {
    auto const* a_edge = a.second.get(g.data_);
    auto const* b_edge = b.second.get(g.data_);
    auto const a_dist = bg::comparable_distance(loc, a_edge->path_);
    auto const b_dist = bg::comparable_distance(loc, b_edge->path_);
    if (check_level) {
      return a_dist + level_penalty(a_edge) < b_dist + level_penalty(b_edge);
    } else {
      return a_dist < b_dist;
    }
  });
  std::vector<edge const*> edges;
  edges.reserve(max_count);
  for (auto const& result : results) {
    auto const* e = result.second.get(g.data_);
    if (check_level && opt.force_level_match_ && e->info_->level_ != level) {
      continue;
    }
    if (edges.size() >= max_count || distance(loc, e->path_) > max_dist) {
      break;
    }
    edges.push_back(e);
  }
  return edges;
}

void find_nearest_edges(routing_graph const& g, std::vector<input_pt>& out_pts,
                        location const& loc,
                        std::optional<std::int16_t> const& opt_level,
                        routing_options const& opt, unsigned max_query,
                        unsigned max_count, double max_dist) {
  auto edges =
      nearest_edges(g, loc, opt_level, opt, max_query, max_count, max_dist);
  std::transform(begin(edges), end(edges), std::back_inserter(out_pts),
                 [&](edge const* e) { return nearest_pt_on_edge(e, loc); });
}

void print_area_info(area const* a) {
  std::cout << "{area osm = " << (a->from_way_ ? "way " : "relation ")
            << a->osm_id_
            << ", name = " << (a->name_ == nullptr ? "" : a->name_->data())
            << ", outer ring with " << a->polygon_.outer_.size() << " nodes, "
            << a->polygon_.inner_.size() << " inner rings}, "
            << a->count_mapped_nodes() << " mapped nodes" << std::endl;
}

bool find_containing_areas(routing_graph const& g,
                           std::vector<input_pt>& out_pts, location const& loc,
                           std::optional<std::int16_t> const& level,
                           routing_options const& opt) {
  auto const force_level = level && opt.force_level_match_;
  std::vector<routing_graph::area_rtree_value_type> results;
  g.area_rtree_->query(
      bgi::contains(loc) &&
          bgi::satisfies([&](routing_graph::area_rtree_value_type const& val) {
            auto const& a = g.data_->areas_[val.second];
            return bg::within(loc, a.polygon_) &&
                   (!force_level || a.level_ == *level);
          }),
      std::back_inserter(results));
  std::transform(begin(results), end(results), std::back_inserter(out_pts),
                 [&](routing_graph::area_rtree_value_type const& val) {
                   return input_pt(loc, &g.data_->areas_[val.second]);
                 });
  return !results.empty();
}

void map_to_area_border(area const* a, input_pt& pt) {
  using loc_segment_t = boost::geometry::model::segment<location>;

  auto const& loc = pt.input_;
  double min_dist = std::numeric_limits<double>::max();
  location seg_from, seg_to;

  auto const map_to_ring = [&](auto const& ring) {
    for (std::size_t i = 0; i < ring.size() - 1; i++) {
      auto const& from = ring[i].location_;
      auto const& to = ring[i + 1].location_;
      auto seg = loc_segment_t{from, to};
      auto const dist = bg::comparable_distance(loc, seg);
      if (dist < min_dist) {
        min_dist = dist;
        seg_from = from;
        seg_to = to;
      }
    }
  };

  map_to_ring(a->polygon_.outer());
  for (auto const& inner : a->polygon_.inners()) {
    map_to_ring(inner);
  }

  pt.in_area_ = a;
  pt.outside_of_area_ = true;
  pt.nearest_pt_ = nearest_pt_on_segment(loc, seg_from, seg_to);
}

void find_nearest_areas(routing_graph const& g, std::vector<input_pt>& out_pts,
                        location const& loc,
                        std::optional<std::int16_t> const& opt_level,
                        routing_options const& opt, unsigned max_query,
                        unsigned max_count, double max_dist) {
  auto const level = opt_level.value_or(0);
  auto const check_level = opt_level.has_value();
  auto const level_penalty = [&](area const& a) {
    return opt.level_dist_penalty_ * (std::abs(level - a.level_));
  };

  std::vector<routing_graph::area_rtree_value_type> results;
  g.area_rtree_->query(bgi::nearest(loc, max_query),
                       std::back_inserter(results));
  std::sort(begin(results), end(results), [&](auto const& a, auto const& b) {
    auto const& a_area = g.data_->areas_[a.second];
    auto const& b_area = g.data_->areas_[b.second];
    auto const a_dist = bg::comparable_distance(loc, a_area.polygon_);
    auto const b_dist = bg::comparable_distance(loc, b_area.polygon_);
    if (check_level) {
      return a_dist + level_penalty(a_area) < b_dist + level_penalty(b_area);
    } else {
      return a_dist < b_dist;
    }
  });
  for (auto const& result : results) {
    auto* a = &g.data_->areas_[result.second];
    if (check_level && opt.force_level_match_ && a->level_ != level) {
      continue;
    }
    if (bg::within(loc, a->polygon_)) {
      continue;
    }
    if (out_pts.size() >= max_count || distance(loc, a->polygon_) > max_dist) {
      break;
    }
    auto& pt = out_pts.emplace_back(loc, a);
    map_to_area_border(a, pt);
  }
}

inline bool level_found(std::vector<input_pt> const& pts,
                        std::optional<int> opt_level) {
  if (opt_level) {
    auto const level = *opt_level;
    return std::any_of(begin(pts), end(pts),
                       [&](auto const& pt) { return pt.level_ == level; });
  } else {
    return !pts.empty();
  }
}

std::vector<input_pt> resolve_input_location(routing_graph const& g,
                                             input_location const& il,
                                             routing_options const& opt,
                                             bool const expanded) {
  std::vector<input_pt> pts;

  // TODO(pablo): osm element

  if (il.location_) {
    auto const& loc = *il.location_;

    find_containing_areas(g, pts, loc, il.level_, opt);

    auto const area_count = static_cast<unsigned>(pts.size());
    auto const max_count = opt.max_pt_count(expanded);
    if (area_count < max_count || !level_found(pts, il.level_)) {
      auto const max_query = opt.max_pt_query(expanded);
      auto const max_dist =
          expanded ? il.expanded_max_distance_ : il.initial_max_distance_;
      auto const max_pts = max_count - area_count;
      find_nearest_areas(g, pts, loc, il.level_, opt, max_query, max_pts,
                         max_dist);
      find_nearest_edges(g, pts, loc, il.level_, opt, max_query, max_pts,
                         max_dist);
      if (pts.size() > max_count) {
        if (il.level_ && !opt.force_level_match_) {
          auto const level = *il.level_;
          auto const level_penalty = [&](input_pt const& pt) {
            return opt.level_dist_penalty_ * (std::abs(level - pt.level_));
          };
          std::sort(begin(pts), end(pts),
                    [&](input_pt const& a, input_pt const& b) {
                      return bg::comparable_distance(loc, a.nearest_pt_) +
                                 level_penalty(a) <
                             bg::comparable_distance(loc, b.nearest_pt_) +
                                 level_penalty(b);
                    });
        } else {
          std::sort(begin(pts), end(pts),
                    [&](input_pt const& a, input_pt const& b) {
                      return bg::comparable_distance(loc, a.nearest_pt_) <
                             bg::comparable_distance(loc, b.nearest_pt_);
                    });
        }
        pts.resize(max_count);
      }
    }
  }

  return pts;
}

}  // namespace ppr::routing
