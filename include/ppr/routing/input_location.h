#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>

#include "ppr/common/location.h"

namespace ppr::routing {

enum class osm_namespace : std::uint8_t { NODE, WAY, RELATION };

struct osm_element {
  std::int64_t id_{};
  osm_namespace type_{osm_namespace::NODE};
};

struct input_location {
  inline bool allows_expansion() const { return expanded_max_distance_ != 0; }

  inline bool valid() const {
    return (location_ && location_->valid()) ||
           (osm_element_ && (osm_element_->type_ == osm_namespace::WAY ||
                             osm_element_->type_ == osm_namespace::RELATION));
  }

  inline double max_distance(bool const expanded) const {
    return expanded ? expanded_max_distance_ : initial_max_distance_;
  }

  // at least one of the following must be set:
  std::optional<location> location_;
  std::optional<osm_element> osm_element_;

  std::optional<std::int16_t> level_;  // stored as level * 10

  // max allowed distance to matched point in routing graph in meters:
  double initial_max_distance_{200};
  // if expanded != 0 and no results are found, a second pass is
  // done with this max distance:
  double expanded_max_distance_{300};
};

inline input_location make_input_location(location const& loc) {
  input_location il;
  il.location_ = loc;
  return il;
}

inline std::ostream& operator<<(std::ostream& os, osm_namespace const ns) {
  switch (ns) {
    case osm_namespace::NODE: os << "node"; break;
    case osm_namespace::WAY: os << "way"; break;
    case osm_namespace::RELATION: os << "relation"; break;
  }
  return os;
}

}  // namespace ppr::routing
