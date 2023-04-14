#include <iostream>
#include <vector>

#include "boost/filesystem.hpp"

#include "conf/options_parser.h"

#include "ppr/cmd/benchmark/bench_spec.h"
#include "ppr/cmd/benchmark/benchmark.h"
#include "ppr/cmd/benchmark/parse_bbox.h"
#include "ppr/cmd/benchmark/parse_poly.h"
#include "ppr/cmd/benchmark/prog_options.h"
#include "ppr/cmd/benchmark/stations.h"
#include "ppr/serialization/reader.h"

using namespace ppr;
using namespace ppr::benchmark;
using namespace ppr::serialization;

std::unique_ptr<bounds> parse_bounds(prog_options const& opt) {
  if (!opt.bbox_.empty()) {
    return parse_bbox(opt.bbox_);
  } else if (!opt.poly_file_.empty()) {
    return parse_poly(opt.poly_file_);
  } else {
    return nullptr;
  }
}

int main(int argc, char const* argv[]) {
  std::cout.precision(12);
  std::cerr.precision(12);
  prog_options opt;
  conf::options_parser parser({&opt});
  parser.read_command_line_args(argc, argv);

  if (parser.help()) {
    parser.print_help(std::cout);
    return 0;
  } else if (parser.version()) {
    return 0;
  }

  parser.read_configuration_file();

  parser.print_unrecognized(std::cout);
  parser.print_used(std::cout);

  if (!boost::filesystem::exists(opt.graph_file_)) {
    std::cerr << "File not found: " << opt.graph_file_ << std::endl;
    return 1;
  }
  if (!opt.station_file_.empty() &&
      !boost::filesystem::exists(opt.station_file_)) {
    std::cerr << "File not found: " << opt.station_file_ << std::endl;
    return 1;
  }

  auto bds = parse_bounds(opt);
  if (!bds) {
    std::cout << "Warning: No bounding box/polygon specified" << std::endl;
    bds = std::make_unique<bbox>();
  }

  if (!boost::filesystem::exists(opt.spec_file_)) {
    std::cerr << "File not found: " << opt.spec_file_ << std::endl;
    return 1;
  }

  auto const graph_path = boost::filesystem::path{opt.graph_file_};
  auto const map_name = graph_path.stem().string();

  auto specs = read_bench_specs(opt.spec_file_, map_name);

  routing_graph rg;
  std::cout << "Loading routing graph..." << std::endl;
  read_routing_graph(rg, opt.graph_file_);

  std::cout << "Routing graph: " << rg.data_->nodes_.size() << " nodes, "
            << rg.data_->areas_.size() << " areas" << std::endl;

  std::cout << "Creating r-trees..." << std::endl;
  rg.prepare_for_routing();

  stations st;
  if (!opt.station_file_.empty()) {
    std::cout << "Loading stations..." << std::endl;
    if (!st.load(opt.station_file_, *bds)) {
      std::cerr << "Loading stations failed" << std::endl;
      return 2;
    }
  }

  std::cout << std::endl;
  std::cout << "Running " << specs.size() << " benchmarks...\n" << std::endl;

  auto i = 0U;
  for (auto const& spec : specs) {
    if (boost::filesystem::exists(spec.stats_file_)) {
      if (opt.override_) {
        std::cout << "WARNING: " << spec.stats_file_
                  << " already exists, overriding..." << std::endl;
      } else {
        std::cout << spec.stats_file_ << " already exists, skipping benchmark"
                  << std::endl;
        continue;
      }
    }
    run_benchmark(rg, opt, st, *bds, spec);
    ++i;

    std::cout << i << "/" << specs.size() << " benchmarks complete\n"
              << std::endl;
  }

  std::cout << "\nAll benchmarks complete!" << std::endl;

  return 0;
}
