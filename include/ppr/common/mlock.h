#pragma once

#ifdef _MSC_VER

#include <cstddef>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ppr {

inline bool lock_memory(void* addr, std::size_t len) {
  HANDLE proc = GetCurrentProcess();
  std::size_t min_size, max_size;
  if (!GetProcessWorkingSetSize(proc, &min_size, &max_size)) {
    return false;
  }
  min_size += len;
  max_size += len;
  if (!SetProcessWorkingSetSize(proc, min_size, max_size)) {
    return false;
  }
  return VirtualLock(addr, len);
}

inline bool unlock_memory(void* addr, std::size_t len) {
  return VirtualUnlock(addr, len);
}

}  // namespace ppr

#else

#include <cstddef>

#include <sys/mman.h>

namespace ppr {

inline bool lock_memory(void* addr, std::size_t len) {
  return mlock(addr, len) == 0;
}

inline bool unlock_memory(void* addr, std::size_t len) {
  return munlock(addr, len) == 0;
}

}  // namespace ppr

#endif
