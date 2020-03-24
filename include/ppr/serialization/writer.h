#pragma once

#include "ppr/common/routing_graph.h"
#include "ppr/preprocessing/statistics.h"

namespace ppr::serialization {

void write_routing_graph(routing_graph const& rg, std::string const& filename,
                         ppr::preprocessing::statistics& stats);

}  // namespace ppr::serialization
