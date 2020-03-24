#pragma once

#include <string>

#include "ppr/common/routing_graph.h"

namespace ppr::backend {

void ppr_server(routing_graph const& rg, std::string const& http_host,
                std::string const& http_port, int num_threads,
                std::string const& static_file_path,
                std::string const& cert_path, std::string const& priv_key_path,
                std::string const& dh_path);

}  // namespace ppr::backend
