#pragma once

#include <cstdint>

#include "ppr/common/data.h"

namespace ppr {

using names_idx_t = std::uint32_t;
using names_vector_t = data::vecvec<names_idx_t, char>;

}  // namespace ppr
