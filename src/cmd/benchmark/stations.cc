#include <cstdlib>
#include <fstream>
#include <iostream>

#include "boost/algorithm/string.hpp"
#include "boost/geometry/geometries/box.hpp"

#include "ppr/cmd/benchmark/stations.h"
#include "ppr/common/geometry/merc.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace ppr::benchmark {

bool stations::load(std::string const& file, bounds const& bds) {
  // read stations file
  std::ifstream f(file);
  if (!f) {
    return false;
  }
  while (f) {
    int eva;
    double lat, lon;
    std::string name;
    f >> eva >> lon >> lat;
    std::getline(f, name);
    auto const loc = make_location(lon, lat);
    if (bds.contains(loc)) {
      stations_.push_back(loc);
    }
  }

  // init random distribution
  std::random_device rd;
  mt_.seed(rd());
  dist_ = std::uniform_int_distribution<>(
      0, static_cast<int>(stations_.size() - 1));

  // create rtree
  rtree_ = rtree_type{stations_};
  return true;
}

location stations::random_station() {
  return stations_[static_cast<std::size_t>(dist_(mt_))];
}

std::vector<location> stations::stations_near(location const& ref,
                                              double max_dist) {
  auto const ref_merc = to_merc(ref);
  auto const offset = max_dist / scale_factor(ref_merc);
  bg::model::box<location> box(to_location(ref_merc - merc{offset, offset}),
                               to_location(ref_merc + merc{offset, offset}));
  std::vector<rtree_value_t> results;
  rtree_.query(bgi::intersects(box) && bgi::satisfies([&](location const& v) {
                 return distance(ref, v) <= max_dist;
               }),
               std::back_inserter(results));
  return results;
}

}  // namespace ppr::benchmark
