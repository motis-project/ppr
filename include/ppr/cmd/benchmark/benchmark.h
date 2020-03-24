#pragma once

#include "ppr/cmd/benchmark/bench_spec.h"
#include "ppr/cmd/benchmark/bounds.h"
#include "ppr/cmd/benchmark/prog_options.h"
#include "ppr/cmd/benchmark/stations.h"
#include "ppr/common/routing_graph.h"
#include "ppr/routing/search_profile.h"

namespace ppr::benchmark {

void run_benchmark(routing_graph const& rg, prog_options const& opt,
                   stations& st, bounds& a, bench_spec const& spec);

}  // namespace ppr::benchmark
