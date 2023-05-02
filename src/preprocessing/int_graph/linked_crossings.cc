#include <algorithm>
#include <iostream>

#include "boost/geometry/geometries/geometries.hpp"
#include "boost/geometry/index/rtree.hpp"
#include "boost/iterator/function_output_iterator.hpp"

#include "ppr/preprocessing/int_graph/int_graph.h"
#include "ppr/preprocessing/int_graph/linked_crossings.h"
#include "ppr/preprocessing/int_graph/path.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace ppr::preprocessing {

using rtree_point_t = merc;
using rtree_value_t = std::pair<segment_t, int_edge*>;
using rtree_type = bgi::rtree<rtree_value_t, bgi::rstar<16>>;

namespace {

bool linked_on_one_side(int_edge const* e) {
  return e->linked_right_ != e->linked_left_;
}

void add_segments(std::vector<rtree_value_t>& values, int_edge* e,
                  side_type side) {
  if (!e->sidewalk(side, false)) {
    return;
  }
  auto const& path = e->path(side);
  for (std::size_t i = 0; i < path.size() - 1; i++) {
    values.emplace_back(segment_t{path[i], path[i + 1]}, e);
  }
}

rtree_type create_rtree(int_graph& ig) {
  std::vector<rtree_value_t> values;

  for (auto const& n : ig.nodes_) {
    for (auto& e : n->out_edges_) {
      if (e->is_linked()) {
        add_segments(values, e.get(), side_type::LEFT);
        add_segments(values, e.get(), side_type::RIGHT);
      }
    }
  }

  return rtree_type(values);
}

void remove_from_rtree(rtree_type& rtree, int_edge* e, side_type side) {
  if (!e->sidewalk(side, false)) {
    return;
  }
  auto const& path = e->path(side);
  for (std::size_t i = 0; i < path.size() - 1; i++) {
    rtree.remove({segment_t{path[i], path[i + 1]}, e});
  }
}

void remove_from_rtree(rtree_type& rtree, int_edge* e) {
  remove_from_rtree(rtree, e, side_type::LEFT);
  remove_from_rtree(rtree, e, side_type::RIGHT);
}

void add_to_rtree(rtree_type& rtree, int_edge* e) {
  std::vector<rtree_value_t> values;
  add_segments(values, e, side_type::LEFT);
  add_segments(values, e, side_type::RIGHT);
  rtree.insert(values);
}

bool has_crossing(int_node const* in) {
  return std::any_of(begin(in->out_edges_), end(in->out_edges_),
                     [](auto const& e) {
                       return e->info_->type_ == edge_type::CROSSING;
                     }) ||
         std::any_of(begin(in->in_edges_), end(in->in_edges_),
                     [](auto const& e) {
                       return e->info_->type_ == edge_type::CROSSING;
                     });
}

constexpr auto RAYCAST_LEN = 50.0;

std::vector<std::pair<double, int_edge*>> get_linked_edges(
    rtree_type const& rtree, segment_t const& ray, int_edge const* from_edge) {
  std::vector<std::pair<double, int_edge*>> results;
  rtree.query(
      bgi::intersects(ray) && bgi::satisfies([&](rtree_value_t const& v) {
        return v.second != from_edge;
      }),
      boost::make_function_output_iterator([&](auto& v) {
        results.emplace_back(bg::comparable_distance(v.first, ray), v.second);
      }));
  std::sort(begin(results), end(results),
            [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });
  return results;
}

// TODO(pablo): refactor (preprocessing)
struct node* create_node(routing_graph& rg, merc const& mc) {
  auto const loc = to_location(mc);
  rg.data_->nodes_.emplace_back(data::make_unique<struct node>(
      make_node(++rg.data_->max_node_id_, 0, loc)));
  return rg.data_->nodes_.back().get();
}

// TODO(pablo): refactor (preprocessing)
void set_node(int_edge* ie, node* n, bool reverse,
              side_type side = side_type::LEFT) {
  if (reverse) {
    if (side == side_type::LEFT) {
      ie->to_left_ = n;
    } else {
      ie->to_right_ = n;
    }
  } else {
    if (side == side_type::LEFT) {
      ie->from_left_ = n;
    } else {
      ie->from_right_ = n;
    }
  }
}

int_node* split_edge(int_graph& ig, routing_graph& rg, rtree_type& rtree,
                     int_edge* orig_edge, side_type side,
                     point_at_result const& cut_pt) {
  assert(orig_edge->sidewalk(side, false));
  assert(!orig_edge->sidewalk(side, true));

  remove_from_rtree(rtree, orig_edge);

  auto* end_node = orig_edge->to_;

  auto const orig_path = orig_edge->path(side);
  auto first_path = std::vector<merc>(
      begin(orig_path),
      std::next(begin(orig_path),
                static_cast<std::ptrdiff_t>(cut_pt.seg_from_idx_ + 1)));
  first_path.push_back(cut_pt.point_);
  auto second_path = std::vector<merc>(
      std::next(begin(orig_path),
                static_cast<std::ptrdiff_t>(cut_pt.seg_to_idx_)),
      end(orig_path));
  second_path.insert(begin(second_path), cut_pt.point_);

  ig.nodes_.emplace_back(
      std::make_unique<int_node>(0, cut_pt.point_, crossing_type::NONE));
  auto* mid_node = ig.nodes_.back().get();

  end_node->remove_incoming_edge(orig_edge);
  orig_edge->to_ = mid_node;
  orig_edge->path(side) = first_path;
  orig_edge->distance_ = path_length(first_path);
  auto const orig_to_angle = orig_edge->to_angle_;
  orig_edge->to_angle_ = get_normalized_angle(
      first_path[first_path.size() - 1] - first_path[first_path.size() - 2]);

  auto* end_rg_node = orig_edge->to(side, false);
  //  assert(end_rg_node != nullptr); // TODO

  auto* mid_rg_node = create_node(rg, cut_pt.point_);
  set_node(orig_edge, mid_rg_node, true, side);

  auto const mid_from_angle =
      get_normalized_angle(second_path[1] - second_path[0]);
  auto empty_path = std::vector<merc>{};
  auto second_left_path = side == side_type::LEFT ? second_path : empty_path;
  auto second_right_path = side == side_type::LEFT ? empty_path : second_path;
  mid_node->emplace_out_edge(
      orig_edge->info_, mid_node, end_node, path_length(second_path),
      std::move(second_left_path), std::move(second_right_path), mid_from_angle,
      orig_to_angle);
  auto* second_edge = mid_node->out_edges_.back().get();
  second_edge->sidewalk_left_ = orig_edge->sidewalk_left_;
  second_edge->sidewalk_right_ = orig_edge->sidewalk_right_;
  second_edge->linked_left_ = orig_edge->linked_left_;
  second_edge->linked_right_ = orig_edge->linked_right_;
  second_edge->layer_ = orig_edge->layer_;
  end_node->in_edges_.emplace_back(second_edge);
  set_node(second_edge, mid_rg_node, false, side);
  set_node(second_edge, end_rg_node, true, side);

  add_to_rtree(rtree, orig_edge);
  add_to_rtree(rtree, second_edge);

  return mid_node;
}

void set_street_name(edge_info* info, int_edge const* e1, int_edge const* e2,
                     int_graph& ig) {
  if (e1->info_->name_ == e2->info_->name_ || e2->info_->name_ == 0) {
    info->name_ = e1->info_->name_;
  } else if (e1->info_->name_ == 0) {
    info->name_ = e2->info_->name_;
  } else {
    auto const e1_name = ig.names_.at(e1->info_->name_).view();
    auto const e2_name = ig.names_.at(e2->info_->name_).view();
    auto const combined_name =
        std::string{e1_name} + ";" + std::string{e2_name};
    info->name_ = get_name(combined_name, ig.names_, ig.names_map_);
  }
}

void create_linked_crossing(int_graph& ig, routing_graph& rg, rtree_type& rtree,
                            int_edge* e, bool reverse, logging& log,
                            routing_graph_statistics& stats) {
  assert(linked_on_one_side(e));
  auto const this_side = e->linked_left_ ? side_type::RIGHT : side_type::LEFT;
  auto const normal_len =
      this_side == side_type::LEFT ? -RAYCAST_LEN : RAYCAST_LEN;
  assert(!e->is_linked(this_side, false));
  if (!e->sidewalk(this_side, false)) {
    return;
  }

  auto& this_path = e->path(this_side);
  auto const this_path_len = path_length(this_path);
  if (this_path_len < 3.0) {
    return;
  }

  auto step_size = std::min(3.0, this_path_len / 3.0);
  bool crossing_created = false;
  double dist = 0.0;
  while (!crossing_created && dist < 0.5 * this_path_len) {
    auto this_pt = point_at(this_path, dist, reverse);
    if (!this_pt.valid()) {
      log.out() << "linked crossing: did not find this_pt" << std::endl;
      return;
    }
    auto const normal = normal_len * this_pt.normal_;
    auto const ray = segment_t{this_pt.point_, this_pt.point_ + normal};
    auto const matches = get_linked_edges(rtree, ray, e);
    for (auto const& match : matches) {
      auto* other_edge = match.second;
      if (!linked_on_one_side(other_edge)) {
        continue;
      }
      auto const other_side =
          other_edge->linked_left_ ? side_type::RIGHT : side_type::LEFT;
      auto& other_path = other_edge->path(other_side);
      assert(!other_path.empty());
      auto const other_pt = intersect_path(other_path, ray);
      assert(other_pt.valid());
      if (!other_pt) {
        log.out() << "linked crossing: did not find other_pt" << std::endl;
        continue;
      }

      auto const other_angle = get_angle_between(normal, other_pt.dir_);
      if (!is_angle_around(other_angle, 90, 10)) {
        continue;
      }

      auto const crossing_dist = distance(this_pt.point_, other_pt.point_);
      if (crossing_dist < 3.0) {
        continue;
      }

      auto const other_path_len = path_length(other_path);

      crossing_created = true;
      stats.n_linked_crossings_created_++;

      auto* from_node = e->int_from(reverse);
      if (this_pt.offset_from_start_ >= 3.0) {
        from_node = split_edge(ig, rg, rtree, e, this_side, this_pt);
      }
      int_node* to_node = nullptr;
      if (other_pt.offset_from_start_ < 3.0) {
        to_node = other_edge->from_;
      } else if (other_pt.offset_from_start_ > other_path_len - 3.0) {
        to_node = other_edge->to_;
      } else {
        to_node = split_edge(ig, rg, rtree, other_edge, other_side, other_pt);
      }

      auto crossing_path =
          std::vector<merc>{from_node->location_, to_node->location_};
      auto const crossing_street_type = static_cast<street_type>(
          std::max(static_cast<uint8_t>(e->info_->street_type_),
                   static_cast<uint8_t>(other_edge->info_->street_type_)));
      auto* info =
          ig.edge_infos_
              .emplace_back(data::make_unique<edge_info>(make_edge_info(
                  -e->info_->osm_way_id_, edge_type::CROSSING,
                  crossing_street_type, crossing_type::GENERATED)))
              .get();
      set_street_name(info, e, other_edge, ig);
      auto const crossing_angle =
          get_normalized_angle(to_node->location_ - from_node->location_);
      from_node->emplace_out_edge(
          info, from_node, to_node,
          distance(from_node->location_, to_node->location_),
          std::move(crossing_path), std::vector<merc>(), crossing_angle,
          crossing_angle);
      auto* crossing_edge = from_node->out_edges_.back().get();
      to_node->in_edges_.emplace_back(crossing_edge);
      from_node->generated_crossing_edges_ = true;
      to_node->generated_crossing_edges_ = true;
      assert(!(crossing_edge->path_left_.empty() &&
               crossing_edge->path_right_.empty()));
      break;
    }
    dist += step_size;
  }
}

void create_linked_crossings(int_graph& ig, routing_graph& rg,
                             rtree_type& rtree, int_edge* e, logging& log,
                             routing_graph_statistics& stats) {
  auto const has_start_crossing = has_crossing(e->from_);
  if (!has_start_crossing) {
    create_linked_crossing(ig, rg, rtree, e, false, log, stats);
  }
  auto const has_end_crossing = has_crossing(e->to_);
  if (!has_end_crossing) {
    create_linked_crossing(ig, rg, rtree, e, true, log, stats);
  }
}

}  // namespace

void add_linked_crossings(int_graph& ig, routing_graph& rg, logging& log,
                          step_progress& progress,
                          routing_graph_statistics& stats) {
  auto rtree = create_rtree(ig);

  auto const node_size = ig.nodes_.size();
  for (std::size_t i = 0; i < node_size; i++) {
    auto const& n = ig.nodes_[i];
    std::vector<int_edge*> edges;
    for (auto& e : n->out_edges_) {
      if (linked_on_one_side(e.get())) {
        edges.push_back(e.get());
      }
    }
    for (auto& e : edges) {
      create_linked_crossings(ig, rg, rtree, e, log, stats);
    }
    progress.add();
  }
}

}  // namespace ppr::preprocessing
