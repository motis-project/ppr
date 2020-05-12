#include <queue>
#include <unordered_map>

#include "ppr/common/routing_graph.h"
#include "ppr/preprocessing/routing_graph/crossing_detour.h"
#include "ppr/preprocessing/thread_pool.h"

namespace ppr::preprocessing {

namespace {

double distance_with_marked_crossings(edge const& crossing_edge,
                                      double distance_limit) {
  using dist_t = std::pair<double, int>;
  using queue_entry = std::pair<dist_t, node const*>;
  std::unordered_map<node const*, dist_t> dists;
  std::priority_queue<queue_entry, std::vector<queue_entry>, std::greater<>> pq;
  auto const start_node = crossing_edge.from_;
  auto const end_node = crossing_edge.to_;
  auto const ref_street_type = crossing_edge.info_->street_type_;

  dists[start_node] = {0.0, 0};
  pq.push({{0.0, 0}, start_node});

  auto const expand_edge = [&](edge const* e, dist_t dist, bool fwd) {
    auto dest = fwd ? e->to_ : e->from_;
    auto const total_dist = dist.first + e->distance_;
    if (total_dist > distance_limit ||
        (e->info_->is_unmarked_crossing() &&
         (static_cast<uint8_t>(e->info_->street_type_) >=
          static_cast<uint8_t>(ref_street_type)))) {
      return;
    }
    auto current_dist = std::numeric_limits<double>::max();
    auto f = dists.find(dest);
    if (f != end(dists)) {
      current_dist = f->second.first;
    }
    if (total_dist < current_dist) {
      dist_t new_dist = {
          total_dist, dist.second + (e->info_->is_marked_crossing() ? 1 : 0)};
      dists[dest] = new_dist;
      pq.push({new_dist, dest});
    }
  };

  while (!pq.empty()) {
    auto const& e = pq.top();
    auto const node = e.second;
    auto const dist = dists[node];
    pq.pop();

    if (node == end_node) {
      return dist.second > 0 ? dist.first : 0;
    }

    for (auto const& edge : node->out_edges_) {
      expand_edge(edge.get(), dist, true);
    }

    for (auto const& edge : node->in_edges_) {
      expand_edge(edge, dist, false);
    }
  }

  return 0;
}

void calc_crossing_detour(edge& e, double distance_limit) {
  auto dist = distance_with_marked_crossings(e, distance_limit);
  auto const int_dist = static_cast<int32_t>(std::ceil(dist));
  const_cast<edge_info*>(static_cast<edge_info const*>(e.info_))  // NOLINT
      ->marked_crossing_detour_ = int_dist;
}

}  // namespace

void calc_crossing_detours(routing_graph& graph, options const& opt,
                           logging& log) {
  step_progress progress{log, pp_step::RG_CROSSING_DETOURS,
                         graph.data_->nodes_.size()};
  thread_pool pool(opt.threads_);

  for (auto& n : graph.data_->nodes_) {
    for (auto& e : n->out_edges_) {
      if (e->info_->is_unmarked_crossing()) {
        pool.post(
            [&]() { calc_crossing_detour(*e, opt.crossing_detours_limit_); });
      }
    }
    progress.add();
  }

  pool.join();
}

}  // namespace ppr::preprocessing
