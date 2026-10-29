#include "ui/dashboard/dashboard.hpp"

#include <cctype>
#include <cmath>
#include <string>

#include "core/platform.hpp"
#include "ui/now_playing.hpp"
#include "ui/theme.hpp"

namespace sienna {

namespace {

constexpr float kW = static_cast<float>(platform::kScreenWidth);
constexpr float kH = static_cast<float>(platform::kScreenHeight);

float ease(float current, float target, float k) {
  const float next = current + (target - current) * k;
  return std::fabs(target - next) < 0.01f ? target : next;
}

std::string upper(const char* s) {
  std::string out(s);
  for (char& c : out) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  return out;
}

}  // namespace

Dashboard::Dashboard(std::vector<std::unique_ptr<Section>> sections, std::size_t selected)
    : selected_(selected) {
  for (auto& s : sections) {
    blades_.push_back(Blade{std::move(s), 0.f});
  }
  if (selected_ >= blades_.size()) {
    selected_ = 0;
  }
  for (std::size_t i = 0; i < blades_.size(); ++i) {
    blades_[i].x = targetX(i);
  }
}

float Dashboard::targetX(std::size_t i) const {
  const float spine = theme::kSpineWidth;
  const std::size_t n = blades_.size();
  if (!open_) {
    if (i <= selected_) {
      return static_cast<float>(i) * spine;
    }
    return kW - static_cast<float>(n - i) * spine;
  }
  // opened: the selected blade takes the screen, the rest slide off the edges.
  if (i < selected_) {
    return -static_cast<float>(selected_ - i + 1) * spine;
  }
  if (i == selected_) {
    return 0.f;
  }
  return kW + static_cast<float>(i - selected_) * spine;
}

void Dashboard::update(Input& input, float dt) {
  for (Blade& b : blades_) {
    b.section->tick(dt);
  }

  if (open_) {
    if (input.pressed(Button::Start)) {
      open_ = false;
      blades_[selected_].section->onClose();
      input.setTextMode(false);
    } else {
      blades_[selected_].section->update(input, dt);
    }
  } else {
    if (input.pressed(Button::L) && selected_ > 0) {
      --selected_;
    }
    if (input.pressed(Button::R) && selected_ + 1 < blades_.size()) {
      ++selected_;
    }
    if (input.pressed(Button::Cross)) {
      open_ = true;
      blades_[selected_].section->onOpen();
    }
  }

  const float k = 1.f - std::exp(-dt * theme::kSlideSpeed);
  for (std::size_t i = 0; i < blades_.size(); ++i) {
    blades_[i].x = ease(blades_[i].x, targetX(i), k);
  }
  openAmount_ = ease(openAmount_, open_ ? 1.f : 0.f, k);
}

void Dashboard::draw(gfx::Gfx& g) {
  // drawn left to right so each blade overlaps the one before it.
  for (std::size_t i = 0; i < blades_.size(); ++i) {
    drawBlade(g, i);
  }
}

void Dashboard::drawBlade(gfx::Gfx& g, std::size_t i) {
  using gfx::fade;
  using Font = gfx::Text::Font;

  const Blade& blade = blades_[i];
  const float x = blade.x;
  if (x >= kW) {
    return;
  }
  const bool selected = i == selected_;
  // the selected blade's spine folds away as it opens.
  const float spineAmount = selected ? 1.f - openAmount_ : 1.f;
  const float spine = theme::kSpineWidth * spineAmount;
  SDL_Renderer* r = g.renderer;

  gfx::hGradient(r, {x - 18.f, 0.f, 18.f, kH}, {0, 0, 0, 0}, theme::kShadow);

  const float midY = kH * theme::kBladeMidStop;
  gfx::vGradient(r, {x, 0.f, kW, midY}, theme::kBladeTop, theme::kBladeMid);
  gfx::vGradient(r, {x, midY, kW, kH - midY}, theme::kBladeMid, theme::kBladeBottom);
  gfx::vGradient(r, {x, 0.f, kW, kH * 0.42f}, theme::kGloss, {255, 255, 255, 0});

  gfx::fillRect(r, {x, 0.f, 1.f, kH}, fade(theme::kEdgeHighlight, spineAmount));
  gfx::fillRect(r, {x + 1.f, 0.f, 1.f, kH}, fade({255, 255, 255, 40}, spineAmount));

  if (spine > 0.5f) {
    gfx::vGradient(r, {x, 0.f, spine, kH}, fade({255, 255, 255, 14}, spineAmount),
                   fade({255, 255, 255, 4}, spineAmount));
    gfx::fillRect(r, {x + spine - 1.f, 0.f, 1.f, kH}, fade({0, 0, 0, 120}, spineAmount));
    gfx::fillRect(r, {x + spine, 0.f, 1.f, kH}, fade({255, 255, 255, 20}, spineAmount));
    if (selected) {
      gfx::fillRect(r, {x + spine - 3.f, 0.f, 2.f, kH}, fade(theme::kAccent, spineAmount * 0.8f));
    }

    const std::string label = upper(blade.section->title());
    const SDL_FPoint size = g.text.measure(label, 15, Font::SemiBold);
    const gfx::Color color = selected ? theme::kAccent : theme::kTextSecondary;
    g.text.drawVertical(label, 15, Font::SemiBold, x + spine / 2.f, kH - 28.f - size.x / 2.f,
                        fade(color, spineAmount));
  }

  if (!selected) {
    return;
  }

  // content runs from the spine to the first blade stacked on the right.
  const float contentX = x + spine;
  const float right = i + 1 < blades_.size() ? std::fmin(blades_[i + 1].x, kW) : kW;
  const SDL_FRect area{contentX, 0.f, right - contentX, kH};
  blade.section->draw(g, area, openAmount_ > 0.5f);

  const NowPlaying& np = nowPlaying();
  if (np.active && openAmount_ < 1.f && blade.section->showsNowPlayingBar()) {
    drawNowPlayingBar(g, {area.x + 24.f, kH - 72.f, area.w - 48.f, 48.f}, np, 1.f - openAmount_);
  }
}

}  // namespace sienna
