#pragma once

#include <cassert>
#include <cstdint>
#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <vector>

#include "ppr/common/timing.h"

namespace ppr::preprocessing {

enum class pp_step {
  OSM_EXTRACT_RELATIONS,
  OSM_EXTRACT_MAIN,
  OSM_EXTRACT_AREAS,
  OSM_DEM,
  INT_PARALLEL_STREETS,
  INT_MOVE_CROSSINGS,
  INT_EDGES,
  INT_AREAS,
  RG_JUNCTIONS,
  RG_LINKED_CROSSINGS,
  RG_EDGES,
  RG_AREAS,
  RG_CROSSING_DETOURS,
  POST_GRAPH_VERIFICATION,
  POST_SERIALIZATION,
  POST_RTREES
};

struct step_progress_data {
  std::uint64_t max_;
  std::atomic<std::uint64_t> current_{0};

  explicit step_progress_data(std::uint64_t max = 1) : max_(max) {}

  ~step_progress_data() = default;

  step_progress_data(step_progress_data const& sp)
      : max_{sp.max_}, current_{sp.current_.load()} {}

  step_progress_data& operator=(step_progress_data const&) = delete;
  step_progress_data(step_progress_data&&) = delete;
  step_progress_data& operator=(step_progress_data&&) = delete;

  inline bool known() const { return max_ != 0; }
  inline bool unknown() const { return max_ == 0; }
  inline double progress() const {
    return current_ / static_cast<double>(max_);
  }

  inline void add(std::uint64_t offset = 1) { current_ += offset; }
  inline void set(std::uint64_t val) { current_ = val; }
  inline void set_max(std::uint64_t val) { max_ = val; }
  inline void finish() { current_ = max_; }
};

struct step_info {
  step_info(pp_step id, char const* name, double est_parent_percentage)
      : id_{id},
        name_{name},
        est_parent_percentage_{est_parent_percentage / 100.0} {}

  char const* name() const { return name_; };

  pp_step id_;
  char const* name_;
  double est_parent_percentage_;
  step_progress_data progress_;
  double duration_{};
};

struct step_progress;

struct logging {
  logging();

  step_info& get_step_info(pp_step step_id);
  double get_step_duration(pp_step step_id);
  std::vector<step_info> const& all_steps() const { return steps_; }

  std::size_t current_step() const {
    return static_cast<std::size_t>(current_step_);
  };

  std::size_t step_count() const { return steps_.size(); }

  double total_progress() const { return total_progress_; }

  void set_current_step(step_info const& step);
  void step_progress_updated(step_info const& step);
  void set_step_finished(step_info& step);

  inline std::ostream& out() const { return *out_; }

private:
  void update_total_progress();
  void publish_step_started(step_info const& step) const;
  void publish_step_progress(step_info const& step) const;
  void publish_step_finished(step_info const& step) const;

public:
  std::function<void(logging const&, step_info const&)> step_started_;
  std::function<void(logging const&, step_info const&)> step_progress_;
  std::function<void(logging const&, step_info const&)> step_finished_;
  std::ostream* out_{&std::clog};
  bool total_progress_updates_only_{true};

private:
  std::vector<step_info> steps_;
  pp_step current_step_{};
  double previous_progress_{};
  double total_progress_{};
  std::mutex mutex_;
};

struct step_progress {
  step_progress(logging& log, pp_step step_id, std::uint64_t max = 1);
  ~step_progress();

  step_progress(step_progress const&) = delete;
  step_progress& operator=(step_progress const&) = delete;
  step_progress(step_progress&&) = delete;
  step_progress& operator=(step_progress&&) = delete;

  void add(std::uint64_t offset = 1);
  void set(std::uint64_t val);
  void set_max(std::uint64_t val);

  logging& log_;
  step_info& step_;
  std::chrono::time_point<std::chrono::steady_clock> started_at_;
};

}  // namespace ppr::preprocessing
