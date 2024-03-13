#pragma once

#include <string>
#include <vector>

#include "ppr/backend/requests.h"
#include "ppr/common/routing_graph.h"
#include "ppr/routing/search.h"

namespace ppr::backend::output {

std::string routes_to_route_response(routing_graph_data const& rg,
                                     ppr::routing::search_result const& result,
                                     route_request const& req);

}  // namespace ppr::backend::output
