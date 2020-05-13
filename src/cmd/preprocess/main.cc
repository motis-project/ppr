#include <cmath>
#include <iostream>
#include <numeric>

#include "conf/options_parser.h"

#include "ppr/cmd/preprocess/prog_options.h"
#include "ppr/common/timing.h"
#include "ppr/common/verify.h"
#include "ppr/preprocessing/default_logging.h"
#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/preprocessing.h"
#include "ppr/serialization/reader.h"

using namespace ppr;
using namespace ppr::preprocessing;
using namespace ppr::serialization;

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

  logging log;
  default_logging default_log{log};

  auto result = create_routing_data(opt.get_options(), log);
  if (!result.successful()) {
    return 1;
  }

  default_log.flush();
  log.out_ = &std::clog;

  if (opt.verify_graph_) {
    log.out() << "Verifying routing graph file..." << std::endl;
    routing_graph rg;
    read_routing_graph(result.rg_, opt.graph_file_);
    if (verify_graph(result.rg_, log.out())) {
      log.out() << "Routing graph file appears to be valid." << std::endl;
    } else {
      log.out() << "Routing graph file is invalid!" << std::endl;
      return 3;
    }
  }

  if (opt.print_timing_overview_) {
    log.out() << "\nTimings:\n";
    auto const total_steps_duration = std::accumulate(
        begin(log.all_steps()), end(log.all_steps()), 0.0,
        [](auto const sum, auto const& step) { return sum + step.duration_; });
    for (auto const& step : log.all_steps()) {
      log.out() << std::setw(3)
                << static_cast<int>(
                       std::round(step.duration_ / total_steps_duration * 100))
                << "%  " << std::setw(10) << static_cast<int>(step.duration_)
                << "ms  " << step.name() << std::endl;
    }
  }

  log.out() << "\nDone!" << std::endl;

  return 0;
}
