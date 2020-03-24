#pragma once

#include <limits>
#include <random>

#include "boost/geometry/algorithms/covered_by.hpp"
#include "boost/geometry/geometries/box.hpp"

#include "ppr/cmd/benchmark/bounds.h"

namespace ppr::benchmark {

class bbox : public bounds {
public:
  bbox()
      : bbox(boost::geometry::model::box<location>(
            make_location(std::numeric_limits<double>::min(),
                          std::numeric_limits<double>::min()),
            make_location(std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max()))) {}

  explicit bbox(boost::geometry::model::box<location> box) : box_(box) {
    std::random_device rd;
    mt_.seed(rd());
    lon_dist_ = std::uniform_real_distribution<double>(box_.min_corner().lon(),
                                                       box_.max_corner().lon());
    lat_dist_ = std::uniform_real_distribution<double>(box_.min_corner().lat(),
                                                       box_.max_corner().lat());
  }

  ~bbox() override = default;

  bbox(bbox const&) = delete;
  bbox& operator=(bbox const&) = delete;
  bbox(bbox&&) = delete;
  bbox& operator=(bbox&&) = delete;

  bool contains(location const& loc) const override {
    return boost::geometry::covered_by(loc, box_);
  }

  location random_pt() override {
    return make_location(lon_dist_(mt_), lat_dist_(mt_));
  }

private:
  boost::geometry::model::box<location> box_;
  std::mt19937 mt_;
  std::uniform_real_distribution<double> lon_dist_;
  std::uniform_real_distribution<double> lat_dist_;
};

}  // namespace ppr::benchmark
