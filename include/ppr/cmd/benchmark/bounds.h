#pragma once

#include "ppr/common/location.h"
#include "ppr/common/location_geometry.h"

namespace ppr::benchmark {

class bounds {
public:
  virtual ~bounds() = default;
  bounds() = default;
  bounds(bounds const&) = delete;
  bounds& operator=(bounds const&) = delete;
  bounds(bounds&&) = delete;
  bounds& operator=(bounds&&) = delete;

  virtual bool contains(location const& loc) const = 0;
  virtual location random_pt() = 0;
};

}  // namespace ppr::benchmark
