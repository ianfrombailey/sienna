#include "ui/widgets/hints.hpp"

#include <vector>

#include "ui/theme.hpp"

namespace sienna {

namespace {

using Font = gfx::Text::Font;

constexpr float kGlyph = 16.f;
constexpr int kLabelSize = 14;
constexpr int kBadgeSize = 10;

const char* badgeText(Button b) {
  switch (b) {
    case Button::L: return "L";
    case Button::R: return "R";
    case Button::Start: return "START";
    case Button::Select: return "SELECT";
    case Button::Up: return "UP";
    case Button::Down: return "DOWN";
    case Button::Left: return "LEFT";
    case Button::Right: return "RIGHT";
    default: return nullptr;
  }
}

float glyphWidth(gfx::Gfx& g, Button b) {
  if (const char* text = badgeText(b)) {
    return g.text.width(text, kBadgeSize, Font::SemiBold) + 10.f;
  }
  return kGlyph;
}

}  // namespace

void drawButtonGlyph(gfx::Gfx& g, Button button, float cx, float cy, float alpha) {
  SDL_Renderer* r = g.renderer;
  const float h = kGlyph / 2.f - 2.f;
  switch (button) {
    case Button::Cross:
      gfx::line(r, cx - h, cy - h, cx + h, cy + h, 2.f, gfx::fade(theme::kCross, alpha));
      gfx::line(r, cx - h, cy + h, cx + h, cy - h, 2.f, gfx::fade(theme::kCross, alpha));
      return;
    case Button::Circle:
      gfx::ring(r, cx, cy, h, 2.f, gfx::fade(theme::kCircle, alpha));
      return;
    case Button::Square:
      gfx::outlineRect(r, {cx - h, cy - h, h * 2.f, h * 2.f}, 2.f,
                       gfx::fade(theme::kSquare, alpha));
      return;
    case Button::Triangle: {
      const gfx::Color c = gfx::fade(theme::kTriangle, alpha);
      const float top = cy - h;
      const float bottom = cy + h * 0.8f;
      gfx::line(r, cx, top, cx + h, bottom, 2.f, c);
      gfx::line(r, cx + h, bottom, cx - h, bottom, 2.f, c);
      gfx::line(r, cx - h, bottom, cx, top, 2.f, c);
      return;
    }
    default:
      break;
  }
  if (const char* text = badgeText(button)) {
    const float w = g.text.width(text, kBadgeSize, Font::SemiBold) + 10.f;
    const SDL_FRect badge{cx - w / 2.f, cy - 8.f, w, 16.f};
    gfx::fillRect(r, badge, gfx::fade({255, 255, 255, 30}, alpha));
    gfx::outlineRect(r, badge, 1.f, gfx::fade({255, 255, 255, 70}, alpha));
    const float th = g.text.lineHeight(kBadgeSize, Font::SemiBold);
    g.text.draw(text, kBadgeSize, Font::SemiBold, cx, cy - th / 2.f,
                gfx::fade(theme::kTextPrimary, alpha), gfx::Text::Align::Center);
  }
}

void drawHints(gfx::Gfx& g, const SDL_FRect& area, std::initializer_list<Hint> hints,
               float alpha) {
  const float cy = area.y + area.h - 22.f;
  const float th = g.text.lineHeight(kLabelSize, Font::Regular);
  float x = area.x + area.w - 24.f;
  // laid out right to left so the last hint sits in the corner.
  std::vector<Hint> list(hints);
  for (auto it = list.rbegin(); it != list.rend(); ++it) {
    const float labelW = g.text.width(it->label, kLabelSize, Font::Regular);
    x -= labelW;
    g.text.draw(it->label, kLabelSize, Font::Regular, x, cy - th / 2.f,
                gfx::fade(theme::kTextSecondary, alpha));
    const float gw = glyphWidth(g, it->button);
    x -= 6.f + gw / 2.f;
    drawButtonGlyph(g, it->button, x, cy, alpha);
    x -= gw / 2.f + 20.f;
  }
}

}  // namespace sienna
