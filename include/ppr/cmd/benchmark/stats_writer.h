#pragma once

#include "ppr/cmd/benchmark/csv_writer.h"
#include "ppr/cmd/benchmark/query.h"
#include "ppr/routing/statistics.h"

namespace ppr::benchmark {

struct stats_writer {
  explicit stats_writer(std::string const& filename);
  void write(routing_query const& query,
             ppr::routing::search_result const& result);

private:
  csv_writer csv_;
  std::size_t lines_;
};

}  // namespace ppr::benchmark
