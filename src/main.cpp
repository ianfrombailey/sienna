#include <SDL.h>

#include "core/platform.hpp"

using namespace sienna;

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  platform::init();

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow("SIENNA", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        platform::kScreenWidth, platform::kScreenHeight,
                                        SDL_WINDOW_ALLOW_HIGHDPI);
  if (!window) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_RenderSetLogicalSize(renderer, platform::kScreenWidth, platform::kScreenHeight);

  SDL_Log("SIENNA started, data dir: %s", platform::dataDir().c_str());

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
    }

    SDL_SetRenderDrawColor(renderer, 18, 18, 24, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
