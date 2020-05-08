#pragma once

#include <cmath>
#include <iostream>

#include "ppr/common/routing_graph.h"

namespace ppr {

inline bool verify_graph(routing_graph const& rg) {
  bool ok = true;
  for (auto const& n : rg.data_->nodes_) {
    if (!n->location_.valid()) {
      std::clog << "ppr routing graph invalid: invalid node coordinates"
                << std::endl;
      std::clog << "  osm node id=" << n->osm_id_
                << ", location=" << n->location_ << std::endl;
      ok = false;
    }
    for (auto const& e : n->out_edges_) {
      if (std::isnan(e->distance_)) {
        std::clog << "ppr routing graph invalid: nan edge found" << std::endl;
        std::clog << "  edge info: osm way id=" << e->info_->osm_way_id_
                  << ", type=" << static_cast<int>(e->info_->type_)
                  << ", area=" << e->info_->area_ << std::endl;
        std::clog << "  from: osm node id=" << e->from_->osm_id_
                  << ", location=" << e->from_->location_ << std::endl;
        std::clog << "  to: osm node id=" << e->to_->osm_id_
                  << ", location=" << e->to_->location_ << std::endl;
        std::clog << "  path: \n";
        for (auto const& l : e->path_) {
          std::clog << "    " << l << "\n";
        }
        ok = false;
      }
    }
  }
  return ok;
}

}  // namespace ppr
