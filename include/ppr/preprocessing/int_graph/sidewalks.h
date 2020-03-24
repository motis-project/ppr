#pragma once

#include "ppr/common/geometry/merc.h"

namespace ppr::preprocessing {

std::pair<std::vector<merc>, std::vector<merc>> generate_sidewalk_paths(
    std::vector<merc> const& way_path, double width);

}  // namespace ppr::preprocessing
