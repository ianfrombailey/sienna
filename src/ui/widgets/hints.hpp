#pragma once

#include <SDL.h>

#include <initializer_list>

#include "core/input.hpp"
#include "gfx/gfx.hpp"

namespace sienna {

struct Hint {
  Button button;
  const char* label;
};

void drawButtonGlyph(gfx::Gfx& g, Button button, float cx, float cy, float alpha = 1.f);

// draws button hints right-aligned along the bottom of area.
void drawHints(gfx::Gfx& g, const SDL_FRect& area, std::initializer_list<Hint> hints,
               float alpha = 1.f);

}  // namespace sienna
