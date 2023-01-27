#pragma once

#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <vector>

#include "ppr/common/routing_graph.h"

#include "ppr/routing/input_location.h"
#include "ppr/routing/route.h"
#include "ppr/routing/routing_query.h"
#include "ppr/routing/search_profile.h"
#include "ppr/routing/statistics.h"

namespace ppr::routing {

struct search_result {
  std::vector<std::vector<route>> routes_;
  routing_statistics stats_;

  int total_route_count() const {
    return std::accumulate(begin(routes_), end(routes_), 0,
                           [](auto const& sum, auto const& routes) {
                             return sum + static_cast<int>(routes.size());
                           });
  }

  int destinations_reached() const {
    return static_cast<int>(
        std::count_if(begin(routes_), end(routes_),
                      [](auto const& routes) { return !routes.empty(); }));
  }
};

search_result find_routes(routing_graph const& g, location const& start,
                          std::vector<location> const& destinations,
                          search_profile const& profile,
                          search_direction dir = search_direction::FWD,
                          bool allow_expansion = true);

search_result find_routes_v2(routing_graph const& g, routing_query const& q);

}  // namespace ppr::routing
