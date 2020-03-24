#pragma once

#include <algorithm>
#include <type_traits>

#include "ppr/common/routing_graph.h"
#include "ppr/preprocessing/int_graph/int_node.h"
#include "ppr/preprocessing/osm_graph/osm_area.h"
#include "ppr/preprocessing/osm_graph/osm_node.h"

namespace ppr::preprocessing {

struct int_area_point {
  explicit int_area_point(osm_node const* on)
      : location_(on->location_), ig_node_(on->int_node_) {}

  node* rg_node() const {
    return ig_node_ != nullptr ? ig_node_->rg_foot_node_ : nullptr;
  }

  merc location_;
  int_node* ig_node_;
};

struct int_area {
  explicit int_area(osm_area const& oa)
      : id_(oa.id_),
        name_(oa.name_),
        osm_id_(oa.osm_id_),
        from_way_(oa.from_way_),
        dist_matrix_(oa.dist_matrix_),
        next_matrix_(oa.next_matrix_),
        exit_nodes_(oa.exit_nodes_),
        adjacent_areas_(begin(oa.adjacent_areas_), end(oa.adjacent_areas_)) {
    auto const transform = [&](std::vector<osm_node*> const& from,
                               std::vector<int_area_point>& to) {
      to.reserve(from.size());
      std::transform(begin(from), end(from), std::back_inserter(to),
                     [&](osm_node const* on) { return int_area_point(on); });
    };

    transform(oa.outer_, outer_);
    inner_.reserve(oa.inner_.size());
    std::transform(begin(oa.inner_), end(oa.inner_), std::back_inserter(inner_),
                   [&](std::vector<osm_node*> const& ring) {
                     std::vector<int_area_point> v;
                     transform(ring, v);
                     return v;
                   });
  }

  area to_area() const {
    area a;
    auto const convert_node = [](int_area_point const& p) {
      return area::point{to_location(p.location_), p.rg_node()};
    };
    auto const transform = [&](std::vector<int_area_point> const& from,
                               auto& to) {
      to.reserve(static_cast<typename std::decay_t<decltype(to)>::size_type>(
          from.size()));
      std::transform(begin(from), end(from), std::back_inserter(to),
                     convert_node);
    };

    transform(outer_, a.polygon_.outer_);
    a.polygon_.inner_.reserve(
        static_cast<
            typename std::decay_t<decltype(a.polygon_.inner_)>::size_type>(
            inner_.size()));
    std::transform(begin(inner_), end(inner_),
                   std::back_inserter(a.polygon_.inner_),
                   [&](std::vector<int_area_point> const& r) {
                     ring<area::point> v;
                     transform(r, v);
                     return v;
                   });

    a.id_ = id_;
    a.name_ = name_;
    a.osm_id_ = osm_id_;
    a.from_way_ = from_way_;
    a.dist_matrix_ = dist_matrix_;
    a.next_matrix_ = next_matrix_;
    a.exit_nodes_ = exit_nodes_;
    a.adjacent_areas_ = adjacent_areas_;
    return a;
  }

  std::uint32_t id_;
  std::vector<int_area_point> outer_;
  std::vector<std::vector<int_area_point>> inner_;
  data::string* name_;
  std::int64_t osm_id_;
  bool from_way_;
  matrix<double, uint16_t> dist_matrix_;
  matrix<uint16_t, uint16_t> next_matrix_;
  data::vector<uint16_t> exit_nodes_;
  data::vector<std::uint32_t> adjacent_areas_;
};

}  // namespace ppr::preprocessing
