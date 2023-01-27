#pragma once

namespace ppr::routing {

struct routing_options {
  inline unsigned max_pt_query(bool const expanded) const {
    return expanded ? expanded_max_pt_query_ : initial_max_pt_query_;
  }

  inline unsigned max_pt_count(bool const expanded) const {
    return expanded ? expanded_max_pt_count_ : initial_max_pt_count_;
  }

  bool allow_expansion_{true};
  bool allow_osm_id_expansion_{true};

  bool force_level_match_{false};
  double level_dist_penalty_{50 * 50};

  unsigned initial_max_pt_query_{10};
  unsigned initial_max_pt_count_{1};

  unsigned expanded_max_pt_query_{40};
  unsigned expanded_max_pt_count_{20};
};

}  // namespace ppr::routing
