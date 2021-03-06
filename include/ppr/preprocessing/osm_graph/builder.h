#pragma once

#include <string>
#include <vector>

#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/options.h"
#include "ppr/preprocessing/osm_graph/osm_graph.h"
#include "ppr/preprocessing/statistics.h"

namespace ppr::preprocessing {

osm_graph build_osm_graph(options const& opt, logging& log, statistics& stats);

}  // namespace ppr::preprocessing
