#pragma once

#include <string>

#include "rapidjson/document.h"

#include "ppr/routing/search_profile.h"

namespace ppr::profiles {

ppr::routing::search_profile parse_search_profile(std::string const& s);

void parse_search_profile(ppr::routing::search_profile& profile,
                          rapidjson::Value const& root);

}  // namespace ppr::profiles
