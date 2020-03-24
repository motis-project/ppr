#include "boost/geometry/algorithms/covered_by.hpp"
#include "boost/geometry/algorithms/envelope.hpp"
#include "boost/geometry/geometries/geometries.hpp"

#include "ppr/cmd/benchmark/query_generator.h"
#include "ppr/common/geometry/merc.h"
#include "ppr/common/math.h"

namespace bg = boost::geometry;

using namespace ppr;
using namespace ppr::routing;

namespace ppr::benchmark {

location query_generator::random_point_near(location const& ref,
                                            double max_dist) {
  auto const ref_merc = to_merc(ref);
  location pt;
  do {
    // http://mathworld.wolfram.com/DiskPointPicking.html
    double radius =
        std::sqrt(real_dist_(mt_)) * (max_dist / scale_factor(ref_merc));
    double angle = real_dist_(mt_) * 2 * PI;
    auto const pt_merc =
        ref_merc + merc{radius * std::cos(angle), radius * std::sin(angle)};
    pt = to_location(pt_merc);
  } while (!area_.contains(pt));
  return pt;
}

location query_generator::random_point_in_area() {
  if (rg_.data_->areas_.empty()) {
    return area_.random_pt();
  }

  auto const& a = rg_.data_->areas_[static_cast<std::size_t>(area_dist_(mt_))];
  auto const box = bg::return_envelope<bg::model::box<location>>(a.polygon_);
  std::uniform_real_distribution<double> lon_dist(box.min_corner().lon(),
                                                  box.max_corner().lon());
  std::uniform_real_distribution<double> lat_dist(box.min_corner().lat(),
                                                  box.max_corner().lat());
  location pt;
  do {
    pt = make_location(lon_dist(mt_), lat_dist(mt_));
  } while (!bg::within(pt, a.polygon_));
  return pt;
}

void query_generator::generate_start_point(routing_query& query) {
  if (start_mode_ == start_generation_mode::AREAS) {
    query.start_ = random_point_in_area();
  } else {
    query.start_ = area_.random_pt();
  }
}

void query_generator::generate_destination_points(routing_query& query,
                                                  double radius) {
  if (dest_mode_ == destination_generation_mode::RANDOM) {
    for (int i = 0; i < destination_count_; i++) {
      query.destinations_.push_back(random_point_near(query.start_, radius));
    }
  } else if (dest_mode_ == destination_generation_mode::STATIONS) {
    query.destinations_ = stations_.stations_near(query.start_, radius);
  }

  query.max_dist_ = 0;
  for (auto const& dest : query.destinations_) {
    auto const dist = distance(query.start_, dest);
    if (dist > query.max_dist_) {
      query.max_dist_ = dist;
    }
  }
}

routing_query query_generator::generate_query() {
  routing_query query(profile_);

  switch (direction_) {
    case search_direction_mode::RANDOM:
      if (real_dist_(mt_) > 0.5) {
        query.direction_ = search_direction::BWD;
      }
      break;
    case search_direction_mode::FWD: break;
    case search_direction_mode::BWD:
      query.direction_ = search_direction::BWD;
      break;
  }

  query.radius_factor_ = radius_factor_;

  while (query.destinations_.empty()) {
    generate_start_point(query);
    generate_destination_points(query, radius_);
  }

  return query;
}

routing_query query_generator::with_radius_factor(routing_query const& base,
                                                  double rf) {
  routing_query query(base.profile_);

  query.direction_ = base.direction_;
  query.radius_factor_ = rf;
  query.allow_expansion_ = base.allow_expansion_;
  query.start_ = base.start_;
  generate_destination_points(query, full_radius_ * rf);

  return query;
}

}  // namespace ppr::benchmark
