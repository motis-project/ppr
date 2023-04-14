#include "ppr/preprocessing/logging.h"

namespace ppr::preprocessing {

logging::logging()
    : steps_{
          {pp_step::OSM_EXTRACT_RELATIONS, "OSM Extract: Relations", 2},
          {pp_step::OSM_EXTRACT_MAIN, "OSM Extract: Nodes + Edges", 22},
          {pp_step::OSM_EXTRACT_AREAS, "OSM Extract: Areas", 30},
          {pp_step::OSM_DEM, "Elevation data", 4},
          {pp_step::INT_PARALLEL_STREETS, "Parallel Street Detection", 2},
          {pp_step::INT_MOVE_CROSSINGS, "Moving Crossings", 1},
          {pp_step::INT_EDGES, "Edge Compression", 4},
          {pp_step::INT_AREAS, "Area Transformation", 0},
          {pp_step::RG_JUNCTIONS, "Junctions", 10},
          {pp_step::RG_LINKED_CROSSINGS, "Linked Street Crossings", 1},
          {pp_step::RG_EDGES, "Edge Creation", 3},
          {pp_step::RG_AREAS, "Area Creation", 0},
          {pp_step::RG_CROSSING_DETOURS, "Crossing Detours", 5},
          {pp_step::POST_GRAPH_VERIFICATION, "Graph Verification", 0},
          {pp_step::POST_SERIALIZATION, "Graph Serialization", 7},
          {pp_step::POST_RTREES, "R-Tree Generation", 9},
      } {}

step_info& logging::get_step_info(pp_step step_id) {
  return steps_.at(static_cast<std::size_t>(step_id));
}

double logging::get_step_duration(pp_step step_id) {
  return get_step_info(step_id).duration_;
}

void logging::set_current_step(step_info const& step) {
  auto const guard = std::lock_guard{mutex_};
  current_step_ = step.id_;

  auto progress = 0.0;
  for (auto i = 0ULL; i < static_cast<std::size_t>(current_step_); ++i) {
    progress += steps_.at(i).est_parent_percentage_;
  }
  previous_progress_ = progress;
  total_progress_ = previous_progress_;

  publish_step_started(step);
}

void logging::step_progress_updated(step_info const& step) {
  if (current_step_ != step.id_) {
    return;
  }
  auto const guard = std::lock_guard{mutex_};
  update_total_progress();

  publish_step_progress(step);
}

void logging::set_step_finished(step_info& step) {
  step.progress_.finish();
  update_total_progress();

  publish_step_progress(step);
  publish_step_finished(step);
}

void logging::publish_step_started(const step_info& step) const {
  auto const& handler = step_started_;
  if (handler) {
    handler(*this, step);
  }
}

void logging::publish_step_progress(const step_info& step) const {
  auto const& handler = step_progress_;
  if (handler) {
    handler(*this, step);
  }
}

void logging::publish_step_finished(const step_info& step) const {
  auto const& handler = step_finished_;
  if (handler) {
    handler(*this, step);
  }
}

void logging::update_total_progress() {
  auto const& step = get_step_info(current_step_);
  total_progress_ = previous_progress_ +
                    step.progress_.progress() * step.est_parent_percentage_;
}

struct check_progress_update {
  explicit check_progress_update(step_progress& sp)
      : sp_{sp}, old_progress_{sp.step_.progress_.progress()} {}

  ~check_progress_update() {
    auto const new_progress = sp_.step_.progress_.progress();

    if (sp_.log_.total_progress_updates_only_) {
      auto const old_total = static_cast<int>(
          old_progress_ * sp_.step_.est_parent_percentage_ * 100);
      auto const new_total = static_cast<int>(
          new_progress * sp_.step_.est_parent_percentage_ * 100);
      if (old_total != new_total) {
        sp_.log_.step_progress_updated(sp_.step_);
      }
    } else {
      auto const old_percentage = static_cast<int>(old_progress_ * 100);
      auto const new_percentage = static_cast<int>(new_progress * 100);
      if (old_percentage != new_percentage) {
        sp_.log_.step_progress_updated(sp_.step_);
      }
    }
  }

  check_progress_update(check_progress_update const&) = delete;
  check_progress_update& operator=(check_progress_update const&) = delete;
  check_progress_update(check_progress_update&&) = delete;
  check_progress_update& operator=(check_progress_update&&) = delete;

  step_progress& sp_;
  double old_progress_;
};

step_progress::step_progress(logging& log, pp_step step_id, std::uint64_t max)
    : log_(log), step_(log.get_step_info(step_id)), started_at_(timing_now()) {
  step_.progress_.set_max(max);
  log_.set_current_step(step_);
  log_.step_progress_updated(step_);
}

step_progress::~step_progress() {
  auto const finished_at = timing_now();
  step_.duration_ = ms_between(started_at_, finished_at);
  log_.set_step_finished(step_);
}

void step_progress::add(std::uint64_t offset) {
  auto const update = check_progress_update{*this};
  step_.progress_.add(offset);
}

void step_progress::set(std::uint64_t val) {
  auto const update = check_progress_update{*this};
  step_.progress_.set(val);
}

void step_progress::set_max(std::uint64_t val) {
  auto const update = check_progress_update{*this};
  step_.progress_.set_max(val);
}

}  // namespace ppr::preprocessing
