#include <algorithm>
#include <functional>
#include <limits>

#include "ppr/preprocessing/osm/parse.h"
#include "ppr/preprocessing/osm/width.h"

namespace ppr::preprocessing::osm {

double get_actual_width(osmium::TagList const& tags, double def) {
  // http://wiki.openstreetmap.org/wiki/Key:width
  auto width = parse_length(tags["width"]);
  if (std::equal_to<>()(width, 0.0)) {
    width = parse_length(tags["est_width"]);
  }
  if (std::equal_to<>()(width, 0.0)) {
    // https://de.wikipedia.org/wiki/Richtlinien_f%C3%BCr_die_Anlage_von_Stra%C3%9Fen_%E2%80%93_Querschnitt
    auto lanes = parse_int(tags["lanes"], 2);
    // auto type = tags["highway"];
    width = 4.0 * lanes;
  }
  if (std::equal_to<>()(width, 0.0)) {
    width = def;
  }
  return width;
}

double get_render_width(edge_type edge, street_type street) {
  if (edge != edge_type::STREET) {
    return 2.0;
  }
  switch (street) {
    case street_type::PEDESTRIAN:
    case street_type::LIVING:
    case street_type::RESIDENTIAL:
    case street_type::UNCLASSIFIED: return 4.0;  // ~3.2
    case street_type::SERVICE: return 3.0;  // ~2.3
    case street_type::PRIMARY:
    case street_type::SECONDARY:
    case street_type::TERTIARY: return 6.0;  // ~5.2
    default: return 2.0;
  }
}

inline std::uint8_t m_to_cm(double m) {
  return static_cast<std::uint8_t>(std::clamp(
      m * 100, 0.,
      static_cast<double>(std::numeric_limits<std::uint8_t>::max())));
}

std::uint8_t get_max_width_as_cm(osmium::TagList const& tags) {
  if (auto const val = tags["maxwidth:physical"]; val != nullptr) {
    return m_to_cm(parse_length(val));
  } else if (auto const val = tags["maxwidth"]; val != nullptr) {
    return m_to_cm(parse_length(val));
  } else if (auto const val = tags["width"]; val != nullptr) {
    return m_to_cm(parse_length(val));
  } else {
    return 0;
  }
}

std::uint8_t get_cycle_barrier_max_width_as_cm(osmium::TagList const& tags) {
  auto const max_width = get_max_width_as_cm(tags);
  auto const opening = m_to_cm(parse_length(tags["opening"]));
  auto const spacing = m_to_cm(parse_length(tags["spacing"]));

  auto result = max_width;
  auto const add_min = [&](auto const& val) {
    if (val != 0) {
      if (result == 0) {
        result = val;
      } else {
        result = std::min(result, val);
      }
    }
  };

  add_min(opening);
  add_min(spacing);

  return result;
}

}  // namespace ppr::preprocessing::osm
