#include <functional>

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

}  // namespace ppr::preprocessing::osm
