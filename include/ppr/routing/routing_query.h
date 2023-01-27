#pragma once

#include <vector>

#include "ppr/routing/input_location.h"
#include "ppr/routing/routing_options.h"
#include "ppr/routing/search_profile.h"

namespace ppr::routing {

enum class search_direction { FWD, BWD };

struct routing_query {
  input_location start_{};
  std::vector<input_location> destinations_;
  search_profile const& profile_;
  search_direction dir_{search_direction::FWD};
  routing_options opt_{};
};

}  // namespace ppr::routing
