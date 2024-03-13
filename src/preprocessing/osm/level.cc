#include <cstdint>
#include <optional>
#include <string_view>

#include "cista/containers/vector.h"

#include "utl/enumerate.h"
#include "utl/zip.h"

#include "ppr/preprocessing/osm/level.h"

namespace ppr::preprocessing::osm {

levels get_levels(char const* level_tag, levels_vector_t& levels_vec) {
  if (level_tag == nullptr) {
    return {};
  }

  auto const input = std::string_view{level_tag};

  auto levels = cista::raw::vector<std::int16_t>{};

  // the current level being read
  auto current_level = std::int64_t{0};

  auto current_range_start = std::optional<std::int16_t>{};

  // number of digits read for the current level
  auto digits = 0;

  // 0 while reading digits before decimal point, then 1 for the first digit
  // after the decimal point and so on
  auto decimal = 0;

  // if the current number is negative
  auto negative = false;

  // how many numbers in the current ;-deliminated section
  // for legal inputs this should be 1 or 2
  auto current_section_numbers = 0;

  // total numbers in the input
  auto total_numbers = 0;

  // set if the current level being parsed is invalid
  // in this case, input until the next level is ignored
  auto invalid = false;

  auto constexpr step_offset = *from_human_level(1);

  auto const finish_current_level = [&](bool const always_add_to_levels) {
    auto result = std::optional<std::int16_t>{};
    if (!invalid && digits != 0) {
      if (decimal < 2) {
        current_level *= 10;
      }
      if (negative) {
        current_level *= -1;
      }
      if (is_acceptable_level(current_level)) {
        result = static_cast<std::int16_t>(current_level);
        if (current_range_start) {
          if (*result >= *current_range_start) {
            for (auto lvl = static_cast<std::int16_t>(*current_range_start +
                                                      step_offset);
                 lvl <= *result; lvl += step_offset) {
              levels.emplace_back(lvl);
            }
          } else {
            for (auto lvl = static_cast<std::int16_t>(*result + step_offset);
                 lvl <= *current_range_start; lvl += step_offset) {
              levels.emplace_back(lvl);
            }
          }
        } else if (total_numbers > 1 || always_add_to_levels) {
          levels.emplace_back(*result);
        }
      }
    }
    current_level = 0;
    digits = 0;
    decimal = 0;
    negative = false;
    return result;
  };

  for (auto i = 0; i < input.length(); ++i) {
    auto const c = input[i];

    if (c == ' ' || (invalid && c != ';')) {
      continue;
    } else if (c == ';') {
      if (!invalid) {
        finish_current_level(true);
      }
      invalid = false;
      current_range_start = {};
      current_section_numbers = 0;
    } else if (c >= '0' && c <= '9') {
      if (digits == 0) {
        // new number
        ++current_section_numbers;
        ++total_numbers;
        if (current_section_numbers > 2) {
          invalid = true;
        }
      }
      // we only support 1 decimal, the rest is ignored
      if (decimal < 2) {
        current_level = (current_level * 10) + (c - '0');
        ++digits;
      }
      if (decimal > 0) {
        ++decimal;
      }
    } else if (c == '.') {
      if (decimal > 0) {
        // more than one . in this level
        invalid = true;
      } else {
        // next digit is the first decimal
        decimal = 1;
      }
    } else if (c == '-') {
      if (digits == 0) {
        // minus sign
        negative = true;
      } else {
        // range
        current_range_start = finish_current_level(true);
        if (!current_range_start) {
          invalid = true;
        }
      }
    } else {
      invalid = true;
    }
  }

  auto const final_level = finish_current_level(false);

  auto const entries = levels.size();
  if (entries == 0) {
    return make_single_level(final_level);
  } else if (entries == 1) {
    return make_single_level(levels[0]);
  } else {
    for (auto const [idx, bucket] : utl::enumerate(levels_vec)) {
      if (bucket.size() == entries) {
        auto match = true;
        for (auto const [a, b] : utl::zip(levels, bucket)) {
          if (a != b) {
            match = false;
            break;
          }
        }
        if (match) {
          return make_multiple_levels(static_cast<std::uint16_t>(idx));
        }
      }
    }
    auto const levels_idx = levels_vec.size();
    levels_vec.emplace_back(levels);
    return make_multiple_levels(levels_idx);
  }
}

levels get_levels(osmium::TagList const& tags, levels_vector_t& levels_vec) {
  return get_levels(tags["level"], levels_vec);
}

}  // namespace ppr::preprocessing::osm
