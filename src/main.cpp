#include <SDL.h>

#include <memory>
#include <vector>

#include "core/input.hpp"
#include "core/platform.hpp"
#include "gfx/gfx.hpp"
#include "modules/placeholder_section.hpp"
#include "ui/dashboard/dashboard.hpp"
#include "ui/theme.hpp"

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
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  int outputW = 0;
  int outputH = 0;
  SDL_GetRendererOutputSize(renderer, &outputW, &outputH);
  const float scale = static_cast<float>(outputW) / platform::kScreenWidth;

  gfx::Gfx g;
  g.renderer = renderer;
  if (!g.text.init(renderer, platform::resDir() + "fonts/", scale)) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_Log("SIENNA started, data dir: %s", platform::dataDir().c_str());

  {
    std::vector<std::unique_ptr<Section>> sections;
    sections.push_back(std::make_unique<PlaceholderSection>("Settings"));
    sections.push_back(std::make_unique<PlaceholderSection>("Notes"));
    sections.push_back(std::make_unique<PlaceholderSection>("Music"));
    sections.push_back(std::make_unique<PlaceholderSection>("Reader"));
    sections.push_back(std::make_unique<PlaceholderSection>("Streaming"));
    // opens on music.
    Dashboard dashboard(std::move(sections), 2);

    Input input;
    Uint64 last = SDL_GetPerformanceCounter();
    bool running = true;
    while (running) {
      input.beginFrame();
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
          running = false;
        }
        input.handleEvent(event);
      }

      const Uint64 now = SDL_GetPerformanceCounter();
      float dt = static_cast<float>(now - last) / static_cast<float>(SDL_GetPerformanceFrequency());
      last = now;
      if (dt > 0.1f) {
        dt = 0.1f;
      }

      dashboard.update(input, dt);

      const gfx::Color bg = theme::kBackground;
      SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 255);
      SDL_RenderClear(renderer);
      dashboard.draw(g);
      SDL_RenderPresent(renderer);
    }
  }

  g.text.shutdown();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
