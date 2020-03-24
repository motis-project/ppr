#pragma once

#include "osmium/osm/tag.hpp"

namespace ppr::preprocessing::osm {

int8_t get_layer(osmium::TagList const& tags);

}  // namespace ppr::preprocessing::osm
