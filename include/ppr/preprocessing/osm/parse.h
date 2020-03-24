#pragma once

#include <string>

namespace ppr::preprocessing::osm {

inline int parse_int(char const* str, int def = 0) {
  if (str == nullptr) {
    return def;
  }
  try {
    return std::stoi(str);
  } catch (...) {
    return def;
  }
}

double parse_length(char const* str, double def = 0.0);

int parse_incline(char const* str);

}  // namespace ppr::preprocessing::osm
