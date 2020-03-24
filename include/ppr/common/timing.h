#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <locale>

#include "boost/io/ios_state.hpp"

namespace ppr {

using milliseconds = std::chrono::duration<double, std::milli>;

inline std::chrono::time_point<std::chrono::steady_clock> timing_now() {
  return std::chrono::steady_clock::now();
}

inline double ms_between(
    std::chrono::time_point<std::chrono::steady_clock> const& start,
    std::chrono::time_point<std::chrono::steady_clock> const& end) {
  return std::chrono::duration_cast<milliseconds>(end - start).count();
}

inline double ms_since(
    std::chrono::time_point<std::chrono::steady_clock> const& start) {
  return ms_between(start, timing_now());
}

inline void print_timing(std::ostream& out, char const* name, double duration) {
  boost::io::ios_all_saver all_saver(out);
  out << std::setfill('.') << std::setw(60) << std::left << name
      << std::setw(10) << std::right << duration << " ms" << std::endl;
}

inline void print_timing(char const* name, double duration) {
  return print_timing(std::cout, name, duration);
}

}  // namespace ppr
