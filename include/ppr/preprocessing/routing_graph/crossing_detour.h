#pragma once

#include "ppr/preprocessing/options.h"

namespace ppr {

struct routing_graph;

namespace preprocessing {

void calc_crossing_detours(routing_graph&, options const&);

}  // namespace preprocessing
}  // namespace ppr
