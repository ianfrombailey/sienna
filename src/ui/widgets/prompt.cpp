#include "ui/widgets/prompt.hpp"

#include <vector>

#include "core/platform.hpp"
#include "core/utf8.hpp"
#include "gfx/wrap.hpp"
#include "ui/theme.hpp"
#include "ui/widgets/panel.hpp"

namespace sienna {

namespace {

using Font = gfx::Text::Font;

constexpr float kWidth = 640.f;
constexpr int kTextSize = 17;
constexpr float kLine = 24.f;

}  // namespace

void TextPrompt::open(std::string title, std::string initial, bool multiline) {
  title_ = std::move(title);
  text_ = std::move(initial);
  multiline_ = multiline;
  caret_ = text_.size();
  blink_ = 0.f;
  open_ = true;
  system_ = platform::openKeyboard(title_, text_, multiline_);
}

void TextPrompt::close(Input& input) {
  open_ = false;
  input.setTextMode(false);
}

bool TextPrompt::update(Input& input, float dt) {
  if (!open_) {
    return false;
  }

  if (system_) {
    std::string result;
    switch (platform::pollKeyboard(result)) {
      case platform::KeyboardStatus::Running:
        return false;
      case platform::KeyboardStatus::Accepted:
        text_ = result;
        open_ = false;
        return true;
      case platform::KeyboardStatus::Cancelled:
        open_ = false;
        return false;
    }
  }

  input.setTextMode(true);
  blink_ += dt;

  if (!input.text().empty()) {
    text_.insert(caret_, input.text());
    caret_ += input.text().size();
    blink_ = 0.f;
  }

  for (const KeyPress& k : input.keys()) {
    switch (k.key) {
      case SDLK_RETURN:
      case SDLK_KP_ENTER:
        if (multiline_ && (k.mod & KMOD_SHIFT)) {
          text_.insert(caret_, "\n");
          ++caret_;
        } else {
          close(input);
          return true;
        }
        break;
      case SDLK_ESCAPE:
        close(input);
        return false;
      case SDLK_BACKSPACE:
        if (caret_ > 0) {
          const std::size_t p = utf8::prev(text_, caret_);
          text_.erase(p, caret_ - p);
          caret_ = p;
        }
        break;
      case SDLK_DELETE:
        if (caret_ < text_.size()) {
          text_.erase(caret_, utf8::next(text_, caret_) - caret_);
        }
        break;
      case SDLK_LEFT:
        caret_ = utf8::prev(text_, caret_);
        break;
      case SDLK_RIGHT:
        caret_ = utf8::next(text_, caret_);
        break;
      case SDLK_HOME:
        caret_ = 0;
        break;
      case SDLK_END:
        caret_ = text_.size();
        break;
      default:
        break;
    }
    blink_ = 0.f;
  }

  // a controller still works while typing on the Mac.
  if (input.pressed(Button::Cross)) {
    close(input);
    return true;
  }
  if (input.pressed(Button::Circle)) {
    close(input);
  }
  return false;
}

void TextPrompt::draw(gfx::Gfx& g) {
  // the system keyboard draws itself.
  if (!open_ || system_) {
    return;
  }
  const float innerW = kWidth - 48.f;

  // split on newlines, then wrap, tracking where the caret lands.
  struct Row {
    std::size_t start, length;
  };
  std::vector<Row> rows;
  std::size_t lineStart = 0;
  while (true) {
    const std::size_t nl = text_.find('\n', lineStart);
    const std::size_t lineEnd = nl == std::string::npos ? text_.size() : nl;
    const std::string line = text_.substr(lineStart, lineEnd - lineStart);
    for (const gfx::Segment& s : gfx::wrap(g.text, line, kTextSize, Font::Regular, innerW)) {
      rows.push_back({lineStart + s.start, s.length});
    }
    if (nl == std::string::npos) {
      break;
    }
    lineStart = nl + 1;
  }

  const float boxH = kLine * static_cast<float>(rows.size()) + 20.f;
  const float h = 54.f + boxH + 44.f;
  const SDL_FRect panel{(platform::kScreenWidth - kWidth) / 2.f, (platform::kScreenHeight - h) / 2.f,
                        kWidth, h};
  drawBackdrop(g);
  drawPanel(g, panel);
  g.text.draw(title_, 16, Font::SemiBold, panel.x + 24.f, panel.y + 18.f, theme::kTextSecondary);

  const SDL_FRect box{panel.x + 14.f, panel.y + 50.f, panel.w - 28.f, boxH};
  gfx::fillRect(g.renderer, box, {0, 0, 0, 90});
  gfx::outlineRect(g.renderer, box, 1.f, {255, 255, 255, 30});

  const float th = g.text.lineHeight(kTextSize, Font::Regular);
  for (std::size_t i = 0; i < rows.size(); ++i) {
    const float y = box.y + 10.f + kLine * static_cast<float>(i);
    const std::string seg = text_.substr(rows[i].start, rows[i].length);
    g.text.draw(seg, kTextSize, Font::Regular, box.x + 10.f, y + (kLine - th) / 2.f,
                theme::kTextPrimary);

    const bool lastOnLine = i + 1 == rows.size() || rows[i + 1].start > rows[i].start + rows[i].length;
    const bool caretHere = caret_ >= rows[i].start &&
                           (caret_ < rows[i].start + rows[i].length ||
                            (caret_ == rows[i].start + rows[i].length && lastOnLine));
    if (caretHere && static_cast<int>(blink_ * 2.f) % 2 == 0) {
      const float cx = box.x + 10.f +
                       g.text.width(text_.substr(rows[i].start, caret_ - rows[i].start), kTextSize,
                                    Font::Regular);
      gfx::fillRect(g.renderer, {cx, y + 3.f, 2.f, kLine - 6.f}, theme::kAccent);
    }
  }

  const char* hint = multiline_ ? "Return  confirm     Shift+Return  new line     Esc  cancel"
                                : "Return  confirm     Esc  cancel";
  g.text.draw(hint, 13, Font::Regular, panel.x + panel.w - 24.f, panel.y + panel.h - 30.f,
              theme::kTextSecondary, gfx::Text::Align::Right);
}

}  // namespace sienna
