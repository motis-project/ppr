#pragma once

#include "ppr/common/data.h"
#include "ppr/output/json.h"

namespace ppr::output::geojson {

template <typename Writer, typename Location>
void write_point(Writer& writer, Location const& loc) {
  writer.StartObject();
  writer.String("type");
  writer.String("Point");
  writer.String("coordinates");
  write_lon_lat(writer, loc);
  writer.EndObject();
}

template <typename Writer, typename Location>
void write_line_string(Writer& writer, data::vector<Location> const& line) {
  writer.StartObject();
  writer.String("type");
  writer.String("LineString");
  writer.String("coordinates");
  writer.StartArray();
  for (auto const& loc : line) {
    write_lon_lat(writer, loc);
  }
  writer.EndArray();
  writer.EndObject();
}

template <typename Writer, typename Polygon>
void write_polygon(Writer& writer, Polygon const& poly) {
  writer.StartObject();
  writer.String("type");
  writer.String("Polygon");
  writer.String("coordinates");
  writer.StartArray();

  writer.StartArray();
  for (auto const& loc : poly.outer()) {
    write_lon_lat(writer, loc);
  }
  writer.EndArray();

  for (auto const& inner : poly.inners()) {
    writer.StartArray();
    for (auto const& loc : inner) {
      write_lon_lat(writer, loc);
    }
    writer.EndArray();
  }

  writer.EndArray();
  writer.EndObject();
}

}  // namespace ppr::output::geojson
