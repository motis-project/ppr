#pragma once

#include <cstdint>
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

  std::optional<int> level_;  // stored as level * 10

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

}  // namespace ppr::routing
