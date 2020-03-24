#pragma once

#include <string>
#include <vector>

#include "ppr/common/routing_graph.h"

namespace ppr::backend::output {

std::string to_graph_response(
    std::vector<routing_graph::edge_rtree_value_type> const&,
    std::vector<routing_graph::area_rtree_value_type> const&,
    routing_graph const&, bool include_visibility_graphs);

}  // namespace ppr::backend::output
