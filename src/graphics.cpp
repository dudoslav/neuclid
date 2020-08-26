#include <graphics.hpp>

#include <cmath>

namespace graphics {
auto operator+(const point& l, const point& r) -> point {
  return {l.first + r.first, l.second + r.second};
}

auto operator-(const point& p) -> point {
  return {-p.first, -p.second};
}

auto operator-(const point& l, const point& r) -> point {
  return {l.first - r.first, l.second - r.second};
}

template<typename T>
auto operator/(const point& l, const T& d) -> point {
  return {l.first / d, l.second / d};
}

template<typename T>
auto operator*(const point& l, const T& d) -> point {
  return {l.first * d, l.second * d};
}

auto translate(const point& source, const point& diff) -> point {
  return source + diff;
}

auto rotate(const point& source, const float angle) -> point {
  const auto sina = std::sin(angle);
  const auto cosa = std::cos(angle);
  // rotate counterclockwise
  return {
    source.first * cosa - source.second * sina,
    source.second * cosa + source.first * sina
  };
}

/**
 * If result is negative then l is on the right of r
 */
auto cross(const point& l, const point& r) -> float {
  return l.first * r.second - r.first * l.second;
}

auto dot(const point& l, const point& r) -> float {
  return l.first * r.first + l.second * r.second;
}

auto distance(const point& l, const point& r) -> float {
  auto diff = r - l;
  return std::sqrt(std::pow(diff.first, 2) + std::pow(diff.second, 2));
}

/**
 * Calculates line line intersection
 */
auto line_line(const point& a, const point& b,
    const point& c, const point& d) -> std::pair<point, float> {
  auto r = b - a;
  auto s = d - c;

  auto t = cross(c - a, s) / cross(r, s);
  return {a + r * t, t};
}

auto in_cone(const point& a, const point& b,
    const point& lc, const point& rc) -> bool {
  if ((cross(lc, a) < 0 && cross(lc, b) < 0) ||
      (cross(rc, a) > 0 && cross(rc, b) > 0)) {
    return false;
  }

  return true;
}

} // namespace graphics
