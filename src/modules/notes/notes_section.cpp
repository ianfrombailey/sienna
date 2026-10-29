#include "modules/notes/notes_section.hpp"

#include "core/fs.hpp"
#include "ui/theme.hpp"

namespace sienna::notes {

namespace {

using Font = gfx::Text::Font;

constexpr std::size_t kMaxTreeRows = 24;

}  // namespace

NotesSection::NotesSection() : browser_(store_) {
  refreshOverview();
}

void NotesSection::refreshOverview() {
  tree_ = store_.folderTree(kMaxTreeRows);
}

void NotesSection::onOpen() {
  browser_.refresh();
}

void NotesSection::onClose() {
  editor_.save();
  refreshOverview();
}

void NotesSection::update(Input& input, float dt) {
  input.setTextMode(false);
  if (editing_) {
    if (!editor_.update(input, dt)) {
      editing_ = false;
      browser_.refresh();
    }
    return;
  }
  const std::string path = browser_.update(input, dt);
  if (!path.empty()) {
    editor_.open(path);
    editing_ = true;
  }
}

void NotesSection::draw(gfx::Gfx& g, const SDL_FRect& area, bool fullscreen) {
  if (!fullscreen) {
    drawOverview(g, area);
    return;
  }
  if (editing_) {
    editor_.draw(g, area);
  } else {
    browser_.draw(g, area);
  }
}

void NotesSection::drawOverview(gfx::Gfx& g, const SDL_FRect& area) {
  const float x = area.x + 36.f;
  const float bottom = area.y + area.h - 24.f;
  g.text.draw("Notes", 44, Font::SemiBold, x, area.y + 34.f, theme::kTextPrimary);
  gfx::fillRect(g.renderer, {x + 2.f, area.y + 94.f, 56.f, 3.f}, theme::kAccent);

  float y = area.y + 118.f;
  g.text.draw("RECENT", 12, Font::SemiBold, x, y, theme::kTextSecondary);
  y += 24.f;
  if (store_.recent().empty()) {
    g.text.draw("No notes opened yet", 16, Font::Regular, x, y, gfx::fade(theme::kTextSecondary, 0.7f));
    y += 28.f;
  }
  for (const std::string& path : store_.recent()) {
    g.text.draw(fs::fileName(path), 17, Font::Regular, x, y, theme::kTextPrimary);
    const std::string folder = store_.relative(fs::parent(path));
    g.text.draw(folder, 13, Font::Regular, area.x + area.w - 36.f, y + 3.f, theme::kTextSecondary,
                gfx::Text::Align::Right);
    y += 28.f;
  }

  y += 14.f;
  if (y + 40.f > bottom) {
    return;
  }
  g.text.draw("FOLDERS", 12, Font::SemiBold, x, y, theme::kTextSecondary);
  y += 24.f;
  if (tree_.empty()) {
    g.text.draw("No folders yet", 16, Font::Regular, x, y, gfx::fade(theme::kTextSecondary, 0.7f));
    return;
  }
  const float th = g.text.lineHeight(15, Font::Regular);
  for (const NotesStore::TreeRow& row : tree_) {
    if (y + 24.f > bottom) {
      g.text.draw("\xE2\x80\xA6", 15, Font::Regular, x, y, theme::kTextSecondary);
      break;
    }
    const float indent = x + 16.f * static_cast<float>(row.depth);
    if (row.depth > 0) {
      gfx::fillRect(g.renderer, {indent - 10.f, y + 11.f, 6.f, 1.f},
                    gfx::fade(theme::kTextSecondary, 0.5f));
    }
    gfx::fillRect(g.renderer, {indent, y + 6.f, 5.f, 2.f}, theme::kTextSecondary);
    gfx::fillRect(g.renderer, {indent, y + 7.f, 12.f, 8.f}, theme::kTextSecondary);
    g.text.draw(row.name, 15, Font::Regular, indent + 20.f, y + (22.f - th) / 2.f,
                theme::kTextPrimary);
    y += 22.f;
  }
}

}  // namespace sienna::notes
