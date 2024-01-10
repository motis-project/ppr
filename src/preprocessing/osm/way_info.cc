#include <cstring>
#include <string_view>

#include "ppr/preprocessing/clamp.h"
#include "ppr/preprocessing/names.h"
#include "ppr/preprocessing/osm/access.h"
#include "ppr/preprocessing/osm/crossing.h"
#include "ppr/preprocessing/osm/handrail.h"
#include "ppr/preprocessing/osm/layer.h"
#include "ppr/preprocessing/osm/level.h"
#include "ppr/preprocessing/osm/parse.h"
#include "ppr/preprocessing/osm/ramp.h"
#include "ppr/preprocessing/osm/surface.h"
#include "ppr/preprocessing/osm/way_info.h"
#include "ppr/preprocessing/osm/wheelchair.h"
#include "ppr/preprocessing/osm/width.h"

using namespace std::literals;

namespace ppr::preprocessing::osm {

bool is_street_with_sidewalks(street_type const street) {
  return street == street_type::RESIDENTIAL || street == street_type::SERVICE ||
         street == street_type::UNCLASSIFIED ||
         street == street_type::TERTIARY || street == street_type::SECONDARY ||
         street == street_type::PRIMARY || street == street_type::LIVING;
}

bool is_footway(street_type const street, osmium::TagList const& tags) {
  if (street == street_type::CYCLEWAY || street == street_type::BRIDLEWAY) {
    return access_allowed(tags["foot"], false);
  }
  return street == street_type::TRACK || street == street_type::FOOTWAY ||
         street == street_type::PATH || street == street_type::PEDESTRIAN ||
         street == street_type::STAIRS;
}

street_type get_street_type(char const* highway) {
  if (highway == nullptr) {
    return street_type::NONE;
  }

  if (strcmp(highway, "residential") == 0) {
    return street_type::RESIDENTIAL;
  } else if (strcmp(highway, "service") == 0) {
    return street_type::SERVICE;
  } else if (strcmp(highway, "track") == 0) {
    return street_type::TRACK;
  } else if (strcmp(highway, "unclassified") == 0 ||
             strcmp(highway, "road") == 0) {
    return street_type::UNCLASSIFIED;
  } else if (strcmp(highway, "footway") == 0) {
    return street_type::FOOTWAY;
  } else if (strcmp(highway, "path") == 0) {
    return street_type::PATH;
  } else if (strcmp(highway, "cycleway") == 0) {
    return street_type::CYCLEWAY;
  } else if (strcmp(highway, "bridleway") == 0) {
    return street_type::BRIDLEWAY;
  } else if (strcmp(highway, "tertiary") == 0 ||
             strcmp(highway, "tertiary_link") == 0) {
    return street_type::TERTIARY;
  } else if (strcmp(highway, "secondary") == 0 ||
             strcmp(highway, "secondary_link") == 0) {
    return street_type::SECONDARY;
  } else if (strcmp(highway, "primary") == 0 ||
             strcmp(highway, "primary_link") == 0) {
    return street_type::PRIMARY;
  } else if (strcmp(highway, "living_street") == 0) {
    return street_type::LIVING;
  } else if (strcmp(highway, "pedestrian") == 0) {
    return street_type::PEDESTRIAN;
  } else if (strcmp(highway, "steps") == 0) {
    return street_type::STAIRS;
  } else {
    return street_type::NONE;
  }
}

street_type extract_conveying(osmium::TagList const& tags, edge_info* info) {
  auto const* conveying = tags["conveying"];
  if (conveying != nullptr) {
    info->street_type_ = info->street_type_ == street_type::STAIRS
                             ? street_type::ESCALATOR
                             : street_type::MOVING_WALKWAY;
    if (strcmp(conveying, "forward") == 0) {
      info->allow_bwd_ = false;
    } else if (strcmp(conveying, "backward") == 0) {
      info->allow_fwd_ = false;
    }
  }
  return info->street_type_;
}

void extract_wheelchair_info(osmium::TagList const& tags, edge_info* info) {
  auto const* wheelchair = tags["wheelchair"];
  if (wheelchair != nullptr) {
    info->wheelchair_ = get_wheelchair_type(wheelchair);
  } else {
    auto const ramp = get_wheelchair_ramp(tags);
    if (ramp == tri_state::YES) {
      info->wheelchair_ = wheelchair_type::YES;
    } else if (ramp == tri_state::NO) {
      info->wheelchair_ = wheelchair_type::NO;
    }
  }
}

void extract_stroller_info(osmium::TagList const& tags, edge_info* info) {
  auto const* stroller = tags["stroller"];
  if (stroller != nullptr) {
    info->stroller_ = get_wheelchair_type(stroller);
  } else {
    auto const ramp = get_stroller_ramp(tags);
    if (ramp == tri_state::YES) {
      info->stroller_ = wheelchair_type::YES;
    } else if (ramp == tri_state::NO) {
      info->stroller_ = wheelchair_type::NO;
    }
  }
}

void extract_common_info(osmium::TagList const& tags, edge_info* info,
                         osm_graph& graph) {
  info->name_ = get_name(tags["name"], graph.names_, graph.names_map_);

  auto const* oneway = tags["oneway"];
  if (oneway != nullptr && strcmp(oneway, "no") != 0) {
    info->oneway_street_ = true;
  }

  extract_wheelchair_info(tags, info);
  extract_stroller_info(tags, info);

  info->area_ = tags.has_tag("area", "yes");

  info->surface_type_ = get_surface_type(tags["surface"]);
  info->smoothness_type_ = get_smoothness_type(tags["smoothness"]);
}

way_info get_platform_info(osmium::Way const& way, osmium::TagList const& tags,
                           osm_graph& graph) {
  auto [info_idx, info] =
      make_edge_info(graph.edge_infos_, way.id(), edge_type::FOOTWAY,
                     street_type::PLATFORM, crossing_type::NONE);

  extract_common_info(tags, info, graph);

  auto const width = 2.0;
  auto const layer = get_layer(tags);

  return {info_idx, false, false, width, layer};
}

bool should_generate_sidewalk(char const* value, bool const def) {
  if (value == nullptr) {
    return def;
  }
  // possible values: yes, no, separate
  return value == "yes"sv;
}

way_info get_highway_info(osmium::Way const& way, osmium::TagList const& tags,
                          osm_graph& graph) {
  auto const* highway = tags["highway"];
  assert(highway);

  auto type = edge_type::FOOTWAY;
  auto street = get_street_type(highway);
  auto crossing = get_way_crossing_type(tags);
  auto sidewalk_left = false;
  auto sidewalk_right = false;
  auto include = true;

  if (!access_allowed(tags, true)) {
    return {};
  }

  if (crossing != crossing_type::NONE) {
    type = edge_type::CROSSING;
  } else if (is_street_with_sidewalks(street)) {
    type = edge_type::STREET;

    auto const* sidewalk = tags["sidewalk"];
    if (sidewalk != nullptr) {
      if (strcmp(sidewalk, "left") == 0) {
        sidewalk_left = true;
      } else if (strcmp(sidewalk, "right") == 0) {
        sidewalk_right = true;
      } else if (strcmp(sidewalk, "separate") == 0) {
        // keep the information for crossing nodes, but don't create edges
        include = false;
      } else if (strcmp(sidewalk, "no") != 0 && strcmp(sidewalk, "none") != 0) {
        sidewalk_left = true;
        sidewalk_right = true;
      }
    } else {
      sidewalk_left = true;
      sidewalk_right = true;
    }

    auto const* sidewalk_both = tags["sidewalk:both"];
    sidewalk_left = should_generate_sidewalk(sidewalk_both, sidewalk_left);
    sidewalk_right = should_generate_sidewalk(sidewalk_both, sidewalk_right);

    sidewalk_left =
        should_generate_sidewalk(tags["sidewalk:left"], sidewalk_left);
    sidewalk_right =
        should_generate_sidewalk(tags["sidewalk:right"], sidewalk_right);

    if ((street == street_type::LIVING || street == street_type::RESIDENTIAL ||
         street == street_type::SERVICE ||
         street == street_type::UNCLASSIFIED) &&
        !sidewalk_left && !sidewalk_right) {
      type = edge_type::FOOTWAY;
    }
  } else if (is_footway(street, tags)) {
    type = edge_type::FOOTWAY;
  } else {
    return {};
  }

  auto [info_idx, info] =
      make_edge_info(graph.edge_infos_, way.id(), type, street, crossing);

  extract_common_info(tags, info, graph);

  auto const incline = parse_incline(tags["incline"]);
  info->incline_up_ = incline.up_.value_or(false);
  info->incline_ = incline.gradient_.value_or(UNKNOWN_INCLINE);

  if (crossing == crossing_type::SIGNALS) {
    extract_traffic_signal_attributes(info, tags);
  }

  if (street == street_type::STAIRS) {
    auto const step_count = parse_int(tags["step_count"]);
    if (step_count > 0) {
      info->step_count_ = clamp_uint8(step_count);
    }
    info->handrail_ = get_handrail(tags);

    if (!incline.up_.has_value()) {
      info->incline_up_ = true;  // default is up in drawing direction
    }

    if (info->wheelchair_ == wheelchair_type::UNKNOWN) {
      // default for steps is ramp=no unless specified otherwise (handled above)
      info->wheelchair_ = wheelchair_type::NO;
    }

    street = extract_conveying(tags, info);
  } else if (street == street_type::FOOTWAY) {
    street = extract_conveying(tags, info);
  }

  if (type == edge_type::FOOTWAY) {
    info->max_width_ = get_max_width_as_cm(tags);
  }

  info->area_ = tags.has_tag("area", "yes");

  info->surface_type_ = get_surface_type(tags["surface"]);
  info->smoothness_type_ = get_smoothness_type(tags["smoothness"]);

  info->level_ = get_level(tags);

  auto const width = get_render_width(type, street);
  auto const layer = get_layer(tags);

  return {info_idx, sidewalk_left, sidewalk_right, width, layer, include};
}

way_info get_railway_info(osmium::Way const& way, osmium::TagList const& tags,
                          osm_graph& graph) {
  auto const* railway = tags["railway"];
  assert(railway);

  auto street = street_type::RAIL;
  if (strcmp(railway, "rail") == 0) {
    street = street_type::RAIL;
    /*
    } else if (strcmp(railway, "tram") == 0 ||
               strcmp(railway, "light_rail") == 0) {
      street = street_type::TRAM;
    */
  } else {
    return {};
  }

  auto [info_idx, info] =
      make_edge_info(graph.edge_infos_, way.id(), edge_type::STREET, street,
                     crossing_type::NONE);

  info->level_ = get_level(tags);

  auto const width = 2.0;
  auto const layer = get_layer(tags);

  return {info_idx, false, false, width, layer};
}

way_info get_way_info(osmium::Way const& way, osm_graph& graph) {
  auto const& tags = way.tags();
  if (tags.has_tag("public_transport", "platform")) {
    return get_platform_info(way, tags, graph);
  } else if (tags.has_key("highway")) {
    return get_highway_info(way, tags, graph);
  } else if (tags.has_key("railway")) {
    return get_railway_info(way, tags, graph);
  } else {
    return {};
  }
}

}  // namespace ppr::preprocessing::osm
