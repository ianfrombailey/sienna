#include "modules/placeholder_section.hpp"

#include "ui/theme.hpp"
#include "ui/widgets/hints.hpp"

namespace sienna {

void PlaceholderSection::draw(gfx::Gfx& g, const SDL_FRect& area, bool fullscreen) {
  using Font = gfx::Text::Font;

  const float x = area.x + 36.f;
  g.text.draw(title_, 44, Font::SemiBold, x, area.y + 34.f, theme::kTextPrimary);
  gfx::fillRect(g.renderer, {x + 2.f, area.y + 94.f, 56.f, 3.f}, theme::kAccent);
  g.text.draw("Coming soon", 18, Font::Regular, x, area.y + 112.f, theme::kTextSecondary);

  if (fullscreen) {
    drawHints(g, area, {{Button::Start, "Dashboard"}});
  }
}

}  // namespace sienna
