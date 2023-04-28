#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

#include "ppr/common/memory_usage.h"

namespace ppr {

struct memory_usage_printer {
  explicit memory_usage_printer(
      std::ostream& out = std::cerr, bool const enabled = true,
      std::chrono::seconds const& interval = std::chrono::seconds{2})
      : out_{out},
        interval_{interval},
        running_{enabled},
        thread_{&memory_usage_printer::loop, this} {}

  ~memory_usage_printer() { stop(); }

  memory_usage_printer(memory_usage_printer const&) = delete;
  memory_usage_printer(memory_usage_printer&&) = delete;
  memory_usage_printer& operator=(memory_usage_printer const&) = delete;
  memory_usage_printer& operator=(memory_usage_printer&&) = delete;

  void loop() {
    constexpr auto const MB = 1024 * 1024;
    while (running_) {
      auto const mu = get_memory_usage();
      max_virt_ = std::max(max_virt_, mu.current_virtual_);
      out_ << "[MEM] rss: " << std::setw(6) << (mu.current_rss / MB)
           << " MB | virt: " << std::setw(6) << (mu.current_virtual_ / MB)
           << " MB | peak rss: " << std::setw(6) << (mu.peak_rss_ / MB)
           << " MB | peak virt*: " << std::setw(6) << (max_virt_ / MB) << " MB"
           << std::endl;

      auto lock = std::unique_lock{mutex_};
      cv_.wait_for(lock, interval_);
    }
  }

  void stop() {
    {
      auto const lock = std::lock_guard{mutex_};
      running_ = false;
    }
    if (thread_.joinable()) {
      cv_.notify_all();
      thread_.join();
    }
  }

  std::ostream& out_;  // NOLINT
  std::chrono::seconds interval_;
  bool running_{true};
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::uint64_t max_virt_{};
};

}  // namespace ppr
