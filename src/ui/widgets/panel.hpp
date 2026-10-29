#pragma once

#include <SDL.h>

#include "gfx/gfx.hpp"

namespace sienna {

// dims everything behind a modal.
void drawBackdrop(gfx::Gfx& g);

// raised metallic panel for modals.
void drawPanel(gfx::Gfx& g, const SDL_FRect& rect);

// highlight behind the selected row of a list.
void drawSelection(gfx::Gfx& g, const SDL_FRect& rect);

}  // namespace sienna
