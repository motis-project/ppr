#pragma once

#include <algorithm>
#include <vector>

namespace ppr::preprocessing {

template <typename T>
struct junction_edge {
  junction_edge(T* edge, bool reverse, double angle)
      : edge_(edge), reverse_(reverse), angle_(angle) {}

  T* edge_;
  bool reverse_;
  double angle_;
};

template <typename T>
bool operator<(junction_edge<T> const& lhs, junction_edge<T> const& rhs) {
  return lhs.angle_ < rhs.angle_;
}

template <typename T, typename AcceptableLhs, typename Handler>
void for_edge_pairs_ccw(std::vector<T>& edges, AcceptableLhs acceptable_lhs,
                        Handler handle) {
  for (size_t i = 0; i < edges.size(); i++) {
    auto& e1 = edges[i];
    if (!acceptable_lhs(e1)) {
      continue;
    }
    for (size_t j = (i + 1) % edges.size(); j != i;
         j = (j + 1) % edges.size()) {
      if (handle(e1, edges[j])) {
        break;
      }
    }
  }
}

template <typename T>
junction_edge<T>* next_edge_ccw(std::vector<junction_edge<T>>& edges,
                                T const* ref_edge) {
  if (edges.size() < 2) {
    return nullptr;
  }
  for (size_t i = 0; i < edges.size(); i++) {
    auto const& e1 = edges[i];
    if (e1.edge_ == ref_edge) {
      return &edges[(i + 1) % edges.size()];
    }
  }
  return nullptr;
}

template <typename T>
inline junction_edge<T>* next_edge_ccw(std::vector<junction_edge<T>>& edges,
                                       junction_edge<T> const& ref_edge) {
  return next_edge_ccw(edges, ref_edge.edge_);
}

}  // namespace ppr::preprocessing
