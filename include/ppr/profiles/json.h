#pragma once

#include "rapidjson/document.h"

namespace ppr::profiles {

inline void get_double(double& field, rapidjson::Value const& doc,
                       char const* key) {
  if (doc.HasMember(key)) {
    auto const& val = doc[key];
    if (val.IsNumber()) {
      field = val.GetDouble();
    }
  }
}

template <typename T>
inline void get_int(T& field, rapidjson::Value const& doc, char const* key) {
  if (doc.HasMember(key)) {
    auto const& val = doc[key];
    if (val.IsInt()) {
      field = static_cast<T>(val.GetInt());
    }
  }
}

inline void get_bool(bool& field, rapidjson::Value const& doc,
                     char const* key) {
  if (doc.HasMember(key)) {
    auto const& val = doc[key];
    if (val.IsBool()) {
      field = val.GetBool();
    }
  }
}

}  // namespace ppr::profiles
