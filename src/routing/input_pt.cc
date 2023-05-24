#include <cassert>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <optional>

#include "boost/geometry/algorithms/comparable_distance.hpp"
#include "boost/geometry/algorithms/for_each.hpp"
#include "boost/geometry/geometries/geometries.hpp"

#include "boost/iterator/function_output_iterator.hpp"

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

  if (merc_loc == merc_seg_from || merc_loc == merc_seg_to) {
    return loc;
  }

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

  assert(!std::isnan(start_angle) && !std::isnan(end_angle));

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

input_pt nearest_pt_on_edge(routing_graph_data const& rg, edge const* e,
                            location const& loc) {
  using loc_segment_t = boost::geometry::model::segment<location>;
  if (e == nullptr) {
    return {};
  }
  assert(!e->path_.empty());
  double min_dist = std::numeric_limits<double>::max();
  auto nearest_segment = 0U;
  auto const& path = e->path_;
  for (auto i = 0U; i < path.size() - 1; i++) {
    auto seg = loc_segment_t{path[i], path[i + 1]};
    auto const dist = bg::comparable_distance(loc, seg);
    if (dist < min_dist) {
      min_dist = dist;
      nearest_segment = i;
    }
  }
  auto const point = nearest_pt_on_segment(loc, path[nearest_segment],
                                           path[nearest_segment + 1]);
  data::vector<location> from_path, to_path;
  std::copy(begin(path), begin(path) + nearest_segment + 1,
            std::back_inserter(from_path));
  from_path.push_back(point);
  std::reverse(begin(from_path), end(from_path));
  to_path.push_back(point);
  std::copy(begin(path) + nearest_segment + 1, end(path),
            std::back_inserter(to_path));

  return {rg, loc, point, e, std::move(from_path), std::move(to_path)};
}

std::vector<std::pair<edge const*, double>> nearest_edges(
    routing_graph const& g, location const& loc,
    std::optional<std::int16_t> const& opt_level, routing_options const& opt,
    unsigned max_query, unsigned max_count, double max_dist) {
  auto const level = opt_level.value_or(0);
  auto const check_level = opt_level.has_value();
  auto const level_penalty = [&](edge const* e) {
    return opt.level_dist_penalty_ * (std::abs(level - e->info(g)->level_));
  };

  auto edges = std::vector<std::pair<edge const*, double>>{};
  edges.reserve(max_query);
  g.edge_rtree_->query(
      bgi::nearest(loc, max_query),
      boost::make_function_output_iterator([&](auto const& entry) {
        auto const* e = entry.second.get(g.data_);
        auto const dist = distance(loc, e->path_);
        if (check_level && opt.force_level_match_ &&
            e->info(g)->level_ != level) {
          return;
        }
        if (dist <= max_dist) {
          edges.emplace_back(e, check_level ? dist + level_penalty(e) : dist);
        }
      }));
  if (edges.empty()) {
    std::clog << "nearest_edges not found!" << std::endl;
  }
  std::sort(begin(edges), end(edges),
            [&](auto const& a, auto const& b) { return a.second < b.second; });
  if (edges.size() > max_count) {
    edges.resize(max_count);
  }
  return edges;
}

void find_nearest_edges(routing_graph const& g, std::vector<input_pt>& out_pts,
                        location const& loc,
                        std::optional<std::int16_t> const& opt_level,
                        routing_options const& opt, unsigned max_query,
                        unsigned max_count, double max_dist) {
  auto const edges =
      nearest_edges(g, loc, opt_level, opt, max_query, max_count, max_dist);
  std::transform(begin(edges), end(edges), std::back_inserter(out_pts),
                 [&](auto const& edge_with_dist) {
                   return nearest_pt_on_edge(*g.data_, edge_with_dist.first,
                                             loc);
                 });
}

void print_area_info(routing_graph_data const& rg, area const* a) {
  std::cout << "{area osm = " << (a->from_way_ ? "way " : "relation ")
            << a->osm_id_ << ", name = "
            << (a->name_ == 0 ? "" : rg.names_.at(a->name_).view())
            << ", outer ring with " << a->polygon_.outer_.size() << " nodes, "
            << a->polygon_.inner_.size() << " inner rings}, "
            << a->count_mapped_nodes() << " mapped nodes" << std::endl;
}

bool find_containing_areas(routing_graph const& g,
                           std::vector<input_pt>& out_pts, location const& loc,
                           std::optional<std::int16_t> const& level,
                           routing_options const& opt) {
  auto const force_level = level && opt.force_level_match_;
  auto found_areas = false;
  g.area_rtree_->query(
      bgi::contains(loc) &&
          bgi::satisfies([&](routing_graph::area_rtree_value_type const& val) {
            auto const& a = g.data_->areas_[val.second];
            return bg::within(loc, a.polygon_) &&
                   (!force_level || a.level_ == *level);
          }),
      boost::make_function_output_iterator([&](auto const& entry) {
        found_areas = true;
        out_pts.emplace_back(input_pt(loc, &g.data_->areas_[entry.second]));
      }));
  return found_areas;
}

void map_to_area_border(area const* a, input_pt& pt) {
  using loc_segment_t = boost::geometry::model::segment<location>;

  auto const& loc = pt.input_;
  double min_dist = std::numeric_limits<double>::max();
  location seg_from, seg_to;

  auto const map_to_ring = [&](auto const& ring) {
    for (auto i = 0U; i < ring.size() - 1; i++) {
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
  auto const level_penalty = [&](area const* a) {
    return opt.level_dist_penalty_ * (std::abs(level - a->level_));
  };

  auto areas = std::vector<std::pair<area const*, double>>{};
  g.area_rtree_->query(
      bgi::nearest(loc, max_query),
      boost::make_function_output_iterator([&](auto const& entry) {
        auto const* a = &g.data_->areas_[entry.second];

        if (check_level && opt.force_level_match_ && a->level_ != level) {
          return;
        }

        if (bg::within(loc, a->polygon_)) {
          return;
        }

        auto const dist = distance(loc, a->polygon_);
        if (dist > max_dist) {
          return;
        }

        areas.emplace_back(a, check_level ? dist + level_penalty(a) : dist);
      }));
  std::sort(begin(areas), end(areas),
            [&](auto const& a, auto const& b) { return a.second < b.second; });
  if (areas.size() > max_count) {
    areas.resize(max_count);
  }
  for (auto const& [a, dist] : areas) {
    auto& pt = out_pts.emplace_back(loc, a);
    if (dist >= 0.0) {
      map_to_area_border(a, pt);
    }
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

void resolve_input_area(std::vector<input_pt>& out_pts, area const& a,
                        input_location const& il) {
  out_pts.emplace_back(input_pt{il.location_.value_or(a.center_), &a});
}

std::vector<input_pt> resolve_input_location(routing_graph const& g,
                                             input_location const& il,
                                             routing_options const& opt,
                                             bool const expanded) {
  std::vector<input_pt> pts;

  if (il.osm_element_) {
    auto const& osm = *il.osm_element_;
    switch (osm.type_) {
      case osm_namespace::WAY: {
        if (auto const area_it = g.osm_index_->ways_to_areas_.find(osm.id_);
            area_it != end(g.osm_index_->ways_to_areas_)) {
          auto const& a = g.data_->areas_.at(area_it->second);
          resolve_input_area(pts, a, il);
        }
        break;
      }
      case osm_namespace::RELATION: {
        if (auto const area_it =
                g.osm_index_->relations_to_areas_.find(osm.id_);
            area_it != end(g.osm_index_->relations_to_areas_)) {
          auto const& a = g.data_->areas_.at(area_it->second);
          resolve_input_area(pts, a, il);
        }
        break;
      }
      default: break;
    }
    if (!opt.allow_osm_id_expansion_ || (!pts.empty() && !expanded)) {
      return pts;
    }
  }

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
