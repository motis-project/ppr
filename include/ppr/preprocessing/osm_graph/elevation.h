#pragma once

#include "ppr/preprocessing/elevation/dem_source.h"
#include "ppr/preprocessing/osm_graph/osm_graph.h"
#include "ppr/preprocessing/statistics.h"

namespace ppr::preprocessing {

void add_elevation_data(osm_graph& og, elevation::dem_source& dem,
                        double sampling_interval, elevation_statistics& stats);

}  // namespace ppr::preprocessing
