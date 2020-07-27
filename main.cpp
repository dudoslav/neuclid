#include <iostream>

#include <SDL2/SDL.h>

#include <la.hpp>

static constexpr auto SKY_COLOR = std::array{0, 0, 200};
static constexpr auto FLOOR_COLOR = std::array{50, 50, 50};
static constexpr auto WALL_COLOR = std::array{200, 200, 200};
static constexpr auto PORTAL_COLOR = std::array{0, 200, 0};
static constexpr auto CLIP_L_A = la::point(-0.0001, 0.0001);
static constexpr auto CLIP_L_B = la::point(-200, 50);
static constexpr auto CLIP_R_A = la::point( 0.0001, 0.0001);
static constexpr auto CLIP_R_B = la::point( 200, 50);

static constexpr auto PLAYER_HEIGHT = 1.7f;

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
  Sector{0, 5, {
    la::point(10, 10),
    la::point(-10, 10),
    la::point(-10, -10),
    la::point(10, -10)
  }, {1, -1, -1, -1}},
  Sector{0.2, 2.6, {
    la::point(20, 30),
    la::point(-20, 30),
    la::point(-10, 10),
    la::point(10, 10)
  }, {-1, -1, 0, -1}}
};

auto vline(int x, int u, int l, std::array<int, 3> c) -> void {
  SDL_SetRenderDrawColor(ren, c[0], c[1], c[2], 255);
  SDL_RenderDrawLine(ren, x, u, x, l);
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

    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
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

    // Side view
    auto curr = world[camera_s];
    auto WALL_H = curr.ceiling - curr.floor;
    for (std::size_t i = 0; i < curr.points.size(); ++i) {
      auto f = rmatrix * tmatrix * curr.points[i];
      auto s = rmatrix * tmatrix * curr.points[(i + 1) % curr.points.size()];
      auto c = curr.neighbors[i] != -1 ? PORTAL_COLOR : WALL_COLOR;

      if (la::segment_intersection(f, s, la::point(0, 0), vec)) {
        std::printf("W: %lu\n", i);
        if (curr.neighbors[i] != -1) {
          camera_s = curr.neighbors[i];
        } else {
          // vec = la::vector(0, 0);
        }
      }

      // If the whole wall is behind player
      if (f[1] < 0 && s[1] < 0) continue;
      if (f[1] < 0 || s[1] < 0) {
        auto ff = la::line_intersection(f, s, CLIP_L_A, CLIP_L_B);
        auto ss = la::line_intersection(f, s, CLIP_R_A, CLIP_R_B);

        if (f[1] <= 0) {
          if (ff[1] > 0) {
            f = ff;
          } else {
            f = ss;
          }
        }

        if (s[1] <= 0) {
          if (ff[1] > 0) {
            s = ff;
          } else {
            s = ss;
          }
        }
      }

      auto fx = f[0] * sw/3 / f[1] + sw/2;
      auto sx = s[0] * sw/3 / s[1] + sw/2;
      auto fuy = (WALL_H-PLAYER_HEIGHT) * sh/2 / f[1];
      auto suy = (WALL_H-PLAYER_HEIGHT) * sh/2 / s[1];
      auto fly = PLAYER_HEIGHT * sh/2 / f[1];
      auto sly = PLAYER_HEIGHT * sh/2 / s[1];

      auto stepu = (suy - fuy) / (sx - fx);
      auto stepl = (sly - fly) / (sx - fx);

      auto ffx = static_cast<int>(fx);
      auto ssx = static_cast<int>(sx);
      if (curr.neighbors[i] != -1) {
        const auto& n = world[curr.neighbors[i]];
        auto udiff = (n.floor + n.ceiling) - (curr.floor + curr.ceiling);
        auto nfuy = (WALL_H+udiff-PLAYER_HEIGHT) * sh/2 / f[1];
        auto nsuy = (WALL_H+udiff-PLAYER_HEIGHT) * sh/2 / s[1];
        auto nstepu = (nsuy - nfuy) / (sx - fx);
        auto ldiff = (n.floor) - (curr.floor);
        auto nfly = (-ldiff+PLAYER_HEIGHT) * sh/2 / f[1];
        auto nsly = (-ldiff+PLAYER_HEIGHT) * sh/2 / s[1];
        auto nstepl = (nsly - nfly) / (sx - fx);
        for (int j = std::max(ssx, 0); j < std::min(ffx, sw); ++j) {
          auto uc = sh/2 - suy - stepu * (j - ssx);
          auto lc = sh/2 + sly + stepl * (j - ssx);
          auto nuc = sh/2 - nsuy - nstepu * (j - ssx);
          auto nlc = sh/2 + nsly + nstepl * (j - ssx);
          nuc = std::max(uc, nuc);
          nlc = std::min(lc, nlc);
          vline(j, 0, uc, SKY_COLOR);
          vline(j, uc, nuc, std::array{200, 0, 0});
          vline(j, nuc, nlc, c);
          vline(j, nlc, lc, std::array{200, 0, 0});
          vline(j, lc, sh, FLOOR_COLOR);
        }
      } else {
        for (int j = std::max(ssx, 0); j < std::min(ffx, sw); ++j) {
          auto uc = sh/2 - suy - stepu * (j - ssx);
          auto lc = sh/2 + sly + stepl * (j - ssx);
          vline(j, 0, uc, SKY_COLOR);
          vline(j, uc, lc, c);
          vline(j, lc, sh, FLOOR_COLOR);
        }
      }
    }

    // SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    // SDL_RenderClear(ren);
    // Relative view
    // auto pr = SDL_Rect{-5 + sw/2,-5 + sh/2, 10, 10};
    // SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    // SDL_RenderDrawRect(ren, &pr);
    // SDL_RenderDrawLine(ren, sw/2, sh/2, vec[0] + sw/2, vec[1] + sh/2);
    // SDL_RenderDrawLine(ren, sw/2 + CLIP_L_A[0], sh/2 + CLIP_L_A[1], sw/2 + CLIP_L_B[0], sh/2 + CLIP_L_B[1]);
    // SDL_RenderDrawLine(ren, sw/2 + CLIP_R_A[0], sh/2 + CLIP_R_A[1], sw/2 + CLIP_R_B[0], sh/2 + CLIP_R_B[1]);
    // for (std::size_t i = 0; i < curr.points.size(); ++i) {
    //   auto f = rmatrix * tmatrix * curr.points[i];
    //   auto s = rmatrix * tmatrix * curr.points[(i + 1) % curr.points.size()];
    //   auto c = curr.neighbors[i] != -1 ? PORTAL_COLOR : WALL_COLOR;

    //   // if (la::segment_intersection(f, s, la::point(0, 0), vec)) {
    //   //   std::printf("W: %lu\n", i);
    //   //   if (curr.neighbors[i] != -1) {
    //   //     camera_s = curr.neighbors[i];
    //   //   } else {
    //   //     vec = la::vector(0, 0);
    //   //   }
    //   // }

    //   if (f[1] < 0 && s[1] < 0) continue;
    //   if (f[1] < 0 || s[1] < 0) {
    //     auto ff = la::line_intersection(f, s, CLIP_L_A, CLIP_L_B);
    //     auto ss = la::line_intersection(f, s, CLIP_R_A, CLIP_R_B);

    //     if (f[1] <= 0)
    //       if (ff[1] > 0)
    //         f = ff;
    //       else
    //         f = ss;

    //     if (s[1] <= 0)
    //       if (ff[1] > 0)
    //         s = ff;
    //       else
    //         s = ss;
    //   }

    //   SDL_SetRenderDrawColor(ren, c[0], c[1], c[2], 255);
    //   SDL_RenderDrawLine(ren, f[0] + sw/2, f[1] + sh/2, s[0] + sw/2, s[1] + sh/2);
    // }

    vec = rmatrix * vec;
    camera_x -= vec[0];
    camera_y += vec[1];

    SDL_RenderPresent(ren);
  }

  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();

  return 0;
}
