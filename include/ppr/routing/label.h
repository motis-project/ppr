#pragma once

#include <functional>
#include <iostream>

#include "ppr/routing/directed_edge.h"
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

  bool create_label(label& l, directed_edge const& e,
                    search_profile const& profile) {
    if (e.to() == edge_.from() || e.to() == edge_.to()) {
      return false;
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

  node const* get_node() const { return edge_.to(); }

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
  auto l = &label;
  while (l != nullptr) {
    os << l->get_node()->id_;
    l = l->pred_;
    if (l != nullptr) {
      os << " <- ";
    }
  }
  return os;
}

}  // namespace ppr::routing
