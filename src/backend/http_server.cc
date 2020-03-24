#include "ppr/backend/http_server.h"

#include "boost/asio/post.hpp"
#include "boost/beast/version.hpp"

#include "boost/filesystem.hpp"

#include "net/web_server/responses.h"
#include "net/web_server/serve_static.h"
#include "net/web_server/web_server.h"

#include "ppr/backend/output/graph_response.h"
#include "ppr/backend/output/route_response.h"
#include "ppr/backend/params.h"
#include "ppr/backend/request_parser.h"
#include "ppr/profiles/json.h"
#include "ppr/profiles/parse_search_profile.h"
#include "ppr/routing/search.h"

using namespace ppr::backend::output;
using namespace ppr::routing;
using namespace ppr::profiles;
using namespace net;
using net::web_server;

namespace http = boost::beast::http;
namespace bgi = boost::geometry::index;
namespace fs = boost::filesystem;

namespace ppr::backend {

template <typename Body>
void set_cors_headers(http::response<Body>& res) {
  using namespace boost::beast::http;
  res.set(field::access_control_allow_origin, "*");
  res.set(field::access_control_allow_headers,
          "X-Requested-With, Content-Type, Accept, Authorization");
  res.set(field::access_control_allow_methods, "GET, POST, OPTIONS");
  res.set(field::access_control_max_age, "3600");
}

web_server::string_res_t json_response(
    web_server::http_req_t const& req, std::string const& content,
    http::status const status = http::status::ok) {
  auto res = net::string_response(req, content, status, "application/json");
  set_cors_headers(res);
  return res;
}

struct route_request {
  std::vector<location> waypoints_;
  search_profile profile_;
  bool preview_{};
};

struct graph_request {
  std::vector<location> waypoints_;
  bool include_areas_{};
  bool include_visibility_graphs_{};
};

rapidjson::Document parse_json(std::string const& s) {
  rapidjson::Document doc;
  doc.Parse<rapidjson::kParseDefaultFlags>(s.c_str());
  if (doc.HasParseError()) {
    std::cerr << "json parse error" << std::endl;
  }
  return doc;
}

route_request parse_route_request(web_server::http_req_t const& req) {
  route_request r{};
  auto doc = parse_json(req.body());
  get_waypoints(r.waypoints_, doc, "waypoints");
  get_profile(r.profile_, doc, "profile");
  get_bool(r.preview_, doc, "preview");
  return r;
}

graph_request parse_graph_request(web_server::http_req_t const& req) {
  graph_request r{};
  auto doc = parse_json(req.body());
  get_waypoints(r.waypoints_, doc, "waypoints");
  get_bool(r.include_areas_, doc, "include_areas");
  get_bool(r.include_visibility_graphs_, doc, "include_visibility_graphs");
  return r;
}

struct http_server::impl {
  impl(boost::asio::io_context& ios, boost::asio::io_context& thread_pool,
       boost::asio::ssl::context& ssl_ctx, routing_graph const& g,
       std::string const& static_file_path)
      : ioc_(ios),
        thread_pool_(thread_pool),
        ssl_ctx_(ssl_ctx),
        graph_(g),
        server_(ioc_, ssl_ctx_) {
    try {
      if (!static_file_path.empty() && fs::is_directory(static_file_path)) {
        static_file_path_ = fs::canonical(static_file_path).string();
        serve_static_files_ = true;
      }
    } catch (fs::filesystem_error const& e) {
      std::cerr << "Static file directory not found: " << e.what() << std::endl;
    }
    if (serve_static_files_) {
      std::cout << "Serving static files from " << static_file_path_
                << std::endl;
    } else {
      std::cout << "Not serving static files" << std::endl;
    }
  }

  void handle_route(web_server::http_req_t const& req,
                    web_server::http_res_cb_t const& cb) {
    auto const r = parse_route_request(req);
    if (r.waypoints_.size() < 2) {
      return cb(json_response(req, R"({"error": "Missing request waypoints"})",
                              http::status::bad_request));
    }

    auto const& start = r.waypoints_.front();
    auto const& destination = r.waypoints_.back();
    auto const result = find_routes(graph_, start, {destination}, r.profile_);
    return cb(
        json_response(req, routes_to_route_response(result, !r.preview_)));
  }

  void handle_graph(web_server::http_req_t const& req,
                    web_server::http_res_cb_t const& cb) {
    auto const r = parse_graph_request(req);
    if (r.waypoints_.size() < 2) {
      return cb(json_response(req, R"({"error": "Missing request points"})",
                              http::status::bad_request));
    }

    auto query_box =
        boost::geometry::return_envelope<routing_graph::rtree_box_type>(
            r.waypoints_);

    std::vector<routing_graph::edge_rtree_value_type> edge_results;
    graph_.edge_rtree_->query(bgi::intersects(query_box),
                              std::back_inserter(edge_results));

    std::vector<routing_graph::area_rtree_value_type> area_results;
    if (r.include_areas_) {
      graph_.area_rtree_->query(bgi::intersects(query_box),
                                std::back_inserter(area_results));
    }

    cb(json_response(req, to_graph_response(edge_results, area_results, graph_,
                                            r.include_visibility_graphs_)));
  }

  void handle_static(web_server::http_req_t&& req,
                     web_server::http_res_cb_t&& cb) {
    if (serve_static_files_) {
      return net::serve_static_file(static_file_path_, req, cb);
    } else {
      return cb(net::not_found_response(req));
    }
  }

  void handle_request(web_server::http_req_t&& req,
                      web_server::http_res_cb_t&& cb) {
    std::cout << "[" << req.method_string() << "] " << req.target()
              << std::endl;
    switch (req.method()) {
      case http::verb::options: return cb(json_response(req, {}));
      case http::verb::post: {
        auto const& target = req.target();
        if (target.starts_with("/api/route")) {
          return run_parallel(
              [this](web_server::http_req_t const& req1,
                     web_server::http_res_cb_t const& cb1) {
                handle_route(req1, cb1);
              },
              req, cb);
        } else if (target.starts_with("/api/graph")) {
          return run_parallel(
              [this](web_server::http_req_t const& req1,
                     web_server::http_res_cb_t const& cb1) {
                handle_graph(req1, cb1);
              },
              req, cb);
        } else {
          return cb(json_response(req, R"({"error": "Not found"})",
                                  http::status::not_found));
        }
      }
      case http::verb::get:
      case http::verb::head:
        return handle_static(std::move(req), std::move(cb));
      default:
        return cb(json_response(req,
                                R"({"error": "HTTP method not supported"})",
                                http::status::bad_request));
    }
  }

  template <typename Fn>
  void run_parallel(Fn handler, web_server::http_req_t const& req,
                    web_server::http_res_cb_t const& cb) {
    boost::asio::post(thread_pool_, [req, cb, handler, this]() {
      handler(req, [req, cb, this](web_server::http_res_t&& res) {
        boost::asio::post(
            ioc_, [cb, res{std::move(res)}]() mutable { cb(std::move(res)); });
      });
    });
  }

  void listen(std::string const& host, std::string const& port) {
    server_.on_http_request([this](web_server::http_req_t req,
                                   web_server::http_res_cb_t cb, bool /*ssl*/) {
      return handle_request(std::move(req), std::move(cb));
    });

    boost::system::error_code ec;
    server_.init(host, port, ec);
    if (ec) {
      std::cerr << "server init error: " << ec.message() << "\n";
    }

    std::cout << "Listening on http://" << host << ":" << port
              << "/ and https://" << host << ":" << port << "/" << std::endl;
    server_.run();
  }

  void stop() { server_.stop(); }

private:
  boost::asio::io_context& ioc_;
  boost::asio::io_context& thread_pool_;
  boost::asio::ssl::context& ssl_ctx_;
  routing_graph const& graph_;
  web_server server_;
  bool serve_static_files_{false};
  std::string static_file_path_;
};

http_server::http_server(boost::asio::io_context& ioc,
                         boost::asio::io_context& thread_pool,
                         boost::asio::ssl::context& ssl_ctx,
                         routing_graph const& g,
                         std::string const& static_file_path)
    : impl_(new impl(ioc, thread_pool, ssl_ctx, g, static_file_path)) {}

http_server::~http_server() = default;

void http_server::listen(std::string const& host, std::string const& port) {
  impl_->listen(host, port);
}

void http_server::stop() { impl_->stop(); }

}  // namespace ppr::backend
