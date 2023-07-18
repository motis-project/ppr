#include "ppr/routing/costs.h"
#include "ppr/routing/stairs.h"

namespace ppr::routing {

cost_factor const default_cost_factor;

inline bool is_main_road(street_type street) {
  return street == street_type::PRIMARY || street == street_type::SECONDARY ||
         street == street_type::TERTIARY;
}

cost_factor const& get_crossing_factor(crossing_cost_factor const& cf,
                                       crossing_type crossing) {
  switch (crossing) {
    case crossing_type::SIGNALS: return cf.signals_;
    case crossing_type::MARKED: return cf.marked_;
    case crossing_type::ISLAND: return cf.island_;
    case crossing_type::UNMARKED:
    case crossing_type::GENERATED: return cf.unmarked_;
    default: return default_cost_factor;
  }
}

cost_factor const& get_crossing_factor(search_profile const& profile,
                                       street_type street,
                                       crossing_type crossing) {
  switch (street) {
    case street_type::PRIMARY:
      return get_crossing_factor(profile.crossing_primary_, crossing);
    case street_type::SECONDARY:
      return get_crossing_factor(profile.crossing_secondary_, crossing);
    case street_type::TERTIARY:
      return get_crossing_factor(profile.crossing_tertiary_, crossing);
    case street_type::RAIL: return profile.crossing_rail_;
    case street_type::TRAM: return profile.crossing_tram_;
    case street_type::SERVICE:
      return get_crossing_factor(profile.crossing_service_, crossing);
    default:
      return get_crossing_factor(profile.crossing_residential_, crossing);
  }
}

cost_factor const& get_stairs_factor(search_profile const& profile,
                                     bool incline_up,
                                     tri_state::tri_state handrail) {
  if (incline_up) {
    return handrail == tri_state::YES ? profile.stairs_with_handrail_up_cost_
                                      : profile.stairs_up_cost_;
  } else {
    return handrail == tri_state::YES ? profile.stairs_with_handrail_down_cost_
                                      : profile.stairs_down_cost_;
  }
}

cost_factor const& get_door_factor(search_profile const& profile,
                                   door_type const type) {
  switch (type) {
    case door_type::NO: return profile.door_.no_;
    case door_type::HINGED: return profile.door_.hinged_;
    case door_type::SLIDING: return profile.door_.sliding_;
    case door_type::REVOLVING: return profile.door_.revolving_;
    case door_type::FOLDING: return profile.door_.folding_;
    case door_type::TRAPDOOR: return profile.door_.trapdoor_;
    case door_type::OVERHEAD: return profile.door_.overhead_;
    case door_type::YES:
    default: return profile.door_.yes_;
  }
}

cost_factor const& get_automatic_door_factor(search_profile const& profile,
                                             automatic_door_type const type) {
  switch (type) {
    case automatic_door_type::NO: return profile.automatic_door_.no_;
    case automatic_door_type::BUTTON: return profile.automatic_door_.button_;
    case automatic_door_type::MOTION: return profile.automatic_door_.motion_;
    case automatic_door_type::FLOOR: return profile.automatic_door_.floor_;
    case automatic_door_type::CONTINUOUS:
      return profile.automatic_door_.continuous_;
    case automatic_door_type::SLOWDOWN_BUTTON:
      return profile.automatic_door_.slowdown_button_;
    case automatic_door_type::YES:
    default: return profile.automatic_door_.yes_;
  }
}

int32_t get_max_crossing_detour(search_profile const& profile,
                                street_type street) {
  switch (street) {
    case street_type::PRIMARY: return profile.max_crossing_detour_primary_;
    case street_type::SECONDARY: return profile.max_crossing_detour_secondary_;
    case street_type::TERTIARY: return profile.max_crossing_detour_tertiary_;
    case street_type::SERVICE: return profile.max_crossing_detour_service_;
    default: return profile.max_crossing_detour_residential_;
  }
}

edge_costs get_edge_costs(routing_graph_data const& rg, edge const* e,
                          edge_info const* info, bool fwd,
                          search_profile const& profile) {
  auto const distance = e->distance_;
  double duration = distance / profile.walking_speed_;
  double accessibility = 0;
  double duration_penalty = 0;
  double accessibility_penalty = 0;
  bool allowed = true;

  auto const check_allowed = [&](cost_factor const& cf) {
    switch (cf.allowed_) {
      case usage_restriction::ALLOWED: break;
      case usage_restriction::PENALIZED: {
        duration_penalty += cf.duration_penalty_;
        accessibility_penalty += cf.accessibility_penalty_;
        break;
      }
      case usage_restriction::FORBIDDEN: allowed = false; break;
    }
  };

  auto const add_factor = [&](cost_factor const& cf, double value) {
    duration += cf.duration_ * value;
    accessibility += cf.accessibility_ * value;
    check_allowed(cf);
  };

  if (info->type_ == edge_type::CROSSING) {
    auto const& cf =
        get_crossing_factor(profile, info->street_type_, info->crossing_type_);
    add_factor(cf, distance);
    if (info->is_unmarked_crossing()) {
      auto const detour = info->marked_crossing_detour_;
      if (detour != 0 &&
          detour <= get_max_crossing_detour(profile, info->street_type_)) {
        allowed = false;
      }
    }
  } else if (info->type_ == edge_type::ELEVATOR) {
    add_factor(profile.elevator_cost_, 1.0);
  } else if (info->type_ == edge_type::CYCLE_BARRIER) {
    add_factor(profile.cycle_barrier_cost_, 1.0);
  } else if (info->type_ == edge_type::ENTRANCE) {
    if (info->door_type_ != door_type::UNKNOWN) {
      add_factor(get_door_factor(profile, info->door_type_), 1.0);
    }
    if (info->automatic_door_type_ != automatic_door_type::UNKNOWN) {
      add_factor(get_automatic_door_factor(profile, info->automatic_door_type_),
                 1.0);
    }
  }

  if (info->max_width_ != 0 && profile.min_required_width_ != 0) {
    if (info->max_width_ < profile.min_required_width_) {
      allowed = false;
    }
  }

  if (info->street_type_ == street_type::STAIRS) {
    auto const steps = edge_step_count(rg, e);
    auto const& cf =
        get_stairs_factor(profile, info->incline_up_, info->handrail_);
    add_factor(cf, steps);
  } else if (info->street_type_ == street_type::ESCALATOR) {
    add_factor(profile.escalator_cost_, distance);
  } else if (info->street_type_ == street_type::MOVING_WALKWAY) {
    add_factor(profile.moving_walkway_cost_, distance);
  }

  auto const elevation_up = fwd ? e->elevation_up_ : e->elevation_down_;
  auto const elevation_down = fwd ? e->elevation_down_ : e->elevation_up_;

  if (elevation_up > 0) {
    add_factor(profile.elevation_up_cost_, elevation_up);
  }
  if (elevation_down > 0) {
    add_factor(profile.elevation_down_cost_, elevation_down);
  }

  if ((fwd && !info->allow_fwd_) || (!fwd && !info->allow_bwd_)) {
    allowed = false;
  }

  return {duration, accessibility, duration_penalty, accessibility_penalty,
          allowed};
}

}  // namespace ppr::routing
