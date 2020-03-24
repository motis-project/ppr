#include <cassert>
#include <algorithm>
#include <numeric>
#include <string>

#include "ppr/cmd/benchmark/stats_writer.h"

using namespace ppr::routing;

namespace ppr::benchmark {

stats_writer::stats_writer(std::string const& filename)
    : csv_(filename), lines_(0) {
  using namespace std::string_literals;
  csv_ << "destinations"
       << "direction"
       << "max_dist"
       << "radius_factor"
       << "profile"
       << "attempts"
       << "routes_total"
       << "destinations_reached"
       << "base_destinations"
       << "base_destinations_reached"
       << "base_routes_total"
       << "d_total"
       << "d_start_pts"
       << "d_start_pts_extended"
       << "d_destination_pts"
       << "d_destination_pts_extended"
       << "d_postprocessing"
       << "max_route_distance"
       << "max_route_duration"
       << "max_route_accessibility";

  for (auto const& prefix : {"f_"s, "i1_"s, "i2_"s, "i3_"s, "i4_"s}) {
    csv_ << prefix + "d_total" << prefix + "d_starts" << prefix + "d_goals"
         << prefix + "d_area_edges" << prefix + "d_search"
         << prefix + "d_labels_to_route" << prefix + "labels_created"
         << prefix + "labels_popped" << prefix + "start_labels"
         << prefix + "additional_nodes" << prefix + "additional_edges"
         << prefix + "additional_areas" << prefix + "goals"
         << prefix + "goals_reached";
  }

  csv_ << end_row;
}

void write_dijkstra_stats(csv_writer& csv, dijkstra_statistics const& ds) {
  csv << ds.d_total_ << ds.d_starts_ << ds.d_goals_ << ds.d_area_edges_
      << ds.d_search_ << ds.d_labels_to_route_ << ds.labels_created_
      << ds.labels_popped_ << ds.start_labels_ << ds.additional_nodes_
      << ds.additional_edges_ << ds.additional_areas_ << ds.goals_
      << ds.goals_reached_;
}

void stats_writer::write(routing_query const& query,
                         search_result const& result) {
  auto const& s = result.stats_;
  assert(!s.dijkstra_statistics_.empty() && s.dijkstra_statistics_.size() <= 4);

  csv_ << query.destinations_.size()
       << (query.direction_ == search_direction::FWD ? "F" : "B")
       << query.max_dist_ << query.radius_factor_ << query.profile_.name_
       << s.attempts_ << result.total_route_count()
       << result.destinations_reached();

  if (query.base_query_ != nullptr && query.base_result_ != nullptr) {
    csv_ << query.base_query_->destinations_.size()
         << query.base_result_->destinations_reached()
         << query.base_result_->total_route_count();
  } else {
    csv_ << 0 << 0 << 0;
  }

  csv_ << s.d_total_ << s.d_start_pts_ << s.d_start_pts_extended_
       << s.d_destination_pts_ << s.d_destination_pts_extended_
       << s.d_postprocessing_;

  double max_route_distance = 0;
  double max_route_duration = 0;
  double max_route_accessibility = 0;
  for (auto const& routes : result.routes_) {
    for (auto const& route : routes) {
      max_route_distance = std::max(max_route_distance, route.distance_);
      max_route_duration = std::max(max_route_duration, route.duration_);
      max_route_accessibility =
          std::max(max_route_accessibility, route.accessibility_);
    }
  }

  csv_ << max_route_distance << max_route_duration << max_route_accessibility;

  // final iteration
  write_dijkstra_stats(csv_, s.dijkstra_statistics_.back());

  // iteration 1-4
  for (auto const& ds : s.dijkstra_statistics_) {
    write_dijkstra_stats(csv_, ds);
  }
  for (int i = 0; i < 4 - static_cast<int>(s.dijkstra_statistics_.size());
       i++) {
    write_dijkstra_stats(csv_, {});
  }

  csv_ << end_row;
  if ((++lines_ % 100) == 0) {
    csv_.flush();
  }
}

}  // namespace ppr::benchmark
