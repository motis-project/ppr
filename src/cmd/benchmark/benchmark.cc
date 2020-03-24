#include <cstdio>
#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "ppr/cmd/benchmark/benchmark.h"
#include "ppr/cmd/benchmark/query_generator.h"
#include "ppr/cmd/benchmark/stats_writer.h"

using ppr::routing::search_result;

namespace ppr::benchmark {

constexpr std::array<double, 5> EVAL_RADIUS_FACTORS{0.9, 0.85, 0.8, 0.7, 0.6};

void run_benchmark(routing_graph const& rg, prog_options const& opt,
                   stations& st, bounds& a, bench_spec const& spec) {
  query_generator qg(rg, a, st, spec.profiles_.front());
  stats_writer stats(spec.stats_file_);
  std::mutex qg_mutex, out_mutex;
  std::atomic<int> done(0), total_queries(0), all_reached(0);

  auto const queries = opt.queries_;
  qg.radius_ = spec.radius_;
  qg.radius_factor_ = spec.radius_factor_;
  qg.full_radius_ = spec.full_radius_;
  qg.destination_count_ = spec.destination_count_;
  qg.start_mode_ = spec.start_mode_;
  qg.dest_mode_ = spec.dest_mode_;
  qg.direction_ = spec.direction_;

  std::cout
      << "====================================================================="
      << std::endl;
  std::cout << "Benchmark: " << queries << " queries, " << opt.threads_
            << " threads" << std::endl;
  std::cout << "Graph: " << opt.graph_file_ << std::endl;
  std::cout << "Start Mode: " << qg.start_mode_ << std::endl;
  std::cout << "Dest Mode: " << qg.dest_mode_
            << ", destinations: " << qg.destination_count_
            << ", direction: " << qg.direction_ << std::endl;
  std::cout << "Radius: " << qg.radius_ << ", duration limit: "
            << (qg.profile_.profile_.duration_limit_ / 60) << std::endl;
  std::cout << "Radius factor: " << spec.radius_factor_
            << ", full radius: " << spec.full_radius_
            << ", eval: " << spec.eval_radius_factor_ << std::endl;
  std::cout << "Profiles:";
  for (auto const& p : spec.profiles_) {
    std::cout << " " << p.name_;
  }
  std::cout << std::endl;
  std::cout << "Output: " << spec.stats_file_ << std::endl;
  std::cout
      << "====================================================================="
      << std::endl;

  auto const generate_query = [&]() {
    std::lock_guard<std::mutex> guard(qg_mutex);
    return qg.generate_query();
  };

  auto const print_progress = [&](int i) {
    if (i != 0 && i % 1000 == 0) {
      std::cout << i << "/" << queries << " ("
                << std::round(i / static_cast<double>(queries) * 100) << "%)"
                << std::endl;
    }
  };

  auto const handle_query = [&](routing_query const& query,
                                search_result const& result,
                                int const progress) {
    std::lock_guard<std::mutex> out_guard(out_mutex);
    stats.write(query, result);
    print_progress(progress);
  };

  auto const print_unreached = [&](routing_query const& query,
                                   search_result const& result) {
    std::lock_guard<std::mutex> out_guard(out_mutex);
    std::cout << "Didn't find routes to all destinations - "
              << result.destinations_reached() << "/"
              << query.destinations_.size() << " reached:" << std::endl;
    std::cout << "Labels stats: ";
    for (auto const& ds : result.stats_.dijkstra_statistics_) {
      std::cout << ds.start_labels_ << "/" << ds.labels_created_ << "/"
                << ds.labels_popped_ << "  ";
    }
    std::cout << std::endl;
    for (auto i = 0ul; i < result.routes_.size(); i++) {
      if (result.routes_[i].empty()) {
        auto const from = query.start_;
        auto const to = query.destinations_[i];
        std::cout << "Found no route from " << from.lat() << ";" << from.lon()
                  << " to " << to.lat() << ";" << to.lon() << " ("
                  << distance(from, to) << "m) - " << from.lon() << ","
                  << from.lat() << ";" << to.lon() << "," << to.lat()
                  << std::endl;
      }
    }
    std::cout << "------" << std::endl;
  };

  auto const run = [&]() {
    if (opt.warmup_ > 0) {
      for (auto i = 0; i < opt.warmup_; i++) {
        generate_query().execute(rg);
      }
    }

    int progress;
    while ((progress = ++done) <= queries) {
      while (true) {
        auto query = generate_query();
        auto result = query.execute(rg);
        auto const all_destinations_reached =
            std::all_of(begin(result.routes_), end(result.routes_),
                        [](auto const& routes) { return !routes.empty(); });

        ++total_queries;
        if (all_destinations_reached) {
          ++all_reached;
        }

        if (!all_destinations_reached && opt.verbose_) {
          print_unreached(query, result);
        }
        if (all_destinations_reached || !spec.ignore_unreachable_) {
          handle_query(query, result, progress);
          if (spec.eval_radius_factor_ &&
              qg.dest_mode_ == destination_generation_mode::STATIONS) {
            for (auto const& rf : EVAL_RADIUS_FACTORS) {
              auto radius_query = qg.with_radius_factor(query, rf);
              auto radius_result = radius_query.execute(rg);
              radius_query.base_query_ = &query;
              radius_query.base_result_ = &result;
              handle_query(radius_query, radius_result, 0);
              ++total_queries;
            }

          } else if (spec.profiles_.size() > 1) {
            for (auto profile_idx = 1UL; profile_idx < spec.profiles_.size();
                 profile_idx++) {
              auto new_query = query.with_profile(spec.profiles_[profile_idx]);
              auto new_result = new_query.execute(rg);
              handle_query(new_query, new_result, 0);
              ++total_queries;
            }
          }
          break;
        }
      }
    }
  };

  std::vector<std::thread> threads(
      static_cast<unsigned>(std::max(1, opt.threads_)));
  for (auto& t : threads) {
    t = std::thread(run);
  }
  std::for_each(begin(threads), end(threads), [](std::thread& t) { t.join(); });

  std::cout << std::endl;
  std::cout << "Executed " << total_queries << "/" << queries << " queries"
            << std::endl;
  std::cout << all_reached << "/" << total_queries << " = "
            << (all_reached / static_cast<double>(total_queries) * 100.0)
            << "% reached all destinations" << std::endl;
  std::cout << std::endl;
}

}  // namespace ppr::benchmark
