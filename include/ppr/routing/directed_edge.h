#pragma once

#include "ppr/common/routing_graph.h"
#include "ppr/routing/costs.h"

namespace ppr::routing {

struct directed_edge {
  directed_edge() = default;

  directed_edge(edge const* edge, edge_costs const& costs, bool fwd)
      : edge_(edge), costs_(costs), fwd_(fwd) {}

  directed_edge(edge const* edge, edge_costs&& costs, bool fwd)
      : edge_(edge), costs_(costs), fwd_(fwd) {}

  node const* from(routing_graph_data const& rg) const {
    return fwd_ ? edge_->from(rg) : edge_->to(rg);
  }

  node const* to(routing_graph_data const& rg) const {
    return fwd_ ? edge_->to(rg) : edge_->from(rg);
  }

  bool valid() const { return edge_ != nullptr; }
  double distance() const { return edge_ != nullptr ? edge_->distance_ : 0; }
  double duration() const { return costs_.duration_; }
  double accessibility() const { return costs_.accessibility_; }
  double duration_penalty() const { return costs_.duration_penalty_; }
  double accessibility_penalty() const { return costs_.accessibility_penalty_; }
  bool allowed() const { return costs_.allowed_; }

  elevation_diff_t elevation_up() const {
    return fwd_ ? edge_->elevation_up_ : edge_->elevation_down_;
  }

  elevation_diff_t elevation_down() const {
    return fwd_ ? edge_->elevation_down_ : edge_->elevation_up_;
  }

  bool incline_up(routing_graph_data const& rg) const {
    return fwd_ == edge_->info(rg)->incline_up_;  // NOLINT
  }

  edge const* edge_{nullptr};
  edge_costs costs_;
  bool fwd_{true};
};

}  // namespace ppr::routing
