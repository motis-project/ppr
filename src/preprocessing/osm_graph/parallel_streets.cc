#include <iostream>
#include <vector>

#include "boost/geometry/geometries/geometries.hpp"
#include "boost/geometry/index/rtree.hpp"

#include "ppr/preprocessing/geo_util.h"
#include "ppr/preprocessing/osm_graph/parallel_streets.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace ppr::preprocessing {

using segment_t = bg::model::segment<merc>;
using box_t = bg::model::box<merc>;
using polygon_t = bg::model::polygon<merc>;
using rtree_point_t = merc;
using rtree_value_t = std::pair<segment_t, osm_edge*>;
using rtree_type = bgi::rtree<rtree_value_t, bgi::rstar<16>>;

constexpr int street_class(street_type street) {
  switch (street) {
    case street_type::SERVICE: return 1;
    case street_type::PEDESTRIAN:
    case street_type::LIVING:
    case street_type::RESIDENTIAL:
    case street_type::UNCLASSIFIED: return 2;
    case street_type::TERTIARY:
    case street_type::SECONDARY:
    case street_type::PRIMARY: return 3;
    default: return 0;
  }
}

// index: street_class
constexpr int MAX_ALLOWED_DISTANCE[] = {0, 8, 8, 35};

inline bool include_edge(osm_edge const& e) {
  auto const cls = street_class(e.info_->street_type_);
  auto const oneway = e.info_->oneway_street_;
  return e.generate_sidewalks() && (cls == 3 || (cls == 2 && oneway));
}

rtree_type create_segment_rtree(osm_graph& og) {
  std::vector<rtree_value_t> values;

  for (auto const& n : og.nodes_) {
    for (auto& e : n->out_edges_) {
      if (include_edge(e)) {
        auto segment = segment_t(e.from_->location_, e.to_->location_);
        values.emplace_back(segment, &e);
      }
    }
  }

  return rtree_type(values);
}

constexpr auto PARALLEL_BOX_SIZE = 25.0;
constexpr auto REQ_OVERLAP = 0.70;

std::vector<rtree_value_t> find_segments_near(rtree_type const& rtree,
                                              osm_edge const& e) {
  auto from = e.from_->location_;
  auto to = e.to_->location_;
  if (from == to) {
    return {};
  }
  auto const scale = scale_factor(from);
  auto direction = to - from;
  auto length = direction.length();
  direction.normalize();
  auto normal = direction.normal();
  normal *= PARALLEL_BOX_SIZE / scale;
  auto offset = direction / scale;
  while (offset.length() > length / 2) {
    offset /= 2;
  }
  from += offset;
  to -= offset;
  auto box_points = {from - normal, from + normal, to + normal, to - normal,
                     from - normal};
  auto query_box = bg::return_envelope<box_t>(std::vector<merc>(box_points));
  auto query_polygon = polygon_t({box_points});
  auto const way_id = e.info_->osm_way_id_;
  auto const layer = e.layer_;
  auto const e_class = street_class(e.info_->street_type_);
  std::vector<rtree_value_t> results;
  rtree.query(
      bgi::intersects(query_box) && bgi::satisfies([&](rtree_value_t const& v) {
        auto const o = v.second;
        auto const seg = v.first;
        return o->info_->osm_way_id_ != way_id && o->layer_ == layer &&
               street_class(o->info_->street_type_) == e_class &&
               bg::intersects(query_polygon, seg);
      }),
      std::back_inserter(results));
  return results;
}

std::pair<osm_node*, osm_node*> check_common_point(osm_node* ref1,
                                                   osm_node* ref2,
                                                   osm_node* other1,
                                                   osm_node* other2) {
  if (ref1 == other1 || ref2 == other1) {
    return {other1, other2};
  } else if (ref1 == other2 || ref2 == other2) {
    return {other2, other1};
  } else {
    return {nullptr, nullptr};
  }
}

// https://en.wikipedia.org/wiki/Scalar_projection
inline double scalar_projection(merc const& a, merc const& base,
                                double base_len) {
  return std::abs(a.dot(base) / base_len);
}

inline void check_edge(rtree_type const& rtree, osm_edge& e,
                       osm_graph_statistics& stats) {
  auto near_segments = find_segments_near(rtree, e);
  if (near_segments.empty()) {
    return;
  }

  auto const e_dir = e.to_->location_ - e.from_->location_;
  auto const e_seg = segment_t(e.from_->location_, e.to_->location_);
  auto const e_len = e_dir.length();
  auto const max_allowed_distance =
      MAX_ALLOWED_DISTANCE[street_class(e.info_->street_type_)];  // NOLINT

  osm_edge* nearest_left_edge = nullptr;
  osm_edge* nearest_right_edge = nullptr;
  double nearest_left_distance = std::numeric_limits<double>::max();
  double nearest_right_distance = std::numeric_limits<double>::max();
  double left_overlap = 0;
  double right_overlap = 0;

  for (auto const& p : near_segments) {
    auto const other = p.second;
    auto const other_dir = other->to_->location_ - other->from_->location_;
    auto const angle = get_angle_between(e_dir, other_dir);

    osm_node* common_pt;
    osm_node* other_pt;
    std::tie(common_pt, other_pt) =
        check_common_point(e.from_, e.to_, other->from_, other->to_);

    auto const parallel =
        common_pt != nullptr ? is_parallel(angle, 60) : is_parallel(angle, 45);

    if (!parallel) {
      continue;
    }

    auto other_seg = segment_t(other->from_->location_, other->to_->location_);

    if ((common_pt == nullptr) && bg::intersects(e_seg, other_seg)) {
      continue;
    }

    auto const proj_len = scalar_projection(other_dir, e_dir, e_len);
    auto const dist = other_pt != nullptr
                          ? bg::distance(e_seg, other_pt->location_)
                          : bg::distance(e_seg, other_seg);
    if (dist > max_allowed_distance) {
      continue;
    }
    auto const side =
        other_pt != nullptr
            ? side_of(e_dir, other_pt->location_ - common_pt->location_)
            : side_of(e.from_->location_, e.to_->location_, e_dir,
                      other->from_->location_, other->to_->location_, other_dir,
                      angle);

    auto const left = side > 0;
    if (left) {
      left_overlap += proj_len;
      if (dist < nearest_left_distance) {
        nearest_left_edge = other;
        nearest_left_distance = dist;
      }
    } else {
      right_overlap += proj_len;
      if (dist < nearest_right_distance) {
        nearest_right_edge = other;
        nearest_right_distance = dist;
      }
    }
  }

  auto const left_overlap_ratio = left_overlap / e_len;
  auto const right_overlap_ratio = right_overlap / e_len;

  if ((nearest_left_edge != nullptr) && left_overlap_ratio >= REQ_OVERLAP) {
    e.sidewalk_left_ = false;
    if (e.linked_left_ != nullptr) {
      std::cerr << "error - linked_left already set!" << std::endl;
    }
    e.linked_left_ = nearest_left_edge;
  }
  if ((nearest_right_edge != nullptr) && right_overlap_ratio >= REQ_OVERLAP) {
    e.sidewalk_right_ = false;
    if (e.linked_right_ != nullptr) {
      std::cerr << "error - linked_right already set!" << std::endl;
    }
    e.linked_right_ = nearest_right_edge;
  }

  if (e.linked_left_ != nullptr || e.linked_right_ != nullptr) {
    stats.n_linked_edges_++;
    if (e.linked_left_ != nullptr && e.linked_right_ != nullptr) {
      stats.n_double_linked_edges_++;
    }
  }
}

void detect_parallel_streets(osm_graph& og, osm_graph_statistics& stats) {
  auto rtree = create_segment_rtree(og);

  for (auto const& n : og.nodes_) {
    for (auto& e : n->out_edges_) {
      if (include_edge(e)) {
        check_edge(rtree, e, stats);
      }
    }
  }
}

}  // namespace ppr::preprocessing
