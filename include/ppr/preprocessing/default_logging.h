#pragma once

#include <sstream>

#include "ppr/preprocessing/logging.h"

namespace ppr::preprocessing {

struct default_logging {
  explicit default_logging(logging& log);
  ~default_logging();

  default_logging(default_logging const&) = delete;
  default_logging& operator=(default_logging const&) = delete;
  default_logging(default_logging&&) = delete;
  default_logging& operator=(default_logging&&) = delete;

  void flush();

  logging& log_;
  std::ostringstream log_stream_;
};

}  // namespace ppr::preprocessing
