#pragma once

#include <cstdint>

namespace ppr {

enum class edge_type : std::uint8_t {
  CONNECTION,
  STREET,
  FOOTWAY,
  CROSSING,
  ELEVATOR,
  ENTRANCE,
  CYCLE_BARRIER
};

enum class side_type : std::uint8_t { CENTER, LEFT, RIGHT };

enum class crossing_type : std::uint8_t {
  NONE,
  GENERATED,
  UNMARKED,
  MARKED,
  ISLAND,
  SIGNALS
};

enum class street_type : std::uint8_t {
  NONE,
  // edge_type = FOOTWAY
  TRACK,
  FOOTWAY,
  PATH,
  CYCLEWAY,
  BRIDLEWAY,
  STAIRS,
  ESCALATOR,
  MOVING_WALKWAY,
  PLATFORM,
  // edge_type = STREET
  SERVICE,
  PEDESTRIAN,
  LIVING,
  RESIDENTIAL,
  UNCLASSIFIED,
  TERTIARY,
  SECONDARY,
  PRIMARY,
  // railway
  RAIL,
  TRAM
};

enum class wheelchair_type : std::uint8_t { UNKNOWN, NO, LIMITED, YES };

// https://wiki.openstreetmap.org/wiki/Key:surface
// (only the most popular ones)
// order is important
enum class surface_type : std::uint8_t {
  UNKNOWN,
  // paved surfaces
  PAVED,
  ASPHALT,
  CONCRETE,
  PAVING_STONES,
  METAL,
  WOOD,
  CONCRETE_LANES,
  COBBLESTONE,
  // unpaved surfaces
  UNPAVED,
  COMPACTED,
  FINE_GRAVEL,
  GRASS_PAVER,
  PEBBLESTONE,
  SAND,
  GROUND,
  EARTH,
  GRASS,
  GRAVEL,
  DIRT,
  MUD,
};

// https://wiki.openstreetmap.org/wiki/Key:smoothness
enum class smoothness_type : std::uint8_t {
  UNKNOWN,
  IMPASSABLE,
  VERY_HORRIBLE,
  HORRIBLE,
  VERY_BAD,
  BAD,
  INTERMEDIATE,
  GOOD,
  EXCELLENT
};

// https://wiki.openstreetmap.org/wiki/Key:door
enum class door_type : std::uint8_t {
  UNKNOWN,
  YES,
  NO,
  HINGED,
  SLIDING,
  REVOLVING,
  FOLDING,
  TRAPDOOR,
  OVERHEAD
};

// https://wiki.openstreetmap.org/wiki/Key:automatic_door
enum class automatic_door_type : std::uint8_t {
  UNKNOWN,
  YES,
  NO,
  BUTTON,
  MOTION,
  FLOOR,
  CONTINUOUS,
  SLOWDOWN_BUTTON
};

}  // namespace ppr
