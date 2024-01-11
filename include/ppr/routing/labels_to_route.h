#pragma once

#include <cassert>
#include <algorithm>

#include "ppr/common/routing_graph.h"
#include "ppr/routing/route.h"
#include "ppr/routing/search_profile.h"

namespace ppr::routing {

template <typename Label>
route::edge to_route_edge(Label const* label, routing_graph_data const& rg) {
  directed_edge const& de = label->edge_;
  auto const e = de.edge_;
  auto const ei = e->info(rg);

  auto re =
      route::edge{.distance_ = e->distance_,
                  .duration_ = de.duration(),
                  .accessibility_ = de.accessibility(),
                  .duration_penalty_ = de.duration_penalty(),
                  .accessibility_penalty_ = de.accessibility_penalty(),
                  .osm_way_id_ = ei->osm_way_id_,
                  .edge_type_ = ei->type_,
                  .street_type_ = ei->street_type_,
                  .crossing_type_ = ei->crossing_type_,
                  .oneway_street_ = ei->oneway_street_,
                  .oneway_foot_ = !(ei->allow_fwd_ && ei->allow_bwd_),
                  .area_ = ei->area_,
                  .incline_up_ = de.incline_up(),
                  .handrail_ = ei->handrail_,
                  .wheelchair_ = ei->wheelchair_,
                  .stroller_ = ei->stroller_,
                  .step_count_ = ei->step_count_,
                  .marked_crossing_detour_ = ei->marked_crossing_detour_,
                  .side_ = de.side(),
                  .graph_side_ = e->side_,
                  .elevation_up_ = de.elevation_up(),
                  .elevation_down_ = de.elevation_down(),
                  .level_ = ei->level_,
                  .from_node_osm_id_ = de.from(rg)->osm_id_,
                  .to_node_osm_id_ = de.to(rg)->osm_id_,
                  .max_width_ = ei->max_width_,
                  .incline_ = de.incline(),
                  .door_type_ = ei->door_type_,
                  .automatic_door_type_ = ei->automatic_door_type_,
                  .traffic_signals_sound_ = ei->traffic_signals_sound_,
                  .traffic_signals_vibration_ = ei->traffic_signals_vibration_,
                  .is_additional_edge_ = e->info_ == 0,
                  .free_crossing_ = de.is_free_crossing()};

  re.path_.reserve(e->path_.size());
  std::copy(begin(e->path_), end(e->path_), std::back_inserter(re.path_));
  if (!de.fwd_) {
    std::reverse(begin(re.path_), end(re.path_));
  }

  if (ei->name_ != 0) {
    re.name_ = rg.names_.at(ei->name_).view();
  }

  assert(re.elevation_up_ >= 0);
  assert(re.elevation_down_ >= 0);
  return re;
}

template <typename Label>
route labels_to_route(Label const* final_label, routing_graph_data const& rg) {
  std::vector<route::edge> edges;

  auto const* label = final_label;
  while (label != nullptr) {
    auto const* pred = label->pred_;
    edges.emplace(begin(edges), to_route_edge(label, rg));
    label = pred;
  }

  if (edges.size() > 1) {
    // additional edges use the default edge info which doesn't have level
    // information -> copy level from the next/previous edge
    if (edges.front().is_additional_edge_) {
      edges.front().level_ = edges[1].level_;
    }
    if (edges.back().is_additional_edge_) {
      edges.back().level_ = edges[edges.size() - 2].level_;
    }
  }

  elevation_diff_t elevation_up = 0;
  elevation_diff_t elevation_down = 0;
  for (auto const& e : edges) {
    elevation_up += e.elevation_up_;
    elevation_down += e.elevation_down_;
  }

  double distance = 0.0;
  double duration = 0.0;
  double accessibility = 0.0;
  double penalized_duration = 0.0;
  double penalized_accessibility = 0.0;
  if (final_label != nullptr) {
    distance = final_label->distance_;
    duration = final_label->real_duration_;
    accessibility = final_label->real_accessibility_;
    penalized_duration = final_label->duration_;
    penalized_accessibility = final_label->real_accessibility_;
  }

  return {std::move(edges),   distance,
          duration,           accessibility,
          elevation_up,       elevation_down,
          penalized_duration, penalized_accessibility};
}

}  // namespace ppr::routing
