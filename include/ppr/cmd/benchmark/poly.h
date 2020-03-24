#pragma once

#include <limits>
#include <random>

#include "boost/geometry/algorithms/covered_by.hpp"
#include "boost/geometry/algorithms/envelope.hpp"
#include "boost/geometry/geometries/geometries.hpp"

#include "ppr/cmd/benchmark/bounds.h"

namespace ppr::benchmark {

class poly : public bounds {
public:
  using polygon_t = boost::geometry::model::polygon<location>;
  using multi_polygon_t = boost::geometry::model::multi_polygon<polygon_t>;
  using box_t = boost::geometry::model::box<location>;

  explicit poly(const multi_polygon_t& poly) : poly_(poly) {
    box_ = boost::geometry::return_envelope<box_t>(poly);
    std::random_device rd;
    mt_.seed(rd());
    lon_dist_ = std::uniform_real_distribution<double>(box_.min_corner().lon(),
                                                       box_.max_corner().lon());
    lat_dist_ = std::uniform_real_distribution<double>(box_.min_corner().lat(),
                                                       box_.max_corner().lat());
  }

  ~poly() override = default;

  poly(poly const&) = delete;
  poly& operator=(poly const&) = delete;
  poly(poly&&) = delete;
  poly& operator=(poly&&) = delete;

  bool contains(location const& loc) const override {
    return boost::geometry::covered_by(loc, poly_);
  }

  location random_pt() override {
    location pt;
    do {
      pt = make_location(lon_dist_(mt_), lat_dist_(mt_));
    } while (!boost::geometry::covered_by(pt, poly_));
    return pt;
  }

private:
  multi_polygon_t poly_;
  box_t box_;
  std::mt19937 mt_;
  std::uniform_real_distribution<double> lon_dist_;
  std::uniform_real_distribution<double> lat_dist_;
};

}  // namespace ppr::benchmark
