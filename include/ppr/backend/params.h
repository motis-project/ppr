#pragma once

#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "ppr/common/location.h"

namespace ppr::backend {

inline std::vector<location> extract_waypoints(std::string const& url) {
  std::regex param_re("\\?(.*&)?waypoints=([^&]+)");
  std::regex pos_re(R"((\d+\.\d+)%2C(\d+\.\d+))", std::regex::icase);
  std::smatch match;
  if (std::regex_search(url, match, param_re)) {
    std::vector<location> waypoints;
    auto const waypoints_str = match[2].str();
    for (auto it = std::sregex_iterator(begin(waypoints_str),
                                        end(waypoints_str), pos_re);
         it != std::sregex_iterator(); ++it) {
      auto const wps = *it;
      waypoints.emplace_back(
          make_location(std::stod(wps[1].str()), std::stod(wps[2].str())));
    }
    return waypoints;
  } else {
    return {};
  }
}

}  // namespace ppr::backend
