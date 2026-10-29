#include "modules/notes/editor.hpp"

#include <algorithm>

#include "core/fs.hpp"
#include "core/utf8.hpp"
#include "ui/theme.hpp"
#include "ui/widgets/hints.hpp"

namespace sienna::notes {

namespace {

using Font = gfx::Text::Font;

constexpr int kTextSize = 17;
constexpr float kLine = 26.f;
constexpr float kGutter = 44.f;
constexpr float kHeader = 66.f;
constexpr float kFooter = 48.f;
constexpr float kPreviewPadX = 12.f;
constexpr float kPreviewPadY = 8.f;

std::vector<std::string> splitLines(const std::string& text) {
  std::vector<std::string> lines;
  std::size_t start = 0;
  while (true) {
    const std::size_t nl = text.find('\n', start);
    std::string line = text.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    lines.push_back(std::move(line));
    if (nl == std::string::npos) {
      break;
    }
    start = nl + 1;
  }
  return lines;
}

bool ctrl(const KeyPress& k) {
  return (k.mod & (KMOD_CTRL | KMOD_GUI)) != 0;
}

}  // namespace

void Editor::open(const std::string& path) {
  path_ = path;
  std::string data;
  fs::readFile(path_, data);
  lines_ = splitLines(data);
  wraps_.assign(lines_.size(), Wrapped{});
  dirty_ = false;
  mode_ = Mode::Edit;
  line_ = 0;
  col_ = 0;
  scroll_ = 0.f;
  previewStale_ = true;
  previewScroll_ = 0.f;
}

void Editor::save() {
  if (!dirty_ || path_.empty()) {
    return;
  }
  std::string data;
  for (std::size_t i = 0; i < lines_.size(); ++i) {
    if (i > 0) {
      data += '\n';
    }
    data += lines_[i];
  }
  if (fs::writeFile(path_, data)) {
    dirty_ = false;
  }
}

void Editor::setLine(std::size_t i, std::string text) {
  lines_[i] = std::move(text);
  wraps_[i].valid = false;
  dirty_ = true;
  previewStale_ = true;
}

void Editor::insertLine(std::size_t i, std::string text) {
  lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i), std::move(text));
  wraps_.insert(wraps_.begin() + static_cast<std::ptrdiff_t>(i), Wrapped{});
  dirty_ = true;
  previewStale_ = true;
}

void Editor::eraseLine(std::size_t i) {
  lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i));
  wraps_.erase(wraps_.begin() + static_cast<std::ptrdiff_t>(i));
  if (lines_.empty()) {
    lines_.emplace_back();
    wraps_.emplace_back();
  }
  dirty_ = true;
  previewStale_ = true;
}

// the system keyboard can return several lines; they replace line i.
void Editor::replaceWithText(std::size_t i, const std::string& text) {
  const std::vector<std::string> parts = splitLines(text);
  setLine(i, parts[0]);
  for (std::size_t k = 1; k < parts.size(); ++k) {
    insertLine(i + k, parts[k]);
  }
  line_ = i + parts.size() - 1;
  col_ = lines_[line_].size();
}

void Editor::editLine() {
  prompt_.open("Line " + std::to_string(line_ + 1), lines_[line_], true);
}

void Editor::runLineAction(LineAction action) {
  switch (action) {
    case LineAction::InsertBelow:
      insertLine(line_ + 1, "");
      ++line_;
      col_ = 0;
      editLine();
      break;
    case LineAction::InsertAbove:
      insertLine(line_, "");
      col_ = 0;
      editLine();
      break;
    case LineAction::Duplicate:
      insertLine(line_ + 1, lines_[line_]);
      ++line_;
      break;
    case LineAction::MoveUp:
      if (line_ > 0) {
        std::swap(lines_[line_], lines_[line_ - 1]);
        std::swap(wraps_[line_], wraps_[line_ - 1]);
        --line_;
        dirty_ = true;
        previewStale_ = true;
      }
      break;
    case LineAction::MoveDown:
      if (line_ + 1 < lines_.size()) {
        std::swap(lines_[line_], lines_[line_ + 1]);
        std::swap(wraps_[line_], wraps_[line_ + 1]);
        ++line_;
        dirty_ = true;
        previewStale_ = true;
      }
      break;
    case LineAction::Delete:
      eraseLine(line_);
      line_ = std::min(line_, lines_.size() - 1);
      col_ = std::min(col_, lines_[line_].size());
      break;
  }
}

void Editor::togglePreview() {
  save();
  mode_ = mode_ == Mode::Edit ? Mode::Preview : Mode::Edit;
}

void Editor::handleKeyboard(Input& input) {
  if (!input.text().empty()) {
    std::string typed;
    for (char c : input.text()) {
      if (static_cast<unsigned char>(c) >= 0x20) {
        typed += c;
      }
    }
    if (!typed.empty()) {
      std::string l = lines_[line_];
      l.insert(col_, typed);
      setLine(line_, std::move(l));
      col_ += typed.size();
      blink_ = 0.f;
    }
  }

  for (const KeyPress& k : input.keys()) {
    std::string& cur = lines_[line_];
    switch (k.key) {
      case SDLK_RETURN:
      case SDLK_KP_ENTER: {
        std::string rest = cur.substr(col_);
        setLine(line_, cur.substr(0, col_));
        insertLine(line_ + 1, std::move(rest));
        ++line_;
        col_ = 0;
        break;
      }
      case SDLK_BACKSPACE:
        if (col_ > 0) {
          const std::size_t p = utf8::prev(cur, col_);
          std::string l = cur;
          l.erase(p, col_ - p);
          setLine(line_, std::move(l));
          col_ = p;
        } else if (line_ > 0) {
          const std::size_t prevLen = lines_[line_ - 1].size();
          setLine(line_ - 1, lines_[line_ - 1] + cur);
          eraseLine(line_);
          --line_;
          col_ = prevLen;
        }
        break;
      case SDLK_DELETE:
        if (col_ < cur.size()) {
          std::string l = cur;
          l.erase(col_, utf8::next(l, col_) - col_);
          setLine(line_, std::move(l));
        } else if (line_ + 1 < lines_.size()) {
          setLine(line_, cur + lines_[line_ + 1]);
          eraseLine(line_ + 1);
        }
        break;
      case SDLK_LEFT:
        if (col_ > 0) {
          col_ = utf8::prev(cur, col_);
        } else if (line_ > 0) {
          --line_;
          col_ = lines_[line_].size();
        }
        break;
      case SDLK_RIGHT:
        if (col_ < cur.size()) {
          col_ = utf8::next(cur, col_);
        } else if (line_ + 1 < lines_.size()) {
          ++line_;
          col_ = 0;
        }
        break;
      case SDLK_UP:
        if (line_ > 0) {
          --line_;
          col_ = utf8::clamp(lines_[line_], col_);
        }
        break;
      case SDLK_DOWN:
        if (line_ + 1 < lines_.size()) {
          ++line_;
          col_ = utf8::clamp(lines_[line_], col_);
        }
        break;
      case SDLK_HOME:
        col_ = 0;
        break;
      case SDLK_END:
        col_ = cur.size();
        break;
      default:
        break;
    }
    blink_ = 0.f;
  }
}

void Editor::updatePreview(Input& input) {
  const float viewH = previewView_.h;
  const float maxScroll = std::max(0.f, preview_.height + kPreviewPadY - viewH);
  if (input.repeated(Button::Up)) {
    previewScroll_ -= 40.f;
  }
  if (input.repeated(Button::Down)) {
    previewScroll_ += 40.f;
  }
  if (input.repeated(Button::L)) {
    previewScroll_ -= viewH * 0.9f;
  }
  if (input.repeated(Button::R)) {
    previewScroll_ += viewH * 0.9f;
  }
  previewScroll_ = std::clamp(previewScroll_, 0.f, maxScroll);

  for (const SDL_FPoint& tap : input.taps()) {
    if (tap.y < previewView_.y || tap.y > previewView_.y + viewH) {
      continue;
    }
    const float x = tap.x - (previewView_.x + kPreviewPadX);
    const float y = tap.y - (previewView_.y + kPreviewPadY) + previewScroll_;
    for (const PreviewItem& item : preview_.items) {
      if (item.kind != PreviewItem::Kind::Checkbox) {
        continue;
      }
      const SDL_FRect& h = item.hit;
      if (x >= h.x && x <= h.x + h.w && y >= h.y && y <= h.y + h.h) {
        std::string l = lines_[item.line];
        if (toggleCheckbox(l)) {
          setLine(item.line, std::move(l));
          save();
        }
        break;
      }
    }
  }
}

bool Editor::update(Input& input, float dt) {
  blink_ += dt;

  if (prompt_.isOpen()) {
    if (prompt_.update(input, dt)) {
      replaceWithText(line_, prompt_.result());
    }
    return true;
  }

  if (lineMenu_.isOpen()) {
    const int choice = lineMenu_.update(input);
    if (choice >= 0) {
      runLineAction(static_cast<LineAction>(choice));
    }
    return true;
  }

  for (const KeyPress& k : input.keys()) {
    if (k.key == SDLK_ESCAPE) {
      save();
      input.setTextMode(false);
      return false;
    }
    if (ctrl(k) && k.key == SDLK_p) {
      togglePreview();
      return true;
    }
  }

  if (input.pressed(Button::Circle)) {
    save();
    input.setTextMode(false);
    return false;
  }
  if (input.pressed(Button::Triangle)) {
    togglePreview();
    return true;
  }

  if (mode_ == Mode::Preview) {
    input.setTextMode(false);
    updatePreview(input);
    return true;
  }

  // a physical keyboard types straight into the note while editing.
  input.setTextMode(true);
  handleKeyboard(input);

  if (input.repeated(Button::Up) && line_ > 0) {
    --line_;
    col_ = lines_[line_].size();
  }
  if (input.repeated(Button::Down) && line_ + 1 < lines_.size()) {
    ++line_;
    col_ = lines_[line_].size();
  }
  if (input.repeated(Button::L)) {
    line_ = line_ > 10 ? line_ - 10 : 0;
    col_ = lines_[line_].size();
  }
  if (input.repeated(Button::R)) {
    line_ = std::min(lines_.size() - 1, line_ + 10);
    col_ = lines_[line_].size();
  }
  if (input.pressed(Button::Cross)) {
    editLine();
  }
  if (input.pressed(Button::Square)) {
    lineMenu_.open("Line " + std::to_string(line_ + 1),
                   {"Insert line below", "Insert line above", "Duplicate", "Move up", "Move down",
                    "Delete line"});
  }
  return true;
}

const Editor::Wrapped& Editor::wrapped(gfx::Gfx& g, std::size_t i, float width) {
  Wrapped& w = wraps_[i];
  if (!w.valid) {
    w.segments = gfx::wrap(g.text, lines_[i], kTextSize, Font::Regular, width);
    w.valid = true;
  }
  return w;
}

void Editor::drawEdit(gfx::Gfx& g, const SDL_FRect& body) {
  const float textX = body.x + kGutter + 12.f;
  const float textW = body.x + body.w - textX - 8.f;
  if (textW != wrapWidth_) {
    wrapWidth_ = textW;
    for (Wrapped& w : wraps_) {
      w.valid = false;
    }
  }

  // keep the selected line in view.
  float selTop = 0.f;
  for (std::size_t i = 0; i < line_; ++i) {
    selTop += kLine * static_cast<float>(wrapped(g, i, textW).segments.size());
  }
  const float selH = kLine * static_cast<float>(wrapped(g, line_, textW).segments.size());
  if (selTop < scroll_) {
    scroll_ = selTop;
  } else if (selTop + selH > scroll_ + body.h) {
    scroll_ = selTop + selH - body.h;
  }

  const SDL_Rect clip{static_cast<int>(body.x), static_cast<int>(body.y), static_cast<int>(body.w),
                      static_cast<int>(body.h)};
  SDL_RenderSetClipRect(g.renderer, &clip);

  const float th = g.text.lineHeight(kTextSize, Font::Regular);
  const float nh = g.text.lineHeight(12, Font::Mono);
  float y = body.y - scroll_;
  for (std::size_t i = 0; i < lines_.size() && y < body.y + body.h; ++i) {
    const Wrapped& w = wrapped(g, i, textW);
    const float h = kLine * static_cast<float>(w.segments.size());
    if (y + h >= body.y) {
      const bool sel = i == line_;
      if (sel) {
        gfx::hGradient(g.renderer, {body.x, y, body.w, h}, {255, 255, 255, 20},
                       {255, 255, 255, 4});
        gfx::fillRect(g.renderer, {body.x, y, 3.f, h}, theme::kAccent);
      }
      g.text.draw(std::to_string(i + 1), 12, Font::Mono, body.x + kGutter, y + (kLine - nh) / 2.f,
                  sel ? theme::kAccent : gfx::fade(theme::kTextSecondary, 0.6f),
                  gfx::Text::Align::Right);
      for (std::size_t s = 0; s < w.segments.size(); ++s) {
        const gfx::Segment& seg = w.segments[s];
        const float sy = y + kLine * static_cast<float>(s);
        g.text.draw(lines_[i].substr(seg.start, seg.length), kTextSize, Font::Regular, textX,
                    sy + (kLine - th) / 2.f, sel ? theme::kTextPrimary : theme::kTextSecondary);

        const bool last = s + 1 == w.segments.size();
        const bool caretHere = sel && col_ >= seg.start &&
                               (col_ < seg.start + seg.length || (last && col_ == seg.start + seg.length));
        if (caretHere && static_cast<int>(blink_ * 2.f) % 2 == 0) {
          const float cx =
              textX + g.text.width(lines_[i].substr(seg.start, col_ - seg.start), kTextSize,
                                   Font::Regular);
          gfx::fillRect(g.renderer, {cx, sy + 4.f, 2.f, kLine - 8.f}, theme::kAccent);
        }
      }
    }
    y += h;
  }
  SDL_RenderSetClipRect(g.renderer, nullptr);
}

void Editor::drawPreviewMode(gfx::Gfx& g, const SDL_FRect& body) {
  const float width = body.w - 2.f * kPreviewPadX;
  if (previewStale_ || preview_.width != width) {
    preview_ = layoutMarkdown(g.text, lines_, width);
    previewStale_ = false;
  }
  previewView_ = body;
  previewScroll_ =
      std::clamp(previewScroll_, 0.f, std::max(0.f, preview_.height + kPreviewPadY - body.h));

  const SDL_Rect clip{static_cast<int>(body.x), static_cast<int>(body.y), static_cast<int>(body.w),
                      static_cast<int>(body.h)};
  SDL_RenderSetClipRect(g.renderer, &clip);
  drawPreview(g, preview_, body.x + kPreviewPadX, body.y + kPreviewPadY - previewScroll_, body.y,
              body.y + body.h);
  SDL_RenderSetClipRect(g.renderer, nullptr);
}

void Editor::draw(gfx::Gfx& g, const SDL_FRect& area) {
  const float x = area.x + 36.f;
  g.text.draw(fs::fileName(path_), 22, Font::SemiBold, x, area.y + 22.f, theme::kTextPrimary);
  const char* badge = mode_ == Mode::Edit ? "EDIT" : "PREVIEW";
  g.text.draw(badge, 12, Font::SemiBold, area.x + area.w - 36.f, area.y + 30.f, theme::kAccent,
              gfx::Text::Align::Right);
  gfx::fillRect(g.renderer, {x, area.y + 58.f, area.w - 72.f, 1.f}, {255, 255, 255, 30});

  const SDL_FRect body{area.x + 24.f, area.y + kHeader, area.w - 48.f,
                       area.h - kHeader - kFooter};
  if (mode_ == Mode::Edit) {
    drawEdit(g, body);
    drawHints(g, area,
              {{Button::Cross, "Edit line"},
               {Button::Square, "Line"},
               {Button::Triangle, "Preview"},
               {Button::Circle, "Back"}});
  } else {
    drawPreviewMode(g, body);
    drawHints(g, area, {{Button::Triangle, "Edit"}, {Button::Circle, "Back"}});
  }

  lineMenu_.draw(g);
  prompt_.draw(g);
}

}  // namespace sienna::notes
