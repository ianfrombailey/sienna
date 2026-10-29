#include "ui/now_playing.hpp"

#include "ui/theme.hpp"

namespace sienna {

NowPlaying& nowPlaying() {
  static NowPlaying state;
  return state;
}

void drawNowPlayingBar(gfx::Gfx& g, const SDL_FRect& area, const NowPlaying& state, float alpha) {
  using gfx::fade;
  using Weight = gfx::Text::Weight;

  gfx::vGradient(g.renderer, area, fade({0, 0, 0, 120}, alpha), fade({0, 0, 0, 170}, alpha));
  gfx::fillRect(g.renderer, {area.x, area.y, area.w, 1.f}, fade({255, 255, 255, 34}, alpha));

  const float textX = area.x + 16.f;
  g.text.draw(state.title, 16, Weight::SemiBold, textX, area.y + 6.f,
              fade(theme::kTextPrimary, alpha));
  g.text.draw(state.artist, 13, Weight::Regular, textX, area.y + 26.f,
              fade(theme::kTextSecondary, alpha));

  const SDL_FRect track{area.x, area.y + area.h - 3.f, area.w, 3.f};
  gfx::fillRect(g.renderer, track, fade({255, 255, 255, 30}, alpha));
  gfx::fillRect(g.renderer, {track.x, track.y, track.w * state.progress, track.h},
                fade(theme::kAccent, alpha));
}

}  // namespace sienna
