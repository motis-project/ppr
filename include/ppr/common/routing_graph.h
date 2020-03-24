#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "boost/geometry/geometries/geometries.hpp"
#include "boost/geometry/geometries/point_xy.hpp"
#include "boost/geometry/geometry.hpp"
#include "boost/geometry/index/rtree.hpp"

#include "boost/interprocess/managed_mapped_file.hpp"

#include "ppr/common/area.h"
#include "ppr/common/data.h"
#include "ppr/common/edge.h"
#include "ppr/common/mlock.h"
#include "ppr/common/node.h"

namespace ppr {

struct routing_graph_data {
  data::vector<data::unique_ptr<data::string>> names_;
  data::vector<data::unique_ptr<edge_info>> edge_infos_;
  data::vector<data::unique_ptr<node>> nodes_;
  data::vector<area> areas_;
  node_id_t max_node_id_{0};
};

struct rg_edge {
  edge const* get(routing_graph_data const* rg) const {
    return rg->nodes_[node_index_]->out_edges_[edge_index_].get();
  }

  uint64_t node_index_;
  uint32_t edge_index_;
};

enum class rtree_options { DEFAULT, PREFETCH, LOCK };

template <typename Value>
struct rtree_data {
  using params_t = boost::geometry::index::rstar<16>;
  using indexable_t = boost::geometry::index::indexable<Value>;
  using equal_to_t = boost::geometry::index::equal_to<Value>;
  using allocator_t = boost::interprocess::allocator<
      Value, boost::interprocess::managed_mapped_file::segment_manager>;
  using rtree_t = boost::geometry::index::rtree<Value, params_t, indexable_t,
                                                equal_to_t, allocator_t>;

  void open(std::string const& filename, std::size_t size) {
    file_ = boost::interprocess::managed_mapped_file(
        boost::interprocess::open_or_create, filename.c_str(), size);
    alloc_ = std::make_unique<allocator_t>(file_.get_segment_manager());
    filename_ = filename;
    max_size_ = size;
  }

  void load_or_construct(
      std::function<std::vector<Value>()> const& create_entries) {
    auto r = file_.find<rtree_t>("rtree");
    if (r.first == nullptr) {
      rtree_ = file_.construct<rtree_t>("rtree")(
          create_entries(), params_t(), indexable_t(), equal_to_t(), *alloc_);
      file_.flush();
      file_ = {};
      boost::interprocess::managed_mapped_file::shrink_to_fit(
          filename_.c_str());
      open(filename_, max_size_);
      load_or_construct(create_entries);
    } else {
      rtree_ = r.first;
    }
  }

  bool lock() { return lock_memory(file_.get_address(), file_.get_size()); }

  bool unlock() { return unlock_memory(file_.get_address(), file_.get_size()); }

  void prefetch() {
    if (lock()) {
      unlock();
    } else {
      char c = 0;
      auto const base = reinterpret_cast<char*>(file_.get_address());
      for (std::size_t i = 0U; i < file_.get_size(); i += 4096) {
        c += base[i];
      }
      volatile char cs = c;
      (void)cs;
    }
  }

  bool initialized() const { return rtree_ != nullptr && !rtree_->empty(); }

  rtree_t const* operator->() const { return rtree_; }
  rtree_t* operator->() { return rtree_; }

  boost::interprocess::managed_mapped_file file_;
  std::unique_ptr<allocator_t> alloc_;
  rtree_t* rtree_{nullptr};
  std::string filename_;
  std::size_t max_size_{};
};

struct routing_graph {
  using rtree_point_type = location;
  using rtree_box_type = boost::geometry::model::box<rtree_point_type>;
  using edge_rtree_value_type = std::pair<rtree_box_type, rg_edge>;
  using area_rtree_value_type = std::pair<rtree_box_type, uint32_t>;

  void init() {
    data_ptr_ = std::make_unique<routing_graph_data>();
    data_ = data_ptr_.get();
    data_->max_node_id_ = 0;
  }

  void create_in_edges() {
    for (auto& n : data_->nodes_) {
      for (auto& edge : n->out_edges_) {
        const_cast<node*>(static_cast<node const*>(edge->to_))  // NOLINT
            ->in_edges_.emplace_back(edge.get());
      }
    }
  }

  void prepare_for_routing(std::string const& edge_rtree_file,
                           std::string const& area_rtree_file,
                           std::size_t edge_rtree_size,
                           std::size_t area_rtree_size,
                           rtree_options rtree_opt) {
    create_edge_rtree(edge_rtree_file, edge_rtree_size, rtree_opt);
    create_area_rtree(area_rtree_file, area_rtree_size, rtree_opt);
  }

  void prepare_for_routing(std::size_t edge_rtree_size = 1024UL * 1024 * 1024 *
                                                         3,
                           std::size_t area_rtree_size = 1024UL * 1024 * 1024 *
                                                         1,
                           rtree_options rtree_opt = rtree_options::DEFAULT) {
    std::string edge_rtree_file =
        filename_.empty() ? "routing-graph.ppr.ert" : filename_ + ".ert";
    std::string area_rtree_file =
        filename_.empty() ? "routing-graph.ppr.art" : filename_ + ".art";
    prepare_for_routing(edge_rtree_file, area_rtree_file, edge_rtree_size,
                        area_rtree_size, rtree_opt);
  }

private:
  void create_edge_rtree(std::string const& filename, std::size_t size,
                         rtree_options rtree_opt) {
    if (edge_rtree_.initialized()) {
      return;
    }
    edge_rtree_.open(filename, size);
    edge_rtree_.load_or_construct(
        [&]() { return create_edge_rtree_entries(); });
    apply_rtree_options(edge_rtree_, rtree_opt);
  }

  std::vector<edge_rtree_value_type> create_edge_rtree_entries() {
    std::vector<edge_rtree_value_type> values;
    values.reserve(data_->nodes_.size() * 2);
    for (auto node_index = 0UL; node_index < data_->nodes_.size();
         ++node_index) {
      auto const& edges = data_->nodes_[node_index]->out_edges_;
      for (auto edge_index = 0U; edge_index < edges.size(); ++edge_index) {
        auto box = boost::geometry::return_envelope<rtree_box_type>(
            edges[edge_index]->path_);
        values.emplace_back(box, rg_edge{node_index, edge_index});
      }
    }
    return values;
  }

  void create_area_rtree(std::string const& filename, std::size_t size,
                         rtree_options rtree_opt) {
    if (area_rtree_.initialized()) {
      return;
    }
    area_rtree_.open(filename, size);
    area_rtree_.load_or_construct(
        [&]() { return create_area_rtree_entries(); });
    apply_rtree_options(area_rtree_, rtree_opt);
  }

  std::vector<area_rtree_value_type> create_area_rtree_entries() {
    std::vector<area_rtree_value_type> values;
    values.reserve(data_->areas_.size());
    for (auto const& area : data_->areas_) {
      auto box =
          boost::geometry::return_envelope<rtree_box_type>(area.polygon_);
      values.emplace_back(box, area.id_);
    }
    return values;
  }

  template <typename T>
  void apply_rtree_options(rtree_data<T>& rtree, rtree_options rtree_opt) {
    if (rtree_opt == rtree_options::LOCK) {
      rtree.lock();
    } else if (rtree_opt == rtree_options::PREFETCH) {
      rtree.prefetch();
    }
  }

public:
  routing_graph_data* data_{nullptr};
  cista::buffer data_buffer_;
  std::unique_ptr<routing_graph_data> data_ptr_;
  std::string filename_;

  rtree_data<edge_rtree_value_type> edge_rtree_;
  rtree_data<area_rtree_value_type> area_rtree_;
};

}  // namespace ppr
