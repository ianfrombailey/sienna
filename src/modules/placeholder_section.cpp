#include "modules/placeholder_section.hpp"

#include "ui/theme.hpp"

namespace sienna {

void PlaceholderSection::draw(gfx::Gfx& g, const SDL_FRect& area, bool fullscreen) {
  using Weight = gfx::Text::Weight;
  using Align = gfx::Text::Align;

  const float x = area.x + 36.f;
  g.text.draw(title_, 44, Weight::SemiBold, x, area.y + 34.f, theme::kTextPrimary);
  gfx::fillRect(g.renderer, {x + 2.f, area.y + 94.f, 56.f, 3.f}, theme::kAccent);
  g.text.draw("Coming soon", 18, Weight::Regular, x, area.y + 112.f, theme::kTextSecondary);

  if (fullscreen) {
    g.text.draw("START  Dashboard", 15, Weight::Regular, area.x + area.w - 28.f,
                area.y + area.h - 40.f, theme::kTextSecondary, Align::Right);
  }
}

}  // namespace sienna
