#pragma once

#include <cstdint>

#include "ppr/common/tri_state.h"

namespace ppr::output {

template <typename Writer, typename Location>
void write_lon_lat(Writer& writer, Location const& location) {
  writer.StartArray();
  writer.Double(location.lon());
  writer.Double(location.lat());
  writer.EndArray();
}

template <typename Writer>
void write_tri_state(Writer& writer, tri_state tri) {
  switch (tri) {
    case tri_state::UNKNOWN: writer.Null(); break;
    case tri_state::NO: writer.Bool(false); break;
    case tri_state::YES: writer.Bool(true); break;
  }
}

template <typename Writer>
void write_level(Writer& writer, std::int16_t level) {
  writer.String("level");
  writer.Double(static_cast<double>(level) / 10.0);
}

}  // namespace ppr::output
