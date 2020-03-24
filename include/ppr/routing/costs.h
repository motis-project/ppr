#pragma once

#include "ppr/common/routing_graph.h"
#include "ppr/routing/search_profile.h"

namespace ppr::routing {

struct edge_costs {
  double duration_{0};
  double accessibility_{0};
  double duration_penalty_{0};
  double accessibility_penalty_{0};
  bool allowed_{false};
};

edge_costs get_edge_costs(edge const* e, bool fwd,
                          search_profile const& profile);

}  // namespace ppr::routing
