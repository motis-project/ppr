#include "ppr/serialization/writer.h"

#include "cista/mmap.h"
#include "cista/targets/buf.h"

#include "ppr/serialization/mode.h"

namespace ppr::serialization {

void write_routing_graph(routing_graph const& rg, std::string const& filename,
                         ppr::preprocessing::statistics& stats) {

  cista::buf<cista::mmap> mmap{cista::mmap{filename.c_str()}};
  cista::serialize<SERIALIZATION_MODE>(mmap, *rg.data_);
  stats.serialized_size_ = static_cast<std::size_t>(mmap.curr_offset_);
}

}  // namespace ppr::serialization
