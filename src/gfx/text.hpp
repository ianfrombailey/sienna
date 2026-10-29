#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <map>
#include <string>
#include <unordered_map>
#include <utility>

#include "gfx/draw.hpp"

namespace sienna::gfx {

// renders and caches text textures. sizes and positions are in logical (960x544) pixels.
class Text {
public:
  enum class Font { Regular, SemiBold, Italic, SemiBoldItalic, Mono };
  enum class Align { Left, Center, Right };

  // scale is output pixels per logical pixel, so text stays sharp on HiDPI screens.
  bool init(SDL_Renderer* renderer, const std::string& fontDir, float scale);
  void shutdown();

  SDL_FPoint measure(const std::string& text, int size, Font font);
  // measures without creating a texture. for layout work.
  float width(const std::string& text, int size, Font font);
  float lineHeight(int size, Font font);

  void draw(const std::string& text, int size, Font font, float x, float y, Color color,
            Align align = Align::Left);
  // draws text rotated to read bottom-to-top, centered on (cx, cy).
  void drawVertical(const std::string& text, int size, Font font, float cx, float cy,
                    Color color);

private:
  struct Entry {
    SDL_Texture* texture;
    int w, h;
  };

  TTF_Font* font(int size, Font font);
  const Entry* entry(const std::string& text, int size, Font font);

  SDL_Renderer* renderer_ = nullptr;
  std::string fontDir_;
  float scale_ = 1.f;
  std::map<std::pair<int, Font>, TTF_Font*> fonts_;
  std::unordered_map<std::string, Entry> cache_;
};

}  // namespace sienna::gfx
