#pragma once

#include "osmium/osm/tag.hpp"

#include "ppr/common/routing_graph.h"

namespace ppr::preprocessing::osm {

double get_actual_width(osmium::TagList const& tags, double def);

double get_render_width(ppr::edge_type edge, ppr::street_type street);

}  // namespace ppr::preprocessing::osm
