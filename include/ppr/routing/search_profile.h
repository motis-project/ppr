#pragma once

#include <cstdint>
#include <functional>
#include <limits>

namespace ppr::routing {

enum class usage_restriction { ALLOWED, PENALIZED, FORBIDDEN };

struct cost_coefficients {
  double c0_{0};
  double c1_{0};
  double c2_{0};
};

inline constexpr double operator*(cost_coefficients const& c, double val) {
  return std::equal_to<>()(val, 0.0) ? 0
                                     : c.c0_ + c.c1_ * val + c.c2_ * val * val;
}

inline constexpr double operator*(cost_coefficients const& c, int val) {
  return val == 0 ? 0 : c.c0_ + c.c1_ * val + c.c2_ * val * val;
}

struct cost_factor {
  cost_coefficients duration_{};
  cost_coefficients accessibility_{};
  usage_restriction allowed_{usage_restriction::ALLOWED};
  double duration_penalty_{0};  // s
  double accessibility_penalty_{0};
};

struct crossing_cost_factor {
  cost_factor signals_{};
  cost_factor blind_signals_{};
  cost_factor marked_{};
  cost_factor island_{};
  cost_factor unmarked_{};
};

struct door_type_factors {
  cost_factor yes_{};
  cost_factor no_{};
  cost_factor hinged_{};
  cost_factor sliding_{};
  cost_factor revolving_{};
  cost_factor folding_{};
  cost_factor trapdoor_{};
  cost_factor overhead_{};
};

struct automatic_door_type_factors {
  cost_factor yes_{};
  cost_factor no_{};
  cost_factor button_{};
  cost_factor motion_{};
  cost_factor floor_{};
  cost_factor continuous_{};
  cost_factor slowdown_button_{};
};

struct search_profile {
  double walking_speed_ = 1.4;  // m/s
  double duration_limit_ = 60 * 60;  // s
  std::int32_t max_crossing_detour_primary_ = 300;  // m
  std::int32_t max_crossing_detour_secondary_ = 200;  // m
  std::int32_t max_crossing_detour_tertiary_ = 200;  // m
  std::int32_t max_crossing_detour_residential_ = 100;  // m
  std::int32_t max_crossing_detour_service_ = 0;  // m
  std::uint8_t min_required_width_ = 0;  // cm (0 = ignored)
  std::int8_t min_allowed_incline_ =
      std::numeric_limits<std::int8_t>::min();  // percent (grade)
  std::int8_t max_allowed_incline_ =
      std::numeric_limits<std::int8_t>::max();  // percent (grade)
  bool wheelchair_{};
  bool stroller_{};

  double max_free_street_crossing_distance_ = 30;  // m
  double max_free_rail_tram_crossing_distance_ = 15;  // m

  double round_distance_ = 0;
  double round_duration_ = 0;
  double round_accessibility_ = 0;
  int max_routes_ = 0;
  int divisions_duration_ = 0;
  int divisions_accessibility_ = 0;

  crossing_cost_factor crossing_primary_{
      .signals_ = {.duration_ = cost_coefficients{120}},
      .blind_signals_ = {.duration_ = cost_coefficients{120}},
      .marked_ = {.duration_ = cost_coefficients{100}},
      .island_ = {.duration_ = cost_coefficients{200}},
      .unmarked_ = {.allowed_ = usage_restriction::PENALIZED,
                    .duration_penalty_ = 200},
  };
  crossing_cost_factor crossing_secondary_{
      .signals_ = {.duration_ = cost_coefficients{60}},
      .blind_signals_ = {.duration_ = cost_coefficients{60}},
      .marked_ = {.duration_ = cost_coefficients{30}},
      .island_ = {.duration_ = cost_coefficients{60}},
      .unmarked_ = {.duration_ = cost_coefficients{100}},
  };
  crossing_cost_factor crossing_tertiary_{
      .signals_ = {.duration_ = cost_coefficients{60}},
      .blind_signals_ = {.duration_ = cost_coefficients{60}},
      .marked_ = {.duration_ = cost_coefficients{30}},
      .island_ = {.duration_ = cost_coefficients{60}},
      .unmarked_ = {.duration_ = cost_coefficients{100}},
  };
  crossing_cost_factor crossing_residential_{
      .signals_ = {.duration_ = cost_coefficients{45}},
      .blind_signals_ = {.duration_ = cost_coefficients{45}},
      .marked_ = {.duration_ = cost_coefficients{20}},
      .island_ = {.duration_ = cost_coefficients{40}},
      .unmarked_ = {.duration_ = cost_coefficients{30}},
  };
  crossing_cost_factor crossing_service_{};

  cost_factor crossing_rail_{.duration_ = cost_coefficients{60}};
  cost_factor crossing_tram_{.duration_ = cost_coefficients{30}};

  cost_factor stairs_up_cost_{};
  cost_factor stairs_down_cost_{};
  cost_factor stairs_with_handrail_up_cost_{};
  cost_factor stairs_with_handrail_down_cost_{};
  cost_factor elevator_cost_{.duration_ = cost_coefficients{60}};
  cost_factor escalator_cost_{};
  cost_factor moving_walkway_cost_{};
  cost_factor cycle_barrier_cost_{};

  cost_factor elevation_up_cost_{};
  cost_factor elevation_down_cost_{};

  door_type_factors door_{};
  automatic_door_type_factors automatic_door_{};
};

}  // namespace ppr::routing
