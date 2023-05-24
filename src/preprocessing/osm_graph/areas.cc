#include <mutex>

#include "ppr/common/area_routing.h"
#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/osm_graph/areas.h"
#include "ppr/preprocessing/thread_pool.h"

namespace ppr::preprocessing {

void process_area(osm_graph& graph, osm_graph_statistics& stats, osm_area* area,
                  std::mutex& mutex) {
  auto info_idx = edge_info_idx_t{};
  {
    auto const graph_guard = std::lock_guard{mutex};
    auto [idx, info] =
        make_edge_info(graph.edge_infos_, -area->osm_id_, edge_type::FOOTWAY,
                       street_type::PEDESTRIAN, crossing_type::NONE);
    info_idx = idx;
    info->name_ = area->name_;
    info->area_ = true;
    info->level_ = area->level_;
  }

  auto vg = build_visibility_graph(area);  // NOLINT
  reduce_visibility_graph(vg);
  {
    auto const graph_guard = std::lock_guard{mutex};
    make_vg_edges(vg, [&](auto const a_idx, auto const b_idx) {
      auto a = vg.nodes_[a_idx];
      auto b = vg.nodes_[b_idx];
      assert(a != b);
      if (!any_edge_between(a, b)) {
        a->out_edges_.emplace_back(info_idx, a, b,
                                   distance(a->location_, b->location_));
        stats.n_edge_area_footways_++;
      }
    });
  }
  area->dist_matrix_ = vg.dist_matrix_;
  area->next_matrix_ = vg.next_matrix_;
  area->exit_nodes_ = vg.exit_nodes_;
}

void process_areas(osm_graph& graph, options const& opt, logging& log,
                   osm_graph_statistics& stats) {
  thread_pool pool(opt.threads_);
  std::mutex mutex;
  step_progress progress{log, pp_step::OSM_EXTRACT_AREAS, graph.areas_.size()};
  for (auto const& a : graph.areas_) {
    pool.post([&]() {
      // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.Assign)
      process_area(graph, stats, a.get(), mutex);
      progress.add();
    });
  }
  pool.join();
}

}  // namespace ppr::preprocessing
