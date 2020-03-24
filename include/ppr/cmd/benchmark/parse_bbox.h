#pragma once

#include <memory>
#include <string>

#include "ppr/cmd/benchmark/bbox.h"

namespace ppr::benchmark {

std::unique_ptr<bbox> parse_bbox(std::string const& input);

}  // namespace ppr::benchmark
