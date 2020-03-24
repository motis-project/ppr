#pragma once

#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ppr/common/data.h"
#include "ppr/common/timing.h"
#include "ppr/routing/additional_edges.h"
#include "ppr/routing/costs.h"
#include "ppr/routing/input_areas.h"
#include "ppr/routing/input_pt.h"
#include "ppr/routing/label.h"
#include "ppr/routing/route.h"
#include "ppr/routing/search_profile.h"
#include "ppr/routing/statistics.h"

namespace bg = boost::geometry;

namespace ppr::routing {

template <typename Label>
struct pareto_dijkstra {
  struct compare_labels {
    bool operator()(Label const* a, Label const* b) const {
      return a->operator>(*b);
    }
  };

  pareto_dijkstra(search_profile const& profile, bool reverse_search)
      : profile_(profile), reverse_search_(reverse_search) {}

  void add_start(location const& loc, std::vector<input_pt> const& pts) {
    auto const t_start = timing_now();
    auto input_node = additional_.create_node(loc);
    start_nodes_.push_back(input_node);
    for (auto const& pt : pts) {
      add_node(input_node, pt);
    }
    stats_.d_starts_ += ms_since(t_start);
  }

  void add_goal(location const& loc, std::vector<input_pt> const& pts) {
    auto const t_start = timing_now();
    auto input_node = additional_.create_node(loc);
    goals_.push_back(input_node);
    for (auto const& pt : pts) {
      add_node(input_node, pt);
    }
    stats_.d_goals_ += ms_since(t_start);
  }

  void search() {
    if (goals_.empty()) {
      return;
    }

    auto const t_before_area_edges = timing_now();
    create_area_edges(additional_);
    stats_.d_area_edges_ = ms_since(t_before_area_edges);

    auto const t_start = timing_now();
    create_start_labels();

    stats_.additional_nodes_ = additional_.nodes_.size();
    stats_.additional_edges_ = additional_.edges_.size();
    stats_.additional_areas_ = additional_.area_nodes_.size();

    while (!queue_.empty()) {
      if (stats_.labels_created_ > max_labels_) {
        stats_.max_label_quit_ = true;
        break;
      }
      auto label = queue_.top();
      queue_.pop();
      stats_.labels_popped_++;

      if (label->dominated_ || dominated_by_results(label)) {
        // TODO(pablo): release label
        continue;
      }

      auto const node = label->get_node();

      if (is_goal(node)) {
        continue;
      }

      for (auto const& e : node->out_edges_) {
        create_label(label, e.get(), true);
      }
      for (auto const& e : node->in_edges_) {
        create_label(label, e, false);
      }

      auto it = additional_.edge_map_.find(node);
      if (it != end(additional_.edge_map_)) {
        for (auto const& e : it->second) {
          create_label(label, e, false);
        }
      }
    }

    stats_.goals_ = goals_.size();
    stats_.goals_reached_ = static_cast<std::size_t>(std::count_if(
        begin(goals_), end(goals_),
        [&](node const* goal) { return !node_labels_[goal].empty(); }));
    stats_.d_search_ = ms_since(t_start);
  }

  std::vector<std::vector<Label*>> get_results() {
    std::vector<std::vector<Label*>> results;
    for (auto const n : goals_) {
      results.emplace_back(node_labels_[n]);
    }
    return results;
  }

  dijkstra_statistics const& get_statistics() const { return stats_; }

private:
  void create_start_labels() {
    for (auto const node : start_nodes_) {
      for (auto const& e : node->out_edges_) {
        create_start_label(e.get(), true);
      }
      auto it = additional_.edge_map_.find(node);
      if (it != end(additional_.edge_map_)) {
        for (auto const& e : it->second) {
          create_start_label(e, false);
        }
      }
    }
  }

  void create_label(Label* pred, edge const* e, bool fwd) {
    if (e == pred->edge_.edge_) {
      return;
    }
    auto de = make_directed_edge(e, fwd);
    if (!de.allowed()) {
      return;
    }

    Label tmp;
    auto created = pred->create_label(tmp, de, profile_);
    if (!created) {
      return;
    }

    labels_.emplace_back(std::make_unique<Label>(tmp));
    auto new_label = labels_.back().get();
    auto const goal = is_goal(new_label->get_node());

    if (!add_label_to_node(new_label)) {
      labels_.pop_back();
      return;
    }

    stats_.labels_created_++;

    if (!goal) {
      queue_.push(new_label);
    }
  }

  directed_edge make_directed_edge(edge const* e, bool fwd) {
    return {e, get_edge_costs(e, reverse_search_ ? !fwd : fwd, profile_), fwd};
  }

  bool add_label_to_node(Label* new_label) {
    auto& dest_labels = node_labels_[new_label->get_node()];
    for (auto it = dest_labels.begin(); it != dest_labels.end();) {
      Label* o = *it;
      if (o->dominates(*new_label)) {
        return false;
      }

      if (new_label->dominates(*o)) {
        it = dest_labels.erase(it);
        // TODO(pablo): release if goal node
        o->dominated_ = true;
      } else {
        ++it;
      }
    }

    dest_labels.insert(begin(dest_labels), new_label);
    return true;
  }

  bool is_goal(node const* n) {
    return std::find(begin(goals_), end(goals_), n) != end(goals_);
  }

  bool dominated_by_results(Label* label) {
    for (auto const& goal : goals_) {
      if (!dominated_by_results(label, node_labels_[goal])) {
        return false;
      }
    }
    return true;
  }

  inline bool dominated_by_results(Label* label,
                                   std::vector<Label*> const& results) {
    for (auto const result : results) {
      if (result->dominates(*label)) {
        return true;
      }
    }
    return false;
  }

  void create_start_label(edge const* e, bool fwd) {
    auto de = make_directed_edge(e, fwd);
    if (!de.allowed()) {
      return;
    }
    labels_.emplace_back(std::make_unique<Label>(de, nullptr));
    stats_.labels_created_++;
    stats_.start_labels_++;
    auto label = labels_.back().get();
    queue_.push(label);
    add_label_to_node(label);
  }

  node* add_node(node* input_node, input_pt const& pt) {
    assert(pt.input_ == input_node->location_);
    if (pt.in_area_ != nullptr) {
      return add_node_in_area(input_node, pt);
    }
    return add_node_near_edge(input_node, pt);
  }

  node* add_node_near_edge(node* input_node, input_pt const& pt) {
    auto node_on_edge = additional_.create_node(pt.nearest_pt_);

    // input_node -> node_on_edge
    additional_.connect(input_node, node_on_edge);

    // node_on_edge -> existing edge ("split edge")
    auto const nearest_edge = pt.nearest_edge_;
    node_on_edge->out_edges_.emplace_back(data::make_unique<edge>(
        make_edge(nearest_edge->info_, node_on_edge, nearest_edge->from_,
                  length(pt.from_path_), pt.from_path_, nearest_edge->side_)));
    node_on_edge->out_edges_.emplace_back(data::make_unique<edge>(
        make_edge(nearest_edge->info_, node_on_edge, nearest_edge->to_,
                  length(pt.to_path_), pt.to_path_, nearest_edge->side_)));
    additional_.edge_map_[nearest_edge->from_].emplace_back(
        node_on_edge->out_edges_[0].get());
    additional_.edge_map_[nearest_edge->to_].emplace_back(
        node_on_edge->out_edges_[1].get());

    return input_node;
  }

  node* add_node_in_area(node* input_node, input_pt const& pt) {
    auto n = create_area_node(input_node, pt, additional_);
    if (n == nullptr) {
      n = add_node_near_edge(input_node, pt);
    }
    return n;
  }

  std::priority_queue<Label*, std::vector<Label*>, compare_labels> queue_;
  std::vector<node const*> start_nodes_;
  std::vector<node const*> goals_;
  std::unordered_map<node const*, std::vector<Label*>> node_labels_;
  std::vector<std::unique_ptr<Label>> labels_;
  search_profile const& profile_;
  bool reverse_search_;
  additional_edges additional_;
  dijkstra_statistics stats_;
  std::size_t max_labels_{1024 * 1024 * 8};
};

}  // namespace ppr::routing
