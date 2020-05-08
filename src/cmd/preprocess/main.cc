#include <iostream>

#include "boost/filesystem.hpp"

#include "conf/options_parser.h"

#include "ppr/cmd/preprocess/prog_options.h"
#include "ppr/common/timing.h"
#include "ppr/common/verify.h"
#include "ppr/preprocessing/preprocessing.h"
#include "ppr/preprocessing/statistics.h"
#include "ppr/preprocessing/stats_writer.h"
#include "ppr/serialization/reader.h"
#include "ppr/serialization/writer.h"

using namespace ppr;
using namespace ppr::preprocessing;
using namespace ppr::serialization;

namespace fs = boost::filesystem;

void write_stats(prog_options const& opt, statistics const& stats) {
  fs::path p = opt.graph_file_;
  p.replace_extension(".stats.csv");
  auto const filename = p.string();
  std::clog << "Writing statistics to: " << filename << std::endl;
  write_stats(stats, filename);
}

int main(int argc, char const* argv[]) {
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

  if (!boost::filesystem::exists(opt.osm_file_)) {
    std::clog << "File not found: " << opt.osm_file_ << std::endl;
    return 1;
  }

  statistics stats;

  {
    auto const t_start = timing_now();
    routing_graph rg = build_routing_graph(opt.get_options(), stats);
    auto const t_after_build = timing_now();
    stats.d_total_pp_ = ms_between(t_start, t_after_build);

    std::clog << "Verifying generated routing graph..." << std::endl;
    if (verify_graph(rg)) {
      std::clog << "Generated routing graph appears to be valid." << std::endl;
    } else {
      std::clog << "Generated routing graph is invalid!" << std::endl;
      return 2;
    }

    std::clog << "Serializing routing graph..." << std::endl;
    rg.filename_ = opt.graph_file_;
    write_routing_graph(rg, opt.graph_file_, stats);
    auto const t_after_write = timing_now();
    stats.d_serialization_ = ms_between(t_after_build, t_after_write);

    if (opt.create_rtrees_) {
      std::clog << "Creating r-trees..." << std::endl;
      fs::remove(fs::path(rg.filename_ + ".ert"));
      fs::remove(fs::path(rg.filename_ + ".art"));
      rg.prepare_for_routing(opt.edge_rtree_max_size_,
                             opt.area_rtree_max_size_);
      auto const t_after_rtrees = timing_now();
      stats.d_rtrees_ = ms_between(t_after_write, t_after_rtrees);
    }

    print_timing("Preprocessing", stats.d_total_pp_);
    print_timing("Serialization", stats.d_serialization_);
    print_timing("R-trees", stats.d_rtrees_);

    write_stats(opt, stats);
  }

  if (opt.verify_graph_) {
    std::clog << "Verifying routing graph file..." << std::endl;
    routing_graph rg;
    read_routing_graph(rg, opt.graph_file_);
    if (verify_graph(rg)) {
      std::clog << "Routing graph file appears to be valid." << std::endl;
    } else {
      std::clog << "Routing graph file is invalid!" << std::endl;
      return 3;
    }
  }

  std::clog << "Done!" << std::endl;

  return 0;
}
