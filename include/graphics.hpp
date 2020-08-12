#pragma once

#include <tuple>

namespace graphics {
using point = std::pair<float, float>;

auto operator+(const point& l, const point& r) -> point;
auto operator-(const point& p) -> point;
auto operator-(const point& l, const point& r) -> point;

template<typename T>
auto operator/(const point& l, const T& d) -> point;

template<typename T>
auto operator*(const point& l, const T& d) -> point;

auto translate(const point& source, const point& diff) -> point;
auto rotate(const point& source, const float angle) -> point;
auto cross(const point& l, const point& r) -> float;
auto dot(const point& l, const point& r) -> float;
auto distance(const point& l, const point& r) -> float;
auto line_line(const point& a, const point& b,
    const point& c, const point& d) -> std::pair<point, float>;
auto in_cone(const point& a, const point& b,
    const point& lc, const point& rc) -> bool;

} // namespace graphics
