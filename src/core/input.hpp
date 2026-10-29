#pragma once

#include <SDL.h>

#include <array>
#include <string>
#include <vector>

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

struct KeyPress {
  SDL_Keycode key;
  Uint16 mod;
};

// Vita buttons come in through SDL's game controller API. on the Mac, a connected
// controller works the same way, and the keyboard is mapped as a stand-in.
class Input {
public:
  Input() = default;
  ~Input();
  Input(const Input&) = delete;
  Input& operator=(const Input&) = delete;

  // call once per frame before feeding events, then update() after them.
  void beginFrame();
  void handleEvent(const SDL_Event& event);
  void update(float dt);

  // true only on the frame the button went down.
  bool pressed(Button b) const { return pressed_[static_cast<int>(b)]; }
  bool held(Button b) const { return held_[static_cast<int>(b)]; }
  // any button, key or tap this frame.
  bool anyPressed() const;
  // like pressed, but keeps firing while held. for moving through lists.
  bool repeated(Button b) const { return repeated_[static_cast<int>(b)]; }

  // while on, the keyboard types text instead of acting as buttons.
  void setTextMode(bool on) { textMode_ = on; }
  bool textMode() const { return textMode_; }

  // physical keyboard keys and typed text from this frame.
  const std::vector<KeyPress>& keys() const { return keys_; }
  const std::string& text() const { return text_; }

  // front touch taps (mouse clicks on the Mac) from this frame, in logical pixels.
  const std::vector<SDL_FPoint>& taps() const { return taps_; }

private:
  void set(int button, bool down);

  static constexpr int kCount = static_cast<int>(Button::Count);
  std::array<bool, kCount> pressed_{};
  std::array<bool, kCount> held_{};
  std::array<bool, kCount> repeated_{};
  std::array<float, kCount> heldTime_{};
  std::array<float, kCount> nextRepeat_{};
  SDL_GameController* controller_ = nullptr;

  bool textMode_ = false;
  std::vector<KeyPress> keys_;
  std::string text_;
  std::vector<SDL_FPoint> taps_;
};

}  // namespace sienna
