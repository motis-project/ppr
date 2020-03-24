#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "ppr/common/elevation.h"
#include "ppr/common/location.h"

namespace ppr::preprocessing::elevation {

struct dem_source {
  dem_source();
  ~dem_source();
  dem_source(dem_source const&) = delete;
  dem_source& operator=(dem_source const&) = delete;
  dem_source(dem_source&&) = delete;
  dem_source& operator=(dem_source&&) = delete;

  void add_file(std::string const& filename);

  elevation_t get(location const& loc) const;

private:
  struct impl;
  std::unique_ptr<impl> impl_;
};

}  // namespace ppr::preprocessing::elevation
