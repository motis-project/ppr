#pragma once

#include "ppr/common/routing_graph.h"

namespace ppr::serialization {

void read_routing_graph(routing_graph& rg, std::string const& filename);

}  // namespace ppr::serialization
