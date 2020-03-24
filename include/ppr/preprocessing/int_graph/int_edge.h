#pragma once

#include <algorithm>

#include "ppr/common/elevation.h"
#include "ppr/common/geometry/merc.h"
#include "ppr/common/routing_graph.h"

namespace ppr::preprocessing {

struct int_node;

struct int_edge {
  int_edge(edge_info* info, int_node* from, int_node* to, double distance,
           std::vector<merc>&& path_left, std::vector<merc>&& path_right,
           double from_angle, double to_angle)
      : info_(info),
        from_(from),
        to_(to),
        distance_(distance),
        sidewalk_left_(false),
        sidewalk_right_(false),
        linked_left_(false),
        linked_right_(false),
        ignore_(false),
        layer_(0),
        from_angle_(from_angle),
        to_angle_(to_angle),
        elevation_up_(0),
        elevation_down_(0),
        from_left_(nullptr),
        from_right_(nullptr),
        to_left_(nullptr),
        to_right_(nullptr),
        path_left_(std::move(path_left)),
        path_right_(std::move(path_right)) {
    assert(!(path_left_.empty() && path_right_.empty()));
  }

  bool generate_sidewalks() const { return info_->type_ == edge_type::STREET; }

  node* from(side_type side, bool reverse) const {
    if (reverse) {
      return side == side_type::LEFT && generate_sidewalks() ? to_right_
                                                             : to_left_;
    } else {
      return side == side_type::RIGHT && generate_sidewalks() ? from_right_
                                                              : from_left_;
    }
  }

  node* to(side_type side, bool reverse) const {
    if (reverse) {
      return side == side_type::LEFT && generate_sidewalks() ? from_right_
                                                             : from_left_;
    } else {
      return side == side_type::RIGHT && generate_sidewalks() ? to_right_
                                                              : to_left_;
    }
  }

  int_node* int_from(bool reverse) const { return reverse ? to_ : from_; }
  int_node* int_to(bool reverse) const { return reverse ? from_ : to_; }

  bool sidewalk(side_type side, bool reverse) const {
    if (reverse) {
      return side == side_type::LEFT ? sidewalk_right_ : sidewalk_left_;
    } else {
      return side == side_type::LEFT ? sidewalk_left_ : sidewalk_right_;
    }
  }

  std::vector<merc>& path(side_type side) {
    return side == side_type::LEFT ? path_left_ : path_right_;
  }

  double from_angle(bool reverse) const {
    return reverse ? to_angle_ : from_angle_;
  }

  double to_angle(bool reverse) const {
    return reverse ? from_angle_ : to_angle_;
  }

  bool is_linked() const { return linked_left_ || linked_right_; }

  bool is_linked(side_type side, bool reverse) const {
    if (reverse) {
      return side == side_type::LEFT ? linked_right_ : linked_left_;
    } else {
      return side == side_type::LEFT ? linked_left_ : linked_right_;
    }
  }

  edge_info* info_;
  int_node* from_;
  int_node* to_;
  double distance_;
  bool sidewalk_left_ : 1;
  bool sidewalk_right_ : 1;
  bool linked_left_ : 1;
  bool linked_right_ : 1;
  bool ignore_ : 1;
  int8_t layer_;
  double from_angle_;
  double to_angle_;
  elevation_diff_t elevation_up_;
  elevation_diff_t elevation_down_;

  node* from_left_;
  node* from_right_;
  node* to_left_;
  node* to_right_;
  std::vector<merc> path_left_;
  std::vector<merc> path_right_;
};

}  // namespace ppr::preprocessing
