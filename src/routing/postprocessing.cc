#include <cmath>
#include <algorithm>
#include <functional>

#include "ppr/common/timing.h"
#include "ppr/routing/postprocessing.h"

namespace ppr::routing {

bool round_values(route& r, search_profile const& profile, double min_dur,
                  double min_acc, double div_size_dur, double div_size_acc,
                  int div_count_dur, int div_count_acc) {
  bool modified = false;

  auto const round_simple = [&](double& value, double factor,
                                bool mark_modified = false) {
    if (std::fabs(factor) > 0.0001) {
      auto rounded = std::round(value / factor) * factor;
      if (!std::equal_to<>()(rounded, value)) {
        value = rounded;
        if (mark_modified) {
          modified = true;
        }
      }
    }
  };

  auto const round = [&](double& value, double& disc_value, double factor,
                         double min_val, double div_size, double div_count) {
    if (std::fabs(factor) > 0.0001) {
      auto const rounded = std::round(value / factor) * factor;
      if (!std::equal_to<>()(rounded, value)) {
        value = rounded;
        modified = true;
      }
    }
    if (std::fabs(div_size) > 0.0001) {
      disc_value = std::max(
          0.0,
          std::min(div_count - 1, std::floor((value - min_val) / div_size)));
      modified = true;
    } else {
      disc_value = value;
    }
  };

  round_simple(r.distance_, profile.round_distance_);
  round(r.duration_, r.disc_duration_, profile.round_duration_, min_dur,
        div_size_dur, div_count_dur);
  round(r.accessibility_, r.disc_accessibility_, profile.round_accessibility_,
        min_acc, div_size_acc, div_count_acc);
  round_simple(r.penalized_duration_, profile.round_duration_, true);
  round_simple(r.penalized_accessibility_, profile.round_accessibility_, true);

  return modified;
}

bool round_values(search_result& result, search_profile const& profile) {
  bool modified = false;

  auto const comp_duration = [](route const& a, route const& b) {
    return a.duration_ < b.duration_;
  };

  auto const comp_accessibility = [](route const& a, route const& b) {
    return a.accessibility_ < b.accessibility_;
  };

  for (auto& routes : result.routes_) {
    if (routes.empty()) {
      continue;
    }

    auto const minmax_duration =
        std::minmax_element(begin(routes), end(routes), comp_duration);
    auto const minmax_accessibility =
        std::minmax_element(begin(routes), end(routes), comp_accessibility);
    auto const min_duration = minmax_duration.first->duration_;
    auto const min_accessibility = minmax_accessibility.first->accessibility_;
    auto const range_duration =
        minmax_duration.second->duration_ - min_duration;
    auto const range_accessibility =
        minmax_accessibility.second->accessibility_ - min_accessibility;

    auto const use_divisions =
        profile.max_routes_ > 0 &&
        routes.size() > static_cast<std::size_t>(profile.max_routes_);
    auto const divisions_duration =
        use_divisions ? profile.divisions_duration_ : 0;
    auto const divisions_accessibility =
        use_divisions ? profile.divisions_accessibility_ : 0;

    auto const division_size_duration =
        divisions_duration > 0 ? range_duration / divisions_duration : 0;
    auto const division_size_accessibility =
        divisions_accessibility > 0
            ? range_accessibility / divisions_accessibility
            : 0;

    minmax_duration.first->best_ = true;
    minmax_accessibility.first->best_ = true;

    for (auto& r : routes) {
      modified |=
          round_values(r, profile, min_duration, min_accessibility,
                       division_size_duration, division_size_accessibility,
                       divisions_duration, divisions_accessibility);
    }
  }

  return modified;
}

void filter_post_dominance(std::vector<route>& routes) {
  auto const almost_eq = [](double const a, double const b) {
    return std::fabs(a - b) < 0.0001;
  };
  bool restart = false;

  for (auto it = begin(routes); it != end(routes);
       it = restart ? begin(routes) : std::next(it)) {
    restart = false;
    auto const size_before = routes.size();
    routes.erase(
        std::remove_if(
            begin(routes), end(routes),
            [&](route const& r) {
              if (&r == &(*it) || r.best_) {
                return false;
              }
              if (almost_eq(it->penalized_duration_, r.penalized_duration_) &&
                  almost_eq(it->penalized_accessibility_,
                            r.penalized_accessibility_)) {
                if (almost_eq(it->orig_duration_, r.orig_duration_)) {
                  return r.orig_accessibility_ >= it->orig_accessibility_;
                } else {
                  return r.orig_duration_ >= it->orig_duration_;
                }
              } else {
                return it->penalized_duration_ <= r.penalized_duration_ &&
                       it->penalized_accessibility_ <=
                           r.penalized_accessibility_;
              }
            }),
        end(routes));
    if (routes.size() != size_before) {
      restart = true;
    }
  }
}

void filter_post_dominance(search_result& result) {
  for (auto& v : result.routes_) {
    filter_post_dominance(v);
  }
}

bool uses_rounding(search_profile const& profile) {
  return profile.round_duration_ > 0 || profile.round_accessibility_ > 0 ||
         profile.round_distance_ > 0 ||
         (profile.max_routes_ > 0 && (profile.divisions_duration_ > 0 ||
                                      profile.divisions_accessibility_ > 0));
}

void postprocess_result(search_result& result, search_profile const& profile) {
  auto const t_start = timing_now();
  if (uses_rounding(profile)) {
    round_values(result, profile);
  }
  filter_post_dominance(result);
  result.stats_.d_postprocessing_ = ms_since(t_start);
}

}  // namespace ppr::routing
