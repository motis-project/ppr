#include <algorithm>

#include "ppr/preprocessing/geo_util.h"
#include "ppr/preprocessing/junction.h"
#include "ppr/preprocessing/osm_graph/move_crossings.h"

namespace bg = boost::geometry;

namespace ppr::preprocessing {

namespace {

using oriented_osm_edge = junction_edge<osm_edge>;
using segment_t = bg::model::segment<merc>;

std::vector<oriented_osm_edge> edges_sorted_by_angle(osm_node* center) {
  std::vector<oriented_osm_edge> sorted_edges;

  for (auto* e : center->all_edges()) {
    auto const reverse = e->to_ == center;
    auto const angle = e->normalized_angle(reverse);
    sorted_edges.emplace_back(e, reverse, angle);
  }
  std::sort(begin(sorted_edges), end(sorted_edges));

  return sorted_edges;
}

double distance(osm_edge const* edge, merc const& point) {
  return bg::distance(segment_t{edge->from_->location_, edge->to_->location_},
                      point) *
         scale_factor(point);
}

void move_away(oriented_osm_edge const& e1, oriented_osm_edge& e2, double dist,
               double target_dist) {
  auto const* center = e1.edge_->osm_from(e1.reverse_);
  auto* target_node = e2.edge_->osm_to(e2.reverse_);
  auto const old_loc = target_node->location_;
  auto e1_dir = e1.edge_->to_->location_ - e1.edge_->from_->location_;
  auto const left = side_of(e1_dir, old_loc - center->location_) > 0;
  e1_dir.normalize();
  auto const normal = e1_dir.normal(left);
  auto const new_loc = old_loc + (target_dist - dist + 1.0) * normal;
  target_node->location_ = new_loc;
}

void check_edge(oriented_osm_edge const& e1, oriented_osm_edge& e2,
                osm_graph_statistics& stats) {
  auto* other_node = e2.edge_->osm_to(e2.reverse_);
  if (other_node->street_edges_ > 2) {
    // don't move other junctions
    return;
  }
  auto const dist = distance(e1.edge_, other_node->location_);
  auto const target_dist = e1.edge_->width_ / 2.0;
  if (dist <= target_dist) {
    move_away(e1, e2, dist, target_dist);
    stats.n_moved_crossing_nodes_++;
    for (auto& e3 : edges_sorted_by_angle(other_node)) {
      if (e3.edge_ == e2.edge_ ||
          e3.edge_->info_->osm_way_id_ == e2.edge_->info_->osm_way_id_) {
        continue;
      }
      check_edge(e1, e3, stats);
    }
  }
}

void check_street_junction(osm_node* n, osm_graph_statistics& stats) {
  auto sorted_edges = edges_sorted_by_angle(n);
  for_edge_pairs_ccw(
      sorted_edges,
      [](oriented_osm_edge& e1) { return e1.edge_->generate_sidewalks(); },
      [&](oriented_osm_edge& e1, oriented_osm_edge& e2) {
        //        if (!e2.edge_->generate_sidewalks()) {
        //          return false; // TODO: why
        //        }
        check_edge(e1, e2, stats);
        return true;
      });
}

}  // namespace

void move_crossings(osm_graph& og, logging& log, osm_graph_statistics& stats) {
  step_progress progress{log, pp_step::INT_MOVE_CROSSINGS, og.nodes_.size()};
  for (auto& n : og.nodes_) {
    if (n->street_edges_ > 2) {
      check_street_junction(n.get(), stats);
    }
  }
  progress.add();
}

}  // namespace ppr::preprocessing
