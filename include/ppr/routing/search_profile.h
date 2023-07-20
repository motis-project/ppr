#pragma once

#include <cstdint>
#include <functional>
#include <limits>

namespace ppr::routing {

enum class usage_restriction { ALLOWED, PENALIZED, FORBIDDEN };

struct cost_coefficients {
  constexpr cost_coefficients() = default;

  explicit constexpr cost_coefficients(double c0, double c1 = 0, double c2 = 0)
      : c0_(c0), c1_(c1), c2_(c2) {}

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
  constexpr cost_factor() = default;
  explicit constexpr cost_factor(usage_restriction allowed)
      : allowed_(allowed) {}

  constexpr cost_factor(cost_coefficients const& duration,
                        cost_coefficients const& accessiblity,
                        usage_restriction allowed = usage_restriction::ALLOWED,
                        double duration_penalty = 0,
                        double accessibility_penalty = 0)
      : duration_(duration),
        accessibility_(accessiblity),
        allowed_(allowed),
        duration_penalty_(duration_penalty),
        accessibility_penalty_(accessibility_penalty) {}

  constexpr cost_factor(cost_coefficients&& duration,
                        cost_coefficients&& accessibility,
                        usage_restriction allowed = usage_restriction::ALLOWED,
                        double duration_penalty = 0,
                        double accessibility_penalty = 0)
      : duration_(duration),
        accessibility_(accessibility),
        allowed_(allowed),
        duration_penalty_(duration_penalty),
        accessibility_penalty_(accessibility_penalty) {}

  cost_coefficients duration_;
  cost_coefficients accessibility_;
  usage_restriction allowed_{usage_restriction::ALLOWED};
  double duration_penalty_{0};  // s
  double accessibility_penalty_{0};
};

struct crossing_cost_factor {
  constexpr crossing_cost_factor() = default;

  constexpr crossing_cost_factor(cost_factor const& signals,
                                 cost_factor const& marked,
                                 cost_factor const& island,
                                 cost_factor const& unmarked)
      : signals_(signals),
        marked_(marked),
        island_(island),
        unmarked_(unmarked) {}

  constexpr crossing_cost_factor(cost_factor&& signals, cost_factor&& marked,
                                 cost_factor&& island, cost_factor&& unmarked)
      : signals_(signals),
        marked_(marked),
        island_(island),
        unmarked_(unmarked) {}

  cost_factor signals_;
  cost_factor marked_;
  cost_factor island_;
  cost_factor unmarked_;
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

  double round_distance_ = 0;
  double round_duration_ = 0;
  double round_accessibility_ = 0;
  int max_routes_ = 0;
  int divisions_duration_ = 0;
  int divisions_accessibility_ = 0;

  crossing_cost_factor crossing_primary_{
      {cost_coefficients{120}, {}},  // signals
      {cost_coefficients{100}, {}},  // marked
      {cost_coefficients{200}, {}},  // island
      {cost_factor{{}, {}, usage_restriction::PENALIZED, 200, 0}},  // unmarked
  };
  crossing_cost_factor crossing_secondary_{
      {cost_coefficients{60}, {}},  // signals
      {cost_coefficients{30}, {}},  // marked
      {cost_coefficients{60}, {}},  // island
      {cost_coefficients{100}, {}},  // unmarked
  };
  crossing_cost_factor crossing_tertiary_{
      {cost_coefficients{60}, {}},  // signals
      {cost_coefficients{30}, {}},  // marked
      {cost_coefficients{60}, {}},  // island
      {cost_coefficients{100}, {}},  // unmarked
  };
  crossing_cost_factor crossing_residential_{
      {cost_coefficients{45}, {}},  // signals
      {cost_coefficients{20}, {}},  // marked
      {cost_coefficients{40}, {}},  // island
      {cost_coefficients{30}, {}},  // unmarked
  };
  crossing_cost_factor crossing_service_{
      {{}, {}},  // signals
      {{}, {}},  // marked
      {{}, {}},  // island
      {{}, {}},  // unmarked
  };

  cost_factor crossing_rail_{cost_coefficients{60}, {}};
  cost_factor crossing_tram_{cost_coefficients{30}, {}};

  cost_factor stairs_up_cost_{};
  cost_factor stairs_down_cost_{};
  cost_factor stairs_with_handrail_up_cost_{};
  cost_factor stairs_with_handrail_down_cost_{};
  cost_factor elevator_cost_{cost_coefficients{60}, {}};
  cost_factor escalator_cost_{};
  cost_factor moving_walkway_cost_{};
  cost_factor cycle_barrier_cost_{};

  cost_factor elevation_up_cost_{};
  cost_factor elevation_down_cost_{};

  door_type_factors door_{};
  automatic_door_type_factors automatic_door_{};
};

}  // namespace ppr::routing
