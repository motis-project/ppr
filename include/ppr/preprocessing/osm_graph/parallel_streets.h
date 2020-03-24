#pragma once

#include "ppr/preprocessing/osm_graph/osm_graph.h"
#include "ppr/preprocessing/statistics.h"

namespace ppr::preprocessing {

void detect_parallel_streets(osm_graph& og, osm_graph_statistics& stats);

}  // namespace ppr::preprocessing
