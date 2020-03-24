#pragma once

#include "ppr/common/routing_graph.h"

namespace ppr::routing {

struct input_pt {
  input_pt() = default;

  input_pt(location input, location nearest_pt, edge const* nearest_edge,
           data::vector<location>&& from_path, data::vector<location>&& to_path)
      : input_(input),
        nearest_pt_(nearest_pt),
        nearest_edge_(nearest_edge),
        in_area_(nullptr),
        outside_of_area_(false),
        from_path_(std::move(from_path)),
        to_path_(std::move(to_path)) {}

  explicit input_pt(location input, area const* in_area = nullptr)
      : input_(input),
        nearest_pt_(input),
        nearest_edge_(nullptr),
        in_area_(in_area),
        outside_of_area_(false) {}

  explicit operator bool() const {
    return nearest_edge_ != nullptr || in_area_ != nullptr;
  }

  location input_{};
  location nearest_pt_{};
  edge const* nearest_edge_{nullptr};
  area const* in_area_{nullptr};
  bool outside_of_area_{false};
  data::vector<location> from_path_;
  data::vector<location> to_path_;
};

std::vector<input_pt> nearest_points(routing_graph const& g,
                                     location const& loc, unsigned max_query,
                                     unsigned max_count, double max_dist);

}  // namespace ppr::routing
