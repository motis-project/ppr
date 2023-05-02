#include <algorithm>
#include <queue>

#include "ppr/common/timing.h"
#include "ppr/routing/label.h"
#include "ppr/routing/labels_to_route.h"
#include "ppr/routing/pareto_dijkstra.h"
#include "ppr/routing/postprocessing.h"
#include "ppr/routing/search.h"

namespace ppr::routing {

using mapped_pt = std::pair<location, std::vector<input_pt>>;

search_result find_routes(routing_graph_data const& rg, search_result& result,
                          mapped_pt const& start,
                          std::vector<mapped_pt> const& destinations,
                          search_profile const& profile, search_direction dir) {
  auto const t_start = timing_now();
  pareto_dijkstra<label> pd(rg, profile, dir == search_direction::BWD);

  pd.add_start(start.first, start.second);

  for (auto const& goal : destinations) {
    pd.add_goal(goal.first, goal.second);
  }

  pd.search();

  auto const t_after_search = timing_now();
  auto results = pd.get_results();
  auto& routes = result.routes_;
  routes.resize(destinations.size());
  assert(results.size() == destinations.size());
  for (auto i = 0UL; i < results.size(); i++) {
    assert(i < routes.size());
    if (!routes[i].empty()) {
      continue;
    }
    auto const& goal_results = results[i];
    std::transform(begin(goal_results), end(goal_results),
                   std::back_inserter(routes[i]),
                   [&](auto& label) { return labels_to_route(label, rg); });
  }

  result.stats_.attempts_++;
  result.stats_.dijkstra_statistics_.push_back(pd.get_statistics());
  auto& stats = result.stats_.dijkstra_statistics_.back();
  stats.d_labels_to_route_ = ms_since(t_after_search);
  stats.d_total_ = ms_since(t_start);

  return result;
}

bool all_goals_reached(search_result const& result) {
  return std::all_of(
      begin(result.routes_), end(result.routes_),
      [](std::vector<route> const& routes) { return !routes.empty(); });
}

constexpr unsigned initial_max_pt_query = 10;
constexpr unsigned initial_max_pt_count = 1;
constexpr double initial_max_pt_dist = 200;

constexpr unsigned expanded_max_pt_query = 40;
constexpr unsigned expanded_max_pt_count = 20;
constexpr double expanded_max_pt_dist = 300;

search_result find_routes(routing_graph const& g, location const& start,
                          std::vector<location> const& destinations,
                          search_profile const& profile, search_direction dir,
                          bool allow_expansion) {
  search_result result;
  auto const t_start = timing_now();
  mapped_pt mapped_start = {
      start, nearest_points(g, start, initial_max_pt_query,
                            initial_max_pt_count, initial_max_pt_dist)};
  auto const t_after_start = timing_now();
  result.stats_.d_start_pts_ = ms_between(t_start, t_after_start);

  std::vector<mapped_pt> mapped_goals;
  mapped_goals.reserve(destinations.size());
  std::transform(
      begin(destinations), end(destinations), std::back_inserter(mapped_goals),
      [&](location const& loc) {
        return mapped_pt{
            loc, nearest_points(g, loc, initial_max_pt_query,
                                initial_max_pt_count, initial_max_pt_dist)};
      });
  auto const t_after_dest = timing_now();
  result.stats_.d_destination_pts_ = ms_between(t_after_start, t_after_dest);

  auto const& rg = *g.data_;

  // 1st attempt: only nearest start + goal points
  find_routes(rg, result, mapped_start, mapped_goals, profile, dir);

  if (allow_expansion && !all_goals_reached(result)) {
    auto const orig_start = mapped_start.second;
    // 2nd attempt: expand start point
    auto const t_before_expand_start = timing_now();
    mapped_start.second =
        nearest_points(g, start, expanded_max_pt_query, expanded_max_pt_count,
                       expanded_max_pt_dist);
    result.stats_.start_pts_extended_++;
    result.stats_.d_start_pts_extended_ = ms_since(t_before_expand_start);
    find_routes(rg, result, mapped_start, mapped_goals, profile, dir);
    if (!all_goals_reached(result)) {
      // 3rd attempt: expand goal points
      auto const expanded_start = mapped_start.second;
      mapped_start.second = orig_start;
      auto const t_before_expand_dest = timing_now();
      for (std::size_t i = 0; i < destinations.size(); i++) {
        if (result.routes_[i].empty()) {
          mapped_goals[i].second =
              nearest_points(g, destinations[i], expanded_max_pt_query,
                             expanded_max_pt_count, expanded_max_pt_dist);
          result.stats_.destination_pts_extended_++;
        }
      }
      result.stats_.d_destination_pts_extended_ =
          ms_since(t_before_expand_dest);
      find_routes(rg, result, mapped_start, mapped_goals, profile, dir);
      if (!all_goals_reached(result)) {
        // 4th attempt: expand start and goal points
        mapped_start.second = expanded_start;
        find_routes(rg, result, mapped_start, mapped_goals, profile, dir);
      }
    }
  }

  postprocess_result(result, profile);
  result.stats_.d_total_ = ms_since(t_start);
  return result;
}

}  // namespace ppr::routing
