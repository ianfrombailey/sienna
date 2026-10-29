#include "gfx/wrap.hpp"

#include "core/utf8.hpp"

namespace sienna::gfx {

std::vector<Segment> wrap(Text& text, const std::string& s, int size, Text::Font font,
                          float maxWidth) {
  std::vector<Segment> out;
  std::size_t lineStart = 0;
  std::size_t pos = 0;

  while (pos < s.size()) {
    // next word plus the spaces after it.
    std::size_t wordEnd = pos;
    while (wordEnd < s.size() && s[wordEnd] != ' ') {
      ++wordEnd;
    }
    std::size_t tokenEnd = wordEnd;
    while (tokenEnd < s.size() && s[tokenEnd] == ' ') {
      ++tokenEnd;
    }

    if (text.width(s.substr(lineStart, wordEnd - lineStart), size, font) <= maxWidth) {
      pos = tokenEnd;
      continue;
    }

    if (pos > lineStart) {
      out.push_back({lineStart, pos - lineStart});
      lineStart = pos;
      continue;
    }

    // a single word wider than the line gets broken between characters.
    std::size_t cut = utf8::next(s, lineStart);
    while (cut < wordEnd) {
      const std::size_t n = utf8::next(s, cut);
      if (text.width(s.substr(lineStart, n - lineStart), size, font) > maxWidth) {
        break;
      }
      cut = n;
    }
    out.push_back({lineStart, cut - lineStart});
    lineStart = cut;
    pos = cut;
  }

  if (lineStart < s.size() || out.empty()) {
    out.push_back({lineStart, s.size() - lineStart});
  }
  return out;
}

std::string ellipsize(Text& text, const std::string& s, int size, Text::Font font, float maxWidth) {
  if (text.width(s, size, font) <= maxWidth) {
    return s;
  }
  const std::string dots = "\xE2\x80\xA6";
  std::size_t end = s.size();
  while (end > 0) {
    end = utf8::prev(s, end);
    const std::string candidate = s.substr(0, end) + dots;
    if (text.width(candidate, size, font) <= maxWidth) {
      return candidate;
    }
  }
  return dots;
}

}  // namespace sienna::gfx
