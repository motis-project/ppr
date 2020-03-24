#include <iostream>

#include "rapidjson/document.h"

#include "ppr/profiles/json.h"
#include "ppr/profiles/parse_search_profile.h"

using namespace rapidjson;
using namespace ppr::routing;

namespace ppr::profiles {

inline void get_usage_restriction(routing::usage_restriction& field,
                                  rapidjson::Value const& doc,
                                  char const* key) {
  if (doc.HasMember(key)) {
    auto const& val = doc[key];
    if (val.IsString()) {
      auto const v = std::string{val.GetString()};
      if (v == "allowed" || v == "true") {
        field = routing::usage_restriction::ALLOWED;
      } else if (v == "penalized") {
        field = routing::usage_restriction::PENALIZED;
      } else if (v == "forbidden" || v == "false") {
        field = routing::usage_restriction::FORBIDDEN;
      }
    } else if (val.IsBool()) {
      field = val.GetBool() ? routing::usage_restriction::ALLOWED
                            : routing::usage_restriction::FORBIDDEN;
    }
  }
}

template <typename Val>
inline void get_cost_coefficients(cost_coefficients& c, Val const& doc,
                                  char const* key) {
  if (doc.HasMember(key)) {
    auto const& val = doc[key];
    if (val.IsArray() && !val.Empty()) {
      c.c0_ = val[0U].GetDouble();
      if (val.Size() > 1) {
        c.c1_ = val[1U].GetDouble();
        if (val.Size() > 2) {
          c.c2_ = val[2U].GetDouble();
        }
      }
    }
  }
}

template <typename Val>
inline void get_cost_factor(cost_factor& cf, Val const& doc, char const* key) {
  if (doc.HasMember(key)) {
    auto const& val = doc[key];
    if (val.IsObject()) {
      get_cost_coefficients(cf.duration_, val, "duration");
      get_cost_coefficients(cf.accessibility_, val, "accessibility");
      get_usage_restriction(cf.allowed_, val, "allowed");
      get_double(cf.duration_penalty_, val, "duration_penalty");
      get_double(cf.accessibility_penalty_, val, "accessibility_penalty");
    }
  }
}

template <typename Val>
inline void get_crossing_cost_factor(crossing_cost_factor& ccf, Val const& doc,
                                     char const* key) {
  if (doc.HasMember(key)) {
    auto const& val = doc[key];
    if (val.IsObject()) {
      get_cost_factor(ccf.signals_, val, "signals");
      get_cost_factor(ccf.marked_, val, "marked");
      get_cost_factor(ccf.island_, val, "island");
      get_cost_factor(ccf.unmarked_, val, "unmarked");
    }
  }
}

search_profile parse_search_profile(std::string const& s) {
  search_profile profile{};
  if (s.empty()) {
    return profile;
  }

  Document doc;
  doc.Parse<kParseDefaultFlags>(s.c_str());

  if (doc.HasParseError()) {
    std::cerr << "parse_search_profile: json parse error" << std::endl;
    return profile;
  }

  parse_search_profile(profile, doc);
  return profile;
}

void parse_search_profile(search_profile& profile,
                          rapidjson::Value const& root) {
  get_double(profile.walking_speed_, root, "walking_speed");
  get_double(profile.duration_limit_, root, "duration_limit");
  get_int(profile.max_crossing_detour_primary_, root,
          "max_crossing_detour_primary");
  get_int(profile.max_crossing_detour_secondary_, root,
          "max_crossing_detour_secondary");
  get_int(profile.max_crossing_detour_tertiary_, root,
          "max_crossing_detour_tertiary");
  get_int(profile.max_crossing_detour_residential_, root,
          "max_crossing_detour_residential");
  get_int(profile.max_crossing_detour_service_, root,
          "max_crossing_detour_service");

  get_double(profile.round_distance_, root, "round_distance");
  get_double(profile.round_duration_, root, "round_duration");
  get_double(profile.round_accessibility_, root, "round_accessibility");

  get_int(profile.max_routes_, root, "max_routes");
  get_int(profile.divisions_duration_, root, "divisions_duration");
  get_int(profile.divisions_accessibility_, root, "divisions_accessibility");

  get_crossing_cost_factor(profile.crossing_primary_, root, "crossing_primary");
  get_crossing_cost_factor(profile.crossing_secondary_, root,
                           "crossing_secondary");
  get_crossing_cost_factor(profile.crossing_tertiary_, root,
                           "crossing_tertiary");
  get_crossing_cost_factor(profile.crossing_residential_, root,
                           "crossing_residential");
  get_crossing_cost_factor(profile.crossing_service_, root, "crossing_service");

  get_cost_factor(profile.crossing_rail_, root, "crossing_rail");
  get_cost_factor(profile.crossing_tram_, root, "crossing_tram");

  get_cost_factor(profile.stairs_up_cost_, root, "stairs_up_cost");
  get_cost_factor(profile.stairs_down_cost_, root, "stairs_down_cost");
  get_cost_factor(profile.stairs_with_handrail_up_cost_, root,
                  "stairs_with_handrail_up_cost");
  get_cost_factor(profile.stairs_with_handrail_down_cost_, root,
                  "stairs_with_handrail_down_cost");
  get_cost_factor(profile.elevator_cost_, root, "elevator_cost");
  get_cost_factor(profile.escalator_cost_, root, "escalator_cost");
  get_cost_factor(profile.moving_walkway_cost_, root, "moving_walkway_cost");

  get_cost_factor(profile.elevation_up_cost_, root, "elevation_up_cost");
  get_cost_factor(profile.elevation_down_cost_, root, "elevation_down_cost");
}

}  // namespace ppr::profiles
