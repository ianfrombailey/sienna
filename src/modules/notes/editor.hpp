#pragma once

#include <SDL.h>

#include <cstddef>
#include <string>
#include <vector>

#include "core/input.hpp"
#include "gfx/gfx.hpp"
#include "gfx/wrap.hpp"
#include "modules/notes/markdown.hpp"
#include "modules/notes/notes_store.hpp"
#include "ui/widgets/menu.hpp"
#include "ui/widgets/prompt.hpp"

namespace sienna::notes {

// line-by-line editor with a markdown preview mode. saves on leave and on preview.
class Editor {
public:
  explicit Editor(NotesStore& store);

  void open(const std::string& path);
  void save();
  // returns false once the user leaves the editor.
  bool update(Input& input, float dt);
  void draw(gfx::Gfx& g, const SDL_FRect& area);

private:
  enum class Mode { Edit, Preview };
  enum class LineAction { InsertBelow, InsertAbove, Duplicate, MoveUp, MoveDown, Delete };

  struct Wrapped {
    bool valid = false;
    std::vector<gfx::Segment> segments;
  };

  void setLine(std::size_t i, std::string text);
  void insertLine(std::size_t i, std::string text);
  void eraseLine(std::size_t i);
  void replaceWithText(std::size_t i, const std::string& text);
  void runLineAction(LineAction action);
  void editLine();
  void togglePreview();
  void handleKeyboard(Input& input);
  void updatePreview(Input& input);

  void drawEdit(gfx::Gfx& g, const SDL_FRect& body);
  void drawPreviewMode(gfx::Gfx& g, const SDL_FRect& body);
  const Wrapped& wrapped(gfx::Gfx& g, std::size_t i, float width);

  NotesStore& store_;
  std::string path_;
  std::vector<std::string> lines_;
  std::vector<Wrapped> wraps_;
  float wrapWidth_ = 0.f;
  bool dirty_ = false;

  Mode mode_ = Mode::Edit;
  std::size_t line_ = 0;
  std::size_t col_ = 0;
  float scroll_ = 0.f;
  float blink_ = 0.f;

  PreviewLayout preview_;
  bool previewStale_ = true;
  float previewScroll_ = 0.f;
  SDL_FRect previewOrigin_{};
  float previewViewH_ = 0.f;

  Menu lineMenu_;
  TextPrompt prompt_;
};

}  // namespace sienna::notes
