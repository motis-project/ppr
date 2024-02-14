#include <algorithm>
#include <iostream>
#include <optional>

#include "ankerl/unordered_dense.h"

#include "ppr/common/geometry/path_conversion.h"
#include "ppr/common/location_geometry.h"
#include "ppr/common/timing.h"
#include "ppr/preprocessing/build_routing_graph.h"
#include "ppr/preprocessing/int_graph/int_graph.h"
#include "ppr/preprocessing/int_graph/linked_crossings.h"
#include "ppr/preprocessing/int_graph/oriented_int_edge.h"
#include "ppr/preprocessing/int_graph/path.h"
#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/osm_graph/builder.h"
#include "ppr/preprocessing/routing_graph/crossing_detour.h"

namespace ppr::preprocessing {

struct preprocessor {
  preprocessor(int_graph& ig, options const& opt, logging& log,
               statistics& stats)
      : ig_(ig), opt_(opt), log_(log), stats_(stats) {}

  void build() {
    auto const t_start = timing_now();
    connection_edge_info_ = create_edge_info(0, edge_type::CONNECTION).first;
    edge_infos_ = &ig_.edge_infos_;

    handle_junctions();
    stats_.routing_.d_junctions_ =
        log_.get_step_duration(pp_step::RG_JUNCTIONS);

    create_linked_crossings();
    stats_.routing_.d_linked_crossings_ =
        log_.get_step_duration(pp_step::RG_LINKED_CROSSINGS);

    create_edges();

    rg_.data_->edge_infos_ = std::move(ig_.edge_infos_);
    edge_infos_ = &rg_.data_->edge_infos_;
    rg_.data_->names_ = std::move(ig_.names_);
    rg_.create_in_edges();
    stats_.routing_.d_edges_ = log_.get_step_duration(pp_step::RG_EDGES);

    create_areas();
    stats_.routing_.d_areas_ = log_.get_step_duration(pp_step::RG_AREAS);

    calc_crossing_detours(rg_, opt_, log_);
    stats_.routing_.d_crossing_detours_ =
        log_.get_step_duration(pp_step::RG_CROSSING_DETOURS);

    stats_.routing_.d_total_ = ms_since(t_start);
  }

private:
  void handle_junctions() {
    step_progress progress{log_, pp_step::RG_JUNCTIONS, ig_.nodes_.size()};
    for (auto const& n : ig_.nodes_) {
      visit_node(n.get());
      progress.add();
    }
  }

  void visit_node(int_node* in) {
    auto edges = edges_sorted_by_angle(in);

    if (edges.empty()) {
      return;
    }

    handle_junction(in, edges);
  }

  void handle_junction(int_node* in,
                       std::vector<oriented_int_edge>& sorted_edges) {

    std::optional<edge_info_idx_t> special_edge_info_idx;
    if (in->is_special_node() && sorted_edges.size() > 1) {
      if (in->elevator_) {
        auto [info_idx, info] =
            create_edge_info(-in->osm_id_, edge_type::ELEVATOR);
        info->max_width_ = in->max_width_;
        special_edge_info_idx = info_idx;

      } else if (in->entrance_) {
        auto [info_idx, info] =
            create_edge_info(-in->osm_id_, edge_type::ENTRANCE);
        info->door_type_ = in->door_type_;
        info->automatic_door_type_ = in->automatic_door_type_;
        info->max_width_ = in->max_width_;
        info->level_ = in->level_;
        special_edge_info_idx = info_idx;

      } else if (in->cycle_barrier_) {
        auto [info_idx, info] =
            create_edge_info(-in->osm_id_, edge_type::CYCLE_BARRIER);
        info->max_width_ = in->max_width_;
        info->level_ = in->level_;
        special_edge_info_idx = info_idx;
      }
    }

    if (should_generate_crossing_at_node(in, sorted_edges)) {
      auto [info_idx, info] =
          create_edge_info(-in->osm_id_, edge_type::CROSSING, in->crossing_);

      if (in->crossing_edge_info_ != NO_EDGE_INFO) {
        auto const& cei = ig_.edge_infos_.at(in->crossing_edge_info_);
        info->street_type_ = cei.street_type_;
        info->name_ = cei.name_;
      }

      special_edge_info_idx = info_idx;
    }

    if (special_edge_info_idx) {
      connect_edges_at_special_node(in, sorted_edges, *special_edge_info_idx);
    } else {
      // TODO(pablo): disabled for now (removes too many edges)
      //      detect_streets_inside_linked_streets(in, sorted_edges);
      connect_streets_at_junction(in, sorted_edges);
      connect_footpaths_at_junction(in, sorted_edges);
    }
    create_crossings_at_junction(in, sorted_edges);
  }

  bool should_generate_crossing_at_node(
      int_node* in, std::vector<oriented_int_edge> const& sorted_edges) const {
    if (!in->is_crossing_node() || !sorted_edges.size() == 2) {
      return false;
    }

    if (in->crossing_edge_info_ == NO_EDGE_INFO) {
      return true;
    }

    auto const& cei = ig_.edge_infos_.at(in->crossing_edge_info_);
    return std::all_of(
        begin(sorted_edges), end(sorted_edges), [&](auto const& oie) {
          return cei.osm_way_id_ !=
                 ig_.edge_infos_.at(oie.edge_->info_).osm_way_id_;
        });
  }
  void detect_streets_inside_linked_streets(
      int_node const* in, std::vector<oriented_int_edge>& sorted_edges) const {
    (void)in;
    for_edge_pairs_ccw(
        sorted_edges,
        [&](oriented_int_edge& e1) { return is_linked(e1, side_type::LEFT); },
        [this](oriented_int_edge& e1, oriented_int_edge& e2) {
          if (is_street(ig_, e2) &&
              is_angle_around(normalized_angle(e2.angle_ - e1.angle_), 90,
                              10)) {
            auto* dest_node = int_to(e2);
            auto dest_edges = edges_sorted_by_angle(dest_node);
            auto const* dest_next_edge = next_edge_ccw(dest_edges, e2);
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
          [this](oriented_int_edge const& oie) { return is_street(ig_, oie); });
      assert(e != end(sorted_edges));
      if (has_sidewalk(*e, side_type::LEFT)) {
        auto const& mc = first_path_pt(*e, side_type::LEFT);
        auto* n = create_node(in->osm_id_, mc);
        set_node(ig_, *e, side_type::LEFT, n);
      }
      if (has_sidewalk(*e, side_type::RIGHT)) {
        auto const& mc = first_path_pt(*e, side_type::RIGHT);
        auto* n = create_node(in->osm_id_, mc);
        set_node(ig_, *e, side_type::RIGHT, n);
      }
    } else {
      for_edge_pairs_ccw(
          sorted_edges,
          [this](oriented_int_edge& e1) {
            return is_street(ig_, e1) && !is_ignored(e1);
          },
          [this, in](oriented_int_edge& e1, oriented_int_edge& e2) {
            if (!is_street(ig_, e2) || is_ignored(e2)) {
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
      auto* single_node = create_foot_node(in, in->location_);
      for (auto& e : sorted_edges) {
        if (is_street(ig_, e)) {
          connect(single_node, rg_from(ig_, e, side_type::LEFT));
          connect(single_node, rg_from(ig_, e, side_type::RIGHT));
        } else {
          set_node(ig_, e, side_type::LEFT, single_node);
        }
      }
      return;
    }

    auto update_streets_required = false;

    for_edge_pairs_ccw(
        sorted_edges,
        [this](oriented_int_edge& e1) {
          return !is_street(ig_, e1) && !is_ignored(e1);
        },
        [this, in, &update_streets_required](oriented_int_edge& e1,
                                             oriented_int_edge& e2) {
          if (!is_street(ig_, e2) || is_ignored(e2)) {
            return false;
          }

          auto* n = rg_from(ig_, e2, side_type::RIGHT);
          auto& e2_path = sidewalk_path(e2, side_type::RIGHT);
          if (n == nullptr) {
            assert(!e2_path.empty());
            n = create_foot_node(in, first_path_pt(e2, side_type::RIGHT));
            set_node(ig_, e2, side_type::RIGHT, n);
            update_streets_required = true;
          }
          if (n != nullptr) {
            auto& e1_path = e1.edge_->path_left_;
            auto const intersection = join_footpath_with_street(
                e1_path, e1.reverse_, e2_path, e2.reverse_);
            n->location_ = to_location(intersection);
            set_node(ig_, e1, side_type::LEFT, n);
          }
          return true;
        });

    if (update_streets_required) {
      set_missing_street_nodes(sorted_edges);
    }
  }

  void connect_edges_at_special_node(
      int_node* in, std::vector<oriented_int_edge>& sorted_edges,
      edge_info_idx_t const info_idx) {
    for (auto& e : sorted_edges) {
      if (is_street(ig_, e)) {
        if (has_sidewalk(e, side_type::LEFT)) {
          auto const& mc = first_path_pt(e, side_type::LEFT);
          auto* n = create_node(in->osm_id_, mc);
          set_node(ig_, e, side_type::LEFT, n);
        }
        if (has_sidewalk(e, side_type::RIGHT)) {
          auto const& mc = first_path_pt(e, side_type::RIGHT);
          auto* n = create_node(in->osm_id_, mc);
          set_node(ig_, e, side_type::RIGHT, n);
        }
      } else {
        auto* n = rg_from(ig_, e, side_type::LEFT);
        if (n == nullptr) {
          n = create_foot_node(in, in->location_);
          set_node(ig_, e, side_type::LEFT, n);
        }
      }
    }

    for_edge_pairs_ccw(
        sorted_edges, [](oriented_int_edge&) { return true; },
        [this, info_idx](oriented_int_edge& e1, oriented_int_edge& e2) {
          auto* e1_left = rg_from(ig_, e1, side_type::LEFT);
          auto* e1_right = rg_from(ig_, e1, side_type::RIGHT);
          auto* e2_left = rg_from(ig_, e2, side_type::LEFT);
          auto* e2_right = rg_from(ig_, e2, side_type::RIGHT);

          connect(e1_left, e2_left, info_idx);
          connect(e1_left, e2_right, info_idx);
          connect(e1_right, e2_left, info_idx);
          connect(e1_right, e2_right, info_idx);
          return false;
        });
  }

  void set_missing_street_nodes(
      std::vector<oriented_int_edge>& sorted_edges) const {
    for_edge_pairs_ccw(
        sorted_edges,
        [this](oriented_int_edge& e1) { return is_street(ig_, e1); },
        [this](oriented_int_edge& e1, oriented_int_edge& e2) {
          if (!is_street(ig_, e2)) {
            return false;
          }
          auto* e1_left = rg_from(ig_, e1, side_type::LEFT);
          auto* e2_right = rg_from(ig_, e2, side_type::RIGHT);
          if ((e1_left != nullptr) && (e2_right == nullptr)) {
            set_node(ig_, e2, side_type::RIGHT, e1_left);
          } else if ((e1_left == nullptr) && (e2_right != nullptr)) {
            set_node(ig_, e1, side_type::LEFT, e2_right);
          }
          return true;
        });
  }

  void create_linked_crossings() {
    step_progress progress{log_, pp_step::RG_LINKED_CROSSINGS,
                           ig_.nodes_.size() * 2};
    add_linked_crossings(ig_, rg_, log_, progress, stats_.routing_);
    connect_linked_crossings(progress);
  }

  void connect_linked_crossings(step_progress& progress) {
    for (auto& n : ig_.nodes_) {
      if (n->generated_crossing_edges_) {
        auto* in = n.get();
        auto edges = edges_sorted_by_angle(in);
        connect_crossing_edges_at_junction(in, edges);
      }
    }
    progress.add();
  }

  void connect_crossing_edges_at_junction(
      int_node* in, std::vector<oriented_int_edge>& sorted_edges) const {
    (void)in;
    for_edge_pairs_ccw(
        sorted_edges,
        [this](oriented_int_edge& e1) {
          return e1.edge_->info(ig_)->type_ == edge_type::CROSSING;
        },
        [this, in](oriented_int_edge& e1, oriented_int_edge& e2) {
          if (!is_linked(e2)) {
            return false;
          }
          auto* n = rg_from(ig_, e2, side_type::LEFT);
          if (n == nullptr) {
            n = rg_from(ig_, e2, side_type::RIGHT);
          }
          if (n != nullptr) {
            set_node(ig_, e1, side_type::LEFT, n);
            if (e1.reverse_) {
              e1.edge_->path_left_[1] = to_merc(n->location_);
            } else {
              e1.edge_->path_left_[0] = to_merc(n->location_);
            }
            return true;
          } else {
            // TODO(pablo):
            if (opt_.print_warnings_) {
              log_.out() << "cannot connect linked crossing at node "
                         << in->osm_id_
                         << " e1=" << e1.edge_->info(ig_)->osm_way_id_
                         << ", e2=" << e2.edge_->info(ig_)->osm_way_id_
                         << std::endl;
            }
          }
          return false;
        });
  }

  bool no_streets_on_footpath_layer(
      std::vector<oriented_int_edge> const& sorted_edges) const {
    ankerl::unordered_dense::set<int8_t> footpath_layers;
    for (auto const& se : sorted_edges) {
      auto const* e = se.edge_;
      if (!e->generate_sidewalks(ig_)) {
        footpath_layers.insert(e->layer_);
      }
    }
    for (auto const& se : sorted_edges) {
      auto const* e = se.edge_;
      if (e->generate_sidewalks(ig_) &&
          footpath_layers.find(e->layer_) != end(footpath_layers)) {
        return false;
      }
    }
    return true;
  }

  void create_crossings_at_junction(
      int_node const* in, std::vector<oriented_int_edge> const& sorted_edges) {
    for (auto const& se : sorted_edges) {
      create_crossing(in, se);
    }
  }

  void create_crossing(int_node const* in, oriented_int_edge const& se) {
    if (is_ignored(se)) {
      return;
    }
    auto* from_node = rg_from(ig_, se, side_type::LEFT);
    auto* to_node = rg_from(ig_, se, side_type::RIGHT);
    if ((from_node != nullptr) && (to_node != nullptr)) {
      create_crossing(from_node, to_node, se.edge_->info_, in);
    }
  }

  void create_crossing(node* from, node* to,
                       edge_info_idx_t crossed_edge_info_idx,
                       int_node const* in) {
    if (has_crossing(from, to) || has_crossing(to, from)) {
      return;
    }
    auto type = in->crossing_;
    if (type == crossing_type::NONE) {
      type = crossing_type::GENERATED;
    }

    auto const& crossed_edge_info = ig_.edge_infos_[crossed_edge_info_idx];
    auto const osm_way_id = -crossed_edge_info.osm_way_id_;
    auto const street_type = crossed_edge_info.street_type_;
    auto const name = crossed_edge_info.name_;

    auto [info_idx, info] =
        create_edge_info(osm_way_id, edge_type::CROSSING, type, street_type);
    info->name_ = name;
    info->traffic_signals_sound_ = in->traffic_signals_sound_;
    info->traffic_signals_vibration_ = in->traffic_signals_vibration_;
    auto width = distance(from->location_, to->location_);
    from->out_edges_.emplace_back(
        data::make_unique<edge>(make_edge(info_idx, from, to, width)));
    stats_.routing_.n_crossings_created_++;
  }

  bool has_crossing(node const* from, node const* to) {
    return std::any_of(
        begin(from->out_edges_), end(from->out_edges_), [this, to](auto&& e) {
          return e->to_ == to &&
                 e->info(*edge_infos_)->type_ == edge_type::CROSSING;
        });
  }

  static std::vector<oriented_int_edge> edges_sorted_by_angle(int_node* in) {
    std::vector<oriented_int_edge> sorted_edges;

    sorted_edges.reserve(in->out_edges_.size() + in->in_edges_.size());

    for (auto& ie : in->out_edges_) {
      sorted_edges.emplace_back(ie.get(), false, ie->from_angle(false));
    }
    for (auto* ie : in->in_edges_) {
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
          log_.out() << "could not join same edge - way "
                     << e1.edge_->info(ig_)->osm_way_id_
                     << ", e1_reverse = " << e1.reverse_
                     << ", e2_reverse = " << e2.reverse_ << std::endl;
        }
        stats_.routing_.n_path_join_failed_same_++;
      }
      if (!e1_sidewalk || !e2_sidewalk) {
        if (opt_.print_warnings_) {
          log_.out() << "skipping node creation because of no sidewalks!"
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
          log_.out() << "could not join ways "
                     << e1.edge_->info(ig_)->osm_way_id_ << " and "
                     << e2.edge_->info(ig_)->osm_way_id_ << " at node "
                     << center->osm_id_ << std::endl;
        }
        stats_.routing_.n_path_join_failed_diff_++;
      }
      n = create_node(center->osm_id_, intersection);
    }

    assert(n != nullptr);

    if (!is_linked(e1, side_type::LEFT)) {
      set_node(ig_, e1, side_type::LEFT, n);
    }
    if (!is_linked(e2, side_type::RIGHT)) {
      set_node(ig_, e2, side_type::RIGHT, n);
    }

    return n;
  }

  void create_edges() {
    step_progress progress{log_, pp_step::RG_EDGES, ig_.nodes_.size()};
    for (auto const& n : ig_.nodes_) {
      visit_edges(n.get());
      progress.add();
    }
  }

  void visit_edges(int_node* in) {
    for (auto& ie : in->out_edges_) {
      visit_edge(*ie);
    }
  }

  void visit_edge(int_edge& ie) const {
    if (ie.ignore_) {
      return;
    }
    if (ie.generate_sidewalks(ig_)) {
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

  void create_areas() {
    auto const progress = step_progress{log_, pp_step::RG_AREAS};
    rg_.data_->areas_.reserve(
        static_cast<decltype(rg_.data_->areas_)::size_type>(ig_.areas_.size()));
    std::transform(begin(ig_.areas_), end(ig_.areas_),
                   std::back_inserter(rg_.data_->areas_),
                   [](int_area const& ia) { return ia.to_area(); });
  }

  static crossing_type combine_crossing(crossing_type a, crossing_type b) {
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
    auto* n = create_node(in->osm_id_, mc);
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

  inline std::pair<edge_info_idx_t, edge_info*> create_edge_info(
      std::int64_t osm_way_id, edge_type type,
      crossing_type crossing = crossing_type::GENERATED,
      street_type street = street_type::NONE) {
    return make_edge_info(ig_.edge_infos_, osm_way_id, type, street, crossing);
  }

  void connect(node* from, node* to) const {
    connect(from, to, connection_edge_info_);
  }

  static void connect(node* from, node* to, edge_info_idx_t const info_idx) {
    if ((from == nullptr) || (to == nullptr)) {
      return;
    }
    if (std::any_of(begin(to->out_edges_), end(to->out_edges_),
                    [from, info_idx](auto&& e) {
                      return e->to_ == from && e->info_ == info_idx;
                    })) {
      // already connected in the other direction
      return;
    }
    auto const dist = distance(from->location_, to->location_);
    from->out_edges_.emplace_back(
        data::make_unique<edge>(make_edge(info_idx, from, to, dist)));
  }

public:
  int_graph& ig_;
  routing_graph rg_;
  options const& opt_;
  logging& log_;
  statistics& stats_;
  data::vector_map<edge_info_idx_t, edge_info>* edge_infos_{};

private:
  edge_info_idx_t connection_edge_info_{};
};

routing_graph build_routing_graph(int_graph& ig, options const& opt,
                                  logging& log, statistics& stats) {
  preprocessor pp(ig, opt, log, stats);
  pp.build();
  collect_stats(stats.routing_, pp.rg_);
  return std::move(pp.rg_);
}

int_graph build_int_graph(options const& opt, logging& log, statistics& stats) {
  auto og = build_osm_graph(opt, log, stats);
  return build_int_graph(og, opt, log, stats);
}

routing_graph build_routing_graph(options const& opt, logging& log,
                                  statistics& stats) {
  auto ig = build_int_graph(opt, log, stats);
  return build_routing_graph(ig, opt, log, stats);
}

}  // namespace ppr::preprocessing
