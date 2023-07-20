#pragma once

#include "osmium/osm/tag.hpp"

#include "ppr/common/routing_graph.h"

namespace ppr::preprocessing::osm {

double get_actual_width(osmium::TagList const& tags, double def);

double get_render_width(ppr::edge_type edge, ppr::street_type street);

std::uint8_t get_max_width_as_cm(osmium::TagList const& tags);

std::uint8_t get_cycle_barrier_max_width_as_cm(osmium::TagList const& tags);

}  // namespace ppr::preprocessing::osm
