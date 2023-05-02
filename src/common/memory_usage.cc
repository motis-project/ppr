#include "ppr/common/memory_usage.h"

#ifdef _WIN32

// windows.h must be included first
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <psapi.h>

#else

#include <sys/resource.h>

#if __linux__
#include <unistd.h>
#include <cstdio>
#endif

#if __APPLE__
#include <mach/mach.h>
#endif

#endif

namespace ppr {

memory_usage get_memory_usage() {
  auto mu = memory_usage{};

#ifdef _WIN32

  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
    mu.peak_rss_ = pmc.PeakWorkingSetSize;
    mu.current_rss_ = pmc.WorkingSetSize;
    mu.current_virtual_ = pmc.PagefileUsage;
  }

#else

  struct rusage r_usage {};
  if (getrusage(RUSAGE_SELF, &r_usage) == 0) {
#ifdef __APPLE__
    mu.peak_rss_ = r_usage.ru_maxrss;
#else
    mu.peak_rss_ = r_usage.ru_maxrss * 1024;
#endif
  }

#ifdef __linux__
  auto const page_size = sysconf(_SC_PAGESIZE);  // bytes
  if (page_size != -1) {
    if (auto const f = std::fopen("/proc/self/statm", "r"); f != nullptr) {
      // NOLINTNEXTLINE(cert-err34-c)
      if (std::fscanf(f, "%lu %lu", &mu.current_virtual_, &mu.current_rss_) ==
          2) {
        // wrong if different page sizes are used (huge pages)
        mu.current_virtual_ *= page_size;
        mu.current_rss_ *= page_size;
      }
      std::fclose(f);  // NOLINT(cert-err33-c)
    }
  }
#endif

#ifdef __APPLE__
  struct task_basic_info t_info;
  mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

  if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&t_info,
                &t_info_count) == KERN_SUCCESS) {
    mu.current_rss_ = t_info.resident_size;
    mu.current_virtual_ = t_info.virtual_size;
  }
#endif

#endif

  return mu;
}

}  // namespace ppr
