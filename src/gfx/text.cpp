#include "gfx/text.hpp"

#include <cmath>

namespace sienna::gfx {

namespace {

const char* fileFor(Text::Font font) {
  switch (font) {
    case Text::Font::SemiBold: return "Inter-SemiBold.ttf";
    case Text::Font::Italic: return "Inter-Italic.ttf";
    case Text::Font::SemiBoldItalic: return "Inter-SemiBoldItalic.ttf";
    case Text::Font::Mono: return "JetBrainsMono-Regular.ttf";
    default: return "Inter-Regular.ttf";
  }
}

}  // namespace

bool Text::init(SDL_Renderer* renderer, const std::string& fontDir, float scale) {
  if (TTF_Init() != 0) {
    SDL_Log("TTF_Init failed: %s", TTF_GetError());
    return false;
  }
  renderer_ = renderer;
  fontDir_ = fontDir;
  scale_ = scale;
  return true;
}

void Text::shutdown() {
  for (auto& [key, e] : cache_) {
    SDL_DestroyTexture(e.texture);
  }
  cache_.clear();
  for (auto& [key, f] : fonts_) {
    TTF_CloseFont(f);
  }
  fonts_.clear();
  TTF_Quit();
}

TTF_Font* Text::font(int size, Font font) {
  const auto key = std::make_pair(size, font);
  if (auto it = fonts_.find(key); it != fonts_.end()) {
    return it->second;
  }
  const std::string path = fontDir_ + fileFor(font);
  TTF_Font* f = TTF_OpenFont(path.c_str(), static_cast<int>(std::lround(size * scale_)));
  if (!f) {
    SDL_Log("TTF_OpenFont failed for %s: %s", path.c_str(), TTF_GetError());
  }
  fonts_[key] = f;
  return f;
}

const Text::Entry* Text::entry(const std::string& text, int size, Font font) {
  if (text.empty()) {
    return nullptr;
  }
  const std::string key =
      std::to_string(static_cast<int>(font)) + "|" + std::to_string(size) + "|" + text;
  if (auto it = cache_.find(key); it != cache_.end()) {
    return &it->second;
  }
  TTF_Font* f = this->font(size, font);
  if (!f) {
    return nullptr;
  }
  // rendered white so color and alpha can be applied per draw with texture mods.
  SDL_Surface* surface = TTF_RenderUTF8_Blended(f, text.c_str(), SDL_Color{255, 255, 255, 255});
  if (!surface) {
    return nullptr;
  }
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
  Entry e{texture, surface->w, surface->h};
  SDL_FreeSurface(surface);
  if (!texture) {
    return nullptr;
  }
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  return &cache_.emplace(key, e).first->second;
}

SDL_FPoint Text::measure(const std::string& text, int size, Font font) {
  const Entry* e = entry(text, size, font);
  if (!e) {
    return {0.f, 0.f};
  }
  return {e->w / scale_, e->h / scale_};
}

float Text::width(const std::string& text, int size, Font font) {
  TTF_Font* f = this->font(size, font);
  int w = 0;
  if (!f || text.empty() || TTF_SizeUTF8(f, text.c_str(), &w, nullptr) != 0) {
    return 0.f;
  }
  return w / scale_;
}

float Text::lineHeight(int size, Font font) {
  TTF_Font* f = this->font(size, font);
  return f ? TTF_FontHeight(f) / scale_ : static_cast<float>(size);
}

void Text::draw(const std::string& text, int size, Font font, float x, float y, Color color,
                Align align) {
  const Entry* e = entry(text, size, font);
  if (!e) {
    return;
  }
  const float w = e->w / scale_;
  const float h = e->h / scale_;
  if (align == Align::Center) {
    x -= w / 2.f;
  } else if (align == Align::Right) {
    x -= w;
  }
  SDL_SetTextureColorMod(e->texture, color.r, color.g, color.b);
  SDL_SetTextureAlphaMod(e->texture, color.a);
  const SDL_FRect dst{x, y, w, h};
  SDL_RenderCopyF(renderer_, e->texture, nullptr, &dst);
}

void Text::drawVertical(const std::string& text, int size, Font font, float cx, float cy,
                        Color color) {
  const Entry* e = entry(text, size, font);
  if (!e) {
    return;
  }
  const float w = e->w / scale_;
  const float h = e->h / scale_;
  SDL_SetTextureColorMod(e->texture, color.r, color.g, color.b);
  SDL_SetTextureAlphaMod(e->texture, color.a);
  const SDL_FRect dst{cx - w / 2.f, cy - h / 2.f, w, h};
  SDL_RenderCopyExF(renderer_, e->texture, nullptr, &dst, -90.0, nullptr, SDL_FLIP_NONE);
}

}  // namespace sienna::gfx
