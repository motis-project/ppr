#include <iostream>
#include <thread>
#include <vector>

#include "boost/asio/executor_work_guard.hpp"
#include "boost/asio/io_context.hpp"

#include "net/stop_handler.h"

#include "ppr/backend/http_server.h"
#include "ppr/backend/server.h"
#include "ppr/backend/ssl_settings.h"

namespace ppr::backend {

auto run(boost::asio::io_context& ioc) {
  return [&ioc]() {
    while (true) {
      try {
        ioc.run();
        break;
      } catch (std::exception const& e) {
        std::cout << "unhandled error: " << e.what();
      } catch (...) {
        std::cout << "unhandled unknown error";
      }
    }
  };
}

void ppr_server(routing_graph const& rg, std::string const& http_host,
                std::string const& http_port, int num_threads,
                std::string const& static_file_path,
                std::string const& cert_path, std::string const& priv_key_path,
                std::string const& dh_path) {
  boost::asio::ssl::context ssl_ctx{boost::asio::ssl::context::tlsv12};
  load_server_certificate(ssl_ctx, cert_path, priv_key_path, dh_path);

  boost::asio::io_context ioc, thread_pool;
  http_server http{ioc, thread_pool, ssl_ctx, rg, static_file_path};

  auto work_guard = boost::asio::make_work_guard(thread_pool);
  std::vector<std::thread> threads(
      static_cast<unsigned>(std::max(1, num_threads)));
  for (auto& t : threads) {
    t = std::thread(run(thread_pool));
  }

  http.listen(http_host, http_port);

  net::stop_handler stop(ioc, [&]() {
    http.stop();
    ioc.stop();
  });

  ioc.run();
  thread_pool.stop();
  std::for_each(begin(threads), end(threads), [](std::thread& t) { t.join(); });
}

}  // namespace ppr::backend
