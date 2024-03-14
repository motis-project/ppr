#pragma once

#include "osmium/osm/tag.hpp"

#include "ppr/common/level.h"

namespace ppr::preprocessing::osm {

levels get_levels(char const* level_tag, levels_vector_t& levels_vec);
levels get_levels(osmium::TagList const& tags, levels_vector_t& levels_vec);

}  // namespace ppr::preprocessing::osm
