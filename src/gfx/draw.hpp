#pragma once

#include <SDL.h>

namespace sienna::gfx {

struct Color {
  Uint8 r, g, b, a = 255;
};

// scales a color's alpha by a 0-1 factor.
inline Color fade(Color c, float amount) {
  c.a = static_cast<Uint8>(c.a * (amount < 0.f ? 0.f : amount > 1.f ? 1.f : amount));
  return c;
}

void fillRect(SDL_Renderer* renderer, const SDL_FRect& rect, Color color);
void vGradient(SDL_Renderer* renderer, const SDL_FRect& rect, Color top, Color bottom);
void hGradient(SDL_Renderer* renderer, const SDL_FRect& rect, Color left, Color right);
void outlineRect(SDL_Renderer* renderer, const SDL_FRect& rect, float thickness, Color color);
void line(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, float thickness,
          Color color);
void ring(SDL_Renderer* renderer, float cx, float cy, float radius, float thickness, Color color);

}  // namespace sienna::gfx
