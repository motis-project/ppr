#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "ppr/common/geometry/path_conversion.h"
#include "ppr/common/location_geometry.h"
#include "ppr/common/timing.h"
#include "ppr/preprocessing/int_graph/linked_crossings.h"
#include "ppr/preprocessing/int_graph/oriented_int_edge.h"
#include "ppr/preprocessing/int_graph/path.h"
#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/osm_graph/builder.h"
#include "ppr/preprocessing/preprocessing.h"
#include "ppr/preprocessing/routing_graph/crossing_detour.h"

namespace ppr::preprocessing {

struct preprocessor {
  preprocessor(int_graph& ig, options const& opt, statistics& stats)
      : ig_(ig), opt_(opt), stats_(stats) {}

  void build() {
    auto const t_start = timing_now();
    rg_.init();
    connection_edge_info_ = create_edge_info(0, edge_type::CONNECTION);

    log_step(pp_step::RG_JUNCTIONS);
    for (auto const& n : ig_.nodes_) {
      visit_node(n.get());
    }
    auto const t_after_junctions = timing_now();
    stats_.routing_.d_junctions_ = ms_between(t_start, t_after_junctions);

    log_step(pp_step::RG_LINKED_CROSSINGS);
    add_linked_crossings(ig_, rg_, stats_.routing_);
    connect_linked_crossings();
    auto const t_after_linked_crossings = timing_now();
    stats_.routing_.d_linked_crossings_ =
        ms_between(t_after_junctions, t_after_linked_crossings);

    log_step(pp_step::RG_EDGES);
    for (auto const& n : ig_.nodes_) {
      visit_edges(n.get());
    }

    rg_.data_->edge_infos_ = std::move(ig_.edge_infos_);
    rg_.data_->names_ = std::move(ig_.names_);
    rg_.create_in_edges();
    auto const t_after_edges = timing_now();
    stats_.routing_.d_edges_ =
        ms_between(t_after_linked_crossings, t_after_edges);

    log_step(pp_step::RG_AREAS);
    rg_.data_->areas_.reserve(
        static_cast<decltype(rg_.data_->areas_)::size_type>(ig_.areas_.size()));
    std::transform(begin(ig_.areas_), end(ig_.areas_),
                   std::back_inserter(rg_.data_->areas_),
                   [](int_area const& ia) { return ia.to_area(); });
    auto const t_after_areas = timing_now();
    stats_.routing_.d_areas_ = ms_between(t_after_edges, t_after_areas);

    log_step(pp_step::RG_CROSSING_DETOURS);
    calc_crossing_detours(rg_, opt_);
    auto const t_after_detours = timing_now();
    stats_.routing_.d_crossing_detours_ =
        ms_between(t_after_areas, t_after_detours);

    stats_.routing_.d_total_ = ms_since(t_start);
    print_timing("Routing Graph: Junctions", stats_.routing_.d_junctions_);
    print_timing("Routing Graph: Linked Crossings",
                 stats_.routing_.d_linked_crossings_);
    print_timing("Routing Graph: Edges", stats_.routing_.d_edges_);
    print_timing("Routing Graph: Areas", stats_.routing_.d_areas_);
    print_timing("Routing Graph: Crossing Detours",
                 stats_.routing_.d_crossing_detours_);
    print_timing("Routing Graph: Total", stats_.routing_.d_total_);
  }

private:
  void visit_node(int_node* in) {
    auto edges = edges_sorted_by_angle(in);

    if (edges.empty()) {
      return;
    }

    handle_junction(in, edges);
  }

  void handle_junction(int_node* in,
                       std::vector<oriented_int_edge>& sorted_edges) {
    if (!in->elevator_) {
      // TODO(pablo): disabled for now (removes too many edges)
      //      detect_streets_inside_linked_streets(in, sorted_edges);
      connect_streets_at_junction(in, sorted_edges);
      connect_footpaths_at_junction(in, sorted_edges);
      create_crossings_at_junction(in, sorted_edges);
    } else {
      connect_elevator_edges(in, sorted_edges);
    }
  }

  static void detect_streets_inside_linked_streets(
      int_node const* in, std::vector<oriented_int_edge>& sorted_edges) {
    (void)in;
    for_edge_pairs_ccw(
        sorted_edges,
        [&](oriented_int_edge& e1) { return is_linked(e1, side_type::LEFT); },
        [&](oriented_int_edge& e1, oriented_int_edge& e2) {
          if (is_street(e2) &&
              is_angle_around(normalized_angle(e2.angle_ - e1.angle_), 90,
                              10)) {
            auto const dest_node = int_to(e2);
            auto dest_edges = edges_sorted_by_angle(dest_node);
            auto dest_next_edge = next_edge_ccw(dest_edges, e2);
            if ((dest_next_edge != nullptr) &&
                is_linked(*dest_next_edge, side_type::RIGHT) &&
                is_angle_around(
                    normalized_angle(dest_next_edge->angle_ - to_angle(e2)), 90,
                    10)) {
              e2.edge_->ignore_ = true;
              //              std::clog << "removing inner linked street "
              //                        << e2.edge_->info_->osm_way_id_ << "
              //                        between nodes "
              //                        << in->osm_id_ << " and " <<
              //                        dest_node->osm_id_
              //                        << std::endl;
            }
          }
          return true;
        });
  }

  void connect_streets_at_junction(
      int_node const* in, std::vector<oriented_int_edge>& sorted_edges) {
    if (in->street_edges_ == 1) {
      auto e = std::find_if(
          begin(sorted_edges), end(sorted_edges),
          [](oriented_int_edge const& oie) { return is_street(oie); });
      assert(e != end(sorted_edges));
      if (has_sidewalk(*e, side_type::LEFT)) {
        auto const& mc = first_path_pt(*e, side_type::LEFT);
        auto n = create_node(in->osm_id_, mc);
        set_node(*e, side_type::LEFT, n);
      }
      if (has_sidewalk(*e, side_type::RIGHT)) {
        auto const& mc = first_path_pt(*e, side_type::RIGHT);
        auto n = create_node(in->osm_id_, mc);
        set_node(*e, side_type::RIGHT, n);
      }
    } else {
      for_edge_pairs_ccw(
          sorted_edges,
          [&](oriented_int_edge& e1) {
            return is_street(e1) && !is_ignored(e1);
          },
          [&](oriented_int_edge& e1, oriented_int_edge& e2) {
            if (!is_street(e2) || is_ignored(e2)) {
              return false;
            }

            cut_junction_edges(in, e1, e2);
            return true;
          });
    }
  }

  void connect_footpaths_at_junction(
      int_node* in, std::vector<oriented_int_edge>& sorted_edges) {
    if (in->footway_edges_ == 0) {
      return;
    }

    if (in->street_edges_ <= 1) {
      auto single_node = create_foot_node(in, in->location_);
      for (auto& e : sorted_edges) {
        if (is_street(e)) {
          connect(single_node, rg_from(e, side_type::LEFT));
          connect(single_node, rg_from(e, side_type::RIGHT));
        } else {
          set_node(e, side_type::LEFT, single_node);
        }
      }
      return;
    }

    auto update_streets_required = false;

    for_edge_pairs_ccw(
        sorted_edges,
        [&](oriented_int_edge& e1) {
          return !is_street(e1) && !is_ignored(e1);
        },
        [&](oriented_int_edge& e1, oriented_int_edge& e2) {
          if (!is_street(e2) || is_ignored(e2)) {
            return false;
          }

          auto n = rg_from(e2, side_type::RIGHT);
          auto& e2_path = sidewalk_path(e2, side_type::RIGHT);
          if (n == nullptr) {
            assert(!e2_path.empty());
            n = create_foot_node(in, first_path_pt(e2, side_type::RIGHT));
            set_node(e2, side_type::RIGHT, n);
            update_streets_required = true;
          }
          if (n != nullptr) {
            auto& e1_path = e1.edge_->path_left_;
            join_footpath_with_street(e1_path, e1.reverse_, e2_path,
                                      e2.reverse_);
            set_node(e1, side_type::LEFT, n);
          }
          return true;
        });

    if (update_streets_required) {
      set_missing_street_nodes(sorted_edges);
    }
  }

  void connect_elevator_edges(int_node* in,
                              std::vector<oriented_int_edge>& sorted_edges) {
    assert(in->elevator_);
    // TODO(pablo): this code assumes that all edges have edge_type::FOOTWAY
    //  if they don't (which sometimes happens), only the left sidewalk of
    //  the street is connected to the elevator. this could maybe be improved.

    for_edge_pairs_ccw(
        sorted_edges,
        [&](oriented_int_edge& e1) {
          auto n = rg_from(e1, side_type::LEFT);
          if (n == nullptr) {
            n = create_foot_node(in, in->location_);
            set_node(e1, side_type::LEFT, n);
          }
          return true;
        },
        [&](oriented_int_edge& e1, oriented_int_edge& e2) {
          auto n1 = rg_from(e1, side_type::LEFT);
          assert(n1 != nullptr);
          auto n2 = rg_from(e2, side_type::LEFT);
          if (n2 == nullptr) {
            n2 = create_foot_node(in, in->location_);
            set_node(e2, side_type::LEFT, n2);
          }

          auto info = create_edge_info(-in->osm_id_, edge_type::ELEVATOR);
          n1->out_edges_.emplace_back(
              data::make_unique<edge>(make_edge(info, n1, n2, 0.0)));
          return false;
        });
  }

  static void set_missing_street_nodes(
      std::vector<oriented_int_edge>& sorted_edges) {
    for_edge_pairs_ccw(
        sorted_edges, [&](oriented_int_edge& e1) { return is_street(e1); },
        [&](oriented_int_edge& e1, oriented_int_edge& e2) {
          if (!is_street(e2)) {
            return false;
          }
          auto const e1_left = rg_from(e1, side_type::LEFT);
          auto const e2_right = rg_from(e2, side_type::RIGHT);
          if ((e1_left != nullptr) && (e2_right == nullptr)) {
            set_node(e2, side_type::RIGHT, e1_left);
          } else if ((e1_left == nullptr) && (e2_right != nullptr)) {
            set_node(e1, side_type::LEFT, e2_right);
          }
          return true;
        });
  }

  void connect_linked_crossings() {
    for (auto& n : ig_.nodes_) {
      if (n->generated_crossing_edges_) {
        auto in = n.get();
        auto edges = edges_sorted_by_angle(in);
        connect_crossing_edges_at_junction(in, edges);
      }
    }
  }

  void connect_crossing_edges_at_junction(
      int_node* in, std::vector<oriented_int_edge>& sorted_edges) {
    (void)in;
    for_edge_pairs_ccw(
        sorted_edges,
        [&](oriented_int_edge& e1) {
          return e1.edge_->info_->type_ == edge_type::CROSSING;
        },
        [&](oriented_int_edge& e1, oriented_int_edge& e2) {
          if (!is_linked(e2)) {
            return false;
          }
          auto n = rg_from(e2, side_type::LEFT);
          if (n == nullptr) {
            n = rg_from(e2, side_type::RIGHT);
          }
          if (n != nullptr) {
            set_node(e1, side_type::LEFT, n);
            if (e1.reverse_) {
              e1.edge_->path_left_[1] = to_merc(n->location_);
            } else {
              e1.edge_->path_left_[0] = to_merc(n->location_);
            }
            return true;
          } else {
            // TODO(pablo):
            if (opt_.print_warnings_) {
              std::clog << "cannot connect linked crossing at node "
                        << in->osm_id_ << " e1=" << e1.edge_->info_->osm_way_id_
                        << ", e2=" << e2.edge_->info_->osm_way_id_ << std::endl;
            }
          }
          return false;
        });
  }

  static bool no_streets_on_footpath_layer(
      std::vector<oriented_int_edge> const& sorted_edges) {
    std::unordered_set<int8_t> footpath_layers;
    for (auto const& se : sorted_edges) {
      auto const e = se.edge_;
      if (!e->generate_sidewalks()) {

        footpath_layers.insert(e->layer_);
      }
    }
    for (auto const& se : sorted_edges) {
      auto const e = se.edge_;
      if (e->generate_sidewalks() &&
          footpath_layers.find(e->layer_) != end(footpath_layers)) {
        return false;
      }
    }
    return true;
  }

  void create_crossings_at_junction(
      int_node const* in, std::vector<oriented_int_edge> const& sorted_edges) {
    for (auto& se : sorted_edges) {
      create_crossing(in, se);
    }
  }

  void create_crossing(int_node const* in, oriented_int_edge const& se) {
    if (is_ignored(se)) {
      return;
    }
    auto from_node = rg_from(se, side_type::LEFT);
    auto to_node = rg_from(se, side_type::RIGHT);
    if ((from_node != nullptr) && (to_node != nullptr)) {
      create_crossing(from_node, to_node, se.edge_->info_, in->crossing_);
    }
  }

  void create_crossing(node* from, node* to, edge_info const* crossed_edge_info,
                       crossing_type::crossing_type type) {
    if (has_crossing(from, to) || has_crossing(to, from)) {
      return;
    }
    if (type == crossing_type::NONE) {
      type = crossing_type::GENERATED;
    }
    auto info =
        create_edge_info(-crossed_edge_info->osm_way_id_, edge_type::CROSSING,
                         type, crossed_edge_info->street_type_);
    info->name_ = crossed_edge_info->name_;
    auto width = distance(from->location_, to->location_);
    from->out_edges_.emplace_back(
        data::make_unique<edge>(make_edge(info, from, to, width)));
    stats_.routing_.n_crossings_created_++;
  }

  static bool has_crossing(node const* from, node const* to) {
    for (auto const& e : from->out_edges_) {
      if (e->to_ == to && e->info_->type_ == edge_type::CROSSING) {
        return true;
      }
    }
    return false;
  }

  static std::vector<oriented_int_edge> edges_sorted_by_angle(int_node* in) {
    std::vector<oriented_int_edge> sorted_edges;

    for (auto& ie : in->out_edges_) {
      sorted_edges.emplace_back(ie.get(), false, ie->from_angle(false));
    }
    for (auto ie : in->in_edges_) {
      sorted_edges.emplace_back(ie, true, ie->from_angle(true));
    }

    std::sort(begin(sorted_edges), end(sorted_edges));

    return sorted_edges;
  }

  node* cut_junction_edges(int_node const* center, oriented_int_edge& e1,
                           oriented_int_edge& e2) {
    node* n = nullptr;

    auto e1_sidewalk = has_sidewalk(e1, side_type::LEFT);
    auto e2_sidewalk = has_sidewalk(e2, side_type::RIGHT);

    if (!e1_sidewalk && !e2_sidewalk) {
      return nullptr;
    }

    auto& e1_path = sidewalk_path(e1, side_type::LEFT);
    auto& e2_path = sidewalk_path(e2, side_type::RIGHT);

    if (e1.edge_ == e2.edge_) {
      assert(e1.reverse_ != e2.reverse_);
      assert(e1_path == e2_path);
      auto joined = join_single_path(e1_path, center->location_);
      auto intersection = joined.first;
      if (!joined.second) {
        if (opt_.print_warnings_) {
          std::clog << "could not join same edge - way "
                    << e1.edge_->info_->osm_way_id_
                    << ", e1_reverse = " << e1.reverse_
                    << ", e2_reverse = " << e2.reverse_ << std::endl;
        }
        stats_.routing_.n_path_join_failed_same_++;
      }
      if (!e1_sidewalk || !e2_sidewalk) {
        if (opt_.print_warnings_) {
          std::clog << "skipping node creation because of no sidewalks!"
                    << std::endl;
        }
        return n;
      }
      n = create_node(center->osm_id_, intersection);
    } else {
      auto joined = join_paths(e1_path, e1.reverse_, e2_path, e2.reverse_,
                               center->location_);
      auto intersection = joined.first;
      if (!joined.second) {
        // TODO(pablo):
        if (opt_.print_warnings_) {
          std::clog << "could not join ways " << e1.edge_->info_->osm_way_id_
                    << " and " << e2.edge_->info_->osm_way_id_ << " at node "
                    << center->osm_id_ << std::endl;
        }
        stats_.routing_.n_path_join_failed_diff_++;
      }
      n = create_node(center->osm_id_, intersection);
    }

    assert(n != nullptr);

    if (!is_linked(e1, side_type::LEFT)) {
      set_node(e1, side_type::LEFT, n);
    }
    if (!is_linked(e2, side_type::RIGHT)) {
      set_node(e2, side_type::RIGHT, n);
    }

    return n;
  }

  static void visit_edges(int_node* in) {
    for (auto& ie : in->out_edges_) {
      visit_edge(*ie);
    }
  }

  static void visit_edge(int_edge& ie) {
    if (ie.ignore_) {
      return;
    }
    if (ie.generate_sidewalks()) {
      if (ie.sidewalk_left_ && ie.from_left_ != nullptr &&
          ie.to_left_ != nullptr) {
        ie.from_left_->out_edges_.emplace_back(
            data::make_unique<edge>(make_edge(
                ie.info_, ie.from_left_, ie.to_left_,
                path_length(ie.path_left_), to_location_vector(ie.path_left_),
                side_type::LEFT, ie.elevation_up_, ie.elevation_down_)));
      }
      if (ie.sidewalk_right_ && (ie.from_right_ != nullptr) &&
          (ie.to_right_ != nullptr)) {
        ie.from_right_->out_edges_.emplace_back(
            data::make_unique<edge>(make_edge(
                ie.info_, ie.from_right_, ie.to_right_,
                path_length(ie.path_right_), to_location_vector(ie.path_right_),
                side_type::RIGHT, ie.elevation_up_, ie.elevation_down_)));
      }
    } else {
      if ((ie.from_left_ != nullptr) && (ie.to_left_ != nullptr)) {
        ie.from_left_->out_edges_.emplace_back(
            data::make_unique<edge>(make_edge(
                ie.info_, ie.from_left_, ie.to_left_,
                path_length(ie.path_left_), to_location_vector(ie.path_left_),
                side_type::CENTER, ie.elevation_up_, ie.elevation_down_)));
      }
    }
  }

  static crossing_type::crossing_type combine_crossing(
      crossing_type::crossing_type a, crossing_type::crossing_type b) {
    return std::max(a, b);
  }

  static bool is_reverse(double ref_angle, double other_angle) {
    auto angle = normalized_angle(ref_angle - other_angle);
    return is_reverse(angle);
  }

  static bool is_reverse(double angle) {
    return angle > PI / 2 && angle < PI * 3 / 2;
  }

  struct node* create_node(std::int64_t osm_id, merc const& mc) {
    auto const loc = to_location(mc);
    rg_.data_->nodes_.emplace_back(data::make_unique<struct node>(
        make_node(++rg_.data_->max_node_id_, osm_id, loc)));
    return rg_.data_->nodes_.back().get();
  }

  struct node* create_foot_node(int_node* in, merc const& mc) {
    auto n = create_node(in->osm_id_, mc);
    if (in->rg_foot_node_ != nullptr) {
      if (distance(in->location_, mc) <
          distance(in->location_, to_merc(in->rg_foot_node_->location_))) {
        in->rg_foot_node_ = n;
      }
    } else {
      in->rg_foot_node_ = n;
    }
    return n;
  }

  inline edge_info* create_edge_info(
      std::int64_t osm_way_id, edge_type type,
      crossing_type::crossing_type crossing = crossing_type::GENERATED,
      street_type street = street_type::NONE) {
    return ig_.edge_infos_
        .emplace_back(data::make_unique<edge_info>(
            make_edge_info(osm_way_id, type, street, crossing)))
        .get();
  }

  void connect(node* from, node* to) {
    if ((from == nullptr) || (to == nullptr)) {
      return;
    }
    from->out_edges_.emplace_back(data::make_unique<edge>(
        make_edge(connection_edge_info_, from, to, 0.0)));
  }

public:
  int_graph& ig_;
  routing_graph rg_;
  options const& opt_;
  statistics& stats_;

private:
  edge_info* connection_edge_info_{};
};

routing_graph build_routing_graph(int_graph& ig, options const& opt,
                                  statistics& stats) {
  preprocessor pp(ig, opt, stats);
  pp.build();
  collect_stats(stats.routing_, pp.rg_);
  return std::move(pp.rg_);
}

int_graph build_int_graph(options const& opt, statistics& stats) {
  auto og = build_osm_graph(opt, stats);
  std::clog << "Building intermediate graph..." << std::endl;
  return build_int_graph(og, opt, stats);
}

routing_graph build_routing_graph(options const& opt, statistics& stats) {
  auto ig = build_int_graph(opt, stats);
  std::clog << "Building routing graph..." << std::endl;
  return build_routing_graph(ig, opt, stats);
}

}  // namespace ppr::preprocessing
