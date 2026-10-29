#include "gfx/draw.hpp"

namespace sienna::gfx {

namespace {

SDL_Vertex vertex(float x, float y, Color c) {
  return SDL_Vertex{{x, y}, {c.r, c.g, c.b, c.a}, {0.f, 0.f}};
}

// corners in order: top-left, top-right, bottom-right, bottom-left.
void quad(SDL_Renderer* renderer, const SDL_FRect& r, Color tl, Color tr, Color br, Color bl) {
  const SDL_Vertex verts[4] = {
      vertex(r.x, r.y, tl),
      vertex(r.x + r.w, r.y, tr),
      vertex(r.x + r.w, r.y + r.h, br),
      vertex(r.x, r.y + r.h, bl),
  };
  static const int indices[6] = {0, 1, 2, 0, 2, 3};
  SDL_RenderGeometry(renderer, nullptr, verts, 4, indices, 6);
}

}  // namespace

void fillRect(SDL_Renderer* renderer, const SDL_FRect& rect, Color color) {
  quad(renderer, rect, color, color, color, color);
}

void vGradient(SDL_Renderer* renderer, const SDL_FRect& rect, Color top, Color bottom) {
  quad(renderer, rect, top, top, bottom, bottom);
}

void hGradient(SDL_Renderer* renderer, const SDL_FRect& rect, Color left, Color right) {
  quad(renderer, rect, left, right, right, left);
}

}  // namespace sienna::gfx
