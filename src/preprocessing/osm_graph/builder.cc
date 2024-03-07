#include <iostream>

#include "ppr/common/timing.h"
#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/osm_graph/builder.h"
#include "ppr/preprocessing/osm_graph/elevation.h"
#include "ppr/preprocessing/osm_graph/extractor.h"

using namespace ppr::preprocessing::elevation;

namespace ppr::preprocessing {

osm_graph build_osm_graph(options const& opt, logging& log, statistics& stats) {
  auto const t_start = timing_now();
  auto og = extract(opt.tmp_dir_, opt.osm_file_, log, stats);
  auto const t_after_extract = timing_now();
  stats.osm_.d_extract_ = ms_between(t_start, t_after_extract);

  if (!opt.dem_files_.empty()) {
    dem_source dem;
    for (auto const& file : opt.dem_files_) {
      dem.add_file(file);
    }
    add_elevation_data(og, dem, opt.elevation_sampling_interval_, log,
                       stats.elevation_);
  }
  auto const t_after_elevation = timing_now();

  stats.osm_.d_elevation_ = ms_between(t_after_extract, t_after_elevation);
  stats.osm_.d_total_ = ms_between(t_start, t_after_elevation);

  collect_stats(stats.osm_, og);

  return og;
}

}  // namespace ppr::preprocessing
