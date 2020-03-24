#include <algorithm>

#include "ppr/routing/route.h"
#include "ppr/routing/route_steps.h"

namespace ppr::routing {

step_type to_step_type(edge_type et) {
  switch (et) {
    case edge_type::CONNECTION: return step_type::INVALID;
    case edge_type::STREET: return step_type::STREET;
    case edge_type::FOOTWAY: return step_type::FOOTWAY;
    case edge_type::CROSSING: return step_type::CROSSING;
    case edge_type::ELEVATOR: return step_type::ELEVATOR;
  }
  throw std::runtime_error{"invalid step type"};
}

inline bool different_street_type(street_type a, street_type b) {
  if (a == street_type::NONE || b == street_type::NONE) {
    return false;
  } else {
    return a != b;
  }
}

inline bool is_new_step(route_step const& prev, route::edge const& e,
                        step_type new_type) {
  if (!prev.valid()) {
    return false;
  }
  if (prev.step_type_ != new_type || prev.street_name_ != e.name_ ||
      different_street_type(prev.street_type_, e.street_type_)) {
    return true;
  }
  if (prev.street_type_ == street_type::STAIRS &&
      e.street_type_ == street_type::STAIRS) {
    return prev.incline_up_ != e.incline_up_ || prev.handrail_ != e.handrail_;
  }
  if (prev.street_type_ == street_type::ESCALATOR &&
      e.street_type_ == street_type::ESCALATOR) {
    return prev.incline_up_ != e.incline_up_;
  }
  return false;
}

std::vector<route_step> get_route_steps(route const& r) {
  std::vector<route_step> steps;

  route_step step;

  for (auto const& e : r.edges_) {
    auto type = to_step_type(e.edge_type_);

    if (type != step_type::INVALID) {
      if (is_new_step(step, e, type)) {
        steps.push_back(step);
        step = {};
      }
      step.step_type_ = type;
      step.street_name_ = e.name_;
      if (e.street_type_ != street_type::NONE) {
        step.street_type_ = e.street_type_;
      }
      step.crossing_ = e.crossing_type_;
      step.side_ = e.side_;
    }

    step.distance_ += e.distance_;
    step.time_ += e.duration_;
    step.accessibility_ += e.accessibility_;
    step.elevation_up_ += e.elevation_up_;
    step.elevation_down_ += e.elevation_down_;
    step.incline_up_ = e.incline_up_;
    step.handrail_ = e.handrail_;
    step.duration_penalty_ += e.duration_penalty_;
    step.accessibility_penalty_ += e.accessibility_penalty_;

    for (auto const& loc : e.path_) {
      if (step.path_.empty() || step.path_.back() != loc) {
        step.path_.push_back(loc);
      }
    }
  }

  if (step.valid()) {
    steps.push_back(step);
  }

  return steps;
}

std::vector<location> get_route_path(route const& r) {
  std::vector<location> path;

  for (auto const& e : r.edges_) {
    for (auto const& loc : e.path_) {
      if (path.empty() || path.back() != loc) {
        path.push_back(loc);
      }
    }
  }

  return path;
}

}  // namespace ppr::routing
