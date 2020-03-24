#include "ppr/preprocessing/statistics.h"
#include "ppr/common/routing_graph.h"
#include "ppr/preprocessing/int_graph/int_graph.h"
#include "ppr/preprocessing/osm_graph/osm_graph.h"

namespace ppr::preprocessing {

namespace {

template <typename Graph>
void collect_general_stats(graph_statistics& stats, Graph const& g) {
  stats.n_nodes_ = g.nodes_.size();
  stats.n_edge_infos_ = g.edge_infos_.size();
  stats.n_areas_ = g.areas_.size();

  stats.n_edges_ = 0;
  for (auto const& n : g.nodes_) {
    stats.n_edges_ += n->out_edges_.size();
  }
}

template <>
void collect_general_stats(graph_statistics& stats, routing_graph const& g) {
  stats.n_nodes_ = g.data_->nodes_.size();
  stats.n_edge_infos_ = g.data_->edge_infos_.size();
  stats.n_areas_ = g.data_->areas_.size();

  stats.n_edges_ = 0;
  for (auto const& n : g.data_->nodes_) {
    stats.n_edges_ += n->out_edges_.size();
  }
}

}  // namespace

void collect_stats(osm_graph_statistics& stats, osm_graph const& og) {
  collect_general_stats(stats.graph_, og);
}

void collect_stats(int_graph_statistics& stats, int_graph const& ig) {
  collect_general_stats(stats.graph_, ig);
}

void collect_stats(routing_graph_statistics& stats, routing_graph const& rg) {
  collect_general_stats(stats.graph_, rg);

  for (auto const& n : rg.data_->nodes_) {
    for (auto const& e : n->out_edges_) {

      switch (e->info_->type_) {
        case edge_type::CONNECTION: stats.n_edge_connections_++; break;
        case edge_type::STREET: stats.n_edge_streets_++; break;
        case edge_type::FOOTWAY: stats.n_edge_footways_++; break;
        case edge_type::CROSSING:
          stats.n_edge_crossings_++;
          switch (e->info_->crossing_type_) {
            case crossing_type::NONE: break;
            case crossing_type::GENERATED:
              stats.n_crossings_generated_++;
              break;
            case crossing_type::UNMARKED: stats.n_crossings_unmarked_++; break;
            case crossing_type::MARKED: stats.n_crossings_marked_++; break;
            case crossing_type::ISLAND: stats.n_crossings_island_++; break;
            case crossing_type::SIGNALS: stats.n_crossings_signals_++; break;
          }
          break;
        case edge_type::ELEVATOR: stats.n_edge_elevators_++; break;
      }
    }
  }
}

}  // namespace ppr::preprocessing
