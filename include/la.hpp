#pragma once

#include <array>
#include <algorithm>

namespace la {

template<std::size_t M, std::size_t N, typename T>
class Matrix {
  using ClassType = Matrix<M, N, T>;
  using DataType = std::array<T, M * N>;

  DataType _data;
  public:
  constexpr Matrix() = default;
  constexpr Matrix(DataType&& data): _data(std::move(data)) {}

  T& operator[](std::size_t i) {
    return _data[i];
  }

  template<std::size_t, std::size_t, typename>
    friend class Matrix;

  template<std::size_t O>
    friend Matrix<M, O, T> operator*(
        const Matrix<M, N, T>& l,
        const Matrix<N, O, T>& r) {
      auto result = Matrix<M, O, T>{};

      for (std::size_t i = 0; i < M; ++i) {
        for (std::size_t j = 0; j < O; ++j) {
          for (std::size_t k = 0; k < N; ++k) {
            result[i * O + j] += l._data[i * N + k] * r._data[k * O + j];
          }
        }
      }

      return result;
    }

  friend ClassType operator-(const ClassType& l, const ClassType& r) {
    auto result = ClassType{};
    std::transform(l.begin(), l.end(), r.begin(), result.begin(), std::minus<>{});
    return result;
  }

  auto begin() { return _data.begin(); }
  auto end() { return _data.end(); }
};

using Point = Matrix<3, 1, float>;

constexpr auto point(float x, float y) -> Point {
  return {{x, y, 1}};
}

constexpr auto vector(float x, float y) -> Matrix<3, 1, float> {
  return {{x, y, 0}};
}

constexpr auto translate(float x, float y) -> Matrix<3, 3, float> {
  return {{
    1, 0, x,
    0, 1, y,
    0, 0, 1
  }};
}

constexpr auto rotate(float a) -> Matrix<3, 3, float> {
  return {{
    std::cos(a), -std::sin(a), 0,
    std::sin(a), std::cos(a), 0,
    0, 0, 1
  }};
}

constexpr auto scale(float x, float y) -> Matrix<3, 3, float> {
  return {{
    x, 0, 0,
    0, y, 0,
    0, 0, 1
  }};
}

constexpr auto intersection(Point a, Point b, Point c, Point d) -> Point {
  auto det = (b[0] - a[0]) * (d[1] - c[1]) - (d[0] - c[0]) * (b[1] - a[1]);
  auto x = (b[0] * a[1] - a[0] * b[1]) * (d[0] - c[0]) - (d[0] * c[1] - c[0] * d[1]) * (b[0] - a[0]);
  x = x / det;
  auto y = (b[0] * a[1] - a[0] * b[1]) * (d[1] - c[1]) - (d[0] * c[1] - c[0] * d[1]) * (b[1] - a[1]);
  y = y / det;
  return point(x, y);
}

auto distance(Point a, Point b) -> float {
  return std::sqrt(std::pow(b[0] - a[0], 2) + std::pow(b[1] - a[1], 2));
}

} // namespace la
