#pragma once

#include "ppr/common/routing_graph.h"
#include "ppr/routing/additional_edges.h"
#include "ppr/routing/input_pt.h"

namespace ppr::routing {

node* create_area_node(node* input_node, input_pt const& pt,
                       additional_edges& additional);
void create_area_edges(additional_edges& additional);

}  // namespace ppr::routing
