#pragma once

#include "ppr/preprocessing/int_graph/int_edge.h"
#include "ppr/preprocessing/junction.h"

namespace ppr::preprocessing {

using oriented_int_edge = junction_edge<int_edge>;

inline int_node* int_from(oriented_int_edge& oie) {
  return oie.edge_->int_from(oie.reverse_);
}

inline int_node* int_to(oriented_int_edge& oie) {
  return oie.edge_->int_to(oie.reverse_);
}

inline node* rg_from(oriented_int_edge const& oie, side_type side) {
  return oie.edge_->from(side, oie.reverse_);
}

inline node* rg_to(oriented_int_edge const& oie, side_type side) {
  return oie.edge_->to(side, oie.reverse_);
}

inline bool has_sidewalk(oriented_int_edge const& oie, side_type side) {
  return oie.edge_->sidewalk(side, oie.reverse_);
}

inline std::vector<merc>& sidewalk_path(oriented_int_edge& oie,
                                        side_type side) {
  if (oie.reverse_) {
    return oie.edge_->path(side == side_type::LEFT ? side_type::RIGHT
                                                   : side_type::LEFT);
  } else {
    return oie.edge_->path(side);
  }
}

inline double from_angle(oriented_int_edge const& oie) {
  return oie.edge_->from_angle(oie.reverse_);
}

inline double to_angle(oriented_int_edge const& oie) {
  return oie.edge_->to_angle(oie.reverse_);
}

inline bool is_linked(oriented_int_edge const& oie) {
  return oie.edge_->is_linked();
}

inline bool is_linked(oriented_int_edge const& oie, side_type side) {
  return oie.edge_->is_linked(side, oie.reverse_);
}

inline bool is_street(oriented_int_edge const& oie) {
  return oie.edge_->generate_sidewalks();
}

inline bool is_ignored(oriented_int_edge const& oie) {
  return oie.edge_->ignore_;
}

inline merc& first_path_pt(oriented_int_edge& oie, side_type side) {
  auto& path = sidewalk_path(oie, side);
  return oie.reverse_ ? path.back() : path.front();
}

inline node** get_edge_slot(oriented_int_edge& oie, side_type side) {
  if (oie.reverse_) {
    return (side == side_type::RIGHT || !is_street(oie))
               ? &oie.edge_->to_left_
               : &oie.edge_->to_right_;
  } else {
    return side == side_type::LEFT ? &oie.edge_->from_left_
                                   : &oie.edge_->from_right_;
  }
}

inline void set_node(oriented_int_edge& oie, side_type side, node* n) {
  *(get_edge_slot(oie, side)) = n;
}

}  // namespace ppr::preprocessing
