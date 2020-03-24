#pragma once

#include <algorithm>

#include "ppr/common/edge.h"
#include "ppr/common/geometry/merc.h"

namespace ppr::preprocessing {

struct osm_node;

struct osm_edge {
  osm_edge(edge_info* info, osm_node* from, osm_node* to, double distance)
      : info_(info),
        from_(from),
        to_(to),
        distance_(distance),
        width_(0),
        elevation_up_(0),
        elevation_down_(0),
        sidewalk_left_(false),
        sidewalk_right_(false),
        layer_(0),
        processed_(false),
        linked_left_(nullptr),
        linked_right_(nullptr) {}

  bool generate_sidewalks() const { return info_->type_ == edge_type::STREET; }

  bool calculate_elevation() const {
    return info_->type_ != edge_type::ELEVATOR &&
           info_->street_type_ != street_type::ESCALATOR &&
           info_->street_type_ != street_type::MOVING_WALKWAY;
  }

  osm_node* osm_from(bool reverse) const { return reverse ? to_ : from_; }
  osm_node* osm_to(bool reverse) const { return reverse ? from_ : to_; }

  osm_edge* linked(side_type side, bool reverse) const {
    if (reverse) {
      return side == side_type::LEFT ? linked_right_ : linked_left_;
    } else {
      return side == side_type::LEFT ? linked_left_ : linked_right_;
    }
  }

  bool sidewalk(side_type side, bool reverse) const {
    if (reverse) {
      return side == side_type::LEFT ? sidewalk_right_ : sidewalk_left_;
    } else {
      return side == side_type::LEFT ? sidewalk_left_ : sidewalk_right_;
    }
  }

  bool is_linked() const {
    return linked_left_ != nullptr || linked_right_ != nullptr;
  }

  double angle(bool reverse = false) const;

  double normalized_angle(bool reverse = false) const {
    auto ang = angle(reverse);
    if (ang < 0) {
      ang += 2 * PI;
    }
    return ang;
  }

  edge_info* info_;
  osm_node* from_;
  osm_node* to_;
  double distance_;
  double width_;
  elevation_diff_t elevation_up_;
  elevation_diff_t elevation_down_;
  bool sidewalk_left_;
  bool sidewalk_right_;
  int8_t layer_;

  bool processed_;
  osm_edge* linked_left_;
  osm_edge* linked_right_;
};

}  // namespace ppr::preprocessing
