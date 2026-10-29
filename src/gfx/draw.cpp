#include "gfx/draw.hpp"

#include <cmath>

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

void outlineRect(SDL_Renderer* renderer, const SDL_FRect& r, float t, Color color) {
  fillRect(renderer, {r.x, r.y, r.w, t}, color);
  fillRect(renderer, {r.x, r.y + r.h - t, r.w, t}, color);
  fillRect(renderer, {r.x, r.y + t, t, r.h - 2.f * t}, color);
  fillRect(renderer, {r.x + r.w - t, r.y + t, t, r.h - 2.f * t}, color);
}

void line(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, float thickness,
          Color color) {
  const float dx = x2 - x1;
  const float dy = y2 - y1;
  const float len = std::sqrt(dx * dx + dy * dy);
  if (len <= 0.f) {
    return;
  }
  const float nx = -dy / len * thickness / 2.f;
  const float ny = dx / len * thickness / 2.f;
  const SDL_Vertex verts[4] = {
      vertex(x1 + nx, y1 + ny, color),
      vertex(x2 + nx, y2 + ny, color),
      vertex(x2 - nx, y2 - ny, color),
      vertex(x1 - nx, y1 - ny, color),
  };
  static const int indices[6] = {0, 1, 2, 0, 2, 3};
  SDL_RenderGeometry(renderer, nullptr, verts, 4, indices, 6);
}

void ring(SDL_Renderer* renderer, float cx, float cy, float radius, float thickness, Color color) {
  constexpr int kSegments = 28;
  SDL_Vertex verts[kSegments * 2];
  int indices[kSegments * 6];
  const float inner = radius - thickness / 2.f;
  const float outer = radius + thickness / 2.f;
  for (int i = 0; i < kSegments; ++i) {
    const float a = static_cast<float>(i) / kSegments * 6.2831853f;
    const float c = std::cos(a);
    const float s = std::sin(a);
    verts[i * 2] = vertex(cx + c * inner, cy + s * inner, color);
    verts[i * 2 + 1] = vertex(cx + c * outer, cy + s * outer, color);
    const int n = (i + 1) % kSegments;
    const int idx[6] = {i * 2, i * 2 + 1, n * 2 + 1, i * 2, n * 2 + 1, n * 2};
    for (int k = 0; k < 6; ++k) {
      indices[i * 6 + k] = idx[k];
    }
  }
  SDL_RenderGeometry(renderer, nullptr, verts, kSegments * 2, indices, kSegments * 6);
}

}  // namespace sienna::gfx
