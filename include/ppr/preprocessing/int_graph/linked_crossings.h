#pragma once

#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/statistics.h"

namespace ppr {

struct routing_graph;

namespace preprocessing {

struct int_graph;

void add_linked_crossings(int_graph&, routing_graph&, logging&,
                          step_progress& progress, routing_graph_statistics&);

}  // namespace preprocessing
}  // namespace ppr
