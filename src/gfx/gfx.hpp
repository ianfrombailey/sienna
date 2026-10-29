#pragma once

#include <SDL.h>

#include "gfx/draw.hpp"
#include "gfx/text.hpp"

namespace sienna::gfx {

// shared drawing context handed to everything that renders.
struct Gfx {
  SDL_Renderer* renderer = nullptr;
  Text text;
};

}  // namespace sienna::gfx
