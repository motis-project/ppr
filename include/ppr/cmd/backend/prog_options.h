#pragma once

#include <string>
#include <thread>

#include "boost/program_options.hpp"

#include "conf/configuration.h"

namespace ppr::backend {

class prog_options : public conf::configuration {
public:
  explicit prog_options() : configuration("Options") {
    param(graph_file_, "graph,g", "Routing graph file");
    param(http_host_, "host", "HTTP host");
    param(http_port_, "port", "HTTP port");
    param(cert_path_, "cert",
          "path to certificate or ::dev:: for self-signed certificate");
    param(priv_key_path_, "privkey",
          "path to private key or ::dev:: for self-signed certificate");
    param(dh_path_, "dhparam",
          "path to dh parameters file or ::dev:: for hardcoded");
    param(static_file_path_, "static", "Path to static files (ui/web)");
    param(threads_, "routing-threads", "Number of routing threads");
    param(edge_rtree_max_size_, "edge-rtree-max-size",
          "Maximum size for edge r-tree file");
    param(area_rtree_max_size_, "area-rtree-max-size",
          "Maximum size for area r-tree file");
    param(lock_rtrees_, "lock-rtrees", "Prefetch and lock r-trees in memory");
    param(prefetch_rtrees_, "prefetch-rtrees", "Prefetch r-trees");
    param(verify_graph_, "verify-graph", "Verify routing graph file");
  }

  std::string graph_file_{"routing-graph.ppr"};
  std::string http_host_{"0.0.0.0"};
  std::string http_port_{"8000"};
  std::string cert_path_{"::dev::"};
  std::string priv_key_path_{"::dev::"};
  std::string dh_path_{"::dev::"};
  std::string static_file_path_;
  int threads_{static_cast<int>(std::thread::hardware_concurrency())};
  std::size_t edge_rtree_max_size_{1024UL * 1024 * 1024 * 3};
  std::size_t area_rtree_max_size_{1024UL * 1024 * 1024};
  bool lock_rtrees_{false};
  bool prefetch_rtrees_{false};
  bool verify_graph_{true};
};

}  // namespace ppr::backend
