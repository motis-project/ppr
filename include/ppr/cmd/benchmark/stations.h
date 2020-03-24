#pragma once

#include <random>
#include <string>
#include <vector>

#include "boost/geometry/index/rtree.hpp"

#include "ppr/cmd/benchmark/bounds.h"
#include "ppr/common/location.h"

namespace ppr::benchmark {

struct stations {
public:
  bool load(std::string const& file, bounds const& bds);

  location random_station();
  std::vector<location> stations_near(location const& ref, double max_dist);

  std::size_t size() const { return stations_.size(); }
  bool empty() const { return stations_.empty(); }

private:
  std::vector<location> stations_;
  std::mt19937 mt_;
  std::uniform_int_distribution<> dist_;

  using rtree_point_t = location;
  using rtree_value_t = rtree_point_t;
  using rtree_type =
      boost::geometry::index::rtree<rtree_value_t,
                                    boost::geometry::index::rstar<16>>;
  rtree_type rtree_;
};

}  // namespace ppr::benchmark
