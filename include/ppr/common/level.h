#pragma once

#include <cassert>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <optional>

#include "cista/serialization.h"

#include "ppr/common/data.h"

namespace ppr {

struct levels {
  [[nodiscard]] constexpr bool has_level() const {
    return data_.value_ != std::numeric_limits<std::uint16_t>::max();
  }

  [[nodiscard]] constexpr bool has_single_level() const {
    return data_.single_.multi_level_flag_ == 0;
  }

  [[nodiscard]] constexpr bool has_multiple_levels() const {
    return has_level() && !has_single_level();
  }

  [[nodiscard]] constexpr std::int16_t single_level() const {
    assert(has_single_level());
    return data_.single_.signed_;
  }

  [[nodiscard]] constexpr std::uint16_t multi_level_index() const {
    assert(has_multiple_levels());
    return data_.multi_.unsigned_;
  }

  [[nodiscard]] constexpr std::uint16_t value() const { return data_.value_; }

  constexpr bool operator==(levels const& o) const {
    return data_.value_ == o.data_.value_;
  }

  constexpr bool operator!=(levels const& o) const {
    return data_.value_ != o.data_.value_;
  }

  union {
    std::uint16_t value_{std::numeric_limits<std::uint16_t>::max()};
    struct {
      std::uint16_t multi_level_flag_ : 1;  // == 0
      std::int16_t signed_ : 15;
    } single_;
    struct {
      std::uint16_t multi_level_flag_ : 1;  // == 1
      std::uint16_t unsigned_ : 15;
    } multi_;
  } data_;
};

inline cista::hash_t type_hash(levels const& lvl, cista::hash_t h,
                               std::map<cista::hash_t, unsigned>& done) {
  return cista::hash_combine(cista::type_hash(lvl.data_.value_, h, done));
}

template <typename T>
constexpr bool is_acceptable_level(T const level) {
  return level >= -(1 << 14) && level <= ((1 << 14) - 1);
}

constexpr levels make_single_level(std::int16_t const level) {
  assert(is_acceptable_level(level));
  return {.data_ = {.single_ = {.multi_level_flag_ = 0, .signed_ = level}}};
}

constexpr levels make_single_level(std::optional<std::int16_t> const& level) {
  return level ? make_single_level(*level) : levels{};
}

constexpr levels make_multiple_levels(std::uint16_t const multi_level_index) {
  assert(multi_level_index <= (1 << 15));
  return {.data_ = {.multi_ = {.multi_level_flag_ = 1,
                               .unsigned_ = multi_level_index}}};
}

constexpr std::optional<std::int16_t> from_human_level(double const level) {
  auto const converted = level * 10.0;
  if (is_acceptable_level(converted)) {
    return static_cast<std::int16_t>(converted);
  } else {
    return {};
  }
}

constexpr double to_human_level(std::int16_t const level) {
  return static_cast<double>(level) / 10.0;
}

using levels_idx_t = std::uint16_t;
using levels_vector_t = data::vecvec<levels_idx_t, std::int16_t>;

inline bool matches_level(levels_vector_t const& levels_vec, levels const& lvl,
                          std::int16_t const check_level,
                          bool const result_if_no_level = false) {
  if (!lvl.has_level()) {
    return result_if_no_level;
  } else if (lvl.has_single_level()) {
    return lvl.single_level() == check_level;
  } else {
    auto const bucket = levels_vec.at(lvl.multi_level_index());
    return std::find(begin(bucket), end(bucket), check_level) != end(bucket);
  }
}

inline bool have_shared_level(levels_vector_t const& levels_vec,
                              levels const& a, levels const& b,
                              bool const result_if_no_level = false) {
  if (!a.has_level() || !b.has_level()) {
    return result_if_no_level;
  } else if (a.has_single_level()) {
    return matches_level(levels_vec, b, a.single_level(), result_if_no_level);
  } else if (b.has_single_level()) {
    return matches_level(levels_vec, a, b.single_level(), result_if_no_level);
  } else {
    auto const bucket_a = levels_vec.at(a.multi_level_index());
    auto const bucket_b = levels_vec.at(b.multi_level_index());
    // return whether bucket_a and bucket_b have at least one common entry
    return std::find_first_of(begin(bucket_a), end(bucket_a), begin(bucket_b),
                              end(bucket_b)) != end(bucket_a);
  }
}

inline std::optional<std::int16_t> closest_level(
    levels_vector_t const& levels_vec, levels const& lvl,
    std::int16_t const ref_level) {
  if (!lvl.has_level()) {
    return {};
  } else if (lvl.has_single_level()) {
    return lvl.single_level();
  } else {
    auto closest = std::optional<std::int16_t>{};
    auto closest_diff = std::numeric_limits<std::int16_t>::max();
    for (auto const l : levels_vec.at(lvl.multi_level_index())) {
      auto const diff = static_cast<std::int16_t>(std::abs(ref_level - l));
      if (diff < closest_diff) {
        closest = l;
        closest_diff = diff;
      }
    }
    return closest;
  }
}

}  // namespace ppr
