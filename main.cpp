#include <iostream>
#include <memory>

#include <SDL2/SDL.h>

#include <la.hpp>

auto WindowDeleter = [](SDL_Window* w){ SDL_DestroyWindow(w); };
using Window = std::unique_ptr<SDL_Window, decltype(WindowDeleter)>;

auto RendererDeleter = [](SDL_Renderer* r){ SDL_DestroyRenderer(r); };
using Renderer = std::unique_ptr<SDL_Renderer, decltype(RendererDeleter)>;

static auto running = true;
static auto camera_x = 0.f;
static auto camera_y = 0.f;
static auto camera_a = 0.f;
static auto win = Window{};
static auto ren = Renderer{};
static auto sw = int{};
static auto sh = int{};

const auto room = std::array<la::Point, 5>{
  la::point(-10.f,  10.f),
  la::point( 10.f,  10.f),
  la::point( 10.f,  -10.f),
  la::point( -10.f,  -10.f),
  la::point( -10.f,  10.f)
};

const auto room_c = std::array<std::array<int, 3>, 4>{
  std::array<int,3>{255, 0, 0},
  std::array<int,3>{0, 255, 0},
  std::array<int,3>{255, 255, 0},
  std::array<int,3>{0, 255, 255}
};

auto vline(int x, int u, int l, std::array<int, 3> c) -> void {
  SDL_SetRenderDrawColor(ren.get(), 0, 0, 255, 255);
  SDL_RenderDrawLine(ren.get(), x, 0, x, u);
  SDL_SetRenderDrawColor(ren.get(), c[0], c[1], c[2], 255);
  SDL_RenderDrawLine(ren.get(), x, u, x, l);
  SDL_SetRenderDrawColor(ren.get(), 100, 100, 100, 255);
  SDL_RenderDrawLine(ren.get(), x, l, x, sh);
}

auto main() -> int {
  SDL_Init(SDL_INIT_EVERYTHING);

  win = Window{SDL_CreateWindow("neuclid",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      800,
      600,
      0)};

  ren = Renderer{SDL_CreateRenderer(win.get(), -1, SDL_RENDERER_SOFTWARE)};

  SDL_GetRendererOutputSize(ren.get(), &sw, &sh);
  std::printf("Renderer dimensions: %d, %d\n", sw, sh);

  while (running) {
    auto event = SDL_Event{};
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_KEYDOWN) {
        auto vec = la::rotate(camera_a) * la::vector(0, 1);
        switch (event.key.keysym.sym) {
          case SDLK_w:
            camera_x += vec[0];
            camera_y -= vec[1];
            break;
          case SDLK_s:
            camera_x -= vec[0];
            camera_y += vec[1];
            break;
          case SDLK_a:
            camera_a -= 0.1f;
            break;
          case SDLK_d:
            camera_a += 0.1f;
            break;
        }
      }
    }

    SDL_RenderClear(ren.get());

    auto tmatrix = la::translate(camera_x, camera_y);
    auto rmatrix = la::rotate(camera_a);

    // Absolute view
    // auto player_x = static_cast<int>(camera_x);
    // auto player_y = static_cast<int>(camera_y);
    // auto pr = SDL_Rect{player_x - 5 + sw/2, player_y - 5 + sh/2, 10, 10};
    // auto vec = rmatrix * la::point(20, 0);
    // SDL_SetRenderDrawColor(ren.get(), 255, 255, 255, 255);
    // SDL_RenderDrawRect(ren.get(), &pr);
    // SDL_RenderDrawLine(ren.get(), player_x + sw/2, player_y + sh/2, player_x + vec[0] + sw/2, player_y + vec[1] + sh/2);
    // for (std::size_t i = 0; i < room.size() - 1; ++i) {
    //   auto f = room[i];
    //   auto s = room[i + 1];
    //   SDL_SetRenderDrawColor(ren.get(), room_c[i][0], room_c[i][1], room_c[i][2], 255);
    //   SDL_RenderDrawLine(ren.get(), f[0] + sw/2, f[1] + sh/2, s[0] + sw/2, s[1] + sh/2);
    // }
    // SDL_SetRenderDrawColor(ren.get(), 0, 0, 0, 255);

    // Relative view
    // auto pr = SDL_Rect{-5 + sw/2,-5 + sh/2, 10, 10};
    // auto vec = la::point(0, 20);
    // SDL_SetRenderDrawColor(ren.get(), 255, 255, 255, 255);
    // SDL_RenderDrawRect(ren.get(), &pr);
    // SDL_RenderDrawLine(ren.get(), sw/2, sh/2, vec[0] + sw/2, vec[1] + sh/2);
    // for (std::size_t i = 0; i < room.size() - 1; ++i) {
    //   auto f = rmatrix * tmatrix * room[i];
    //   auto s = rmatrix * tmatrix * room[i + 1];
    //   SDL_SetRenderDrawColor(ren.get(), room_c[i][0], room_c[i][1], room_c[i][2], 255);
    //   SDL_RenderDrawLine(ren.get(), f[0] + sw/2, f[1] + sh/2, s[0] + sw/2, s[1] + sh/2);
    // }
    // SDL_SetRenderDrawColor(ren.get(), 0, 0, 0, 255);

    // Side view
    const auto WALL_H = 400;
    for (std::size_t i = 0; i < room.size() - 1; ++i) {
      auto f = rmatrix * tmatrix * room[i];
      auto s = rmatrix * tmatrix * room[i + 1];

      // If the whole wall is behind player
      if (f[1] < 0 && s[1] < 0) continue;

      if (f[1] < 0) {
        f = intersection(f, s, la::point(-0.01, 0.01), la::point(-20, 5));
      }

      if (s[1] < 0) {
        s = intersection(s, f, la::point(0.01, 0.01), la::point(20, 5));
      }

      auto fx = f[0] * 200 / f[1] + sw/2;
      auto sx = s[0] * 200 / s[1] + sw/2;
      auto fy = WALL_H / f[1];
      auto sy = WALL_H / s[1];
      
      if (fx > sx) {
        std::swap(fx, sx);
        std::swap(fy, sy);
      }

      auto step = (sy - fy) / (sx - fx);

      std::printf("f:(%f, %f) s:(%f, %f)\n", f[0], f[1], s[0], s[1]);
      std::printf("fx:(%f) sx:(%f)\n", fx, sx);

      auto ffx = static_cast<int>(fx);
      auto ssx = static_cast<int>(sx);
      for (int j = std::max(ffx, 0); j < std::min(ssx, sw); ++j) {
        vline(j, sh/2 - fy - step * (j - ffx), sh/2 + fy + step * (j - ffx), room_c[i]);
      }
    }

    // Relative view
    auto pr = SDL_Rect{-5 + sw/2,-5 + sh/2, 10, 10};
    auto vec = la::point(0, 20);
    SDL_SetRenderDrawColor(ren.get(), 255, 255, 255, 255);
    SDL_RenderDrawRect(ren.get(), &pr);
    SDL_RenderDrawLine(ren.get(), sw/2, sh/2, vec[0] + sw/2, vec[1] + sh/2);
    SDL_RenderDrawLine(ren.get(), sw/2 - 0.001, sh/2 + 0.001, sw/2 - 200, sh/2 + 50);
    SDL_RenderDrawLine(ren.get(), sw/2 + 0.001, sh/2 + 0.001, sw/2 + 200, sh/2 + 50);
    for (std::size_t i = 0; i < room.size() - 1; ++i) {
      auto f = rmatrix * tmatrix * room[i];
      auto s = rmatrix * tmatrix * room[i + 1];
      if (f[1] < 0 && s[1] < 0) continue;
      if (f[1] < 0) {
        f = intersection(f, s, la::point(-0.001, 0.001), la::point(-200, 50));
      }

      if (s[1] < 0) {
        s = intersection(s, f, la::point(0.001, 0.001), la::point(200, 50));
      }
      SDL_SetRenderDrawColor(ren.get(), room_c[i][0], room_c[i][1], room_c[i][2], 255);
      SDL_RenderDrawLine(ren.get(), f[0] + sw/2, f[1] + sh/2, s[0] + sw/2, s[1] + sh/2);
    }
    SDL_SetRenderDrawColor(ren.get(), 0, 0, 0, 255);

    SDL_SetRenderDrawColor(ren.get(), 0, 0, 0, 255);
    SDL_RenderPresent(ren.get());
  }

  SDL_Quit();

  return 0;
}
