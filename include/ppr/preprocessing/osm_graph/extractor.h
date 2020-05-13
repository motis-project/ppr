#pragma once

#include <string>

#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/options.h"
#include "ppr/preprocessing/osm_graph/osm_graph.h"
#include "ppr/preprocessing/statistics.h"

namespace ppr::preprocessing {

osm_graph extract(std::string const& osm_file, options const& opt, logging& log,
                  statistics& stats);

}  // namespace ppr::preprocessing
