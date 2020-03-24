#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace ppr {

struct routing_graph;

namespace preprocessing {

// all durations (d_*) are in milliseconds
using timing_t = double;

struct graph_statistics {
  std::size_t n_nodes_ = 0;
  std::size_t n_edges_ = 0;
  std::size_t n_edge_infos_ = 0;
  std::size_t n_areas_ = 0;
};

struct osm_graph_statistics {
  timing_t d_extract_ = 0;
  timing_t d_elevation_ = 0;
  timing_t d_total_ = 0;

  struct {
    timing_t d_relations_pass_ = 0;
    timing_t d_main_pass_ = 0;
    timing_t d_areas_ = 0;
    timing_t d_total_ = 0;
  } extract_;

  graph_statistics graph_;

  std::size_t n_access_not_allowed_nodes_ = 0;
  std::size_t n_crossing_nodes_ = 0;
  std::size_t n_elevators_ = 0;

  std::size_t n_linked_edges_ = 0;
  std::size_t n_double_linked_edges_ = 0;
  std::size_t n_moved_crossing_nodes_ = 0;

  std::size_t n_edge_footways_ = 0;  // excl. area_footways
  std::size_t n_edge_streets_ = 0;  // excl. railways
  std::size_t n_edge_railways_ = 0;
  std::size_t n_edge_crossings_ = 0;
  std::size_t n_edge_area_footways_ = 0;
};

struct elevation_statistics {
  std::size_t n_queries_ = 0;
  std::size_t n_misses_ = 0;
};

struct int_graph_statistics {
  timing_t d_parallel_streets_ = 0;
  timing_t d_move_crossings_ = 0;
  timing_t d_handle_edges_ = 0;
  timing_t d_areas_ = 0;
  timing_t d_total_ = 0;

  graph_statistics graph_;

  std::size_t n_compressed_edges_ = 0;

  std::size_t n_edge_footways_ = 0;  // incl. area_footways
  std::size_t n_edge_streets_ = 0;  // excl. railways
  std::size_t n_edge_railways_ = 0;
  std::size_t n_edge_crossings_ = 0;
};

struct routing_graph_statistics {
  timing_t d_junctions_ = 0;
  timing_t d_linked_crossings_ = 0;
  timing_t d_edges_ = 0;
  timing_t d_areas_ = 0;
  timing_t d_crossing_detours_ = 0;
  timing_t d_total_ = 0;

  graph_statistics graph_;

  std::size_t n_crossings_created_ = 0;
  std::size_t n_linked_crossings_created_ = 0;
  std::size_t n_path_join_failed_diff_ = 0;
  std::size_t n_path_join_failed_same_ = 0;

  std::size_t n_edge_footways_ = 0;
  std::size_t n_edge_streets_ = 0;
  std::size_t n_edge_crossings_ = 0;
  std::size_t n_edge_elevators_ = 0;
  std::size_t n_edge_connections_ = 0;

  std::size_t n_crossings_generated_ = 0;
  std::size_t n_crossings_unmarked_ = 0;
  std::size_t n_crossings_marked_ = 0;
  std::size_t n_crossings_island_ = 0;
  std::size_t n_crossings_signals_ = 0;
};

struct statistics {
  timing_t d_total_pp_ = 0;
  timing_t d_serialization_ = 0;
  timing_t d_rtrees_ = 0;
  std::size_t osm_input_size_ = 0;
  std::size_t serialized_size_ = 0;

  osm_graph_statistics osm_;
  elevation_statistics elevation_;
  int_graph_statistics int_;
  routing_graph_statistics routing_;
};

struct osm_graph;
struct int_graph;

void collect_stats(osm_graph_statistics&, osm_graph const&);
void collect_stats(int_graph_statistics&, int_graph const&);
void collect_stats(routing_graph_statistics&, routing_graph const&);

}  // namespace preprocessing
}  // namespace ppr
