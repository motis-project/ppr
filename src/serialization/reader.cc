#include "ppr/serialization/reader.h"

#include "boost/filesystem.hpp"

#include "ppr/preprocessing/names.h"
#include "ppr/serialization/mode.h"

namespace fs = boost::filesystem;

namespace ppr::serialization {

routing_graph_data* deserialize_routing_graph(cista::mmap& mmap,
                                              bool const check_integrity) {
  return check_integrity
             ? cista::deserialize<routing_graph_data, SERIALIZATION_MODE>(mmap)
             : cista::deserialize<routing_graph_data,
                                  SERIALIZATION_MODE_SKIP_INTEGRITY>(mmap);
}

void read_routing_graph(routing_graph& rg, std::string const& filename,
                        bool const check_integrity) {
  if (!fs::exists(filename)) {
    throw std::runtime_error{"ppr routing graph file not found"};
  }
  auto mmap = cista::mmap{filename.c_str(), cista::mmap::protection::READ};
  auto const ptr = deserialize_routing_graph(mmap, check_integrity);
  rg.data_.mem_ = cista::buf<cista::mmap>{std::move(mmap)};
  rg.data_.el_ = cista::raw::unique_ptr<routing_graph_data>{ptr, false};
  rg.filename_ = filename;
}

routing_graph read_routing_graph(std::string const& filename,
                                 bool const check_integrity) {
  if (!fs::exists(filename)) {
    throw std::runtime_error{"ppr routing graph file not found"};
  }
  auto mmap = cista::mmap{filename.c_str(), cista::mmap::protection::READ};
  auto const ptr = deserialize_routing_graph(mmap, check_integrity);
  return routing_graph{
      cista::wrapped(cista::buf<cista::mmap>{std::move(mmap)}, ptr), filename};
}

}  // namespace ppr::serialization
