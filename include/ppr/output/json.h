#pragma once

#include <optional>

#include "ppr/common/level.h"
#include "ppr/common/routing_graph.h"
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
void write_levels(Writer& writer, routing_graph_data const& rg,
                  levels const& lvl) {
  auto first_level = std::optional<double>{};
  writer.String("levels");
  writer.StartArray();
  if (lvl.has_single_level()) {
    first_level = to_human_level(lvl.single_level());
    writer.Double(*first_level);
  } else if (lvl.has_multiple_levels()) {
    for (auto const level : rg.levels_.at(lvl.multi_level_index())) {
      auto const human_level = to_human_level(level);
      writer.Double(human_level);
      if (!first_level) {
        first_level = human_level;
      }
    }
  }
  writer.EndArray();

  writer.String("level");
  if (first_level) {
    writer.Double(*first_level);
  } else {
    writer.Null();
  }
}

}  // namespace ppr::output
