#pragma once

#include <string>
#include <vector>

#include "ppr/common/elevation.h"
#include "ppr/common/enums.h"
#include "ppr/common/location.h"
#include "ppr/common/tri_state.h"

namespace ppr::routing {

struct route {
  struct edge {
    double distance_{0};
    double duration_{0};
    double accessibility_{0};
    double duration_penalty_{0};
    double accessibility_penalty_{0};
    std::vector<location> path_;
    std::int64_t osm_way_id_{0};
    std::string name_;
    edge_type edge_type_{edge_type::CONNECTION};
    street_type street_type_{street_type::NONE};
    crossing_type::crossing_type crossing_type_{crossing_type::NONE};
    bool oneway_street_{false};
    bool oneway_foot_{false};
    bool area_{false};
    bool incline_up_{false};
    tri_state::tri_state handrail_{tri_state::UNKNOWN};
    wheelchair_type::wheelchair_type wheelchair_{wheelchair_type::UNKNOWN};
    uint8_t step_count_{0};
    int32_t marked_crossing_detour_{0};
    side_type side_{side_type::CENTER};
    elevation_diff_t elevation_up_{0};
    elevation_diff_t elevation_down_{0};
  };

  route(std::vector<edge>&& edges, double distance, double duration,
        double accessibility, elevation_diff_t elevation_up,
        elevation_diff_t elevation_down, double penalized_duration,
        double penalized_accessibility)
      : edges_(std::move(edges)),
        distance_(distance),
        duration_(duration),
        accessibility_(accessibility),
        elevation_up_(elevation_up),
        elevation_down_(elevation_down),
        orig_duration_(duration),
        disc_duration_(duration),
        orig_accessibility_(accessibility),
        disc_accessibility_(accessibility),
        best_(false),
        penalized_duration_(penalized_duration),
        penalized_accessibility_(penalized_accessibility) {}

  std::vector<edge> edges_;
  double distance_;
  double duration_;
  double accessibility_;
  elevation_diff_t elevation_up_;
  elevation_diff_t elevation_down_;

  double orig_duration_;
  double disc_duration_;
  double orig_accessibility_;
  double disc_accessibility_;
  bool best_;
  double penalized_duration_;
  double penalized_accessibility_;
};

}  // namespace ppr::routing
