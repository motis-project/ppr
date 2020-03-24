#include <cstring>

#include "ppr/preprocessing/osm/access.h"

namespace ppr::preprocessing::osm {

bool access_allowed(char const* access, bool def) {
  if (access == nullptr || strlen(access) == 0) {
    return def;
  }
  if (def) {
    return strcmp(access, "no") != 0 && strcmp(access, "private") != 0;
  } else {
    return strcmp(access, "yes") == 0 || strcmp(access, "permissive") == 0 ||
           strcmp(access, "designated") == 0 ||
           strcmp(access, "official") == 0 ||
           strcmp(access, "destination") == 0 ||
           strcmp(access, "public") == 0 || strcmp(access, "delivery") == 0;
  }
}

bool access_allowed(osmium::TagList const& tags, bool def) {
  auto access = tags["foot"];
  if (access == nullptr) {
    access = tags["access"];
  }
  return access_allowed(access, def);
}

}  // namespace ppr::preprocessing::osm
