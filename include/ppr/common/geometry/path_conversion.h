#pragma once

#include <algorithm>
#include <vector>

#include "ppr/common/geometry/merc.h"
#include "ppr/common/location.h"

#include "ppr/common/data.h"

namespace ppr {

template <typename Location>
inline std::vector<merc> to_merc_vector(std::vector<Location> const& loc_path) {
  std::vector<merc> merc_path;
  merc_path.reserve(loc_path.size());
  std::transform(begin(loc_path), end(loc_path), std::back_inserter(merc_path),
                 to_merc);
  return merc_path;
}

inline data::vector<location> to_location_vector(
    std::vector<merc> const& merc_path) {
  data::vector<location> loc_path;
  loc_path.reserve(
      static_cast<decltype(loc_path)::size_type>(merc_path.size()));
  std::transform(begin(merc_path), end(merc_path), std::back_inserter(loc_path),
                 to_location);
  return loc_path;
}

}  // namespace ppr
