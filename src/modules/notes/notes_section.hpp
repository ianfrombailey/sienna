#pragma once

#include <vector>

#include "modules/notes/browser.hpp"
#include "modules/notes/editor.hpp"
#include "modules/notes/notes_store.hpp"
#include "ui/section.hpp"

namespace sienna::notes {

class NotesSection : public Section {
public:
  NotesSection();

  const char* title() const override { return "Notes"; }
  void onOpen() override;
  void onClose() override;
  void update(Input& input, float dt) override;
  void draw(gfx::Gfx& g, const SDL_FRect& area, bool fullscreen) override;

private:
  // what the blade shows on the dashboard: recent notes, then the folder tree.
  void drawOverview(gfx::Gfx& g, const SDL_FRect& area);
  void refreshOverview();

  NotesStore store_;
  Browser browser_;
  Editor editor_;
  bool editing_ = false;
  std::vector<NotesStore::TreeRow> tree_;
};

}  // namespace sienna::notes
