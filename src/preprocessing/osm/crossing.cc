#include <cstring>

#include "ppr/preprocessing/osm/crossing.h"

namespace ppr::preprocessing::osm {

crossing_type get_crossing_type(osmium::TagList const& tags) {
  auto const* crossing = tags["crossing"];
  if (crossing != nullptr) {
    if (strcmp(crossing, "marked") == 0) {
      return tags.has_tag("crossing:signals", "yes") ? crossing_type::SIGNALS
                                                     : crossing_type::MARKED;
    } else if (strcmp(crossing, "traffic_signals") == 0 ||
               strcmp(crossing, "pelican") == 0 ||
               strcmp(crossing, "toucan") == 0 ||
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
      if (tags.has_tag("crossing:signals", "yes")) {
        return crossing_type::SIGNALS;
      }
      return tags.has_tag("crossing:island", "yes") ? crossing_type::ISLAND
                                                    : crossing_type::UNMARKED;
    }
  }

  return crossing_type::UNMARKED;
}

crossing_type get_node_crossing_type(osmium::TagList const& tags) {
  if (tags.has_tag("highway", "crossing") || tags.has_key("crossing")) {
    return get_crossing_type(tags);
  } else {
    return crossing_type::NONE;
  }
}

crossing_type get_way_crossing_type(osmium::TagList const& tags) {
  if ((tags.has_tag("highway", "footway") &&
       tags.has_tag("footway", "crossing")) ||
      (tags.has_tag("highway", "path") && tags.has_tag("path", "crossing"))) {
    return get_crossing_type(tags);
  } else {
    return crossing_type::NONE;
  }
}

}  // namespace ppr::preprocessing::osm
