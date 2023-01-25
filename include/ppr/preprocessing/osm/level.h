#pragma once

#include <cstdint>

#include "osmium/osm/tag.hpp"

#include "ppr/preprocessing/osm/parse.h"

namespace ppr::preprocessing::osm {

inline std::int16_t get_level(osmium::TagList const& tags) {
  return static_cast<std::int16_t>(parse_float(tags["level"]) * 10.0F);
}

}  // namespace ppr::preprocessing::osm
