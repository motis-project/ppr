#include <string_view>

#include "rapidjson/document.h"

#include "utl/verify.h"

#include "ppr/common/location.h"
#include "ppr/routing/input_location.h"

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

inline ppr::routing::osm_namespace parse_osm_namespace(
    rapidjson::Value const& val) {
  using ppr::routing::osm_namespace;
  auto const sv = std::string_view{val.GetString(), val.GetStringLength()};
  if (sv == "node") {
    return osm_namespace::NODE;
  } else if (sv == "way") {
    return osm_namespace::WAY;
  } else if (sv == "relation") {
    return osm_namespace::RELATION;
  } else {
    throw utl::fail("invalid osm type in request: {}", sv);
  }
}

inline void get_location(location& loc, rapidjson::Value const& doc,
                         char const* key) {
  if (doc.HasMember(key)) {
    auto const& val = doc[key];
    if (val.IsObject()) {
      auto const& lng = val.FindMember("lng");
      auto const& lat = val.FindMember("lat");
      if (lng != val.MemberEnd() && lat != val.MemberEnd() &&
          lng->value.IsNumber() && lat->value.IsNumber()) {
        loc = make_location(lng->value.GetDouble(), lat->value.GetDouble());
      }
    }
  }
}

inline void get_input_location(ppr::routing::input_location& loc,
                               rapidjson::Value const& doc, char const* key) {
  if (doc.HasMember(key)) {
    auto const& val = doc[key];
    if (val.IsObject()) {
      auto const& lng = val.FindMember("lng");
      auto const& lat = val.FindMember("lat");
      if (lng != val.MemberEnd() && lat != val.MemberEnd() &&
          lng->value.IsNumber() && lat->value.IsNumber()) {
        loc.location_ =
            make_location(lng->value.GetDouble(), lat->value.GetDouble());
      }

      auto const& osm_id = val.FindMember("osm_id");
      auto const& osm_type = val.FindMember("osm_type");
      if (osm_id != val.MemberEnd() && osm_type != val.MemberEnd() &&
          osm_id->value.IsNumber() && osm_type->value.IsString()) {
        loc.osm_element_ = ppr::routing::osm_element{
            osm_id->value.GetInt64(), parse_osm_namespace(osm_type->value)};
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
