#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/input.hpp"
#include "gfx/gfx.hpp"
#include "ui/section.hpp"

namespace sienna {

// 360-style blades. sections to the left of the selected one stack at the left edge,
// the rest stack at the right. L/R moves between them, X opens one fullscreen and
// START brings the blades back.
class Dashboard {
public:
  Dashboard(std::vector<std::unique_ptr<Section>> sections, std::size_t selected);

  void update(const Input& input, float dt);
  void draw(gfx::Gfx& g);

private:
  struct Blade {
    std::unique_ptr<Section> section;
    float x = 0.f;
  };

  float targetX(std::size_t i) const;
  void drawBlade(gfx::Gfx& g, std::size_t i);

  std::vector<Blade> blades_;
  std::size_t selected_;
  bool open_ = false;
  // 0 = dashboard, 1 = section fullscreen; eased between the two.
  float openAmount_ = 0.f;
};

}  // namespace sienna
