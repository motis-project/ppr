#pragma once

#include <string>
#include <vector>

#include "ppr/cmd/benchmark/bounds.h"
#include "ppr/cmd/benchmark/query_generator.h"
#include "ppr/routing/search_profile.h"

namespace ppr::benchmark {

struct bench_spec {
  std::vector<named_profile> profiles_;
  double radius_ = 0;
  double radius_factor_ = 1.0;
  bool eval_radius_factor_ = false;
  bool ignore_unreachable_ = false;
  start_generation_mode start_mode_ = start_generation_mode::RANDOM;
  destination_generation_mode dest_mode_ = destination_generation_mode::RANDOM;
  search_direction_mode direction_ = search_direction_mode::RANDOM;
  int destination_count_ = 1;
  std::string stats_file_;

  // automatically calculated
  double full_radius_ = 0;
};

std::vector<bench_spec> read_bench_specs(std::string const& filename,
                                         std::string const& map_name);

}  // namespace ppr::benchmark
