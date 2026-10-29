#pragma once

#include <SDL.h>

#include <cstddef>
#include <string>
#include <vector>

#include "core/fs.hpp"
#include "core/input.hpp"
#include "gfx/gfx.hpp"
#include "modules/notes/notes_store.hpp"
#include "ui/widgets/menu.hpp"
#include "ui/widgets/prompt.hpp"

namespace sienna::notes {

// folder view of the notes directory with create, rename and delete.
class Browser {
public:
  explicit Browser(NotesStore& store);

  void refresh();
  // returns the path of a note the user opened, otherwise an empty string.
  std::string update(Input& input, float dt);
  void draw(gfx::Gfx& g, const SDL_FRect& area);

private:
  enum class Action { NewNote, NewFolder, Rename, Delete };
  enum class Pending { None, NewNote, NewFolder, Rename };

  void select(const std::string& name);
  void openOptions();
  std::string finishPrompt(const std::string& text);

  NotesStore& store_;
  std::string dir_;
  std::vector<fs::Entry> entries_;
  std::size_t selected_ = 0;
  float scroll_ = 0.f;

  Menu options_;
  std::vector<Action> optionActions_;
  Menu confirm_;
  TextPrompt prompt_;
  Pending pending_ = Pending::None;
};

}  // namespace sienna::notes
