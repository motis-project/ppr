#pragma once

#include "osmium/osm.hpp"

namespace ppr::preprocessing::osm {

bool access_allowed(char const* access, bool def);

bool access_allowed(osmium::TagList const& tags, bool def);

}  // namespace ppr::preprocessing::osm
