#pragma once

#include <algorithm>
#include <limits>

#include "ppr/common/data.h"

namespace ppr {

template <typename T, typename SizeType = std::size_t>
struct matrix {
  using size_type = SizeType;
  using value_type = T;

  T& at(SizeType i, SizeType j) {
    assert(i < m_);
    assert(j < n_);
    return data_[i * n_ + j];
  }

  T const& at(SizeType i, SizeType j) const {
    assert(i < m_);
    assert(j < n_);
    return data_[i * n_ + j];
  }

  void set_size(SizeType m, SizeType n) {
    assert(m_ == 0 && n_ == 0);
    m_ = m;
    n_ = n;
    data_.resize(m * n);
  }

  std::pair<SizeType, SizeType> dimension() const { return {m_, n_}; }

  void init(T const& diagonal, T const& rest) {
    for (SizeType i = 0; i < m_; i++) {
      for (SizeType j = 0; j < n_; j++) {
        at(i, j) = (i == j) ? diagonal : rest;
      }
    }
  }

  data::vector<T> const& data() const { return data_; }
  data::vector<T>& data() { return data_; }

  SizeType m_{0};
  SizeType n_{0};
  data::vector<T> data_;
};

template <typename T, typename SizeType = std::size_t>
inline matrix<T, SizeType> make_matrix(SizeType m, SizeType n) {
  return matrix<T, SizeType>{m, n, data::vector<T>(m * n)};
}

template <typename T, typename SizeType = std::size_t>
inline matrix<T, SizeType> make_matrix(matrix<T, SizeType> const& o, SizeType m,
                                       SizeType n, T const& diagonal,
                                       T const& rest) {
  auto mx = make_matrix<T, SizeType>(m, n);
  for (SizeType i = 0; i < m; i++) {
    for (SizeType j = 0; j < n; j++) {
      if (i < o.m_ && j < o.n_) {
        mx.at(i, j) = o.at(i, j);
      } else {
        mx.at(i, j) = (i == j) ? diagonal : rest;
      }
    }
  }
  return mx;
}

}  // namespace ppr
