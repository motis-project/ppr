#pragma once

#include <string>
#include <vector>

#include "ppr/common/routing_graph.h"

namespace ppr::routing {

struct route;

enum class step_type : uint8_t { INVALID, STREET, FOOTWAY, CROSSING, ELEVATOR };

struct route_step {
  bool valid() const { return step_type_ != step_type::INVALID; }

  step_type step_type_ = step_type::INVALID;
  std::string street_name_;
  street_type street_type_{street_type::NONE};
  side_type side_{side_type::CENTER};
  crossing_type::crossing_type crossing_{crossing_type::NONE};
  double distance_{0.0};
  double time_{0.0};
  double accessibility_{0.0};
  elevation_diff_t elevation_up_{0};
  elevation_diff_t elevation_down_{0};
  bool incline_up_{false};
  tri_state::tri_state handrail_{tri_state::UNKNOWN};
  double duration_penalty_{0};
  double accessibility_penalty_{0};

  std::vector<location> path_;
};

std::vector<route_step> get_route_steps(route const&);
std::vector<location> get_route_path(route const& r);

}  // namespace ppr::routing
