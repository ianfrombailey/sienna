#pragma once

#include "gfx/draw.hpp"

// dark gunmetal blades with silver edges and a Vita blue accent.
namespace sienna::theme {

inline constexpr gfx::Color kBackground{10, 11, 14};

inline constexpr gfx::Color kBladeTop{70, 74, 84};
inline constexpr gfx::Color kBladeMid{34, 36, 43};
inline constexpr gfx::Color kBladeBottom{17, 18, 22};
inline constexpr float kBladeMidStop = 0.45f;

inline constexpr gfx::Color kEdgeHighlight{222, 228, 238, 180};
inline constexpr gfx::Color kGloss{255, 255, 255, 26};
inline constexpr gfx::Color kShadow{0, 0, 0, 150};

inline constexpr gfx::Color kAccent{64, 170, 255};
inline constexpr gfx::Color kTextPrimary{236, 239, 244};
inline constexpr gfx::Color kTextSecondary{148, 155, 167};

// width of the visible strip of each stacked blade.
inline constexpr float kSpineWidth = 44.f;
// higher is snappier; used as an exponential easing rate per second.
inline constexpr float kSlideSpeed = 14.f;

}  // namespace sienna::theme
