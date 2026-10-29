#pragma once

#include <string>
#include <utility>

#include "ui/section.hpp"

namespace sienna {

// stands in for a section that hasn't been built yet.
class PlaceholderSection : public Section {
public:
  explicit PlaceholderSection(std::string title) : title_(std::move(title)) {}

  const char* title() const override { return title_.c_str(); }
  void draw(gfx::Gfx& g, const SDL_FRect& area, bool fullscreen) override;

private:
  std::string title_;
};

}  // namespace sienna
