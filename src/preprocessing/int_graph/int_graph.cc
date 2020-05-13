#include <algorithm>
#include <iostream>

#include "ppr/common/timing.h"
#include "ppr/preprocessing/int_graph/int_graph.h"
#include "ppr/preprocessing/int_graph/sidewalks.h"
#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/osm_graph/move_crossings.h"
#include "ppr/preprocessing/osm_graph/osm_graph.h"
#include "ppr/preprocessing/osm_graph/parallel_streets.h"

namespace ppr::preprocessing {

struct int_graph_builder {
  int_graph_builder(osm_graph& og, int_graph& ig, options const& opt,
                    logging& log, statistics& stats)
      : og_(og), ig_(ig), opt_(opt), log_(log), stats_(stats) {}

  void build() {
    auto const t_start = timing_now();

    detect_parallel_streets(og_, log_, stats_.osm_);
    stats_.int_.d_parallel_streets_ =
        log_.get_step_duration(pp_step::INT_PARALLEL_STREETS);

    if (opt_.move_crossings_) {
      move_crossings(og_, log_, stats_.osm_);
    }
    stats_.int_.d_move_crossings_ =
        log_.get_step_duration(pp_step::INT_MOVE_CROSSINGS);

    compress_edges();
    stats_.int_.d_handle_edges_ = log_.get_step_duration(pp_step::INT_EDGES);

    build_areas();
    stats_.int_.d_areas_ = log_.get_step_duration(pp_step::INT_AREAS);

    ig_.create_in_edges();
    ig_.count_edges();
    stats_.int_.d_total_ = ms_since(t_start);
  }

private:
  void compress_edges() {
    step_progress progress{log_, pp_step::INT_EDGES, og_.nodes_.size()};
    for (auto& n : og_.nodes_) {
      if (!n->compressed_) {
        for (auto& e : n->out_edges_) {
          handle_edge(e);
        }
      }
      progress.add();
    }
  }

  void handle_edge(osm_edge& oe) {
    if (oe.processed_) {
      return;
    }

    elevation_diff_t elevation_up = 0;
    elevation_diff_t elevation_down = 0;
    edge_info* info = oe.info_;
    assert(!(info->area_ && oe.generate_sidewalks()));
    auto const sidewalk_left = oe.sidewalk_left_;
    auto const sidewalk_right = oe.sidewalk_right_;
    auto const linked_left = oe.linked_left_ != nullptr;
    auto const linked_right = oe.linked_right_ != nullptr;
    auto from = oe.from_;
    auto to = oe.to_;
    std::vector<merc> path{from->location_, to->location_};
    std::vector<osm_edge*> edges{&oe};
    double distance = oe.distance_;
    oe.processed_ = true;

    auto const different_edge_attrs = [&](osm_edge* o) {
      return o->info_ != info || o->processed_ ||
             o->sidewalk_left_ != sidewalk_left ||
             o->sidewalk_right_ != sidewalk_right ||
             (o->linked_left_ != nullptr) != linked_left ||
             (o->linked_right_ != nullptr) != linked_right;
    };

    auto const update_elevation = [&](osm_edge const* e) {
      elevation_up += e->elevation_up_;
      elevation_down += e->elevation_down_;
    };

    auto e = &oe;
    update_elevation(e);
    while (e->from_->can_be_compressed()) {
      auto prev = e->from_->in_edges_[0];
      if (different_edge_attrs(prev)) {
        break;
      }
      from->compressed_ = true;
      e = prev;
      from = e->from_;
      path.insert(begin(path), from->location_);
      edges.insert(begin(edges), e);
      distance += e->distance_;
      e->processed_ = true;
      update_elevation(e);
      stats_.int_.n_compressed_edges_++;
    }

    e = &oe;
    while (e->to_->can_be_compressed()) {
      auto& next = e->to_->out_edges_[0];
      if (different_edge_attrs(&next)) {
        break;
      }
      to->compressed_ = true;
      e = &next;
      to = e->to_;
      path.push_back(to->location_);
      edges.push_back(e);
      distance += e->distance_;
      e->processed_ = true;
      update_elevation(e);
      stats_.int_.n_compressed_edges_++;
    }

    auto ig_from = get_or_create_node(from);
    auto ig_to = get_or_create_node(to);
    auto const from_angle = edges.front()->normalized_angle(false);
    auto const to_angle = edges.back()->normalized_angle(true);

    path.erase(std::unique(begin(path), end(path)), end(path));

    if (path.size() < 2) {
      log_.out() << "WARNING: Path with length " << path.size()
                 << " created for osm way " << info->osm_way_id_ << "\n";
    }

    if (oe.generate_sidewalks()) {
      auto paths = generate_sidewalk_paths(path, oe.width_);
      ig_from->emplace_out_edge(info, ig_from, ig_to, distance,
                                std::move(paths.first), std::move(paths.second),
                                from_angle, to_angle);
    } else {
      ig_from->emplace_out_edge(info, ig_from, ig_to, distance, std::move(path),
                                std::vector<merc>(), from_angle, to_angle);
    }
    auto ie = ig_from->out_edges_.back().get();
    ie->sidewalk_left_ = sidewalk_left;
    ie->sidewalk_right_ = sidewalk_right;
    ie->linked_left_ = linked_left;
    ie->linked_right_ = linked_right;
    ie->layer_ = oe.layer_;
    ie->elevation_up_ = elevation_up;
    ie->elevation_down_ = elevation_down;
    assert(!(ie->path_left_.empty() && ie->path_right_.empty()));
    assert(ie->elevation_up_ >= 0);
    assert(ie->elevation_down_ >= 0);

    switch (info->type_) {
      case edge_type::STREET:
        if (!info->is_rail_edge()) {
          stats_.int_.n_edge_streets_++;
        } else {
          stats_.int_.n_edge_railways_++;
        }
        break;
      case edge_type::FOOTWAY: stats_.int_.n_edge_footways_++; break;
      case edge_type::CROSSING: stats_.int_.n_edge_crossings_++; break;
      case edge_type::CONNECTION:
      case edge_type::ELEVATOR: /* not in int graph */ break;
    }
  }

  int_node* get_or_create_node(osm_node* on) {
    // split node when access not allowed
    if (on->int_node_ == nullptr || !on->access_allowed_) {
      auto const crossing = on->crossing_;
      ig_.nodes_.emplace_back(
          std::make_unique<int_node>(on->osm_id_, on->location_, crossing));
      auto in = ig_.nodes_.back().get();
      in->access_allowed_ = on->access_allowed_;
      in->elevator_ = on->elevator_;
      on->int_node_ = in;
    }
    return on->int_node_;
  }

  void build_areas() {
    step_progress progress{log_, pp_step::INT_AREAS};
    ig_.areas_.reserve(og_.areas_.size());
    std::transform(begin(og_.areas_), end(og_.areas_),
                   std::back_inserter(ig_.areas_),
                   [](auto const& oa) { return int_area(*oa); });
  }

  osm_graph& og_;
  int_graph& ig_;
  options const& opt_;
  logging& log_;
  statistics& stats_;
};

int_graph build_int_graph(osm_graph& og, options const& opt, logging& log,
                          statistics& stats) {
  int_graph ig;
  int_graph_builder builder(og, ig, opt, log, stats);
  builder.build();
  ig.edge_infos_ = std::move(og.edge_infos_);
  ig.names_ = std::move(og.names_);
  ig.names_map_ = std::move(og.names_map_);
  collect_stats(stats.int_, ig);
  return ig;
}

}  // namespace ppr::preprocessing
