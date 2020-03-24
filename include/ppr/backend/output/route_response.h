#pragma once

#include <string>
#include <vector>

#include "ppr/common/routing_graph.h"
#include "ppr/routing/search.h"

namespace ppr::backend::output {

std::string routes_to_route_response(ppr::routing::search_result const&,
                                     bool full);

}  // namespace ppr::backend::output
