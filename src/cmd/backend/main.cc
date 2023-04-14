#include <iostream>
#include <vector>

#include "boost/filesystem.hpp"

#include "conf/options_parser.h"

#include "ppr/backend/server.h"
#include "ppr/cmd/backend/prog_options.h"
#include "ppr/common/timing.h"
#include "ppr/common/verify.h"
#include "ppr/serialization/reader.h"

using namespace ppr;
using namespace ppr::backend;
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

  if (!boost::filesystem::exists(opt.graph_file_)) {
    std::cerr << "File not found: " << opt.graph_file_ << std::endl;
    return 1;
  }

  std::cout << "Loading routing graph..." << std::endl;
  auto const t_deserialize_start = timing_now();
  routing_graph rg;
  try {
    read_routing_graph(rg, opt.graph_file_);
  } catch (std::runtime_error const& err) {
    std::cout << "Deserialization failed: " << err.what() << std::endl;
    return 1;
  }
  auto const t_deserialize_duration = ms_since(t_deserialize_start);
  std::cout << "Deserialization: " << t_deserialize_duration << "ms"
            << std::endl;

  std::cout << "Routing graph: " << rg.data_->nodes_.size() << " nodes, "
            << rg.data_->areas_.size() << " areas" << std::endl;

  std::cout << "Preparing r-trees..." << std::endl;
  auto const t_rtrees_start = timing_now();
  auto const rtree_opt = opt.lock_rtrees_
                             ? rtree_options::LOCK
                             : (opt.prefetch_rtrees_ ? rtree_options::PREFETCH
                                                     : rtree_options::DEFAULT);
  rg.prepare_for_routing(opt.edge_rtree_max_size_, opt.area_rtree_max_size_,
                         rtree_opt);
  auto const t_rtrees_duration = ms_since(t_rtrees_start);
  std::cout << "R-trees: " << t_rtrees_duration << "ms" << std::endl;

  if (opt.verify_graph_) {
    std::cout << "Verifying routing graph file..." << std::endl;
    if (verify_graph(rg)) {
      std::cout << "Routing graph file appears to be valid." << std::endl;
    } else {
      std::cout << "Routing graph file is invalid!" << std::endl;
      return 2;
    }
  }

  // HTTP SERVER

  ppr_server(rg, opt.http_host_, opt.http_port_, opt.threads_,
             opt.static_file_path_, opt.cert_path_, opt.priv_key_path_,
             opt.dh_path_);

  return 0;
}
