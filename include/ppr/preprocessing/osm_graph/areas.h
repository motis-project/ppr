#pragma once

#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/osm_graph/osm_graph.h"
#include "ppr/preprocessing/statistics.h"

namespace ppr::preprocessing {

void process_areas(osm_graph& graph, logging& log, osm_graph_statistics& stats);

}  // namespace ppr::preprocessing
