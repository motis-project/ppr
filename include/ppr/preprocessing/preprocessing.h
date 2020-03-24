#pragma once

#include <string>

#include "ppr/preprocessing/int_graph/int_graph.h"
#include "ppr/preprocessing/options.h"
#include "ppr/preprocessing/statistics.h"

namespace ppr::preprocessing {

routing_graph build_routing_graph(options const& opt, statistics& stats);

}  // namespace ppr::preprocessing
