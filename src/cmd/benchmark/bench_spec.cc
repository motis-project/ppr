#include <sstream>
#include <unordered_map>

#include "boost/algorithm/string/predicate.hpp"
#include "boost/filesystem.hpp"
#include "cpptoml.h"

#include "ppr/cmd/benchmark/bench_spec.h"
#include "ppr/profiles/parse_search_profile.h"

using namespace ppr;
using namespace ppr::routing;

namespace ppr::benchmark {

namespace {

named_profile load_search_profile(std::string const& filename) {
  std::ifstream f(filename);
  if (!f) {
    std::cerr << "ERROR: Search profile not found: " << filename << std::endl;
    return {};
  }
  std::stringstream ss;
  ss << f.rdbuf();
  auto name = boost::filesystem::path(filename).stem().string();
  if (boost::starts_with(name, "sp_")) {
    name = name.substr(3);
  }
  return {name, ppr::profiles::parse_search_profile(ss.str())};
}

named_profile get_profile(
    std::string const& filename,
    std::unordered_map<std::string, named_profile>& profiles) {
  if (profiles.find(filename) == std::end(profiles)) {
    profiles[filename] = load_search_profile(filename);
  }
  return profiles[filename];
}

void parse_start_generation_mode(bench_spec& spec,
                                 cpptoml::option<std::string> const& val) {
  if (val) {
    if (*val == "rnd") {
      spec.start_mode_ = start_generation_mode::RANDOM;
    } else if (*val == "areas") {
      spec.start_mode_ = start_generation_mode::AREAS;
    }
  }
}

void parse_destination_generation_mode(
    bench_spec& spec, cpptoml::option<std::string> const& val) {
  if (val) {
    if (*val == "rnd") {
      spec.dest_mode_ = destination_generation_mode::RANDOM;
    } else if (*val == "stations") {
      spec.dest_mode_ = destination_generation_mode::STATIONS;
    }
  }
}

void parse_search_direction_mode(bench_spec& spec,
                                 cpptoml::option<std::string> const& val) {
  if (val) {
    if (*val == "rnd") {
      spec.direction_ = search_direction_mode::RANDOM;
    } else if (*val == "fwd") {
      spec.direction_ = search_direction_mode::FWD;
    } else if (*val == "bwd") {
      spec.direction_ = search_direction_mode::BWD;
    }
  }
}

std::string default_stat_filename(bench_spec const& spec,
                                  std::string const& map_name) {
  std::stringstream ss;
  ss << "stats_" << map_name;
  if (spec.start_mode_ == start_generation_mode::AREAS) {
    ss << "_areas";
  }
  switch (spec.dest_mode_) {
    case destination_generation_mode::RANDOM:
      ss << "_1to" << spec.destination_count_;
      break;
    case destination_generation_mode::STATIONS: ss << "_stations"; break;
  }
  switch (spec.direction_) {
    case search_direction_mode::RANDOM: ss << "_rnd"; break;
    case search_direction_mode::FWD: ss << "_fwd"; break;
    case search_direction_mode::BWD: ss << "_bwd"; break;
  }

  std::string profile_name = "multi";
  if (spec.profiles_.size() == 1) {
    profile_name = spec.profiles_.front().name_;
  }

  ss << "_" << profile_name;
  ss << "_d"
     << static_cast<int>(spec.profiles_.front().profile_.duration_limit_ / 60);
  ss << "_r" << static_cast<int>(spec.radius_);
  ss << "_rf" << static_cast<int>(spec.radius_factor_ * 100);
  if (spec.eval_radius_factor_) {
    ss << "_evalrf";
  }
  if (spec.ignore_unreachable_) {
    ss << "_igur";
  }
  ss << ".csv";
  return ss.str();
}

void parse_spec(bench_spec& spec, std::shared_ptr<cpptoml::table> const& bench,
                std::unordered_map<std::string, named_profile>& profiles,
                std::string const& map_name) {
  if (!bench) {
    return;
  }

  auto profile_file = bench->get_as<std::string>("profile");
  if (profile_file) {
    spec.profiles_.clear();
    spec.profiles_.emplace_back(get_profile(*profile_file, profiles));
  } else {
    auto profile_files = bench->get_array_of<std::string>("profile");
    if (profile_files) {
      spec.profiles_.clear();
      spec.profiles_.reserve(profile_files->size());
      for (auto const& file : *profile_files) {
        spec.profiles_.emplace_back(get_profile(file, profiles));
      }
    }
  }

  if (spec.profiles_.empty()) {
    spec.profiles_.emplace_back();
  }

  auto override_duration_limit = bench->get_as<double>("duration_limit");
  if (override_duration_limit) {
    for (auto& profile : spec.profiles_) {
      profile.profile_.duration_limit_ = (*override_duration_limit) * 60;
    }
  }

  auto const& first_profile = spec.profiles_.front().profile_;
  spec.full_radius_ =
      first_profile.duration_limit_ * first_profile.walking_speed_;

  auto radius = bench->get_as<double>("radius");
  auto radius_factor = bench->get_as<double>("radius_factor");

  if (radius) {
    spec.radius_ = *radius;
  } else if (radius_factor) {
    spec.radius_factor_ = *radius_factor;
    spec.radius_ = spec.full_radius_ * spec.radius_factor_;
  } else if (spec.radius_ == 0.0) {
    spec.radius_ = spec.full_radius_;
  }

  spec.eval_radius_factor_ = bench->get_as<bool>("eval_radius_factor")
                                 .value_or(spec.eval_radius_factor_);

  spec.ignore_unreachable_ = bench->get_as<bool>("ignore_unreachable")
                                 .value_or(spec.ignore_unreachable_);
  parse_start_generation_mode(spec, bench->get_as<std::string>("start"));
  parse_destination_generation_mode(spec, bench->get_as<std::string>("mode"));
  parse_search_direction_mode(spec, bench->get_as<std::string>("direction"));
  spec.destination_count_ =
      bench->get_as<int>("destinations").value_or(spec.destination_count_);

  auto stats_file = bench->get_as<std::string>("stats");
  if (stats_file) {
    spec.stats_file_ = *stats_file;
  } else {
    spec.stats_file_ = default_stat_filename(spec, map_name);
  }
}

}  // namespace

std::vector<bench_spec> read_bench_specs(std::string const& filename,
                                         std::string const& map_name) {
  std::vector<bench_spec> specs;
  std::unordered_map<std::string, named_profile> profiles;
  bench_spec default_spec;

  auto config = cpptoml::parse_file(filename);
  parse_spec(default_spec, config->get_table("default"), profiles, map_name);
  auto benchs = config->get_table_array("bench");
  for (auto const& bench : *benchs) {
    if (bench->get_as<bool>("ignore").value_or(false)) {
      continue;
    }
    bench_spec spec{default_spec};
    parse_spec(spec, bench, profiles, map_name);
    specs.push_back(spec);
  }

  return specs;
}

}  // namespace ppr::benchmark
