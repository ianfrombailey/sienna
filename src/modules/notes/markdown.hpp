#pragma once

#include <SDL.h>

#include <cstddef>
#include <string>
#include <vector>

#include "gfx/gfx.hpp"

namespace sienna::notes {

// one positioned piece of the rendered preview, relative to the preview's top-left.
struct PreviewItem {
  enum class Kind { Text, Rect, Checkbox };
  Kind kind = Kind::Text;
  SDL_FRect rect{};
  std::string text;
  int size = 0;
  gfx::Text::Font font = gfx::Text::Font::Regular;
  gfx::Color color{0, 0, 0};
  bool checked = false;
  // for checkboxes: the area that toggles it and the source line it came from.
  SDL_FRect hit{};
  std::size_t line = 0;
};

struct PreviewLayout {
  std::vector<PreviewItem> items;
  float height = 0.f;
  float width = 0.f;
};

// lays out the markdown subset SIENNA supports: headings, emphasis, lists, checkboxes,
// quotes, inline code, code blocks, links and horizontal rules.
PreviewLayout layoutMarkdown(gfx::Text& text, const std::vector<std::string>& lines, float width);

// draws the items that fall between top and bottom (in screen space).
void drawPreview(gfx::Gfx& g, const PreviewLayout& layout, float x, float y, float top,
                 float bottom);

// flips "[ ]" and "[x]" on a checkbox line. returns false if the line isn't one.
bool toggleCheckbox(std::string& line);

}  // namespace sienna::notes
