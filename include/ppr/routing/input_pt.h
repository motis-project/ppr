#pragma once

#include "ppr/common/routing_graph.h"
#include "ppr/routing/input_location.h"
#include "ppr/routing/routing_options.h"

namespace ppr::routing {

struct input_pt {
  input_pt() = default;

  input_pt(routing_graph_data const& rg, location input, location nearest_pt,
           edge const* nearest_edge, data::vector<location>&& from_path,
           data::vector<location>&& to_path)
      : input_(input),
        nearest_pt_(nearest_pt),
        nearest_edge_(nearest_edge),
        in_area_(nullptr),
        outside_of_area_(false),
        from_path_(std::move(from_path)),
        to_path_(std::move(to_path)),
        level_(nearest_edge->info(rg)->level_) {}

  explicit input_pt(location input, area const* in_area = nullptr)
      : input_(input),
        nearest_pt_(input),
        nearest_edge_(nullptr),
        in_area_(in_area),
        outside_of_area_(false),
        level_(in_area != nullptr ? in_area->level_ : 0) {}

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
  std::int16_t level_{};
};

std::vector<input_pt> resolve_input_location(routing_graph const& g,
                                             input_location const& il,
                                             routing_options const& opt,
                                             bool const expanded);

}  // namespace ppr::routing
