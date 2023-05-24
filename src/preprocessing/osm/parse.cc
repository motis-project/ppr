#include <cstring>
#include <sstream>

#include "ppr/preprocessing/osm/parse.h"

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

int parse_incline(char const* str) {
  if (str == nullptr || strlen(str) == 0) {
    return 0;
  }

  if (strcmp(str, "up") == 0 || (str[0] >= '1' && str[0] <= '9')) {
    return 1;
  } else if (strcmp(str, "down") == 0 || str[0] == '-') {
    return -1;
  } else {
    return 0;
  }
}

}  // namespace ppr::preprocessing::osm
