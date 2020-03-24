#pragma once

#include <algorithm>

#include "ppr/common/routing_graph.h"
#include "ppr/output/geojson/base.h"
#include "ppr/output/geojson/edge_info.h"
#include "ppr/preprocessing/osm_graph/osm_graph.h"

namespace ppr::output::geojson {

// routing graph

template <typename Writer>
void write_node(Writer& writer, node const* n) {
  writer.StartObject();
  writer.String("type");
  writer.String("Feature");

  writer.String("properties");
  writer.StartObject();
  writer.String("id");
  writer.Uint64(n->id_);
  writer.String("osm_node_id");
  writer.Int64(n->osm_id_);
  writer.EndObject();

  writer.String("geometry");
  write_point(writer, n->location_);

  writer.String("id");
  writer.Uint64(n->id_);
  writer.EndObject();
}

template <typename Writer>
void write_edge_properties(Writer& writer, edge const& e) {
  write_edge_info(writer, e.info_);
  writer.String("side");
  switch (e.side_) {
    case side_type::CENTER: writer.String("center"); break;
    case side_type::LEFT: writer.String("left"); break;
    case side_type::RIGHT: writer.String("right"); break;
  }
  writer.String("distance");
  writer.Double(e.distance_);
  writer.String("elevation_up");
  writer.Int(e.elevation_up_);
  writer.String("elevation_down");
  writer.Int(e.elevation_down_);
}

template <typename Writer>
void write_edge(Writer& writer, edge const& e) {
  writer.StartObject();
  writer.String("type");
  writer.String("Feature");

  writer.String("properties");
  writer.StartObject();
  write_edge_properties(writer, e);
  writer.EndObject();

  writer.String("geometry");
  write_line_string(writer, e.path_);

  writer.String("style");
  writer.StartObject();
  write_edge_style(writer, e.info_);
  writer.EndObject();

  writer.EndObject();
}

template <typename Writer>
void write_area_properties(Writer& writer, area const& a) {
  writer.String("id");
  writer.Int64(a.id_);

  if (a.from_way_) {
    writer.String("osm_way_id");
    writer.Int64(a.osm_id_);
  } else {
    writer.String("osm_relation_id");
    writer.Int64(a.osm_id_);
  }

  writer.String("name");
  if (a.name_ != nullptr) {
    writer.String(a.name_->data(), a.name_->size());
  } else {
    writer.String("");
  }

  writer.String("from_way");
  writer.Bool(a.from_way_);

  writer.String("exit_node_count");
  writer.Uint64(a.exit_nodes_.size());

  writer.String("vg_dim");
  writer.Uint(a.dist_matrix_.dimension().first);

  writer.String("adjacent_areas");
  writer.StartArray();
  for (auto const aa : a.adjacent_areas_) {
    writer.Uint(aa);
  }
  writer.EndArray();
}

template <typename Writer>
void write_visibility_graph(Writer& writer, area const& a) {
  writer.StartObject();
  writer.String("type");
  writer.String("Feature");

  writer.String("geometry");
  writer.StartObject();
  writer.String("type");
  writer.String("MultiLineString");
  writer.String("coordinates");
  writer.StartArray();

  auto const& nodes = a.get_nodes();
  for (uint16_t i = 0U; i < a.next_matrix_.dimension().first; ++i) {
    for (uint16_t j = 0U; j < a.next_matrix_.dimension().second; ++j) {
      if (a.next_matrix_.at(i, j) == std::numeric_limits<uint16_t>::max()) {
        continue;
      }
      writer.StartArray();
      auto u = i;
      write_lon_lat(writer, nodes[i].location_);
      while (u != j) {
        auto const next_node = a.next_matrix_.at(u, j);
        write_lon_lat(writer, nodes[next_node].location_);
        u = next_node;
      }
      writer.EndArray();
    }
  }
  writer.EndArray();
  writer.EndObject();

  writer.String("style");
  writer.StartObject();
  writer.String("stroke");
  writer.String("#f475e1");
  writer.EndObject();

  writer.EndObject();
}

template <typename Writer>
void write_area_polygon(Writer& writer, area const& a) {
  writer.StartObject();
  writer.String("type");
  writer.String("Feature");

  writer.String("properties");
  writer.StartObject();
  write_area_properties(writer, a);
  writer.EndObject();

  writer.String("geometry");
  write_polygon(writer, a.polygon_);

  writer.String("style");
  writer.StartObject();
  writer.String("stroke");
  writer.String("#ff19dc");
  writer.EndObject();

  writer.EndObject();
}

template <typename Writer>
void write_area(Writer& writer, area const& a,
                bool const include_visibility_graph) {
  write_area_polygon(writer, a);
  if (include_visibility_graph) {
    write_visibility_graph(writer, a);
  }
}

// intermediate graph

template <typename Writer>
void write_node(Writer& writer, preprocessing::osm_node const* n) {
  writer.StartObject();
  writer.String("type");
  writer.String("Feature");

  writer.String("properties");
  writer.StartObject();

  writer.String("osm_node_id");
  writer.Int64(n->osm_id_);

  writer.String("access_allowed");
  writer.Bool(n->access_allowed_);

  writer.String("crossing");
  write_crossing_type(writer, n->crossing_);

  writer.String("compressed");
  writer.Bool(n->compressed_);

  writer.EndObject();

  writer.String("geometry");
  write_point(writer, to_location(n->location_));
  writer.EndObject();
}

template <typename Writer>
void write_edge_properties(Writer& writer, preprocessing::osm_edge const& e) {
  write_edge_info(writer, e.info_);

  writer.String("width");
  writer.Double(e.width_);

  writer.String("generate_sidewalks");
  writer.Bool(e.generate_sidewalks());

  writer.String("sidewalk_left");
  writer.Bool(e.sidewalk_left_);

  writer.String("sidewalk_right");
  writer.Bool(e.sidewalk_right_);

  //  writer.String("crossings");
  //  writer.Uint64(e.crossings_.size());
}

template <typename Writer>
void write_edge(Writer& writer, preprocessing::osm_edge const& e) {
  writer.StartObject();
  writer.String("type");
  writer.String("Feature");

  writer.String("properties");
  writer.StartObject();
  write_edge_properties(writer, e);
  writer.EndObject();

  writer.String("geometry");
  {
    std::vector<merc> merc_path{e.from_->location_, e.to_->location_};
    std::vector<location> loc_path;
    loc_path.reserve(merc_path.size());
    std::transform(begin(merc_path), end(merc_path),
                   std::back_inserter(loc_path), to_location);
    write_line_string(writer, loc_path);
  }

  writer.String("style");
  writer.StartObject();
  write_edge_style(writer, e.info_);
  writer.EndObject();

  writer.EndObject();
}

// shared

template <typename Writer, typename Graph>
void write_graph(Writer& writer, Graph const& g) {
  writer.SetDoublePrecision(12);
  writer.StartObject();
  writer.String("type");
  writer.String("FeatureCollection");
  writer.String("features");
  writer.StartArray();

  for (auto const& n : g.nodes_) {
    write_node(writer, n.get());
    for (auto const& e : n->out_edges_) {
      write_edge(writer, e);
    }
  }

  writer.EndArray();
  writer.EndObject();
}

}  // namespace ppr::output::geojson
