#include <iostream>

#include <SDL2/SDL.h>

#include <la.hpp>

static constexpr auto WALL_COLOR = std::array{200, 200, 200};
static constexpr auto PORTAL_COLOR = std::array{0, 200, 0};
static constexpr auto CLIP_L_A = la::point(-0.0001, 0.0001);
static constexpr auto CLIP_L_B = la::point(-200, 50);
static constexpr auto CLIP_R_A = la::point( 0.0001, 0.0001);
static constexpr auto CLIP_R_B = la::point( 200, 50);

struct Sector {
  float floor;
  float ceiling;
  std::vector<la::Point> points;
  std::vector<int> neighbors;
};

static auto running = true;
static auto camera_x = 0.f;
static auto camera_y = 0.f;
static auto camera_a = 0.f;
static auto camera_s = 0;
static SDL_Window* win = nullptr;
static SDL_Renderer* ren = nullptr;
static auto sw = 0;
static auto sh = 0;
static auto wsad = std::array{false, false, false, false};

const auto world = std::array{
  Sector{0, 400, {
    la::point(-10, -10),
    la::point(10, -10),
    la::point(20, 10),
    la::point(-20, 10)
  }, {1, -1, -1, -1}},
  Sector{0, 800, {
    la::point(-20, -30),
    la::point(20, -30),
    la::point(10, -10),
    la::point(-10, -10)
  }, {-1, -1, 0, -1}}
};

auto vline(int x, int u, int l, std::array<int, 3> c) -> void {
  SDL_SetRenderDrawColor(ren, 0, 0, 255, 255);
  SDL_RenderDrawLine(ren, x, 0, x, u);
  SDL_SetRenderDrawColor(ren, c[0], c[1], c[2], 255);
  SDL_RenderDrawLine(ren, x, u, x, l);
  SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
  SDL_RenderDrawLine(ren, x, l, x, sh);
}

auto main() -> int {
  SDL_Init(SDL_INIT_EVERYTHING);

  win = SDL_CreateWindow("neuclid",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      800,
      600,
      0);

  if (win == nullptr) {
    std::printf("Failed to create window: %s\n", SDL_GetError());
    return 1;
  }

  ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);

  if (ren == nullptr) {
    std::printf("Failed to create renderer: %s\n", SDL_GetError());
    return 2;
  }

  SDL_GetRendererOutputSize(ren, &sw, &sh);
  std::printf("Renderer dimensions: %d, %d\n", sw, sh);

  while (running) {
    auto event = SDL_Event{};
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          SDL_GetRendererOutputSize(ren, &sw, &sh);
          std::printf("Renderer dimensions: %d, %d\n", sw, sh);
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

    SDL_RenderClear(ren);

    auto tmatrix = la::translate(camera_x, camera_y);
    auto rmatrix = la::rotate(camera_a);

    auto vec = la::vector(0, 0);
    if (wsad[0]) {
      vec[1] = -0.3;
    } else if (wsad[1]) {
      vec[1] = 0.3;
    } else if (wsad[2]) {
      camera_a -= 0.01f;
    } else if (wsad[3]) {
      camera_a += 0.01f;
    }
    vec = rmatrix * vec;
    auto nx = camera_x - vec[0];
    auto ny = camera_y + vec[1];

    // Side view
    auto curr = world[camera_s];
    auto WALL_H = curr.ceiling - curr.floor;
    for (std::size_t i = 0; i < curr.points.size(); ++i) {
      auto f = rmatrix * tmatrix * curr.points[i];
      auto s = rmatrix * tmatrix * curr.points[(i + 1) % curr.points.size()];
      auto c = curr.neighbors[i] != -1 ? PORTAL_COLOR : WALL_COLOR;

      if (la::segment_intersection(f, s, la::point(camera_x, camera_y), la::point(nx, ny))) {
        std::printf("F: %f %f\n", f[0], f[1]);
        std::printf("S: %f %f\n", s[0], s[1]);
        std::printf("C: %f %f\n", camera_x, camera_y);
        std::printf("N: %f %f\n", nx, ny);
        std::printf("W: %lu\n", i);
        if (curr.neighbors[i] != -1) {
          camera_s = curr.neighbors[i];
        }
      }

      // If the whole wall is behind player
      if (f[1] < 0 && s[1] < 0) continue;

      if (f[0] < 0) {
        if (la::side(CLIP_L_A, CLIP_L_B, f) < 0) {
          f = la::line_intersection(f, s, CLIP_L_A, CLIP_L_B);
        }
      } else {
        if (la::side(CLIP_R_A, CLIP_R_B, f) > 0) {
          f = la::line_intersection(f, s, CLIP_R_A, CLIP_R_B);
        }
      }

      if (s[0] < 0) {
        if (la::side(CLIP_L_A, CLIP_L_B, s) < 0) {
          s = la::line_intersection(f, s, CLIP_L_A, CLIP_L_B);
        }
      } else {
        if (la::side(CLIP_R_A, CLIP_R_B, s) > 0) {
          s = la::line_intersection(f, s, CLIP_R_A, CLIP_R_B);
        }
      }

      auto fx = f[0] * sw/2 / f[1] + sw/2;
      auto sx = s[0] * sw/2 / s[1] + sw/2;
      auto fy = WALL_H / f[1];
      auto sy = WALL_H / s[1];
      
      if (fx > sx) {
        std::swap(fx, sx);
        std::swap(fy, sy);
      }

      auto step = (sy - fy) / (sx - fx);

      // std::printf("f:(%f, %f) s:(%f, %f)\n", f[0], f[1], s[0], s[1]);
      // std::printf("fx:(%f) sx:(%f)\n", fx, sx);

      auto ffx = static_cast<int>(fx);
      auto ssx = static_cast<int>(sx);
      for (int j = std::max(ffx, 0); j < std::min(ssx, sw); ++j) {
        vline(j, sh/2 - fy - step * (j - ffx), sh/2 + fy + step * (j - ffx), c);
      }
    }

    camera_x = nx;
    camera_y = ny;

    // SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    // SDL_RenderClear(ren);
    // Relative view
    auto pr = SDL_Rect{-5 + sw/2,-5 + sh/2, 10, 10};
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderDrawRect(ren, &pr);
    SDL_RenderDrawLine(ren, sw/2, sh/2, vec[0] + sw/2, vec[1] + sh/2);
    SDL_RenderDrawLine(ren, sw/2 - 0.0001, sh/2 + 0.0001, sw/2 - 200, sh/2 + 50);
    SDL_RenderDrawLine(ren, sw/2 + 0.0001, sh/2 + 0.0001, sw/2 + 200, sh/2 + 50);
    for (std::size_t i = 0; i < curr.points.size(); ++i) {
      auto f = rmatrix * tmatrix * curr.points[i];
      auto s = rmatrix * tmatrix * curr.points[(i + 1) % curr.points.size()];
      auto c = curr.neighbors[i] != -1 ? PORTAL_COLOR : WALL_COLOR;

      if (f[1] < 0 && s[1] < 0) continue;

      if (f[0] < 0) {
        if (la::side(CLIP_L_A, CLIP_L_B, f) < 0) {
          f = la::line_intersection(f, s, CLIP_L_A, CLIP_L_B);
        }
      } else {
        if (la::side(CLIP_R_A, CLIP_R_B, f) > 0) {
          f = la::line_intersection(f, s, CLIP_R_A, CLIP_R_B);
        }
      }

      if (s[0] < 0) {
        if (la::side(CLIP_L_A, CLIP_L_B, s) < 0) {
          s = la::line_intersection(f, s, CLIP_L_A, CLIP_L_B);
        }
      } else {
        if (la::side(CLIP_R_A, CLIP_R_B, s) > 0) {
          s = la::line_intersection(f, s, CLIP_R_A, CLIP_R_B);
        }
      }

      SDL_SetRenderDrawColor(ren, c[0], c[1], c[2], 255);
      SDL_RenderDrawLine(ren, f[0] + sw/2, f[1] + sh/2, s[0] + sw/2, s[1] + sh/2);
    }

    SDL_RenderPresent(ren);
  }

  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();

  return 0;
}
