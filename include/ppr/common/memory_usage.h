#pragma once

#include <cstdint>

namespace ppr {

// all values are in bytes
struct memory_usage {
  // resident set size / working set
  std::uint64_t peak_rss_{};
  std::uint64_t current_rss_{};

  // linux/macos: virtual memory size
  // windows: commit charge
  std::uint64_t current_virtual_{};
};

memory_usage get_memory_usage();

}  // namespace ppr
