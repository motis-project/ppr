#include <algorithm>
#include <queue>

#include "utl/to_vec.h"

#include "ppr/common/timing.h"
#include "ppr/routing/label.h"
#include "ppr/routing/labels_to_route.h"
#include "ppr/routing/pareto_dijkstra.h"
#include "ppr/routing/postprocessing.h"
#include "ppr/routing/search.h"

namespace ppr::routing {

search_result find_routes(
    routing_graph_data const& rg, search_result& result,
    std::vector<input_pt> const& start,
    std::vector<std::vector<input_pt>> const& destinations,
    search_profile const& profile, search_direction dir) {

  auto const t_start = timing_now();
  pareto_dijkstra<label> pd(rg, profile, dir == search_direction::BWD);

  if (!start.empty()) {
    pd.add_start(start.front().input_, start);
  }

  for (auto const& goal : destinations) {
    if (!goal.empty()) {
      pd.add_goal(goal.front().input_, goal);
    }
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

search_result find_routes(routing_graph const& g, location const& start,
                          std::vector<location> const& destinations,
                          search_profile const& profile, search_direction dir,
                          bool allow_expansion) {
  return find_routes_v2(
      g, routing_query{make_input_location(start),
                       utl::to_vec(destinations,
                                   [&](auto const& loc) {
                                     return make_input_location(loc);
                                   }),
                       profile, dir, routing_options{allow_expansion}});
}

search_result find_routes_v2(routing_graph const& g, routing_query const& q) {
  search_result result;
  auto const t_start = timing_now();

  auto start = resolve_input_location(g, q.start_, q.opt_, false);
  auto const t_after_start = timing_now();
  result.stats_.d_start_pts_ = ms_between(t_start, t_after_start);

  auto destinations = utl::to_vec(q.destinations_, [&](auto const& il) {
    return resolve_input_location(g, il, q.opt_, false);
  });
  auto const t_after_dest = timing_now();
  result.stats_.d_destination_pts_ = ms_between(t_after_start, t_after_dest);

  auto const& rg = *g.data_;
  auto const search = [&](auto const& from, auto const& to) {
    find_routes(rg, result, from, to, q.profile_, q.dir_);
  };

  // 1st attempt: only nearest start + goal points
  search(start, destinations);

  if (q.opt_.allow_expansion_ && !all_goals_reached(result)) {
    // 2nd attempt: expand start point
    auto expanded_start = std::vector<input_pt>{};
    if (q.start_.allows_expansion()) {
      auto const t_before_expand_start = timing_now();
      expanded_start = resolve_input_location(g, q.start_, q.opt_, true);
      ++result.stats_.start_pts_extended_;
      result.stats_.d_start_pts_extended_ = ms_since(t_before_expand_start);
      search(expanded_start, destinations);
    }

    // 3rd attempt: expand goal points
    if (!all_goals_reached(result) &&
        std::any_of(begin(q.destinations_), end(q.destinations_),
                    [](auto const& il) { return il.allows_expansion(); })) {
      auto const t_before_expand_dest = timing_now();
      auto expanded_destinations =
          utl::to_vec(q.destinations_, [&](auto const& il) {
            auto const expand = il.allows_expansion();
            if (expand) {
              ++result.stats_.destination_pts_extended_;
            }
            return resolve_input_location(g, il, q.opt_, expand);
          });
      result.stats_.d_destination_pts_extended_ =
          ms_since(t_before_expand_dest);
      search(start, expanded_destinations);

      if (!expanded_start.empty() && !all_goals_reached(result)) {
        // 4th attempt: expand start and goal points
        search(expanded_start, expanded_destinations);
      }
    }
  }

  postprocess_result(result, q.profile_);
  result.stats_.d_total_ = ms_since(t_start);
  return result;
}

}  // namespace ppr::routing
