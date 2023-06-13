#pragma once

#include "cista/serialization.h"

namespace ppr {

constexpr auto const SERIALIZATION_MODE =
    cista::mode::WITH_INTEGRITY | cista::mode::WITH_VERSION;

constexpr auto const SERIALIZATION_MODE_SKIP_INTEGRITY =
    cista::mode::SKIP_INTEGRITY | cista::mode::WITH_VERSION;

}  // namespace ppr
