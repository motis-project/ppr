#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ppr::routing {

// all durations (d_*) are in milliseconds

struct dijkstra_statistics {
  std::size_t labels_created_ = 0;
  std::size_t labels_popped_ = 0;
  std::size_t start_labels_ = 0;
  std::size_t additional_nodes_ = 0;
  std::size_t additional_edges_ = 0;
  std::size_t additional_areas_ = 0;
  std::size_t goals_ = 0;
  std::size_t goals_reached_ = 0;
  double d_starts_ = 0;
  double d_goals_ = 0;
  double d_area_edges_ = 0;
  double d_search_ = 0;
  double d_labels_to_route_ = 0;
  double d_total_ = 0;
  bool max_label_quit_ = false;
};

struct routing_statistics {
  int attempts_ = 0;
  int start_pts_extended_ = 0;
  int destination_pts_extended_ = 0;
  double d_start_pts_ = 0;
  double d_start_pts_extended_ = 0;
  double d_destination_pts_ = 0;
  double d_destination_pts_extended_ = 0;
  double d_postprocessing_ = 0;
  double d_total_ = 0;
  std::vector<dijkstra_statistics> dijkstra_statistics_;
};

}  // namespace ppr::routing
