#pragma once

#include <cstdint>

namespace ppr {

enum class edge_type : uint8_t {
  CONNECTION,
  STREET,
  FOOTWAY,
  CROSSING,
  ELEVATOR
};

enum class side_type : uint8_t { CENTER, LEFT, RIGHT };

namespace crossing_type {
// enum class crossing_type : uint8_t { NONE, UNMARKED, MARKED, ISLAND, SIGNALS
// };
using crossing_type = uint8_t;
constexpr crossing_type NONE = 0;
constexpr crossing_type GENERATED = 1;
constexpr crossing_type UNMARKED = 2;
constexpr crossing_type MARKED = 3;
constexpr crossing_type ISLAND = 4;
constexpr crossing_type SIGNALS = 5;
}  // namespace crossing_type

enum class street_type : uint8_t {
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

namespace wheelchair_type {
// enum wheelchair_type : uint8_t { UNKNOWN, NO, LIMITED, YES };
using wheelchair_type = uint8_t;
constexpr wheelchair_type UNKNOWN = 0;
constexpr wheelchair_type NO = 1;
constexpr wheelchair_type LIMITED = 2;
constexpr wheelchair_type YES = 3;
}  // namespace wheelchair_type

// https://wiki.openstreetmap.org/wiki/Key:surface
// (only the most popular ones)
// order is important
enum class surface_type : uint8_t {
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
enum class smoothness_type : uint8_t {
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

}  // namespace ppr
