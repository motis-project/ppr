#include <algorithm>
#include <limits>
#include <sstream>

#include "ppr/preprocessing/osm/layer.h"
#include "ppr/preprocessing/osm/parse.h"

namespace ppr::preprocessing::osm {

int8_t get_layer(osmium::TagList const& tags) {
  auto tag = tags["layer"];
  if (tag == nullptr) {
    return 0;
  } else {
    auto const value = parse_int(tag, 100);
    return static_cast<int8_t>(std::min(
        std::max(value, static_cast<int>(std::numeric_limits<int8_t>::min())),
        static_cast<int>(std::numeric_limits<int8_t>::max())));
  }
}

}  // namespace ppr::preprocessing::osm
