#include "ppr/serialization/writer.h"

#include "cista/mmap.h"
#include "cista/targets/buf.h"

#include "ppr/serialization/mode.h"

namespace ppr::serialization {

// NOLINTBEGIN(clang-analyzer-core.StackAddressEscape)

// cista/reflection/to_tuple.h:133:5: error: Address of stack memory associated
// with temporary object of type 'bool' is still referred to by a temporary
// object on the stack upon returning to the caller.  This will be a dangling
// reference

void write_routing_graph(routing_graph const& rg, std::string const& filename,
                         ppr::preprocessing::statistics& stats) {

  cista::buf<cista::mmap> mmap{cista::mmap{filename.c_str()}};
  cista::serialize<SERIALIZATION_MODE>(mmap, *rg.data_);
  stats.serialized_size_ = static_cast<std::size_t>(mmap.size());
}

// NOLINTEND(clang-analyzer-core.StackAddressEscape)

}  // namespace ppr::serialization
