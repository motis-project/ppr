#pragma once

#include "utl/get_or_create.h"

#include "ppr/common/data.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ppr::preprocessing {

using names_vector_t = data::vector<data::unique_ptr<data::string>>;
using names_map_t = std::unordered_map<std::string, data::string*>;

inline data::string* get_name(std::string const& name, names_vector_t& names,
                              names_map_t& names_map) {
  if (!name.empty()) {
    return utl::get_or_create(names_map, name, [&]() {
      return names
          .emplace_back(data::make_unique<data::string>(name.c_str(),
                                                        data::string::owning))
          .get();
    });
  } else {
    return nullptr;
  }
}

inline data::string* get_name(char const* name, names_vector_t& names,
                              names_map_t& names_map) {
  if (name != nullptr) {
    return get_name(std::string(name), names, names_map);
  } else {
    return nullptr;
  }
}

}  // namespace ppr::preprocessing
