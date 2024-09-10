#include <algorithm>

#include "boost/geometry/algorithms/covered_by.hpp"

#include "ppr/routing/input_areas.h"

#include "ppr/common/area_routing.h"

namespace bg = boost::geometry;

namespace ppr::routing {

node* create_edge_to_area(input_pt const& pt, node* input_node,
                          additional_edges& additional) {
  auto* area_node = additional.create_node(pt.nearest_pt_);
  additional.connect(input_node, area_node);
  return area_node;
}

node* create_area_node(node* input_node, input_pt const& pt,
                       additional_edges& additional) {
  assert(pt.in_area_ != nullptr);

  node* area_node = input_node;
  if (pt.outside_of_area_) {
    area_node = create_edge_to_area(pt, input_node, additional);
  }

  additional.area_nodes_[pt.in_area_].emplace_back(area_node);

  return input_node;
}

void create_area_edges(area const* ar, std::vector<node*>& nodes,
                       additional_edges& additional) {
  std::vector<area::point> additional_points;
  additional_points.reserve(nodes.size());
  std::transform(begin(nodes), end(nodes),
                 std::back_inserter(additional_points),
                 [](node* n) { return area::point{n->location_, n}; });

  auto vg = extend_visibility_graph(ar, additional_points);  // NOLINT

  auto const ensure_node = [&](area::point& p) {
    if (p.node_ == nullptr) {
      p.node_ = additional.create_node(p.location_);
    }
  };

  auto const additional_edge_between = [&](node* a, node* b) {
    auto it = additional.edge_map_.find(a);
    if (it != end(additional.edge_map_)) {
      return std::any_of(begin(it->second), end(it->second),
                         [&](auto const& e) {
                           return (e->from_ == a && e->to_ == b) ||
                                  (e->from_ == b && e->to_ == a);
                         });
    }
    return false;
  };

  reduce_extended_visibility_graph(vg);
  make_vg_edges(vg, [&](auto const a_idx, auto const b_idx) {
    auto& a = vg.nodes_[a_idx];
    auto& b = vg.nodes_[b_idx];
    ensure_node(a);
    ensure_node(b);
    if (!any_edge_between(a.node_, b.node_) &&
        !additional_edge_between(a.node_, b.node_)) {
      additional.connect(a.node_, b.node_, ar->edge_info_);
    }
  });
}

void connect_adjacent_areas(additional_edges& additional, area const* a1,
                            node* n1, area const* a2, node* n2) {
  auto const mp = bg::model::multi_polygon<typename area::polygon_t>{
      a1->polygon_, a2->polygon_};
  auto const line =
      bg::model::linestring<location>{n1->location_, n2->location_};

  if (bg::covered_by(line, mp)) {
    additional.connect(n1, n2);
  }
}

void check_adjacent_areas(additional_edges& additional) {
  std::vector<std::pair<node*, area const*>> nodes;
  for (auto& it : additional.area_nodes_) {
    std::transform(begin(it.second), end(it.second), std::back_inserter(nodes),
                   [&](node* n) { return std::make_pair(n, it.first); });
  }
  if (nodes.size() < 2) {
    return;
  }
  for (std::size_t i = 0; i < nodes.size() - 1; i++) {
    for (std::size_t j = i + 1; j < nodes.size(); j++) {
      auto const* a1 = nodes[i].second;
      auto const* a2 = nodes[j].second;
      if (a1 == a2 ||
          std::find(begin(a1->adjacent_areas_), end(a1->adjacent_areas_),
                    a2->id_) == end(a1->adjacent_areas_)) {
        continue;
      }
      auto* n1 = nodes[i].first;
      auto* n2 = nodes[j].first;
      connect_adjacent_areas(additional, a1, n1, a2, n2);
    }
  }
}

void create_area_edges(additional_edges& additional) {
  for (auto& it : additional.area_nodes_) {
    create_area_edges(it.first, it.second, additional);  // NOLINT
  }
  check_adjacent_areas(additional);
}

}  // namespace ppr::routing
