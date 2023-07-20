#pragma once

#include <string_view>

#include "osmium/osm.hpp"

#include "ppr/common/enums.h"
#include "ppr/common/tri_state.h"

namespace ppr::preprocessing::osm {

crossing_type get_node_crossing_type(osmium::TagList const& tags);

crossing_type get_way_crossing_type(osmium::TagList const& tags);

template <typename T>
inline void extract_traffic_signal_attributes(T* info,
                                              osmium::TagList const& tags) {
  using namespace std::literals;

  auto const sound = tags["traffic_signals:sound"];
  if (sound != nullptr) {
    info->traffic_signals_sound_ =
        sound != "no"sv ? tri_state::YES : tri_state::NO;
  }

  auto const vibration = tags["traffic_signals:vibration"];
  if (vibration != nullptr) {
    info->traffic_signals_vibration_ =
        vibration == "yes"sv ? tri_state::YES : tri_state::NO;
  }
}

}  // namespace ppr::preprocessing::osm
