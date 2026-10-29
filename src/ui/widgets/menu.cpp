#include "ui/widgets/menu.hpp"

#include "core/platform.hpp"
#include "ui/theme.hpp"
#include "ui/widgets/hints.hpp"
#include "ui/widgets/panel.hpp"

namespace sienna {

namespace {

using Font = gfx::Text::Font;

constexpr float kWidth = 380.f;
constexpr float kRow = 38.f;
constexpr float kHeader = 50.f;
constexpr float kFooter = 44.f;

}  // namespace

void Menu::open(std::string title, std::vector<std::string> items) {
  title_ = std::move(title);
  items_ = std::move(items);
  selected_ = 0;
  open_ = !items_.empty();
}

int Menu::update(Input& input) {
  if (!open_) {
    return -1;
  }
  if (input.repeated(Button::Up)) {
    selected_ = selected_ == 0 ? items_.size() - 1 : selected_ - 1;
  }
  if (input.repeated(Button::Down)) {
    selected_ = (selected_ + 1) % items_.size();
  }
  if (input.pressed(Button::Cross)) {
    open_ = false;
    return static_cast<int>(selected_);
  }
  if (input.pressed(Button::Circle)) {
    open_ = false;
  }
  return -1;
}

void Menu::draw(gfx::Gfx& g) {
  if (!open_) {
    return;
  }
  drawBackdrop(g);
  const float h = kHeader + kRow * static_cast<float>(items_.size()) + kFooter;
  const SDL_FRect panel{(platform::kScreenWidth - kWidth) / 2.f, (platform::kScreenHeight - h) / 2.f,
                        kWidth, h};
  drawPanel(g, panel);

  g.text.draw(title_, 16, Font::SemiBold, panel.x + 22.f, panel.y + 16.f, theme::kTextSecondary);

  const float th = g.text.lineHeight(17, Font::Regular);
  for (std::size_t i = 0; i < items_.size(); ++i) {
    const SDL_FRect row{panel.x + 10.f, panel.y + kHeader + kRow * static_cast<float>(i),
                        panel.w - 20.f, kRow};
    if (i == selected_) {
      drawSelection(g, row);
    }
    g.text.draw(items_[i], 17, Font::Regular, row.x + 14.f, row.y + (kRow - th) / 2.f,
                i == selected_ ? theme::kTextPrimary : theme::kTextSecondary);
  }

  drawHints(g, panel, {{Button::Cross, "Select"}, {Button::Circle, "Cancel"}});
}

}  // namespace sienna
