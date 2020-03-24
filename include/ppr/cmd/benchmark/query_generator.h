#pragma once

#include <iostream>
#include <random>

#include "ppr/cmd/benchmark/bounds.h"
#include "ppr/cmd/benchmark/query.h"
#include "ppr/cmd/benchmark/stations.h"

namespace ppr::benchmark {

enum class start_generation_mode { RANDOM, AREAS };
enum class destination_generation_mode { RANDOM, STATIONS };
enum class search_direction_mode { RANDOM, FWD, BWD };

struct query_generator {
  query_generator(routing_graph const& rg, bounds& a, stations& s,
                  named_profile const& profile)
      : rg_(rg),
        area_(a),
        stations_(s),
        profile_(profile),
        radius_(1000),
        radius_factor_(1.0),
        full_radius_(profile.profile_.walking_speed_ *
                     profile.profile_.duration_limit_),
        start_mode_(start_generation_mode::RANDOM),
        dest_mode_(destination_generation_mode::RANDOM),
        direction_(search_direction_mode::RANDOM),
        destination_count_(1),
        area_dist_(0, static_cast<int>(rg.data_->areas_.size() - 1)) {
    std::random_device rd;
    mt_.seed(rd());
  }

  routing_query generate_query();
  routing_query with_radius_factor(routing_query const& base, double rf);

private:
  location random_point_near(location const& ref, double max_dist);
  location random_point_in_area();
  void generate_start_point(routing_query& query);
  void generate_destination_points(routing_query& query, double radius);

public:
  routing_graph const& rg_;
  bounds& area_;
  stations& stations_;
  named_profile const& profile_;
  double radius_;
  double radius_factor_;
  double full_radius_;
  start_generation_mode start_mode_;
  destination_generation_mode dest_mode_;
  search_direction_mode direction_;
  int destination_count_;

private:
  std::mt19937 mt_;
  std::uniform_real_distribution<double> real_dist_;
  std::uniform_int_distribution<> area_dist_;
};

inline std::ostream& operator<<(std::ostream& os,
                                start_generation_mode const mode) {
  switch (mode) {
    case start_generation_mode::RANDOM: os << "rnd"; break;
    case start_generation_mode::AREAS: os << "areas"; break;
  }
  return os;
}

inline std::ostream& operator<<(std::ostream& os,
                                destination_generation_mode const mode) {
  switch (mode) {
    case destination_generation_mode::RANDOM: os << "rnd"; break;
    case destination_generation_mode::STATIONS: os << "stations"; break;
  }
  return os;
}

inline std::ostream& operator<<(std::ostream& os,
                                search_direction_mode const mode) {
  switch (mode) {
    case search_direction_mode::RANDOM: os << "rnd"; break;
    case search_direction_mode::FWD: os << "fwd"; break;
    case search_direction_mode::BWD: os << "bwd"; break;
  }
  return os;
}

}  // namespace ppr::benchmark
