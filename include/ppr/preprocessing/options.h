#pragma once

#include <string>
#include <thread>
#include <vector>

namespace ppr::preprocessing {

struct options {
  std::string osm_file_;
  std::vector<std::string> dem_files_;

  std::string graph_file_;

  double elevation_sampling_interval_{30};  // meters
  double crossing_detours_limit_{600};  // meters

  bool print_warnings_{true};
  bool move_crossings_{false};
  bool create_rtrees_{true};
  std::size_t edge_rtree_max_size_{1024UL * 1024 * 1024 * 3};
  std::size_t area_rtree_max_size_{1024UL * 1024 * 1024};
};

}  // namespace ppr::preprocessing
