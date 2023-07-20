#pragma once

#include <string_view>

#include "osmium/osm/tag.hpp"

#include "ppr/common/enums.h"

namespace ppr::preprocessing::osm {

inline door_type get_door_type(osmium::TagList const& tags) {
  using namespace std::literals;

  auto const val = tags["door"];
  if (val == nullptr) {
    return door_type::UNKNOWN;
  }

  if (val == "yes"sv) {
    return door_type::YES;
  } else if (val == "no"sv) {
    return door_type::NO;
  } else if (val == "hinged"sv) {
    return door_type::HINGED;
  } else if (val == "sliding"sv) {
    return door_type::SLIDING;
  } else if (val == "revolving"sv) {
    return door_type::REVOLVING;
  } else if (val == "folding"sv) {
    return door_type::FOLDING;
  } else if (val == "trapdoor"sv) {
    return door_type::TRAPDOOR;
  } else if (val == "overhead"sv) {
    return door_type::OVERHEAD;
  }

  return door_type::UNKNOWN;
}

inline automatic_door_type get_automatic_door_type(
    osmium::TagList const& tags) {
  using namespace std::literals;

  auto const val = tags["automatic_door"];
  if (val == nullptr) {
    return automatic_door_type::UNKNOWN;
  }

  if (val == "yes"sv) {
    return automatic_door_type::YES;
  } else if (val == "no"sv) {
    return automatic_door_type::NO;
  } else if (val == "button"sv) {
    return automatic_door_type::BUTTON;
  } else if (val == "motion"sv) {
    return automatic_door_type::MOTION;
  } else if (val == "floor"sv) {
    return automatic_door_type::FLOOR;
  } else if (val == "continuous"sv) {
    return automatic_door_type::CONTINUOUS;
  } else if (val == "slowdown_button"sv) {
    return automatic_door_type::SLOWDOWN_BUTTON;
  }

  return automatic_door_type::UNKNOWN;
}

}  // namespace ppr::preprocessing::osm
