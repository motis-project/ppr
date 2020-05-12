#pragma once

#include <string>

#include "ppr/common/routing_graph.h"
#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/options.h"
#include "ppr/preprocessing/statistics.h"

namespace ppr::preprocessing {

struct preprocessing_result {
  bool successful() const { return success_; }

  routing_graph rg_;
  statistics stats_;
  bool success_{true};
};

preprocessing_result create_routing_data(options const& opt, logging& log);

}  // namespace ppr::preprocessing
