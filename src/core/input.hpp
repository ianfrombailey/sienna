#pragma once

#include <SDL.h>

#include <array>

namespace sienna {

enum class Button {
  Up,
  Down,
  Left,
  Right,
  Cross,
  Circle,
  Square,
  Triangle,
  L,
  R,
  Start,
  Select,
  Count
};

// Vita buttons come in through SDL's game controller API. on the Mac, a connected
// controller works the same way, and the keyboard is mapped as a stand-in.
class Input {
public:
  Input() = default;
  ~Input();
  Input(const Input&) = delete;
  Input& operator=(const Input&) = delete;

  // call once per frame before feeding events.
  void beginFrame();
  void handleEvent(const SDL_Event& event);

  // true only on the frame the button went down.
  bool pressed(Button b) const { return pressed_[static_cast<int>(b)]; }
  bool held(Button b) const { return held_[static_cast<int>(b)]; }

private:
  void set(int button, bool down);

  static constexpr int kCount = static_cast<int>(Button::Count);
  std::array<bool, kCount> pressed_{};
  std::array<bool, kCount> held_{};
  SDL_GameController* controller_ = nullptr;
};

}  // namespace sienna
