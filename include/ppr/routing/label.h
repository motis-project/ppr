#pragma once

#include <functional>
#include <iostream>

#include "ppr/common/routing_graph.h"
#include "ppr/routing/directed_edge.h"
#include "ppr/routing/last_crossing_info.h"
#include "ppr/routing/search_profile.h"

namespace ppr::routing {

struct label {
  label() = default;

  label(directed_edge& e, label* pred)
      : pred_(pred),
        edge_(e),
        dominated_(false),
        distance_(e.distance()),
        duration_(e.duration() + e.duration_penalty()),
        accessibility_(e.accessibility() + e.accessibility_penalty()),
        real_duration_(e.duration()),
        real_accessibility_(e.accessibility()) {}

  bool create_label(routing_graph_data const& rg, label& l,
                    directed_edge const& e, search_profile const& profile) {
    if (e.to(rg) == edge_.from(rg) || e.to(rg) == edge_.to(rg)) {
      return false;
    }

    if (edge_.level() != e.level()) {
      // don't allow crossing between areas with different levels
      if (edge_.in_area() && e.in_area()) {
        return false;
      }

      auto const et1 = edge_.edge_info_->type_;
      auto const et2 = e.edge_info_->type_;
      if ((et1 == edge_type::STREET && et2 == edge_type::FOOTWAY) ||
          (et1 == edge_type::FOOTWAY && et2 == edge_type::STREET)) {
        return false;
      }
    }

    l.pred_ = this;
    l.edge_ = e;
    l.dominated_ = dominated_;
    l.distance_ = distance_ + e.distance();
    l.duration_ = duration_ + e.duration() + e.duration_penalty();
    l.accessibility_ =
        accessibility_ + e.accessibility() + e.accessibility_penalty();
    l.real_duration_ = real_duration_ + e.duration();
    l.real_accessibility_ = real_accessibility_ + e.accessibility();

    return !l.is_filtered(profile);
  }

  bool is_filtered(search_profile const& profile) const {
    return real_duration_ > profile.duration_limit_;
  }

  node const* get_node(routing_graph_data const& rg) const {
    return edge_.to(rg);
  }

  bool dominates(label const& o) const {
    return duration_ <= o.duration_ && accessibility_ <= o.accessibility_;
  }

  bool operator<(label const& o) const {
    return duration_ < o.duration_ ||
           (std::equal_to<>()(duration_, o.duration_) &&
            accessibility_ < o.accessibility_);
  }

  bool operator>(label const& o) const {
    return duration_ > o.duration_ ||
           (std::equal_to<>()(duration_, o.duration_) &&
            accessibility_ > o.accessibility_);
  }

  label* pred_{nullptr};
  directed_edge edge_;
  bool dominated_{false};

  double distance_{0};
  double duration_{0};  // including penalties
  double accessibility_{0};  // including penalties

  double real_duration_{0};
  double real_accessibility_{0};
};

inline std::ostream& operator<<(std::ostream& os, label const& label) {
  os << "label: [dist = " << label.distance_ << ", dur = " << label.duration_
     << "[real=" << label.real_duration_ << "]"
     << ", acc=" << label.accessibility_
     << "[real=" << label.real_accessibility_ << "]] ";
  /*
  auto const* l = &label;
  while (l != nullptr) {
    os << l->get_node()->id_;
    l = l->pred_;
    if (l != nullptr) {
      os << " <- ";
    }
  }
   */
  return os;
}

}  // namespace ppr::routing
