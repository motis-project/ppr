#pragma once

#include <cmath>
#include <iostream>

#include "ppr/common/routing_graph.h"

namespace ppr {

inline bool verify_graph(routing_graph const& rg,
                         std::ostream& out = std::cout) {
  bool ok = true;
  for (auto const& n : rg.data_->nodes_) {
    if (!n->location_.valid()) {
      out << "ppr routing graph invalid: invalid node coordinates" << std::endl;
      out << "  osm node id=" << n->osm_id_ << ", location=" << n->location_
          << std::endl;
      ok = false;
    }
    for (auto const& e : n->out_edges_) {
      if (std::isnan(e->distance_)) {
        auto const info = e->info(rg);
        out << "ppr routing graph invalid: nan edge found" << std::endl;
        out << "  edge info: osm way id=" << info->osm_way_id_
            << ", type=" << static_cast<int>(info->type_)
            << ", area=" << info->area_ << std::endl;
        out << "  from: osm node id=" << e->from_->osm_id_
            << ", location=" << e->from_->location_ << std::endl;
        out << "  to: osm node id=" << e->to_->osm_id_
            << ", location=" << e->to_->location_ << std::endl;
        out << "  path: \n";
        for (auto const& l : e->path_) {
          out << "    " << l << "\n";
        }
        ok = false;
      }
    }
  }
  return ok;
}

}  // namespace ppr
