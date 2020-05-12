#pragma once

#include <sstream>

#include "ppr/preprocessing/logging.h"

namespace ppr::preprocessing {

struct default_logging {
  explicit default_logging(logging& log);

  logging& log_;
  std::ostringstream log_stream_;
};

}  // namespace ppr::preprocessing
