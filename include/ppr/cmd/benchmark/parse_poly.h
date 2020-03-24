#pragma once

#include <memory>
#include <string>

#include "ppr/cmd/benchmark/poly.h"

namespace ppr::benchmark {

std::unique_ptr<poly> parse_poly(std::string const& filename);

}  // namespace ppr::benchmark
