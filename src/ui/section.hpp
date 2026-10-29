#pragma once

#include <SDL.h>

#include "core/input.hpp"
#include "gfx/gfx.hpp"

namespace sienna {

// one part of the app (notes, music, ...), shown on its own blade.
class Section {
public:
  virtual ~Section() = default;

  virtual const char* title() const = 0;

  // only called while the section is open fullscreen. START is reserved for the dashboard.
  virtual void update(const Input& input, float dt) {
    (void)input;
    (void)dt;
  }

  // area is the blade's content region. fullscreen is true once the section is opened.
  virtual void draw(gfx::Gfx& g, const SDL_FRect& area, bool fullscreen) = 0;
};

}  // namespace sienna
