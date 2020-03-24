#include "rapidjson/document.h"

#include "ppr/common/location.h"

#include "ppr/profiles/parse_search_profile.h"

namespace ppr::backend {

inline void get_waypoints(std::vector<location>& waypoints,
                          rapidjson::Value const& doc, char const* key) {
  if (doc.HasMember(key)) {
    auto const& val = doc[key];
    if (val.IsArray()) {
      for (auto i = 0U; i < val.Size() - 1; i += 2) {
        waypoints.emplace_back(
            make_location(val[i].GetDouble(), val[i + 1].GetDouble()));
      }
    }
  }
}

inline void get_profile(ppr::routing::search_profile& profile,
                        rapidjson::Value const& doc, char const* key) {
  if (doc.HasMember(key)) {
    profiles::parse_search_profile(profile, doc[key]);
  }
}

}  // namespace ppr::backend
