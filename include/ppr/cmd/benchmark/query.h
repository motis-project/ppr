#pragma once

#include <utility>

#include "ppr/routing/search.h"
#include "ppr/routing/statistics.h"

namespace ppr::benchmark {

struct named_profile {
  named_profile() = default;
  named_profile(std::string name, ppr::routing::search_profile const& profile)
      : name_(std::move(name)), profile_(profile) {}

  std::string name_;
  ppr::routing::search_profile profile_;
};

struct routing_query {
  explicit routing_query(named_profile const& profile) : profile_(profile) {}

  location start_;
  std::vector<location> destinations_;
  named_profile const& profile_;
  ppr::routing::search_direction direction_ =
      ppr::routing::search_direction::FWD;
  bool allow_expansion_ = true;

  double max_dist_ = 0;
  double radius_factor_ = 1.0;

  routing_query* base_query_ = nullptr;
  ppr::routing::search_result* base_result_ = nullptr;

  ppr::routing::search_result execute(routing_graph const& rg) const {
    return ppr::routing::find_routes(rg, start_, destinations_,
                                     profile_.profile_, direction_,
                                     allow_expansion_);
  }

  routing_query with_profile(named_profile const& profile) const {
    routing_query q(profile);
    q.start_ = start_;
    q.destinations_ = destinations_;
    q.direction_ = direction_;
    q.allow_expansion_ = allow_expansion_;
    q.max_dist_ = max_dist_;
    q.radius_factor_ = radius_factor_;
    return q;
  }
};

}  // namespace ppr::benchmark
