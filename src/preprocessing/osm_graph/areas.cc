#include <mutex>

#include "ppr/common/area_routing.h"
#include "ppr/preprocessing/osm_graph/areas.h"
#include "ppr/preprocessing/thread_pool.h"

namespace ppr::preprocessing {

void process_area(osm_graph& graph, osm_graph_statistics& stats, osm_area* area,
                  std::mutex& mutex) {
  edge_info* info;
  {
    std::lock_guard<std::mutex> graph_guard(mutex);
    info = graph.edge_infos_
               .emplace_back(data::make_unique<edge_info>(make_edge_info(
                   -area->osm_id_, edge_type::FOOTWAY, street_type::PEDESTRIAN,
                   crossing_type::NONE)))
               .get();
  }
  info->name_ = area->name_;
  info->area_ = true;

  auto vg = build_visibility_graph(area);  // NOLINT
  reduce_visibility_graph(vg);
  {
    std::lock_guard<std::mutex> graph_guard(mutex);
    make_vg_edges(vg, [&](auto const a_idx, auto const b_idx) {
      auto a = vg.nodes_[a_idx];
      auto b = vg.nodes_[b_idx];
      assert(a != b);
      if (!any_edge_between(a, b)) {
        a->out_edges_.emplace_back(info, a, b,
                                   distance(a->location_, b->location_));
        stats.n_edge_area_footways_++;
      }
    });
  }
  area->dist_matrix_ = vg.dist_matrix_;
  area->next_matrix_ = vg.next_matrix_;
  area->exit_nodes_ = vg.exit_nodes_;
}

void process_areas(osm_graph& graph, options const& opt,
                   osm_graph_statistics& stats) {
  thread_pool pool(opt.threads_);
  std::mutex mutex;
  for (auto const& a : graph.areas_) {
    pool.post([&]() { process_area(graph, stats, a.get(), mutex); });
  }
  pool.join();
}

}  // namespace ppr::preprocessing
