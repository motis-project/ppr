#pragma once

#include <cassert>
#include <iomanip>
#include <iostream>
#include <vector>

#include "boost/io/ios_state.hpp"

namespace ppr::preprocessing {

enum class pp_step {
  OSM_EXTRACT_RELATIONS,
  OSM_EXTRACT_MAIN,
  OSM_EXTRACT_AREAS,
  OSM_DEM,
  INT_PARALLEL_STREETS,
  INT_MOVE_CROSSINGS,
  INT_EDGES,
  INT_AREAS,
  RG_JUNCTIONS,
  RG_LINKED_CROSSINGS,
  RG_EDGES,
  RG_AREAS,
  RG_CROSSING_DETOURS,
  DONE
};

constexpr char const* pp_step_names[]{"OSM Extract: Relations",
                                      "OSM Extract: Nodes + Edges",
                                      "OSM Extract: Areas",
                                      "Elevation data",
                                      "Parallel Street Detection",
                                      "Moving Crossings",
                                      "Edge Compression",
                                      "Area Transformation",
                                      "Junctions",
                                      "Linked Street Crossings",
                                      "Edge Creation",
                                      "Area Creation",
                                      "Crossing Detours"};

inline void log_step(pp_step step) {
  assert(step < pp_step::DONE);
  boost::io::ios_all_saver all_saver(std::cout);
  std::cout << "====[" << std::setw(2) << (static_cast<int>(step) + 1) << "/"
            << std::setw(2) << static_cast<int>(pp_step::DONE)
            << "]==== " << pp_step_names[static_cast<std::size_t>(step)]
            << std::endl;
}

}  // namespace ppr::preprocessing
