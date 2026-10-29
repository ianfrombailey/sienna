#include "modules/notes/markdown.hpp"

#include <algorithm>
#include <cctype>

#include "gfx/wrap.hpp"
#include "ui/theme.hpp"

namespace sienna::notes {

namespace {

using Font = gfx::Text::Font;

constexpr int kBody = 17;
constexpr float kBodyLine = 26.f;
constexpr int kCode = 14;
constexpr float kCodeLine = 20.f;
constexpr float kIndent = 22.f;
constexpr float kBox = 16.f;
constexpr int kHeadingSizes[6] = {28, 23, 20, 18, 17, 17};

struct Span {
  std::string text;
  Font font;
  gfx::Color color;
  bool code = false;
  bool link = false;
};

bool isAlnum(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) != 0;
}

bool startsWith(const std::string& s, const std::string& prefix) {
  return s.compare(0, prefix.size(), prefix) == 0;
}

Font styled(Font base, bool bold, bool italic) {
  const bool b = bold || base == Font::SemiBold || base == Font::SemiBoldItalic;
  const bool i = italic || base == Font::Italic || base == Font::SemiBoldItalic;
  if (b && i) {
    return Font::SemiBoldItalic;
  }
  if (b) {
    return Font::SemiBold;
  }
  return i ? Font::Italic : Font::Regular;
}

std::vector<Span> parseInline(const std::string& s, Font base, gfx::Color color) {
  std::vector<Span> spans;
  std::string buf;
  bool bold = false;
  bool italic = false;
  auto flush = [&] {
    if (!buf.empty()) {
      spans.push_back({buf, styled(base, bold, italic), color});
      buf.clear();
    }
  };

  for (std::size_t i = 0; i < s.size();) {
    const char c = s[i];
    if (c == '\\' && i + 1 < s.size() && std::ispunct(static_cast<unsigned char>(s[i + 1]))) {
      buf += s[i + 1];
      i += 2;
      continue;
    }
    if (c == '`') {
      const std::size_t end = s.find('`', i + 1);
      if (end != std::string::npos) {
        flush();
        spans.push_back({s.substr(i + 1, end - i - 1), Font::Mono, theme::kTextPrimary, true});
        i = end + 1;
        continue;
      }
    }
    if (c == '[') {
      const std::size_t mid = s.find("](", i + 1);
      const std::size_t end = mid == std::string::npos ? mid : s.find(')', mid + 2);
      if (end != std::string::npos) {
        flush();
        spans.push_back({s.substr(i + 1, mid - i - 1), styled(base, bold, italic), theme::kAccent,
                         false, true});
        i = end + 1;
        continue;
      }
    }
    if ((c == '*' || c == '_') && i + 1 < s.size() && s[i + 1] == c) {
      flush();
      bold = !bold;
      i += 2;
      continue;
    }
    // underscores inside words (snake_case) stay literal.
    const bool wordUnderscore = c == '_' && i > 0 && isAlnum(s[i - 1]) && i + 1 < s.size() &&
                                isAlnum(s[i + 1]);
    if (c == '*' || (c == '_' && !wordUnderscore)) {
      flush();
      italic = !italic;
      ++i;
      continue;
    }
    buf += c;
    ++i;
  }
  flush();
  return spans;
}

class Builder {
public:
  Builder(gfx::Text& text, float width) : text_(text), width_(width) {}

  float y = 0.f;
  PreviewLayout out;

  void rect(SDL_FRect r, gfx::Color c) {
    PreviewItem item{PreviewItem::Kind::Rect, r};
    item.color = c;
    out.items.push_back(item);
  }

  void label(const std::string& s, float x, float top, float lineH, int size, Font font,
             gfx::Color c) {
    const float th = text_.lineHeight(size, font);
    PreviewItem item{PreviewItem::Kind::Text, {x, top + (lineH - th) / 2.f, 0.f, th}, s};
    item.size = size;
    item.font = font;
    item.color = c;
    out.items.push_back(item);
  }

  // word-wraps spans between x0 and the right edge, advancing y past them.
  void flow(const std::vector<Span>& spans, float x0, int size, float lineH) {
    float x = x0;
    float top = y;
    for (const Span& sp : spans) {
      const int sz = sp.code ? size - 2 : size;
      std::size_t pos = 0;
      while (pos < sp.text.size()) {
        std::size_t wordEnd = pos;
        while (wordEnd < sp.text.size() && sp.text[wordEnd] != ' ') {
          ++wordEnd;
        }
        std::size_t tokenEnd = wordEnd;
        while (tokenEnd < sp.text.size() && sp.text[tokenEnd] == ' ') {
          ++tokenEnd;
        }
        const std::string word = sp.text.substr(pos, wordEnd - pos);
        const float wordW = text_.width(word, sz, sp.font);
        if (x + wordW > width_ && x > x0) {
          top += lineH;
          x = x0;
        }
        if (x == x0 && word.empty()) {
          pos = tokenEnd;
          continue;
        }
        const std::string token = sp.text.substr(pos, tokenEnd - pos);
        if (sp.code) {
          rect({x - 3.f, top + 4.f, wordW + 6.f, lineH - 8.f}, {255, 255, 255, 22});
        }
        label(token, x, top, lineH, sz, sp.font, sp.color);
        if (sp.link) {
          const float th = text_.lineHeight(sz, sp.font);
          rect({x, top + (lineH + th) / 2.f - 3.f, wordW, 1.f}, gfx::fade(theme::kAccent, 0.7f));
        }
        x += text_.width(token, sz, sp.font);
        pos = tokenEnd;
      }
    }
    y = top + lineH;
  }

  void codeBlock(const std::vector<std::string>& lines) {
    std::vector<std::string> rows;
    for (std::string l : lines) {
      std::string expanded;
      for (char c : l) {
        expanded += c == '\t' ? std::string("    ") : std::string(1, c);
      }
      for (const gfx::Segment& s : gfx::wrap(text_, expanded, kCode, Font::Mono, width_ - 28.f)) {
        rows.push_back(expanded.substr(s.start, s.length));
      }
    }
    const float h = kCodeLine * static_cast<float>(rows.size()) + 20.f;
    y += 4.f;
    rect({0.f, y, width_, h}, {0, 0, 0, 80});
    rect({0.f, y, 2.f, h}, gfx::fade(theme::kAccent, 0.5f));
    for (std::size_t r = 0; r < rows.size(); ++r) {
      label(rows[r], 14.f, y + 10.f + kCodeLine * static_cast<float>(r), kCodeLine, kCode,
            Font::Mono, theme::kTextPrimary);
    }
    y += h + 8.f;
  }

private:
  gfx::Text& text_;
  float width_;
};

std::size_t leadingSpaces(const std::string& s) {
  std::size_t n = 0;
  for (char c : s) {
    if (c == ' ') {
      n += 1;
    } else if (c == '\t') {
      n += 4;
    } else {
      break;
    }
  }
  return n;
}

bool isRule(const std::string& t) {
  char mark = 0;
  int count = 0;
  for (char c : t) {
    if (c == ' ') {
      continue;
    }
    if (c != '-' && c != '*' && c != '_') {
      return false;
    }
    if (mark && c != mark) {
      return false;
    }
    mark = c;
    ++count;
  }
  return count >= 3;
}

// returns the position of the "[" if t is "- [ ] text" style, or npos.
std::size_t checkboxAt(const std::string& t) {
  if (t.size() < 5 || (t[0] != '-' && t[0] != '*' && t[0] != '+') || t[1] != ' ' || t[2] != '[' ||
      t[4] != ']') {
    return std::string::npos;
  }
  if (t[3] != ' ' && t[3] != 'x' && t[3] != 'X') {
    return std::string::npos;
  }
  if (t.size() > 5 && t[5] != ' ') {
    return std::string::npos;
  }
  return 2;
}

std::string afterMarker(const std::string& t, std::size_t from) {
  std::size_t p = from;
  while (p < t.size() && t[p] == ' ') {
    ++p;
  }
  return t.substr(p);
}

}  // namespace

PreviewLayout layoutMarkdown(gfx::Text& text, const std::vector<std::string>& lines, float width) {
  Builder b(text, width);
  bool inCode = false;
  bool lastBlank = true;
  std::vector<std::string> code;

  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string& raw = lines[i];
    const std::size_t spaces = leadingSpaces(raw);
    const std::string t = raw.substr(std::min(raw.find_first_not_of(" \t"), raw.size()));

    if (startsWith(t, "```")) {
      if (inCode) {
        b.codeBlock(code);
        code.clear();
      }
      inCode = !inCode;
      lastBlank = false;
      continue;
    }
    if (inCode) {
      code.push_back(raw);
      continue;
    }
    if (t.empty()) {
      if (!lastBlank) {
        b.y += 12.f;
      }
      lastBlank = true;
      continue;
    }
    lastBlank = false;

    const float x0 = static_cast<float>(std::min<std::size_t>(spaces / 2, 6)) * kIndent;

    std::size_t hashes = 0;
    while (hashes < t.size() && hashes < 7 && t[hashes] == '#') {
      ++hashes;
    }
    if (hashes >= 1 && hashes <= 6 && hashes < t.size() && t[hashes] == ' ') {
      const int size = kHeadingSizes[hashes - 1];
      if (b.y > 0.f) {
        b.y += 10.f;
      }
      b.flow(parseInline(afterMarker(t, hashes), Font::SemiBold, theme::kTextPrimary), 0.f, size,
             size * 1.4f);
      if (hashes <= 2) {
        b.rect({0.f, b.y + 2.f, width, 1.f}, {255, 255, 255, 34});
        b.y += 8.f;
      }
      b.y += 4.f;
      continue;
    }

    if (isRule(t)) {
      b.rect({0.f, b.y + 11.f, width, 1.f}, {255, 255, 255, 50});
      b.y += 24.f;
      continue;
    }

    if (t[0] == '>') {
      const float top = b.y;
      b.flow(parseInline(afterMarker(t, 1), Font::Italic, theme::kTextSecondary), 18.f, kBody,
             kBodyLine);
      b.rect({2.f, top + 3.f, 3.f, b.y - top - 6.f}, gfx::fade(theme::kAccent, 0.7f));
      continue;
    }

    if (checkboxAt(t) != std::string::npos) {
      const bool checked = t[3] != ' ';
      const float top = b.y;
      const std::size_t boxIndex = b.out.items.size();
      PreviewItem box{PreviewItem::Kind::Checkbox,
                      {x0 + 2.f, top + (kBodyLine - kBox) / 2.f, kBox, kBox}};
      box.checked = checked;
      box.line = i;
      b.out.items.push_back(box);
      b.flow(parseInline(afterMarker(t, 5), Font::Regular,
                         checked ? theme::kTextSecondary : theme::kTextPrimary),
             x0 + 28.f, kBody, kBodyLine);
      b.out.items[boxIndex].hit = {x0 - 10.f, top - 2.f, width - x0 + 10.f, b.y - top + 4.f};
      continue;
    }

    if ((t[0] == '-' || t[0] == '*' || t[0] == '+') && t.size() > 1 && t[1] == ' ') {
      b.label("\xE2\x80\xA2", x0 + 5.f, b.y, kBodyLine, kBody, Font::Regular, theme::kAccent);
      b.flow(parseInline(afterMarker(t, 2), Font::Regular, theme::kTextPrimary), x0 + 22.f, kBody,
             kBodyLine);
      continue;
    }

    std::size_t digits = 0;
    while (digits < t.size() && std::isdigit(static_cast<unsigned char>(t[digits]))) {
      ++digits;
    }
    if (digits > 0 && digits + 1 < t.size() && (t[digits] == '.' || t[digits] == ')') &&
        t[digits + 1] == ' ') {
      const std::string marker = t.substr(0, digits) + ".";
      const float mw = text.width(marker, kBody, Font::SemiBold);
      b.label(marker, x0, b.y, kBodyLine, kBody, Font::SemiBold, theme::kTextSecondary);
      b.flow(parseInline(afterMarker(t, digits + 1), Font::Regular, theme::kTextPrimary),
             x0 + std::max(26.f, mw + 8.f), kBody, kBodyLine);
      continue;
    }

    b.flow(parseInline(t, Font::Regular, theme::kTextPrimary), x0, kBody, kBodyLine);
  }

  if (inCode) {
    b.codeBlock(code);
  }

  b.out.height = b.y + 16.f;
  b.out.width = width;
  return std::move(b.out);
}

void drawPreview(gfx::Gfx& g, const PreviewLayout& layout, float x, float y, float top,
                 float bottom) {
  for (const PreviewItem& item : layout.items) {
    const SDL_FRect r{item.rect.x + x, item.rect.y + y, item.rect.w, item.rect.h};
    if (r.y + r.h < top || r.y > bottom) {
      continue;
    }
    switch (item.kind) {
      case PreviewItem::Kind::Text:
        g.text.draw(item.text, item.size, item.font, r.x, r.y, item.color);
        break;
      case PreviewItem::Kind::Rect:
        gfx::fillRect(g.renderer, r, item.color);
        break;
      case PreviewItem::Kind::Checkbox:
        if (item.checked) {
          gfx::fillRect(g.renderer, r, theme::kAccent);
          const gfx::Color mark{255, 255, 255};
          gfx::line(g.renderer, r.x + 3.5f, r.y + 8.5f, r.x + 6.5f, r.y + 11.5f, 2.f, mark);
          gfx::line(g.renderer, r.x + 6.5f, r.y + 11.5f, r.x + 12.5f, r.y + 4.5f, 2.f, mark);
        } else {
          gfx::fillRect(g.renderer, r, {0, 0, 0, 70});
          gfx::outlineRect(g.renderer, r, 1.5f, theme::kTextSecondary);
        }
        break;
    }
  }
}

bool toggleCheckbox(std::string& line) {
  const std::size_t indent = std::min(line.find_first_not_of(" \t"), line.size());
  if (checkboxAt(line.substr(indent)) == std::string::npos) {
    return false;
  }
  char& mark = line[indent + 3];
  mark = mark == ' ' ? 'x' : ' ';
  return true;
}

}  // namespace sienna::notes
