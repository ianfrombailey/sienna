#include "modules/notes/browser.hpp"

#include <algorithm>

#include "ui/theme.hpp"
#include "ui/widgets/hints.hpp"
#include "ui/widgets/panel.hpp"

namespace sienna::notes {

namespace {

using Font = gfx::Text::Font;

constexpr float kRow = 40.f;
constexpr float kListTop = 78.f;
constexpr float kListBottom = 48.f;

// strips characters that can't go in a file name and trims spaces.
std::string cleanName(const std::string& s) {
  std::string out;
  for (char c : s) {
    if (c != '/' && c != '\\' && c != ':' && c != '\n' && c != '\r') {
      out += c;
    }
  }
  const std::size_t a = out.find_first_not_of(' ');
  if (a == std::string::npos) {
    return {};
  }
  const std::size_t b = out.find_last_not_of(' ');
  return out.substr(a, b - a + 1);
}

void drawFolderIcon(SDL_Renderer* r, float x, float cy, gfx::Color c) {
  gfx::fillRect(r, {x, cy - 6.f, 7.f, 3.f}, c);
  gfx::fillRect(r, {x, cy - 4.f, 18.f, 11.f}, c);
}

void drawNoteIcon(SDL_Renderer* r, float x, float cy, gfx::Color c) {
  gfx::outlineRect(r, {x + 2.f, cy - 8.f, 14.f, 16.f}, 1.5f, c);
  gfx::fillRect(r, {x + 5.f, cy - 3.f, 8.f, 1.5f}, c);
  gfx::fillRect(r, {x + 5.f, cy + 1.f, 8.f, 1.5f}, c);
}

}  // namespace

Browser::Browser(NotesStore& store) : store_(store), dir_(store.root()) {
  refresh();
}

void Browser::refresh() {
  if (!fs::isDir(dir_)) {
    dir_ = store_.root();
  }
  entries_ = store_.list(dir_);
  if (selected_ >= entries_.size()) {
    selected_ = entries_.empty() ? 0 : entries_.size() - 1;
  }
}

void Browser::select(const std::string& name) {
  for (std::size_t i = 0; i < entries_.size(); ++i) {
    if (entries_[i].name == name) {
      selected_ = i;
      return;
    }
  }
}

void Browser::openOptions() {
  std::vector<std::string> labels = {"New note", "New folder"};
  optionActions_ = {Action::NewNote, Action::NewFolder};
  if (!entries_.empty()) {
    labels.push_back("Rename");
    labels.push_back("Delete");
    optionActions_.push_back(Action::Rename);
    optionActions_.push_back(Action::Delete);
  }
  options_.open("Options", labels);
}

std::string Browser::finishPrompt(const std::string& text) {
  const Pending pending = pending_;
  pending_ = Pending::None;
  std::string name = cleanName(text);
  if (name.empty()) {
    return {};
  }

  switch (pending) {
    case Pending::NewNote: {
      if (!NotesStore::isNote(name)) {
        name += ".md";
      }
      const std::string path = fs::join(dir_, name);
      if (!fs::exists(path)) {
        fs::writeFile(path, "");
      }
      refresh();
      select(name);
      store_.touch(path);
      return path;
    }
    case Pending::NewFolder:
      fs::makeDir(fs::join(dir_, name));
      refresh();
      select(name);
      return {};
    case Pending::Rename: {
      const fs::Entry& e = entries_[selected_];
      // files keep a note extension so they stay visible.
      if (!e.isDir && !NotesStore::isNote(name)) {
        name += "." + fs::extension(e.name);
      }
      const std::string from = fs::join(dir_, e.name);
      const std::string to = fs::join(dir_, name);
      if (from != to && !fs::exists(to) && fs::move(from, to)) {
        store_.renamed(from, to);
      }
      refresh();
      select(name);
      return {};
    }
    case Pending::None:
      break;
  }
  return {};
}

std::string Browser::update(Input& input, float dt) {
  if (prompt_.isOpen()) {
    if (prompt_.update(input, dt)) {
      return finishPrompt(prompt_.result());
    }
    if (!prompt_.isOpen()) {
      pending_ = Pending::None;
    }
    return {};
  }

  if (confirm_.isOpen()) {
    if (confirm_.update(input) == 0 && !entries_.empty()) {
      const std::string path = fs::join(dir_, entries_[selected_].name);
      fs::removeAll(path);
      store_.removed(path);
      refresh();
    }
    return {};
  }

  if (options_.isOpen()) {
    const int choice = options_.update(input);
    if (choice < 0) {
      return {};
    }
    switch (optionActions_[static_cast<std::size_t>(choice)]) {
      case Action::NewNote:
        pending_ = Pending::NewNote;
        prompt_.open("New note", "", false);
        break;
      case Action::NewFolder:
        pending_ = Pending::NewFolder;
        prompt_.open("New folder", "", false);
        break;
      case Action::Rename:
        pending_ = Pending::Rename;
        prompt_.open("Rename", entries_[selected_].name, false);
        break;
      case Action::Delete: {
        const fs::Entry& e = entries_[selected_];
        confirm_.open(std::string("Delete ") + (e.isDir ? "folder" : "note") + " \"" + e.name + "\"?",
                      {"Delete", "Cancel"});
        break;
      }
    }
    return {};
  }

  const std::size_t n = entries_.size();
  if (n > 0) {
    if (input.repeated(Button::Up) && selected_ > 0) {
      --selected_;
    }
    if (input.repeated(Button::Down) && selected_ + 1 < n) {
      ++selected_;
    }
    if (input.repeated(Button::L)) {
      selected_ = selected_ > 8 ? selected_ - 8 : 0;
    }
    if (input.repeated(Button::R)) {
      selected_ = std::min(n - 1, selected_ + 8);
    }
  }

  if (input.pressed(Button::Square)) {
    openOptions();
    return {};
  }

  if (input.pressed(Button::Circle) && dir_ != store_.root()) {
    const std::string from = fs::fileName(dir_);
    dir_ = fs::parent(dir_);
    scroll_ = 0.f;
    refresh();
    select(from);
    return {};
  }

  if (input.pressed(Button::Cross) && n > 0) {
    const fs::Entry& e = entries_[selected_];
    const std::string path = fs::join(dir_, e.name);
    if (e.isDir) {
      dir_ = path;
      selected_ = 0;
      scroll_ = 0.f;
      refresh();
      return {};
    }
    store_.touch(path);
    return path;
  }
  return {};
}

void Browser::draw(gfx::Gfx& g, const SDL_FRect& area) {
  const float x = area.x + 36.f;
  g.text.draw("Notes", 26, Font::SemiBold, x, area.y + 24.f, theme::kTextPrimary);
  const std::string where = store_.relative(dir_);
  g.text.draw(where, 15, Font::Regular, area.x + area.w - 36.f, area.y + 34.f,
              theme::kTextSecondary, gfx::Text::Align::Right);
  gfx::fillRect(g.renderer, {x, area.y + 66.f, area.w - 72.f, 1.f}, {255, 255, 255, 30});

  const float top = area.y + kListTop;
  const float bottom = area.y + area.h - kListBottom;
  const float viewH = bottom - top;

  if (entries_.empty()) {
    g.text.draw("This folder is empty.", 17, Font::Regular, x, top + 16.f, theme::kTextSecondary);
    g.text.draw("Press Square to make a note or folder.", 15, Font::Regular, x, top + 42.f,
                theme::kTextSecondary);
  } else {
    // keep the selection in view.
    const float selTop = kRow * static_cast<float>(selected_);
    if (selTop < scroll_) {
      scroll_ = selTop;
    } else if (selTop + kRow > scroll_ + viewH) {
      scroll_ = selTop + kRow - viewH;
    }

    const SDL_Rect clip{static_cast<int>(area.x), static_cast<int>(top), static_cast<int>(area.w),
                        static_cast<int>(viewH)};
    SDL_RenderSetClipRect(g.renderer, &clip);
    const float th = g.text.lineHeight(17, Font::Regular);
    for (std::size_t i = 0; i < entries_.size(); ++i) {
      const float y = top + kRow * static_cast<float>(i) - scroll_;
      if (y + kRow < top || y > bottom) {
        continue;
      }
      const SDL_FRect row{area.x + 24.f, y, area.w - 48.f, kRow};
      const bool sel = i == selected_;
      if (sel) {
        drawSelection(g, row);
      }
      const fs::Entry& e = entries_[i];
      const gfx::Color iconColor = sel ? theme::kAccent : theme::kTextSecondary;
      if (e.isDir) {
        drawFolderIcon(g.renderer, row.x + 14.f, y + kRow / 2.f, iconColor);
      } else {
        drawNoteIcon(g.renderer, row.x + 14.f, y + kRow / 2.f, iconColor);
      }
      g.text.draw(e.name, 17, e.isDir ? Font::SemiBold : Font::Regular, row.x + 46.f,
                  y + (kRow - th) / 2.f, sel ? theme::kTextPrimary : theme::kTextSecondary);
    }
    SDL_RenderSetClipRect(g.renderer, nullptr);
  }

  drawHints(g, area,
            {{Button::Cross, "Open"},
             {Button::Circle, "Back"},
             {Button::Square, "Options"},
             {Button::Start, "Dashboard"}});

  options_.draw(g);
  confirm_.draw(g);
  prompt_.draw(g);
}

}  // namespace sienna::notes
