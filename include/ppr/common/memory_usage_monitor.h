#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <algorithm>
#include <functional>
#include <mutex>
#include <thread>

#include "ppr/common/memory_usage.h"

namespace ppr {

struct peak_memory_usage {
  std::uint64_t rss_{};  // from OS
  std::uint64_t virtual_{};  // from sampling
};

struct memory_usage_monitor {
  using callback_t =
      std::function<void(memory_usage, peak_memory_usage const&)>;

  explicit memory_usage_monitor(
      callback_t&& callback, bool const enabled = true,
      std::chrono::seconds const& interval = std::chrono::seconds{2})
      : callback_{std::move(callback)},
        interval_{interval},
        running_{enabled},
        thread_{&memory_usage_monitor::loop, this} {}

  ~memory_usage_monitor() { stop(); }

  memory_usage_monitor(memory_usage_monitor const&) = delete;
  memory_usage_monitor(memory_usage_monitor&&) = delete;
  memory_usage_monitor& operator=(memory_usage_monitor const&) = delete;
  memory_usage_monitor& operator=(memory_usage_monitor&&) = delete;

  void loop() {
    while (running_) {
      auto const mu = get_memory_usage();
      peak_.virtual_ = std::max(peak_.virtual_, mu.current_virtual_);

      if (callback_) {
        callback_(mu, peak_);
      }

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

  peak_memory_usage const& get_peak_memory_usage() const { return peak_; }

  callback_t callback_;
  std::chrono::seconds interval_;
  bool running_{true};
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cv_;
  peak_memory_usage peak_;
};

}  // namespace ppr
