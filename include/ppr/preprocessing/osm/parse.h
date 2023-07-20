#pragma once

#include <charconv>
#include <cstdint>
#include <cstring>
#include <locale>
#include <optional>
#include <sstream>

namespace ppr::preprocessing::osm {

template <typename T>
inline T parse_number(char const* str, T const def = 0.0F,
                      bool const accept_partial_match = true) {
  if (str == nullptr) {
    return def;
  }
  auto const len = std::strlen(str);
  auto value = def;
  auto const result = std::from_chars(str, str + len, value);
  if (result.ec == std::errc{} &&
      (accept_partial_match || result.ptr == str + len)) {
    return value;
  } else {
    return def;
  }
}

inline int parse_int(char const* str, int const def = 0,
                     bool const accept_partial_match = true) {
  return parse_number(str, def, accept_partial_match);
}

inline float parse_float(char const* str, float const def = 0) {
  if (str == nullptr) {
    return def;
  }
  std::stringstream ss;
  ss.imbue(std::locale::classic());
  auto val = def;
  ss << str;
  ss >> val;
  return ss ? val : def;
}

double parse_length(char const* str, double def = 0.0);

struct incline_info {
  std::optional<bool> up_;
  std::optional<std::int8_t> gradient_;
};

incline_info parse_incline(char const* str);

}  // namespace ppr::preprocessing::osm
