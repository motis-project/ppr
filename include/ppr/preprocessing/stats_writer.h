#pragma once

#include <string>

#include "ppr/preprocessing/statistics.h"

namespace ppr::preprocessing {

void write_stats(statistics const& s, std::string const& filename);

}  // namespace ppr::preprocessing
