#include "ppr/serialization/reader.h"

#include "boost/filesystem.hpp"

#include "ppr/preprocessing/names.h"
#include "ppr/serialization/mode.h"

namespace fs = boost::filesystem;

namespace ppr::serialization {

void read_routing_graph(routing_graph& rg, std::string const& filename) {
  if (!fs::exists(filename)) {
    throw std::runtime_error{"ppr routing graph file not found"};
  }
  auto mmap = cista::mmap{filename.c_str(), cista::mmap::protection::READ};
  auto const ptr =
      cista::deserialize<routing_graph_data, SERIALIZATION_MODE>(mmap);
  rg.data_.mem_ = cista::buf<cista::mmap>{std::move(mmap)};
  rg.data_.el_ = cista::raw::unique_ptr<routing_graph_data>{ptr, false};
  rg.filename_ = filename;
}

routing_graph read_routing_graph(std::string const& filename) {
  if (!fs::exists(filename)) {
    throw std::runtime_error{"ppr routing graph file not found"};
  }
  auto mmap = cista::mmap{filename.c_str(), cista::mmap::protection::READ};
  auto const ptr = reinterpret_cast<routing_graph_data*>(
      &mmap[cista::data_start(SERIALIZATION_MODE)]);
  return routing_graph{
      cista::wrapped(cista::buf<cista::mmap>{std::move(mmap)}, ptr), filename};
}

}  // namespace ppr::serialization
