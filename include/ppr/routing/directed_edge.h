#pragma once

#include <optional>

#include "ppr/common/enums.h"
#include "ppr/common/routing_graph.h"
#include "ppr/routing/costs.h"

namespace ppr::routing {

struct directed_edge {
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

  bool incline_up() const {
    return fwd_ == static_cast<bool>(edge_info_->incline_up_);
  }

  std::optional<std::int8_t> incline() const {
    if (edge_info_->incline_ == UNKNOWN_INCLINE) {
      return {};
    } else {
      return fwd_ ? edge_info_->incline_ : -edge_info_->incline_;
    }
  }

  side_type side() const {
    if (fwd_) {
      return edge_->side_;
    } else {
      switch (edge_->side_) {
        case side_type::LEFT: return side_type::RIGHT;
        case side_type::RIGHT: return side_type::LEFT;
        default: return edge_->side_;
      }
    }
  }

  std::uint16_t level() const { return edge_info_->level_; }

  bool in_area() const { return edge_info_->area_; }

  edge const* edge_{};
  edge_info const* edge_info_{};
  edge_costs costs_;
  bool fwd_{true};
};

}  // namespace ppr::routing
