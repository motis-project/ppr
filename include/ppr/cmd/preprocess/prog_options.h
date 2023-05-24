#pragma once

#include <algorithm>
#include <string>
#include <thread>
#include <vector>

#include "boost/program_options.hpp"

#include "conf/configuration.h"

#include "ppr/preprocessing/options.h"

namespace ppr::preprocessing {

class prog_options : public conf::configuration {
public:
  explicit prog_options() : configuration("Preprocessing Options") {
    param(osm_file_, "osm,o", "OSM dataset");
    param(graph_file_, "graph,g", "output file");
    param(dem_files_, "dem", "DEM (elevation) files");
    param(elevation_sampling_interval_, "dem-interval",
          "Elevation sampling interval (meters, 0 to disable)");
    param(crossing_detours_limit_, "detours-limit",
          "Limit for unmarked crossing detours (meters)");
    param(print_warnings_, "warnings", "Print warnings");
    param(move_crossings_, "move-crossings", "Move nodes away from junctions");
    param(create_rtrees_, "create-rtrees", "Create r-tree files");
    param(threads_, "threads", "Number of threads");
    param(edge_rtree_max_size_, "edge-rtree-max-size",
          "Maximum size for edge r-tree file");
    param(area_rtree_max_size_, "area-rtree-max-size",
          "Maximum size for area r-tree file");
    param(verify_graph_, "verify-graph", "Verify generated graph file");
    param(print_timing_overview_, "timings", "Print timing overview");
    param(print_memory_usage_, "mem", "Print memory usage");
  }

  options get_options() const {
    options opt;
    opt.osm_file_ = osm_file_;
    opt.dem_files_ = dem_files_;
    opt.graph_file_ = graph_file_;
    opt.elevation_sampling_interval_ =
        static_cast<double>(elevation_sampling_interval_);
    opt.crossing_detours_limit_ = static_cast<double>(crossing_detours_limit_);
    opt.print_warnings_ = print_warnings_;
    opt.move_crossings_ = move_crossings_;
    opt.create_rtrees_ = create_rtrees_;
    opt.threads_ = static_cast<unsigned>(std::max(1, threads_));
    opt.edge_rtree_max_size_ = edge_rtree_max_size_;
    opt.area_rtree_max_size_ = area_rtree_max_size_;
    return opt;
  }

  std::string osm_file_{"germany-latest.osm.pbf"};
  std::string graph_file_{"routing-graph.ppr"};
  std::vector<std::string> dem_files_;
  int elevation_sampling_interval_{30};
  int crossing_detours_limit_{600};
  bool print_warnings_{false};
  bool move_crossings_{false};
  bool create_rtrees_{true};
  bool verify_graph_{false};
  bool print_timing_overview_{false};
  bool print_memory_usage_{false};
  int threads_{static_cast<int>(std::thread::hardware_concurrency())};
  std::size_t edge_rtree_max_size_{1024UL * 1024 * 1024 * 3};
  std::size_t area_rtree_max_size_{1024UL * 1024 * 1024};
};

}  // namespace ppr::preprocessing
