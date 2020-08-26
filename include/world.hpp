#pragma once

#include <vector>
#include <string>

#include <graphics.hpp>

struct Sector {
  float floor, ceil;
  // Points in clockwise order
  std::vector<graphics::point> points;
  std::vector<int> neighbours;
};

auto load_world(const std::string& path) -> std::vector<Sector>;
