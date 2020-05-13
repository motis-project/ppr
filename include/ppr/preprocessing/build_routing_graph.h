#pragma once

#include <string>

#include "ppr/common/routing_graph.h"
#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/options.h"
#include "ppr/preprocessing/statistics.h"

namespace ppr::preprocessing {

routing_graph build_routing_graph(options const& opt, logging& log,
                                  statistics& stats);

}  // namespace ppr::preprocessing
