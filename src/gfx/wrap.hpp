#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "gfx/text.hpp"

namespace sienna::gfx {

// a byte range of the source string that fits on one line.
struct Segment {
  std::size_t start;
  std::size_t length;
};

// breaks text into lines no wider than maxWidth, at spaces where possible.
// always returns at least one segment, even for an empty string.
std::vector<Segment> wrap(Text& text, const std::string& s, int size, Text::Font font,
                          float maxWidth);

// shortens text with a trailing ellipsis so it fits in maxWidth.
std::string ellipsize(Text& text, const std::string& s, int size, Text::Font font, float maxWidth);

}  // namespace sienna::gfx
