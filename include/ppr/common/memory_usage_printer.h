#pragma once

#include <iomanip>

#include "ppr/common/memory_usage_monitor.h"

namespace ppr {

struct memory_usage_printer {
  enum class mode { DISABLED, SILENT, PRINT };

  explicit memory_usage_printer(
      std::ostream& out = std::cerr, mode const m = mode::PRINT,
      std::chrono::seconds const& interval = std::chrono::seconds{2})
      : out_{out},
        mode_{m},
        monitor_{[this](memory_usage const mu, peak_memory_usage const& peak) {
                   print(mu, peak);
                 },
                 mode_ != mode::DISABLED, interval} {}

  ~memory_usage_printer() { stop(); }

  memory_usage_printer(memory_usage_printer const&) = delete;
  memory_usage_printer(memory_usage_printer&&) = delete;
  memory_usage_printer& operator=(memory_usage_printer const&) = delete;
  memory_usage_printer& operator=(memory_usage_printer&&) = delete;

  void stop() { monitor_.stop(); }

  peak_memory_usage const& get_peak_memory_usage() const {
    return monitor_.get_peak_memory_usage();
  }

private:
  void print(memory_usage const mu, peak_memory_usage const& peak) {
    if (mode_ == mode::SILENT) {
      return;
    }
    constexpr auto const MB = 1024 * 1024;
    out_ << "[MEM] rss: " << std::setw(6) << (mu.current_rss_ / MB) << " / "
         << std::setw(6) << (mu.peak_rss_ / MB)
         << " MB | virt: " << std::setw(6) << (mu.current_virtual_ / MB)
         << " / " << std::setw(6) << (peak.virtual_ / MB) << " MB" << std::endl;
  }

  std::ostream& out_;  // NOLINT
  mode mode_;
  memory_usage_monitor monitor_;
};

}  // namespace ppr
