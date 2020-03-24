#pragma once

#include "osmium/osm/tag.hpp"

#include "ppr/common/enums.h"

namespace ppr::preprocessing::osm {

surface_type get_surface_type(char const* tag);

smoothness_type get_smoothness_type(char const* tag);

}  // namespace ppr::preprocessing::osm
