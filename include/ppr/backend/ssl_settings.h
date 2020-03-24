#pragma once

#include <string>

#include "boost/asio/ssl/context.hpp"

namespace ppr::backend {

void load_server_certificate(boost::asio::ssl::context& ctx,
                             std::string const& cert_path = "::dev::",
                             std::string const& priv_key_path = "::dev::",
                             std::string const& dh_path = "::dev::");

}  // namespace ppr::backend
