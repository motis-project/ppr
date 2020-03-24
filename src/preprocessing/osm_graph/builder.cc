#include <iostream>

#include "ppr/common/timing.h"
#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/osm_graph/builder.h"
#include "ppr/preprocessing/osm_graph/elevation.h"
#include "ppr/preprocessing/osm_graph/extractor.h"

using namespace ppr::preprocessing::elevation;

namespace ppr::preprocessing {

osm_graph build_osm_graph(options const& opt, statistics& stats) {
  auto const t_start = timing_now();
  std::cout << "Building osm graph..." << std::endl;
  auto og = extract(opt.osm_file_, opt, stats);
  auto const t_after_extract = timing_now();
  stats.osm_.d_extract_ = ms_between(t_start, t_after_extract);

  log_step(pp_step::OSM_DEM);
  if (!opt.dem_files_.empty()) {
    std::cout << "Adding elevation data..." << std::endl;
    dem_source dem;
    for (auto const& file : opt.dem_files_) {
      dem.add_file(file);
    }
    add_elevation_data(og, dem, opt.elevation_sampling_interval_,
                       stats.elevation_);
  } else {
    std::cout << "No elevation data available" << std::endl;
  }
  auto const t_after_elevation = timing_now();

  stats.osm_.d_elevation_ = ms_between(t_after_extract, t_after_elevation);
  stats.osm_.d_total_ = ms_between(t_start, t_after_elevation);
  print_timing("OSM Graph: OSM Extract", stats.osm_.d_extract_);
  print_timing("OSM Graph: Elevation", stats.osm_.d_elevation_);
  print_timing("OSM Graph: Total", stats.osm_.d_total_);

  collect_stats(stats.osm_, og);

  return og;
}

}  // namespace ppr::preprocessing
