#include "ppr/preprocessing/osm_graph/elevation.h"

#include <cmath>

using namespace ppr::preprocessing::elevation;

namespace ppr::preprocessing {

void sample_elevation(osm_edge& edge, elevation::dem_source& dem,
                      double sampling_interval, elevation_statistics& stats) {
  assert(edge.elevation_down_ == 0);
  assert(edge.elevation_up_ == 0);
  auto start_pt = edge.from_->location_;
  auto end_pt = edge.to_->location_;
  auto const scaled_interval = sampling_interval * scale_factor(start_pt);
  auto step = end_pt - start_pt;
  step.normalize();
  step *= scaled_interval;

  auto last_value = edge.from_->elevation_;
  auto const update_elevation = [&](elevation_t value) {
    if (value >= last_value) {
      edge.elevation_up_ += (value - last_value);
    } else {
      edge.elevation_down_ += (last_value - value);
    }
    last_value = value;
  };
  auto pt = start_pt;
  auto const samples =
      static_cast<int>(std::floor(edge.distance_ / sampling_interval));
  stats.n_queries_ += static_cast<std::size_t>(samples - 1);
  for (auto i = 1; i < samples; i++) {
    pt += step;
    auto const value = dem.get(to_location(pt));
    if (value != NO_ELEVATION_DATA) {
      update_elevation(value);
    } else {
      stats.n_misses_++;
    }
  }
  update_elevation(edge.to_->elevation_);
}

void add_elevation_data(osm_graph& og, elevation::dem_source& dem,
                        double sampling_interval, elevation_statistics& stats) {
  stats.n_queries_ += og.nodes_.size();
  for (auto& node : og.nodes_) {
    auto const val = dem.get(to_location(node->location_));
    node->elevation_ = val;
    if (val == NO_ELEVATION_DATA) {
      stats.n_misses_++;
    }
  }
  auto const use_sampling = sampling_interval > 0;
  for (auto& node : og.nodes_) {
    for (auto& edge : node->out_edges_) {
      if (!edge.from_->has_elevation_data() ||
          !edge.to_->has_elevation_data() || !edge.calculate_elevation()) {
        continue;
      }
      if (use_sampling && edge.distance_ > sampling_interval) {
        sample_elevation(edge, dem, sampling_interval, stats);
      } else {
        auto const diff = edge.to_->elevation_ - edge.from_->elevation_;
        if (diff >= 0) {
          edge.elevation_up_ = static_cast<elevation_diff_t>(diff);
        } else {
          edge.elevation_down_ = static_cast<elevation_diff_t>(-diff);
        }
      }
    }
  }
}

}  // namespace ppr::preprocessing
