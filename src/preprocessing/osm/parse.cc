#include <cmath>
#include <cstring>
#include <algorithm>
#include <limits>
#include <sstream>
#include <string_view>

#include "ppr/common/math.h"
#include "ppr/preprocessing/osm/parse.h"

using namespace std::literals;

namespace ppr::preprocessing::osm {

double parse_length(char const* str, double def) {
  // http://wiki.openstreetmap.org/wiki/Map_Features/Units
  if (str == nullptr || strlen(str) == 0) {
    return def;
  }

  std::istringstream istr(str);
  double val = def;
  istr >> val;

  if (!istr.eof() && !istr.fail()) {
    auto c = istr.get();
    if (c == ' ') {
      std::string unit;
      istr >> unit;
      if (unit == "km") {
        val *= 1000.0;
      } else if (unit == "mi") {
        val *= 1609.344;
      } else if (unit == "nmi") {
        val *= 1852.0;
      }
    } else if (c == '\'') {
      auto const feet = val;
      double inch = 0.0;
      istr >> inch;
      val = (feet * 12 + inch) * 0.0254;
    }
  }

  return val;
}

incline_info parse_incline(char const* str) {
  auto info = incline_info{};
  if (str == nullptr || strlen(str) == 0) {
    return info;
  }

  if (str == "up"sv) {
    info.up_ = true;
  } else if (str == "down"sv) {
    info.up_ = false;
  } else {
    std::istringstream istr(str);
    auto val = 0.0;
    istr >> val;
    if (!istr.eof() && !istr.fail()) {
      auto c = istr.get();
      if (c == 0xC2 && istr.get() == 0xB0) {  // '°' utf-8
        val = std::tan(to_rad(val)) * 100.0;
      }
    }
    auto const gradient = static_cast<std::int8_t>(std::clamp(
        static_cast<int>(val),
        static_cast<int>(std::numeric_limits<std::int8_t>::min()) + 1,
        static_cast<int>(std::numeric_limits<std::int8_t>::max())));
    info.gradient_ = gradient;
    if (gradient != 0) {
      info.up_ = gradient > 0;
    }
  }

  return info;
}

}  // namespace ppr::preprocessing::osm
