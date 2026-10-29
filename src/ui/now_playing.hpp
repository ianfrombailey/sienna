#pragma once

#include <SDL.h>

#include <string>

#include "gfx/gfx.hpp"

namespace sienna {

// what the music player is doing, shown on the dashboard. filled in by the music module.
struct NowPlaying {
  bool active = false;
  std::string title;
  std::string artist;
  float progress = 0.f;  // 0-1
};

NowPlaying& nowPlaying();

void drawNowPlayingBar(gfx::Gfx& g, const SDL_FRect& area, const NowPlaying& state, float alpha);

}  // namespace sienna
