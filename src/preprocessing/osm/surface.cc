#include "ppr/preprocessing/osm/surface.h"

namespace ppr::preprocessing::osm {

surface_type get_surface_type(char const* tag) {
  // https://wiki.openstreetmap.org/wiki/Key:surface
  // https://taginfo.openstreetmap.org/keys/surface#values
  if (tag == nullptr) {
    return surface_type::UNKNOWN;
  }

  if (strcmp(tag, "asphalt") == 0) {
    return surface_type::ASPHALT;
  } else if (strcmp(tag, "unpaved") == 0) {
    return surface_type::UNPAVED;
  } else if (strcmp(tag, "paved") == 0) {
    return surface_type::PAVED;
  } else if (strcmp(tag, "ground") == 0) {
    return surface_type::GROUND;
  } else if (strcmp(tag, "gravel") == 0) {
    return surface_type::GRAVEL;
  } else if (strcmp(tag, "concrete") == 0 ||
             strcmp(tag, "concrete:plates") == 0) {
    return surface_type::CONCRETE;
  } else if (strcmp(tag, "dirt") == 0) {
    return surface_type::DIRT;
  } else if (strcmp(tag, "paving_stones") == 0) {
    return surface_type::PAVING_STONES;
  } else if (strcmp(tag, "grass") == 0) {
    return surface_type::GRASS;
  } else if (strcmp(tag, "compacted") == 0) {
    return surface_type::COMPACTED;
  } else if (strcmp(tag, "sand") == 0) {
    return surface_type::SAND;
  } else if (strcmp(tag, "cobblestone") == 0 || strcmp(tag, "sett") == 0) {
    return surface_type::COBBLESTONE;
  } else if (strcmp(tag, "fine_gravel") == 0) {
    return surface_type::FINE_GRAVEL;
  } else if (strcmp(tag, "wood") == 0) {
    return surface_type::WOOD;
  } else if (strcmp(tag, "earth") == 0) {
    return surface_type::EARTH;
  } else if (strcmp(tag, "pebblestone") == 0) {
    return surface_type::PEBBLESTONE;
  } else if (strcmp(tag, "mud") == 0) {
    return surface_type::MUD;
  } else if (strcmp(tag, "grass_paver") == 0) {
    return surface_type::GRASS_PAVER;
  } else if (strcmp(tag, "metal") == 0) {
    return surface_type::METAL;
  } else if (strcmp(tag, "concrete:lanes") == 0) {
    return surface_type::CONCRETE_LANES;
  } else {
    return surface_type::UNKNOWN;
  }
}

smoothness_type get_smoothness_type(char const* tag) {
  // https://wiki.openstreetmap.org/wiki/Key:smoothness
  // https://taginfo.openstreetmap.org/keys/smoothness#values
  if (tag == nullptr) {
    return smoothness_type::UNKNOWN;
  }

  if (strcmp(tag, "good") == 0) {
    return smoothness_type::GOOD;
  } else if (strcmp(tag, "excellent") == 0) {
    return smoothness_type::EXCELLENT;
  } else if (strcmp(tag, "intermediate") == 0) {
    return smoothness_type::INTERMEDIATE;
  } else if (strcmp(tag, "bad") == 0) {
    return smoothness_type::BAD;
  } else if (strcmp(tag, "very_bad") == 0) {
    return smoothness_type::VERY_BAD;
  } else if (strcmp(tag, "horrible") == 0) {
    return smoothness_type::HORRIBLE;
  } else if (strcmp(tag, "very_horrible") == 0) {
    return smoothness_type::VERY_HORRIBLE;
  } else if (strcmp(tag, "impassable") == 0) {
    return smoothness_type::IMPASSABLE;
  } else {
    return smoothness_type::UNKNOWN;
  }
}

}  // namespace ppr::preprocessing::osm
