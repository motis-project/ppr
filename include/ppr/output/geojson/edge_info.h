#pragma once

#include "ppr/common/routing_graph.h"

namespace ppr::output::geojson {

template <typename Writer>
void write_edge_style(Writer& writer, edge_info const* info) {
  writer.String("stroke");
  switch (info->type_) {
    case edge_type::CROSSING:
      if (info->street_type_ == street_type::RAIL ||
          info->street_type_ == street_type::TRAM) {
        writer.String("#ff9131");
        return;
      }
      switch (info->crossing_type_) {
        case crossing_type::GENERATED: writer.String("#39c0f5"); break;
        case crossing_type::UNMARKED: writer.String("#9c3af5"); break;
        case crossing_type::MARKED: writer.String("#ff278e"); break;
        case crossing_type::SIGNALS: writer.String("#ff1813"); break;
        case crossing_type::ISLAND: writer.String("#a0520e"); break;
        default: writer.String("#9c3af5"); break;
      }
      break;
    case edge_type::FOOTWAY:
      if (info->area_) {
        writer.String("#008f2a");
      } else {
        writer.String("#00ff00");
      }
      break;
    case edge_type::STREET: writer.String("#0000ff"); break;
    case edge_type::CONNECTION:
    case edge_type::ELEVATOR: writer.String("#000000"); break;
  }
}

template <typename Writer>
void write_edge_type(Writer& writer, edge_type const type) {
  switch (type) {
    case edge_type::CROSSING: writer.String("crossing"); break;
    case edge_type::FOOTWAY: writer.String("footway"); break;
    case edge_type::STREET: writer.String("street"); break;
    case edge_type::CONNECTION: writer.String("connection"); break;
    case edge_type::ELEVATOR: writer.String("elevator"); break;
  }
}

template <typename Writer>
void write_street_type(Writer& writer, street_type const type) {
  switch (type) {
    case street_type::NONE: writer.String("none"); break;
    case street_type::PEDESTRIAN: writer.String("pedestrian"); break;
    case street_type::LIVING: writer.String("living"); break;
    case street_type::RESIDENTIAL: writer.String("residential"); break;
    case street_type::SERVICE: writer.String("service"); break;
    case street_type::UNCLASSIFIED: writer.String("unclassified"); break;
    case street_type::TERTIARY: writer.String("tertiary"); break;
    case street_type::SECONDARY: writer.String("secondary"); break;
    case street_type::PRIMARY: writer.String("primary"); break;
    case street_type::TRACK: writer.String("track"); break;
    case street_type::FOOTWAY: writer.String("footway"); break;
    case street_type::PATH: writer.String("path"); break;
    case street_type::CYCLEWAY: writer.String("cycleway"); break;
    case street_type::BRIDLEWAY: writer.String("bridleway"); break;
    case street_type::STAIRS: writer.String("stairs"); break;
    case street_type::ESCALATOR: writer.String("escalator"); break;
    case street_type::MOVING_WALKWAY: writer.String("moving_walkway"); break;
    case street_type::RAIL: writer.String("rail"); break;
    case street_type::TRAM: writer.String("tram"); break;
    case street_type::PLATFORM: writer.String("platform"); break;
  }
}

template <typename Writer>
void write_crossing_type(Writer& writer,
                         crossing_type::crossing_type const type) {
  switch (type) {
    case crossing_type::NONE: writer.String("none"); break;
    case crossing_type::GENERATED: writer.String("generated"); break;
    case crossing_type::UNMARKED: writer.String("unmarked"); break;
    case crossing_type::MARKED: writer.String("marked"); break;
    case crossing_type::SIGNALS: writer.String("signals"); break;
    case crossing_type::ISLAND: writer.String("island"); break;
  }
}

template <typename Writer>
void write_tri_state(Writer& writer, tri_state::tri_state const state) {
  switch (state) {
    case tri_state::UNKNOWN: writer.String("unknown"); break;
    case tri_state::YES: writer.String("yes"); break;
    case tri_state::NO: writer.String("no"); break;
  }
}

template <typename Writer>
void write_wheelchair_type(Writer& writer,
                           wheelchair_type::wheelchair_type const type) {
  switch (type) {
    case wheelchair_type::UNKNOWN: writer.String("unknown"); break;
    case wheelchair_type::NO: writer.String("no"); break;
    case wheelchair_type::LIMITED: writer.String("limited"); break;
    case wheelchair_type::YES: writer.String("yes"); break;
  }
}

template <typename Writer>
void write_surface_type(Writer& writer, surface_type const type) {
  switch (type) {
    case surface_type::UNKNOWN: writer.String("unknown"); break;
    case surface_type::PAVED: writer.String("paved"); break;
    case surface_type::ASPHALT: writer.String("asphalt"); break;
    case surface_type::CONCRETE: writer.String("concrete"); break;
    case surface_type::PAVING_STONES: writer.String("paving_stones"); break;
    case surface_type::METAL: writer.String("metal"); break;
    case surface_type::WOOD: writer.String("wood"); break;
    case surface_type::CONCRETE_LANES: writer.String("concrete_lanes"); break;
    case surface_type::COBBLESTONE: writer.String("cobblestone"); break;
    case surface_type::UNPAVED: writer.String("unpaved"); break;
    case surface_type::COMPACTED: writer.String("compacted"); break;
    case surface_type::FINE_GRAVEL: writer.String("fine_gravel"); break;
    case surface_type::GRASS_PAVER: writer.String("grass_paver"); break;
    case surface_type::PEBBLESTONE: writer.String("pebblestone"); break;
    case surface_type::SAND: writer.String("sand"); break;
    case surface_type::GROUND: writer.String("ground"); break;
    case surface_type::EARTH: writer.String("earth"); break;
    case surface_type::GRASS: writer.String("grass"); break;
    case surface_type::GRAVEL: writer.String("gravel"); break;
    case surface_type::DIRT: writer.String("dirt"); break;
    case surface_type::MUD: writer.String("mud"); break;
  }
}

template <typename Writer>
void write_smoothness_type(Writer& writer, smoothness_type const type) {
  switch (type) {
    case smoothness_type::UNKNOWN: writer.String("unknown"); break;
    case smoothness_type::IMPASSABLE: writer.String("impassable"); break;
    case smoothness_type::VERY_HORRIBLE: writer.String("very_horrible"); break;
    case smoothness_type::HORRIBLE: writer.String("horrible"); break;
    case smoothness_type::VERY_BAD: writer.String("very_bad"); break;
    case smoothness_type::BAD: writer.String("bad"); break;
    case smoothness_type::INTERMEDIATE: writer.String("intermediate"); break;
    case smoothness_type::GOOD: writer.String("good"); break;
    case smoothness_type::EXCELLENT: writer.String("excellent"); break;
  }
}

template <typename Writer>
void write_edge_info(Writer& writer, edge_info const* info) {
  writer.String("osm_way_id");
  writer.Int64(info->osm_way_id_);

  writer.String("edge_type");
  write_edge_type(writer, info->type_);

  writer.String("street_type");
  write_street_type(writer, info->street_type_);

  writer.String("crossing_type");
  write_crossing_type(writer, info->crossing_type_);

  writer.String("surface_type");
  write_surface_type(writer, info->surface_type_);

  writer.String("smoothness_type");
  write_smoothness_type(writer, info->smoothness_type_);

  writer.String("name");
  if (info->name_ != nullptr) {
    writer.String(info->name_->data(), info->name_->size());
  } else {
    writer.String("");
  }

  writer.String("area");
  writer.Bool(info->area_);

  writer.String("oneway_street");
  writer.Bool(info->oneway_street_);

  writer.String("allow_fwd");
  writer.Bool(info->allow_fwd_);

  writer.String("allow_bwd");
  writer.Bool(info->allow_bwd_);

  writer.String("handrail");
  write_tri_state(writer, info->handrail_);

  writer.String("wheelchair");
  write_wheelchair_type(writer, info->wheelchair_);

  writer.String("incline_up");
  writer.Bool(info->incline_up_);

  writer.String("step_count");
  writer.Int(info->step_count_);

  writer.String("marked_crossing_detour");
  writer.Int(info->marked_crossing_detour_);
}

}  // namespace ppr::output::geojson
