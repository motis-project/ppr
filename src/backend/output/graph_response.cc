#include <unordered_set>

#include "ppr/backend/output/graph_response.h"
#include "ppr/output/geojson/graph.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace ppr;
using namespace ppr::output;

namespace ppr::backend::output {

std::string to_graph_response(
    std::vector<routing_graph::edge_rtree_value_type> const& edge_results,
    std::vector<routing_graph::area_rtree_value_type> const& area_results,
    routing_graph const& g, bool const include_visibility_graphs) {
  rapidjson::StringBuffer sb;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

  writer.StartObject();
  writer.String("type");
  writer.String("FeatureCollection");
  writer.String("features");
  writer.StartArray();

  for (auto const& r : area_results) {
    auto const& a = g.data_->areas_[r.second];
    geojson::write_area(writer, a, include_visibility_graphs);
  }

  std::unordered_set<node const*> nodes;

  for (auto const& r : edge_results) {
    auto const e = r.second.get(g.data_);
    nodes.insert(e->from_);
    nodes.insert(e->to_);
  }

  for (auto const n : nodes) {
    geojson::write_node(writer, n);
  }

  for (auto const& r : edge_results) {
    auto const e = r.second.get(g.data_);
    geojson::write_edge(writer, *e);
  }

  writer.EndArray();
  writer.EndObject();

  return sb.GetString();
}

}  // namespace ppr::backend::output
