#pragma once

#include <cstdint>

#include "ppr/common/location.h"
#include "ppr/common/location_geometry.h"

#include "ppr/common/data.h"

namespace ppr {

using node_id_t = std::uint64_t;

struct edge;

struct node {
  node_id_t id_;
  std::int64_t osm_id_;
  location location_;

  data::vector<data::unique_ptr<edge>> out_edges_;
  data::vector<data::ptr<edge>> in_edges_;
};

inline node make_node(node_id_t id, std::int64_t osm_id, location const& loc) {
  return node{id, osm_id, loc, data::vector<data::unique_ptr<edge>>(),
              data::vector<data::ptr<edge>>()};
}

}  // namespace ppr
