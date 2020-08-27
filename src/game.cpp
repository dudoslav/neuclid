#include <game.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <vector>

#include <SDL2/SDL.h>

#include <graphics.hpp>
#include <world.hpp>

constexpr auto TARGET_W = 640;
constexpr auto TARGET_H = 480;
constexpr auto TARGET_WH = TARGET_W / 2;
constexpr auto TARGET_HH = TARGET_H / 2;

constexpr auto CLIP_L = graphics::point{-50, -50};
constexpr auto CLIP_R = graphics::point{ 50, -50};
constexpr auto CLIP_NEAR = 0.0001f;

constexpr auto WALL_TYPE_SOLID = -1;

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
static SDL_Texture* target;
// static SDL_Texture* stone;

// Game
static bool running = true;
static auto wsad = std::array{false, false, false, false};
static auto screen = std::pair<int, int>{};
static auto delta = 0;
static auto mouse_delta = 0;
static auto portal_h = std::array<std::pair<int, int>, TARGET_W>{};

// Scene
static auto player = Player{{0, 0}, PLAYER_HEIGHT, 0, 0};
static auto world = std::vector<Sector>{};

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
    } else if (event.type == SDL_MOUSEMOTION) {
      mouse_delta = event.motion.xrel;
      if (mouse_delta >= -1 && mouse_delta <= 1)
        mouse_delta = 0;
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
  if (wsad[2]) { move.first = -0.3f * delta / 16; }
  if (wsad[3]) { move.first =  0.3f * delta / 16; }
  // if (wsad[2]) { player.angle -= 0.04f * delta / 16; }
  // if (wsad[3]) { player.angle += 0.04f * delta / 16; }
  player.angle += 10.f * mouse_delta / screen.second;
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
auto render_sector(int sector, int ps, int pe) -> void {
  using namespace graphics;

  auto current = world[sector];

  for_wall(sector, [&](auto f, auto s, auto n){
      // Relative transformation
      f = rotate(translate(f, -player.position), -player.angle);
      s = rotate(translate(s, -player.position), -player.angle);

      // If the whole wall is behind player
      if (f.second > -CLIP_NEAR && s.second > -CLIP_NEAR) return;

      // If out of player vision
      if (!in_cone(f, s, CLIP_L, CLIP_R)) return;

      // Clipping
      auto [cl, tl] = line_line(f, s, {0, 0}, CLIP_L);
      auto [cr, tr] = line_line(f, s, {0, 0}, CLIP_R);
      if (cl.second < -CLIP_NEAR && tl >= 0 && tl <= 1) {
        if (cross(CLIP_L, f) < 0) { f = cl; } else { s = cl; }
      }
      if (cr.second < -CLIP_NEAR && tr >= 0 && tr <= 1) {
        if (cross(CLIP_R, f) > 0) { f = cr; } else { s = cr; }
      }

      // Perspective transformation
      f.first = TARGET_WH * (f.first / -f.second);
      s.first = TARGET_WH * (s.first / -s.second);

      // Move to middle of the screen
      f.first += TARGET_WH;
      s.first += TARGET_WH;

      // If player is behind the wall
      if (f.first >= s.first) return;

      // Upper part of wall
      auto u = player.z - current.ceil;
      // Lower part of wall
      auto l = player.z - current.floor;
      auto sl = point{u * TARGET_HH / -f.second, l * TARGET_HH / -f.second};
      auto el = point{u * TARGET_HH / -s.second, l * TARGET_HH / -s.second};
      // Move to middle of screen
      sl = sl + point{TARGET_HH, TARGET_HH};
      el = el + point{TARGET_HH, TARGET_HH};

      auto start = std::max(static_cast<int>(f.first), ps);
      auto end = std::min(static_cast<int>(s.first), pe);

      if (n == WALL_TYPE_SOLID) {
        // Draw simple wall
        for (int i = start; i < end; ++i) {
          const auto& ph = portal_h[i];
          auto progress = (i - f.first) / (s.first - f.first);
          int uw = std::lerp(sl.first, el.first, progress);
          int lw = std::lerp(sl.second, el.second, progress);
          uw = std::max(uw, ph.first);
          lw = std::min(lw, ph.second);

          vline(i, ph.first, uw, COLOR_CEIL);
          vline(i, uw, lw, COLOR_WALL);
          vline(i, lw, ph.second, COLOR_FLOOR);
        }
      } else {
        // Draw ceiling floor and recursive portal
        // Additional calculations are needed
        auto nbr = world[n];
        // Clipped ceiling
        auto cc = std::min(current.ceil, nbr.ceil);
        // Clipped floor
        auto cf = std::max(current.floor, nbr.floor);
        auto pc = player.z - cc;
        auto pf = player.z - cf;
        auto spl = point{pc * TARGET_HH / -f.second, pf * TARGET_HH / -f.second};
        auto epl = point{pc * TARGET_HH / -s.second, pf * TARGET_HH / -s.second};
        // Move to middle of screen
        spl = spl + point{TARGET_HH, TARGET_HH};
        epl = epl + point{TARGET_HH, TARGET_HH};

        for (int i = start; i < end; ++i) {
          auto& ph = portal_h[i];
          auto progress = (i - f.first) / (s.first - f.first);
          int uw = std::lerp(sl.first, el.first, progress);
          int lw = std::lerp(sl.second, el.second, progress);
          int cw = std::lerp(spl.first, epl.first, progress);
          int fw = std::lerp(spl.second, epl.second, progress);
          uw = std::max(uw, ph.first);
          lw = std::min(lw, ph.second);
          cw = std::max(cw, ph.first);
          fw = std::min(fw, ph.second);

          vline(i, ph.first, uw, COLOR_CEIL);
          vline(i, uw, cw, COLOR_WALL_CEIL);
          vline(i, fw, lw, COLOR_WALL_FLOOR);
          vline(i, lw, ph.second, COLOR_FLOOR);

          ph = {cw, fw};
        }
        render_sector(n, start, end);
      }
    });
}

auto render() -> void {
  // Change render target to target
  SDL_SetRenderTarget(ren, target);
  SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
  SDL_RenderClear(ren);

  // RENDER
  // Create portal with size of whole target
  for (auto& l: portal_h)
    l = {0, TARGET_H};
  // Render sector that player is in with current portal
  render_sector(player.sector, 0, TARGET_W);

  // Change render target bac to window
  SDL_SetRenderTarget(ren, nullptr);
  // Scale whole target into window
  SDL_RenderCopy(ren, target, NULL, NULL);
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
      TARGET_W,
      TARGET_H,
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

  SDL_SetRelativeMouseMode(SDL_TRUE);
  
  world = load_world("../assets/map.txt");
  target = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, TARGET_W, TARGET_H);
  // auto ss = SDL_LoadBMP("../assets/stone.bmp");
  // stone = SDL_CreateTextureFromSurface(ren, ss);
  // SDL_FreeSurface(ss);
}

auto cleanup() -> void {
  // SDL_DestroyTexture(stone);
  SDL_DestroyTexture(target);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
}
