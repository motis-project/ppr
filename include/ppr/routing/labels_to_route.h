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
  route::edge re;
  re.distance_ = e->distance_;
  re.duration_ = de.duration();
  re.accessibility_ = de.accessibility();
  re.duration_penalty_ = de.duration_penalty();
  re.accessibility_penalty_ = de.accessibility_penalty();

  re.path_.reserve(e->path_.size());
  std::copy(begin(e->path_), end(e->path_), std::back_inserter(re.path_));

  if (!de.fwd_) {
    std::reverse(begin(re.path_), end(re.path_));
  }
  re.osm_way_id_ = ei->osm_way_id_;
  if (ei->name_ != 0) {
    re.name_ = rg.names_.at(ei->name_).view();
  }
  re.edge_type_ = ei->type_;
  re.street_type_ = ei->street_type_;
  re.crossing_type_ = ei->crossing_type_;
  re.oneway_street_ = ei->oneway_street_;
  re.oneway_foot_ = !(ei->allow_fwd_ && ei->allow_bwd_);
  re.area_ = ei->area_;
  re.incline_up_ = de.incline_up();
  re.handrail_ = ei->handrail_;
  re.wheelchair_ = ei->wheelchair_;
  re.step_count_ = ei->step_count_;
  re.marked_crossing_detour_ = ei->marked_crossing_detour_;
  re.side_ = e->side_;
  re.elevation_up_ = de.elevation_up();
  re.elevation_down_ = de.elevation_down();
  re.from_node_osm_id_ = de.from(rg)->osm_id_;
  re.to_node_osm_id_ = de.to(rg)->osm_id_;
  assert(re.elevation_up_ >= 0);
  assert(re.elevation_down_ >= 0);
  re.level_ = ei->level_;
  re.max_width_ = ei->max_width_;
  re.incline_ = de.incline();
  re.door_type_ = ei->door_type_;
  re.automatic_door_type_ = ei->automatic_door_type_;
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
