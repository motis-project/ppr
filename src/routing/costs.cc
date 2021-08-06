#include "ppr/routing/costs.h"
#include "ppr/routing/stairs.h"

namespace ppr::routing {

cost_factor const default_cost_factor;

inline bool is_main_road(street_type street) {
  return street == street_type::PRIMARY || street == street_type::SECONDARY ||
         street == street_type::TERTIARY;
}

cost_factor const& get_crossing_factor(crossing_cost_factor const& cf,
                                       crossing_type::crossing_type crossing) {
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
                                       crossing_type::crossing_type crossing) {
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

edge_costs get_edge_costs(edge const* e, bool fwd,
                          search_profile const& profile) {
  auto const distance = e->distance_;
  auto const info = e->info_;
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
  }

  if (info->street_type_ == street_type::STAIRS) {
    auto const steps = edge_step_count(e);
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
