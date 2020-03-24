#pragma once

#include <string>
#include <thread>

#include "boost/program_options.hpp"

#include "conf/configuration.h"

namespace ppr::benchmark {

class prog_options : public conf::configuration {
public:
  explicit prog_options() : configuration("Options") {
    param(graph_file_, "graph,g", "Routing graph");
    param(station_file_, "stations,s",
          "Public transport station file (dbkoord_geo.101)");
    param(spec_file_, "spec", "Spec file");
    param(bbox_, "bbox,b",
          "Bounding box for points/stations "
          "(bot_left_lon,bot_left_lat,top_right_lon,top_right_lat)");
    param(poly_file_, "poly", "Poly file for points/stations");
    param(threads_, "threads,t", "Number of threads");
    param(warmup_, "warmup", "Number of warmup queries (per thread)");
    param(queries_, "queries,n", "Number of queries (total)");
    param(verbose_, "verbose,v", "Verbose output");
    param(override_, "override", "Override existing output files");
  }

  std::string graph_file_{"routing-graph.ppr"};
  std::string station_file_;
  std::string spec_file_{"benchmark.toml"};
  std::string bbox_;
  std::string poly_file_;
  int warmup_{10};
  int queries_{1000};
  int threads_{static_cast<int>(std::thread::hardware_concurrency())};
  bool verbose_{false};
  bool override_{false};
};

}  // namespace ppr::benchmark
