#include <fstream>

#include "ppr/preprocessing/stats_writer.h"

namespace ppr::preprocessing {

namespace {

template <typename T>
void write(std::ofstream& out, char const* key, T const& val) {
  out << key << "," << val << std::endl;
}

}  // namespace

void write_stats(statistics const& s, std::string const& filename) {
  std::ofstream out(filename);
  write(out, "key", "value");

  write(out, "d_total_pp", s.d_total_pp_);
  write(out, "d_serialization", s.d_serialization_);
  write(out, "d_rtrees", s.d_rtrees_);
  write(out, "osm_input_size", s.osm_input_size_);
  write(out, "serialized_size", s.serialized_size_);

  write(out, "osm.d_extract", s.osm_.d_extract_);
  write(out, "osm.d_elevation", s.osm_.d_elevation_);
  write(out, "osm.d_total", s.osm_.d_total_);
  write(out, "osm.extract.d_relations_pass", s.osm_.extract_.d_relations_pass_);
  write(out, "osm.extract.d_main_pass", s.osm_.extract_.d_main_pass_);
  write(out, "osm.extract.d_areas", s.osm_.extract_.d_areas_);
  write(out, "osm.extract.d_total", s.osm_.extract_.d_total_);
  write(out, "osm.graph.n_nodes", s.osm_.graph_.n_nodes_);
  write(out, "osm.graph.n_edges", s.osm_.graph_.n_edges_);
  write(out, "osm.graph.n_edge_infos", s.osm_.graph_.n_edge_infos_);
  write(out, "osm.graph.n_areas", s.osm_.graph_.n_areas_);
  write(out, "osm.n_access_not_allowed_nodes",
        s.osm_.n_access_not_allowed_nodes_);
  write(out, "osm.n_crossing_nodes", s.osm_.n_crossing_nodes_);
  write(out, "osm.n_elevators", s.osm_.n_elevators_);
  write(out, "osm.n_linked_edges", s.osm_.n_linked_edges_);
  write(out, "osm.n_double_linked_edges", s.osm_.n_double_linked_edges_);
  write(out, "osm.n_moved_crossing_nodes", s.osm_.n_moved_crossing_nodes_);
  write(out, "osm.n_edge_footways", s.osm_.n_edge_footways_);
  write(out, "osm.n_edge_streets", s.osm_.n_edge_streets_);
  write(out, "osm.n_edge_railways", s.osm_.n_edge_railways_);
  write(out, "osm.n_edge_crossings", s.osm_.n_edge_crossings_);
  write(out, "osm.n_edge_area_footways", s.osm_.n_edge_area_footways_);

  write(out, "elevation.n_queries", s.elevation_.n_queries_);
  write(out, "elevation.n_misses", s.elevation_.n_misses_);

  write(out, "int.d_parallel_streets", s.int_.d_parallel_streets_);
  write(out, "int.d_move_crossings", s.int_.d_move_crossings_);
  write(out, "int.d_handle_edges", s.int_.d_handle_edges_);
  write(out, "int.d_areas", s.int_.d_areas_);
  write(out, "int.d_total", s.int_.d_total_);
  write(out, "int.graph.n_nodes", s.int_.graph_.n_nodes_);
  write(out, "int.graph.n_edges", s.int_.graph_.n_edges_);
  write(out, "int.graph.n_edge_infos", s.int_.graph_.n_edge_infos_);
  write(out, "int.graph.n_areas", s.int_.graph_.n_areas_);
  write(out, "int.n_compressed_edges", s.int_.n_compressed_edges_);
  write(out, "int.n_edge_footways", s.int_.n_edge_footways_);
  write(out, "int.n_edge_streets", s.int_.n_edge_streets_);
  write(out, "int.n_edge_railways", s.int_.n_edge_railways_);
  write(out, "int.n_edge_crossings", s.int_.n_edge_crossings_);

  write(out, "routing.d_junctions", s.routing_.d_junctions_);
  write(out, "routing.d_linked_crossings", s.routing_.d_linked_crossings_);
  write(out, "routing.d_edges", s.routing_.d_edges_);
  write(out, "routing.d_areas", s.routing_.d_areas_);
  write(out, "routing.d_crossing_detours", s.routing_.d_crossing_detours_);
  write(out, "routing.d_total", s.routing_.d_total_);
  write(out, "routing.graph.n_nodes", s.routing_.graph_.n_nodes_);
  write(out, "routing.graph.n_edges", s.routing_.graph_.n_edges_);
  write(out, "routing.graph.n_edge_infos", s.routing_.graph_.n_edge_infos_);
  write(out, "routing.graph.n_areas", s.routing_.graph_.n_areas_);
  write(out, "routing.n_crossings_created", s.routing_.n_crossings_created_);
  write(out, "routing.n_linked_crossings_created",
        s.routing_.n_linked_crossings_created_);
  write(out, "routing.n_path_join_failed_diff",
        s.routing_.n_path_join_failed_diff_);
  write(out, "routing.n_path_join_failed_same",
        s.routing_.n_path_join_failed_same_);
  write(out, "routing.n_edge_footways", s.routing_.n_edge_footways_);
  write(out, "routing.n_edge_streets", s.routing_.n_edge_streets_);
  write(out, "routing.n_edge_crossings", s.routing_.n_edge_crossings_);
  write(out, "routing.n_edge_elevators", s.routing_.n_edge_elevators_);
  write(out, "routing.n_edge_connections", s.routing_.n_edge_connections_);
  write(out, "routing.n_crossings_generated",
        s.routing_.n_crossings_generated_);
  write(out, "routing.n_crossings_unmarked", s.routing_.n_crossings_unmarked_);
  write(out, "routing.n_crossings_marked", s.routing_.n_crossings_marked_);
  write(out, "routing.n_crossings_island", s.routing_.n_crossings_island_);
  write(out, "routing.n_crossings_signals", s.routing_.n_crossings_signals_);
}

}  // namespace ppr::preprocessing
