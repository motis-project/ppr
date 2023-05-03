#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "ankerl/unordered_dense.h"

#include "utl/get_or_create.h"

#include "ppr/common/data.h"
#include "ppr/common/names.h"

namespace ppr::preprocessing {

using names_map_t = ankerl::unordered_dense::map<std::string, std::uint32_t>;

inline names_idx_t get_name(std::string const& name, names_vector_t& names,
                            names_map_t& names_map) {
  if (!name.empty()) {
    return utl::get_or_create(names_map, name, [&]() {
      auto const idx = names.size();
      names.emplace_back(name);
      return idx;
    });
  } else {
    return 0;
  }
}

inline names_idx_t get_name(char const* name, names_vector_t& names,
                            names_map_t& names_map) {
  if (name != nullptr) {
    return get_name(std::string{name}, names, names_map);
  } else {
    return 0;
  }
}

}  // namespace ppr::preprocessing
