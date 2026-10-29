#include "ui/widgets/panel.hpp"

#include "core/platform.hpp"
#include "ui/theme.hpp"

namespace sienna {

void drawBackdrop(gfx::Gfx& g) {
  gfx::fillRect(g.renderer,
                {0.f, 0.f, static_cast<float>(platform::kScreenWidth),
                 static_cast<float>(platform::kScreenHeight)},
                {0, 0, 0, 150});
}

void drawPanel(gfx::Gfx& g, const SDL_FRect& r) {
  SDL_Renderer* rr = g.renderer;
  gfx::fillRect(rr, {r.x + 4.f, r.y + 6.f, r.w, r.h}, {0, 0, 0, 90});
  gfx::vGradient(rr, r, theme::kBladeTop, theme::kBladeBottom);
  gfx::vGradient(rr, {r.x, r.y, r.w, r.h * 0.4f}, theme::kGloss, {255, 255, 255, 0});
  gfx::outlineRect(rr, r, 1.f, {255, 255, 255, 46});
  gfx::fillRect(rr, {r.x, r.y, r.w, 1.f}, theme::kEdgeHighlight);
}

void drawSelection(gfx::Gfx& g, const SDL_FRect& r) {
  gfx::hGradient(g.renderer, r, {255, 255, 255, 26}, {255, 255, 255, 6});
  gfx::fillRect(g.renderer, {r.x, r.y, 3.f, r.h}, theme::kAccent);
}

}  // namespace sienna
