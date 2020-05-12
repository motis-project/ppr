#pragma once

#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/options.h"

namespace ppr {

struct routing_graph;

namespace preprocessing {

void calc_crossing_detours(routing_graph&, options const&, logging&);

}  // namespace preprocessing
}  // namespace ppr
