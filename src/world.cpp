#include <world.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

auto load_world(const std::string& path) -> std::vector<Sector> {
  auto world = std::vector<Sector>{};
  auto points = std::vector<graphics::point>{};

  auto file = std::ifstream{path};
  // TODO(dudoslav): For god's sake refactor this code below...

  while (!file.eof()) {
    auto op = file.get();
    if (file.eof()) break;

    switch (op) {
      case 'p': {
                  auto x = 0.f;
                  auto y = 0.f;
                  file >> x;
                  file >> y;
                  std::printf("p %f %f\n", x, y);
                  points.emplace_back(x, y);
                  file.ignore(64, '\n');
                }
        break;
      case 's': {
                  auto sector = Sector{};
                  file >> sector.floor;
                  file >> sector.ceil;
                  file.ignore(64, '\n');
                  auto line = std::string{};
                  std::getline(file, line);
                  auto iss = std::istringstream{line};
                  auto n = int{};
                  while (iss >> n) {
                    sector.points.push_back(points[n]);
                  }
                  std::getline(file, line);
                  iss = std::istringstream{line};
                  while (iss >> n) {
                    sector.neighbours.push_back(n);
                  }
                  std::printf("s %f %f\n", sector.floor, sector.ceil);
                  for (const auto& p: sector.points) {
                    std::printf("[%f, %f] ", p.first, p.second);
                  }
                  std::cout << '\n';
                  for (const auto& n: sector.neighbours) {
                    std::printf("%d ", n);
                  }
                  std::cout << '\n';
                  world.push_back(sector);
                }
        break;
    }
  }

  return world;
}
