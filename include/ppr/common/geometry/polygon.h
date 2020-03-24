#pragma once

#include "ppr/common/geometry/merc.h"

namespace ppr {

using area_polygon_t = boost::geometry::model::polygon<merc, false>;
using inner_area_polygon_t = boost::geometry::model::polygon<merc, true>;

}  // namespace ppr
