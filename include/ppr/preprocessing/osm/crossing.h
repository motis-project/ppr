#pragma once

#include "osmium/osm.hpp"

#include "ppr/common/enums.h"

namespace ppr::preprocessing::osm {

crossing_type::crossing_type get_crossing_type(osmium::TagList const& tags);

}  // namespace ppr::preprocessing::osm
