#include <cstdint>

#include "ankerl/unordered_dense.h"

#include "boost/filesystem.hpp"
#include "boost/geometry/geometries/geometries.hpp"
#include "boost/geometry/geometries/point_xy.hpp"
#include "boost/geometry/geometry.hpp"

#include "osmium/area/assembler.hpp"
#include "osmium/area/multipolygon_manager.hpp"
#include "osmium/handler.hpp"
#include "osmium/handler/node_locations_for_ways.hpp"
#include "osmium/index/map/flex_mem.hpp"
#include "osmium/io/pbf_input.hpp"
#include "osmium/relations/relations_manager.hpp"
#include "osmium/visitor.hpp"

#include "ppr/common/timing.h"
#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/names.h"
#include "ppr/preprocessing/osm/access.h"
#include "ppr/preprocessing/osm/crossing.h"
#include "ppr/preprocessing/osm/entrance.h"
#include "ppr/preprocessing/osm/level.h"
#include "ppr/preprocessing/osm/way_info.h"
#include "ppr/preprocessing/osm/width.h"
#include "ppr/preprocessing/osm_graph/areas.h"
#include "ppr/preprocessing/osm_graph/extractor.h"
#include "ppr/preprocessing/statistics.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using namespace ppr::preprocessing::osm;

namespace ppr::preprocessing {

using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type,
                                               osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

using point_type = bg::model::d2::point_xy<int32_t>;
using value_type = std::pair<point_type, uint32_t>;
using rtree_type = bgi::rtree<value_type, bgi::rstar<64>>;

struct extract_handler : public osmium::handler::Handler {
  extract_handler(
      osm_graph& g,
      ankerl::unordered_dense::set<osmium::object_id_type>& multipolygon_ways,
      osm_graph_statistics& stats)
      : graph_(g), multipolygon_ways_(multipolygon_ways), stats_(stats) {}

  void node(osmium::Node const& n) noexcept {
    auto const& tags = n.tags();
    if (!access_allowed(tags, true)) {
      auto* node = get_node(n.id(), n.location());
      node->access_allowed_ = false;
      stats_.n_access_not_allowed_nodes_++;
    }
    auto crossing = get_node_crossing_type(tags);
    if (crossing != crossing_type::NONE) {
      auto* node = get_node(n.id(), n.location());
      node->crossing_ = crossing;
      stats_.n_crossing_nodes_++;
    }
    if (tags.has_tag("highway", "elevator")) {
      auto* node = get_node(n.id(), n.location());
      node->elevator_ = true;
      stats_.n_elevators_++;
    }
    if (tags.has_key("entrance")) {
      auto* node = get_node(n.id(), n.location());
      node->entrance_ = true;
      node->door_type_ = get_door_type(tags);
      node->automatic_door_type_ = get_automatic_door_type(tags);
      node->max_width_ = get_max_width_as_cm(tags);
      stats_.n_entrances_++;
    }
    if (tags.has_tag("barrier", "cycle_barrier")) {
      auto* node = get_node(n.id(), n.location());
      node->cycle_barrier_ = true;
      node->max_width_ = get_cycle_barrier_max_width_as_cm(tags);
      stats_.n_cycle_barriers_++;
    }
  }

  void way(osmium::Way const& way) noexcept {
    auto info = get_way_info(way, graph_);
    auto e_info = &graph_.edge_infos_[info.edge_info_];
    if (!info.include_ || e_info->area_) {
      return;
    }

    auto const& way_nodes = way.nodes();
    std::vector<osm_node*> nodes;
    nodes.reserve(way_nodes.size());

    for (auto const& node : way_nodes) {
      auto* current_node = get_node(node.ref(), node.location());
      if (!nodes.empty() && nodes.back() == current_node) {
        continue;
      }
      nodes.push_back(current_node);
      if (!e_info->area_) {
        current_node->exit_ = true;
      }
    }

    osm_node* last_node = nullptr;
    unsigned edges_created = 0;
    for (auto* current_node : nodes) {
      if (last_node != nullptr) {
        auto dist = distance(last_node->location_, current_node->location_);
        last_node->out_edges_.emplace_back(info.edge_info_, last_node,
                                           current_node, dist);
        auto& edge = last_node->out_edges_.back();
        edge.sidewalk_left_ = info.sidewalk_left_;
        edge.sidewalk_right_ = info.sidewalk_right_;
        edge.width_ = info.width_;
        edge.layer_ = info.layer_;
        edges_created++;
      }
      last_node = current_node;
    }

    switch (e_info->type_) {
      case edge_type::STREET:
        if (!e_info->is_rail_edge()) {
          stats_.n_edge_streets_ += edges_created;
        } else {
          stats_.n_edge_railways_ += edges_created;
        }
        break;
      case edge_type::FOOTWAY: stats_.n_edge_footways_ += edges_created; break;
      case edge_type::CROSSING:
        stats_.n_edge_crossings_ += edges_created;
        break;
      case edge_type::CONNECTION:
      case edge_type::ELEVATOR:
      case edge_type::ENTRANCE:
      case edge_type::CYCLE_BARRIER:
        /* not in osm graph */
        break;
    }
  }

  void area(osmium::Area const& area) noexcept {
    auto const from_way = area.from_way();
    auto const orig_id = area.orig_id();
    if (from_way && way_is_part_of_multipolygon(orig_id)) {
      return;
    }
    auto const& area_tags = area.tags();

    for (auto const& outer : area.outer_rings()) {
      auto outer_nodes = get_nodes(outer);
      for (std::size_t i = 0; i < outer_nodes.size() - 1; i++) {
        auto* node = outer_nodes[i];
        if (node->area_outer_) {
          // part of more than one area
          node->exit_ = true;
        }
        node->area_outer_ = true;
      }
      std::vector<std::vector<osm_node*>> inner_nodes;
      for (auto const& inner : area.inner_rings(outer)) {
        inner_nodes.emplace_back(get_nodes(inner));
      }
      graph_.areas_.emplace_back(std::make_unique<osm_area>(
          graph_.areas_.size(), std::move(outer_nodes),
          std::move(inner_nodes)));
      auto* a = graph_.areas_.back().get();
      a->name_ = get_name(area_tags["name"], graph_.names_, graph_.names_map_);
      a->osm_id_ = orig_id;
      a->from_way_ = from_way;
      a->level_ = get_level(area_tags);
      for (auto const& node : a->outer_) {
        auto& areas = node_areas_[node];
        for (auto const& other_area : areas) {
          if (other_area != a && other_area->level_ == a->level_) {
            a->adjacent_areas_.insert(other_area->id_);
            other_area->adjacent_areas_.insert(a->id_);
          }
        }
        areas.insert(a);
      }
    }
  }

private:
  struct osm_node* get_node(std::int64_t osm_id, osmium::Location const& loc) {
    auto n = node_map_.find(osm_id);
    if (n != std::end(node_map_)) {
      return n->second;
    } else {
      graph_.nodes_.emplace_back(
          std::make_unique<struct osm_node>(osm_id, to_merc(loc)));
      auto* node = graph_.nodes_.back().get();
      node_map_[osm_id] = node;
      return node;
    }
  }

  inline bool way_is_part_of_multipolygon(osmium::object_id_type id) const {
    return multipolygon_ways_.find(id) != end(multipolygon_ways_);
  }

  std::vector<osm_node*> get_nodes(osmium::NodeRefList const& nrl) {
    std::vector<osm_node*> v;
    v.reserve(nrl.size());
    std::transform(nrl.begin(), nrl.end(), std::back_inserter(v),
                   [this](osmium::NodeRef const& nr) {
                     return get_node(nr.ref(), nr.location());
                   });
    return v;
  }

  osm_graph& graph_;
  ankerl::unordered_dense::map<std::int64_t, struct osm_node*> node_map_;
  ankerl::unordered_dense::set<osmium::object_id_type> const&
      multipolygon_ways_;
  ankerl::unordered_dense::map<osm_node*,
                               ankerl::unordered_dense::set<osm_area*>>
      node_areas_;
  osm_graph_statistics& stats_;
};

struct multipolygon_way_manager
    : public osmium::relations::RelationsManager<multipolygon_way_manager,
                                                 false, true, false> {

  multipolygon_way_manager(
      osmium::TagsFilter const& filter,
      ankerl::unordered_dense::set<osmium::object_id_type>& ways)
      : filter_(filter), ways_(ways) {}

  bool new_relation(osmium::Relation const& relation) noexcept {
    return relation.tags().has_tag("type", "multipolygon") &&
           osmium::tags::match_any_of(relation.tags(), filter_);
  }

  bool new_member(const osmium::Relation& /*relation*/,
                  const osmium::RelationMember& member,
                  std::size_t /*n*/) noexcept {
    ways_.insert(member.ref());
    return true;
  }

  osmium::TagsFilter const& filter_;
  ankerl::unordered_dense::set<osmium::object_id_type>& ways_;
};

osm_graph extract(std::string const& osm_file, logging& log,
                  statistics& stats) {
  auto const t_start = timing_now();
  auto const infile = osmium::io::File(osm_file);
  stats.osm_input_size_ = boost::filesystem::file_size(osm_file);

  // multipolygon assembler
  auto const assembler_config = osmium::area::Assembler::config_type{};
  osmium::TagsFilter filter{false};
  filter.add_rule(true, "highway", "pedestrian");
  filter.add_rule(true, "area:highway", "pedestrian");
  filter.add_rule(true, "public_transport", "platform");
  filter.add_rule(true, "highway", "platform");
  filter.add_rule(true, "railway", "platform");
  osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager{
      assembler_config, filter};
  ankerl::unordered_dense::set<osmium::object_id_type> multipolygon_ways;
  multipolygon_way_manager mp_way_manager{filter, multipolygon_ways};

  {
    osmium::io::Reader reader{infile, osmium::osm_entity_bits::relation};
    step_progress progress{log, pp_step::OSM_EXTRACT_RELATIONS,
                           reader.file_size()};
    while (auto buffer = reader.read()) {
      progress.set(reader.offset());
      osmium::apply(buffer, mp_manager, mp_way_manager);
    }
    reader.close();
    mp_manager.prepare_for_lookup();
    mp_way_manager.prepare_for_lookup();
  }

  stats.osm_.extract_.d_relations_pass_ =
      log.get_step_duration(pp_step::OSM_EXTRACT_RELATIONS);

  osm_graph og;
  {
    osmium::io::Reader reader{
        infile, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way,
        osmium::io::read_meta::no};

    index_type index;
    location_handler_type location_handler{index};
    location_handler.ignore_errors();

    extract_handler handler(og, multipolygon_ways, stats.osm_);
    step_progress progress{log, pp_step::OSM_EXTRACT_MAIN, reader.file_size()};
    while (auto buffer = reader.read()) {
      progress.set(reader.offset());
      osmium::apply(
          buffer, location_handler, handler,
          mp_manager.handler([&handler](osmium::memory::Buffer&& buffer) {
            osmium::apply(buffer, handler);
          }));
    }
    reader.close();
  }
  stats.osm_.extract_.d_main_pass_ =
      log.get_step_duration(pp_step::OSM_EXTRACT_MAIN);

  process_areas(og, log, stats.osm_);
  stats.osm_.extract_.d_areas_ =
      log.get_step_duration(pp_step::OSM_EXTRACT_AREAS);

  og.create_in_edges();
  og.count_edges();

  stats.osm_.extract_.d_total_ = ms_since(t_start);

  return og;
}

}  // namespace ppr::preprocessing
