#include <game.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <vector>

#include <SDL2/SDL.h>

#include <graphics.hpp>
#include <world.hpp>

constexpr auto CLIP_L = graphics::point{-50, -50};
constexpr auto CLIP_R = graphics::point{ 50, -50};
constexpr auto CLIP_NEAR = 0.0001f;

constexpr auto COLOR_CEIL = 0x888888FF;
constexpr auto COLOR_FLOOR = 0x444444FF;
constexpr auto COLOR_WALL = 0xAAAAAAFF;
constexpr auto COLOR_WALL_CEIL = 0x666666FF;
constexpr auto COLOR_WALL_FLOOR = 0x333333FF;

constexpr auto PLAYER_HEIGHT = 2.f;

struct Player {
  graphics::point position;
  float z;
  float angle;
  int sector;
};

// SDL
static SDL_Window* win;
static SDL_Renderer* ren;

// Game
static bool running = true;
static auto wsad = std::array{false, false, false, false};
static auto screen = std::pair<int, int>{};
static auto delta = 0;

// Scene
static auto player = Player{{0, 0}, 2, 0, 0};
static auto world = std::vector<Sector>{};
// static const auto world = std::array{
//   Sector{0, 5, {
//     {-20, 10},
//     {-10, -10},
//     {10, -10},
//     {20, 10}
//   }, {-1, 1, -1, -1}},
//   Sector{1, 4, {
//     {-10, -10},
//     {-10, -40},
//     {10, -40},
//     {10, -10}
//   }, {-1, -1, -1, 0}}
// };

auto event() -> void {
  auto event = SDL_Event{};
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      running = false;
    } else if (event.type == SDL_WINDOWEVENT) {
      if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
        SDL_GetRendererOutputSize(ren, &screen.first, &screen.second);
      }
    } else if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        case SDLK_w:
          wsad[0] = true;
          break;
        case SDLK_s:
          wsad[1] = true;
          break;
        case SDLK_a:
          wsad[2] = true;
          break;
        case SDLK_d:
          wsad[3] = true;
          break;
      }
    } else if (event.type == SDL_KEYUP) {
      switch (event.key.keysym.sym) {
        case SDLK_w:
          wsad[0] = false;
          break;
        case SDLK_s:
          wsad[1] = false;
          break;
        case SDLK_a:
          wsad[2] = false;
          break;
        case SDLK_d:
          wsad[3] = false;
          break;
      }
    }
  }
}

template<typename Fun>
auto for_wall(const int sector_id, Fun&& fun) -> void {
  auto sector = world[sector_id];
  auto points_count = sector.points.size();
  for (std::size_t i = 0; i < points_count; ++i) {
    auto f = sector.points[i];
    auto s = sector.points[(i + 1) % points_count];
    fun(f, s, sector.neighbours[i]);
  }
}

auto update() -> void {
  using namespace graphics;

  auto move = point{0, 0};

  if (wsad[0]) { move.second = -0.3f * delta / 16; }
  if (wsad[1]) { move.second =  0.3f * delta / 16; }
  if (wsad[2]) { player.angle -= 0.04f * delta / 16; }
  if (wsad[3]) { player.angle += 0.04f * delta / 16; }
  auto np = player.position + rotate(move, player.angle);

  // Collision detection

  for_wall(player.sector, [&np](auto f, auto s, auto n){
      auto [p, t] = line_line(player.position, np, f, s);
      auto [_, u] = line_line(f, s, player.position, np);
      if (t > 0 && t <= 1 && u > 0 && u <= 1) {
        if (n != -1) {
          auto neighbour = world[n];
          if (neighbour.ceil - neighbour.floor < PLAYER_HEIGHT) {
            // If next sector is too small for player
            np = player.position;
          } else {
            // If player fits into next sector
            player.sector = n;
            player.z = PLAYER_HEIGHT + neighbour.floor;
          }
        } else {
          // If collided with wall
          np = player.position;
        }
      }
      });

  player.position = np;
}

auto vline(int x, int u, int l, int c) -> void {
  SDL_SetRenderDrawColor(ren, c >> 24, c >> 16, c >> 8, c);
  SDL_RenderDrawLine(ren, x, u, x, l);
  SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
  SDL_RenderDrawPoint(ren, x, u);
  SDL_RenderDrawPoint(ren, x, l);
}

/**
 * Renders sector in window defined by points a, b, c, d.
 * Points are defined in clockwise order starting from top left
 * point.
 */
auto render_sector(graphics::point a,
    graphics::point b,
    graphics::point c,
    graphics::point d,
    int sector) -> void {
  using namespace graphics;

  auto current = world[sector];

  for_wall(sector, [&](auto f, auto s, auto n){
      // Relative transformation
      f = rotate(translate(f, -player.position), -player.angle);
      s = rotate(translate(s, -player.position), -player.angle);

      // Behind player
      if (f.second > -CLIP_NEAR && s.second > -CLIP_NEAR) {
        return;
      }

      // If out of player vision
      if (!in_cone(f, s, CLIP_L, CLIP_R)) {
        return;
      }

      // Clipping
      auto [cl, tl] = line_line(f, s, {0, 0}, CLIP_L);
      auto [cr, tr] = line_line(f, s, {0, 0}, CLIP_R);
      if (cl.second < -CLIP_NEAR && tl >= 0 && tl <= 1) {
        if (cross(CLIP_L, f) < 0) {
          f = cl;
        } else {
          s = cl;
        }
      }
      if (cr.second < -CLIP_NEAR && tr >= 0 && tr <= 1) {
        if (cross(CLIP_R, f) > 0) {
          f = cr;
        } else {
          s = cr;
        }
      }

      auto df = -f.second;
      auto ds = -s.second;

      // Perspective transformation
      f.first = (screen.first / 2) * (f.first / df);
      s.first = (screen.first / 2) * (s.first / ds);

      // Move to middle of the screen
      f.first += screen.first / 2;
      s.first += screen.first / 2;

      // If player is behind the wall
      if (f.first >= s.first) {
        return;
      }

      auto start = std::max(a.first, f.first);
      auto end = std::min(b.first, s.first);

      auto swhu = (current.ceil - player.z) * screen.second / 2 / df;
      auto ewhu = (current.ceil - player.z) * screen.second / 2 / ds;
      auto swhl = (player.z - current.floor) * screen.second / 2 / df;
      auto ewhl = (player.z - current.floor) * screen.second / 2 / ds;
      swhu = std::lerp(swhu, ewhu, (start - f.first) / (s.first - f.first));
      ewhu = std::lerp(swhu, ewhu, (end - f.first) / (s.first - f.first));
      swhl = std::lerp(swhl, ewhl, (start - f.first) / (s.first - f.first));
      ewhl = std::lerp(swhl, ewhl, (end - f.first) / (s.first - f.first));
      swhu = screen.second / 2 - swhu;
      ewhu = screen.second / 2 - ewhu;
      swhl = screen.second / 2 + swhl;
      ewhl = screen.second / 2 + ewhl;

      if (n != -1) {
        // If portal render floor and ceiling and then define next rendering window
        auto neighbour = world[n];
        auto ceil = std::min(current.ceil, neighbour.ceil);
        auto flor = std::max(current.floor, neighbour.floor);
        auto swhc = (ceil - player.z) * screen.second / 2 / df;
        auto ewhc = (ceil - player.z) * screen.second / 2 / ds;
        auto swhf = (player.z - flor) * screen.second / 2 / df;
        auto ewhf = (player.z - flor) * screen.second / 2 / ds;
        swhc = std::lerp(swhc, ewhc, (start - f.first) / (s.first - f.first));
        ewhc = std::lerp(swhc, ewhc, (end - f.first) / (s.first - f.first));
        swhf = std::lerp(swhf, ewhf, (start - f.first) / (s.first - f.first));
        ewhf = std::lerp(swhf, ewhf, (end - f.first) / (s.first - f.first));
        swhc = screen.second / 2 - swhc;
        ewhc = screen.second / 2 - ewhc;
        swhf = screen.second / 2 + swhf;
        ewhf = screen.second / 2 + ewhf;

        for (int i = start; i < end; ++i) {
          auto p = (i - start) / (end - start);
          auto cup = std::lerp(a.second, b.second, (i - a.first) / (b.first - a.first));
          auto clp = std::lerp(d.second, c.second, (i - d.first) / (c.first - d.first));
          auto cu = std::lerp(swhu, ewhu, p);
          auto cl = std::lerp(swhl, ewhl, p);
          cu = std::max(cup, cu);
          cl = std::min(clp, cl);
          auto cc = std::lerp(swhc, ewhc, p);
          auto cf = std::lerp(swhf, ewhf, p);
          vline(i, cup, cu, COLOR_CEIL);
          vline(i, cu, cc, COLOR_WALL_CEIL);
          vline(i, cf, cl, COLOR_WALL_FLOOR);
          vline(i, cl, clp, COLOR_FLOOR);
        }

        render_sector({start, swhc}, {end, ewhc},
            {end, ewhf}, {start, swhf}, n);
      } else {
        // If not portal render basic wall
        for (int i = start; i < end; ++i) {
          auto p = (i - start) / (end - start);
          auto cup = std::lerp(a.second, b.second, (i - a.first) / (b.first - a.first));
          auto clp = std::lerp(d.second, c.second, (i - d.first) / (c.first - d.first));
          auto cu = std::lerp(swhu, ewhu, p);
          auto cl = std::lerp(swhl, ewhl, p);
          cu = std::max(cup, cu);
          cl = std::min(clp, cl);
          vline(i, cup, cu, COLOR_CEIL);
          vline(i, cu, cl, COLOR_WALL);
          vline(i, cl, clp, COLOR_FLOOR);
        }
        vline(start, swhu, swhl, 0x000000FF);
        vline(end, ewhu, ewhl, 0x000000FF);
      }
    });
}

auto render() -> void {
  SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
  SDL_RenderClear(ren);

  // Render
  render_sector({0, 0}, {screen.first, 0},
      screen, {0, screen.second}, player.sector);

  SDL_RenderPresent(ren);
}

auto start() -> void {
  auto clock = std::chrono::high_resolution_clock{};
  auto old = clock.now();
  while (running) {
    auto curr = clock.now();
    delta = std::chrono::duration_cast<std::chrono::milliseconds>(curr - old).count();
    old = curr;

    event();
    update();
    render();
  }
}

auto init() -> void {
  SDL_Init(SDL_INIT_EVERYTHING);

  win = SDL_CreateWindow("neuclid",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      800,
      600,
      0);

  if (win == nullptr) {
    std::fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
    std::exit(1);
  }

  ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);

  if (ren == nullptr) {
    std::fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
    std::exit(2);
  }

  SDL_GetRendererOutputSize(ren, &screen.first, &screen.second);

  world = load_world("../assets/map.txt");
}

auto cleanup() -> void {
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
}
