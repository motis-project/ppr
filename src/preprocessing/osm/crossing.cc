#include <cstring>

#include "ppr/preprocessing/osm/crossing.h"

namespace ppr::preprocessing::osm {

crossing_type::crossing_type get_crossing_type(osmium::TagList const& tags) {
  auto const* crossing = tags["crossing"];
  if (crossing != nullptr) {
    if (strcmp(crossing, "traffic_signals") == 0 ||
        strcmp(crossing, "pelican") == 0 || strcmp(crossing, "toucan") == 0 ||
        strcmp(crossing, "pegasus") == 0) {
      return crossing_type::SIGNALS;
    } else if (strcmp(crossing, "uncontrolled") == 0 ||
               strcmp(crossing, "zebra") == 0 ||
               strcmp(crossing, "tiger") == 0) {
      return crossing_type::MARKED;
    } else if (strcmp(crossing, "island") == 0) {
      return crossing_type::ISLAND;
    } else if (strcmp(crossing, "no") == 0) {
      return crossing_type::NONE;
    } else {
      // crossing=unmarked and unknown crossing types
      return crossing_type::UNMARKED;
    }
  } else {
    return tags.has_tag("highway", "crossing") ? crossing_type::UNMARKED
                                               : crossing_type::NONE;
  }
}

}  // namespace ppr::preprocessing::osm
