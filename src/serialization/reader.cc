#include "ppr/serialization/reader.h"

#include "boost/filesystem.hpp"

#include "ppr/preprocessing/names.h"
#include "ppr/serialization/mode.h"

namespace fs = boost::filesystem;

namespace ppr::serialization {

// NOLINTBEGIN(clang-analyzer-core.StackAddressEscape)

// cista/reflection/to_tuple.h:133:5: error: Address of stack memory associated
// with temporary object of type 'bool' is still referred to by a temporary
// object on the stack upon returning to the caller.  This will be a dangling
// reference

void read_routing_graph(routing_graph& rg, std::string const& filename) {
  if (!fs::exists(filename)) {
    throw std::runtime_error{"ppr routing graph file not found"};
  }
  rg.data_buffer_ = cista::file(filename.c_str(), "r").content();
  rg.data_ = data::deserialize<routing_graph_data, SERIALIZATION_MODE>(
      rg.data_buffer_);
  rg.filename_ = filename;
}

// NOLINTEND(clang-analyzer-core.StackAddressEscape)

}  // namespace ppr::serialization
