#include "core/input.hpp"

#include "core/platform.hpp"

namespace sienna {

namespace {

constexpr float kRepeatDelay = 0.35f;
constexpr float kRepeatRate = 0.07f;

int fromController(Uint8 button) {
  switch (button) {
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return static_cast<int>(Button::Up);
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return static_cast<int>(Button::Down);
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return static_cast<int>(Button::Left);
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return static_cast<int>(Button::Right);
    case SDL_CONTROLLER_BUTTON_A: return static_cast<int>(Button::Cross);
    case SDL_CONTROLLER_BUTTON_B: return static_cast<int>(Button::Circle);
    case SDL_CONTROLLER_BUTTON_X: return static_cast<int>(Button::Square);
    case SDL_CONTROLLER_BUTTON_Y: return static_cast<int>(Button::Triangle);
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return static_cast<int>(Button::L);
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return static_cast<int>(Button::R);
    case SDL_CONTROLLER_BUTTON_START: return static_cast<int>(Button::Start);
    case SDL_CONTROLLER_BUTTON_BACK: return static_cast<int>(Button::Select);
    default: return -1;
  }
}

// Mac keyboard layout: arrows = d-pad, z/x/a/s = cross/circle/square/triangle,
// q/e = L/R, return = START, backspace = SELECT.
int fromKey(SDL_Keycode key) {
  switch (key) {
    case SDLK_UP: return static_cast<int>(Button::Up);
    case SDLK_DOWN: return static_cast<int>(Button::Down);
    case SDLK_LEFT: return static_cast<int>(Button::Left);
    case SDLK_RIGHT: return static_cast<int>(Button::Right);
    case SDLK_z: return static_cast<int>(Button::Cross);
    case SDLK_x: return static_cast<int>(Button::Circle);
    case SDLK_a: return static_cast<int>(Button::Square);
    case SDLK_s: return static_cast<int>(Button::Triangle);
    case SDLK_q: return static_cast<int>(Button::L);
    case SDLK_e: return static_cast<int>(Button::R);
    case SDLK_RETURN: return static_cast<int>(Button::Start);
    case SDLK_BACKSPACE: return static_cast<int>(Button::Select);
    default: return -1;
  }
}

#ifdef __vita__
// the Vita's SDL only reports raw keys from a Bluetooth keyboard, so text is built
// here assuming a US layout.
char shifted(char c) {
  switch (c) {
    case '1': return '!';
    case '2': return '@';
    case '3': return '#';
    case '4': return '$';
    case '5': return '%';
    case '6': return '^';
    case '7': return '&';
    case '8': return '*';
    case '9': return '(';
    case '0': return ')';
    case '-': return '_';
    case '=': return '+';
    case '[': return '{';
    case ']': return '}';
    case '\\': return '|';
    case ';': return ':';
    case '\'': return '"';
    case ',': return '<';
    case '.': return '>';
    case '/': return '?';
    case '`': return '~';
    default: return c;
  }
}

std::string typedText(const SDL_Keysym& k) {
  if (k.mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI)) {
    return {};
  }
  if (k.sym < 32 || k.sym > 126) {
    return {};
  }
  char c = static_cast<char>(k.sym);
  const bool shift = (k.mod & KMOD_SHIFT) != 0;
  if (c >= 'a' && c <= 'z') {
    if (shift != ((k.mod & KMOD_CAPS) != 0)) {
      c = static_cast<char>(c - 'a' + 'A');
    }
  } else if (shift) {
    c = shifted(c);
  }
  return std::string(1, c);
}
#endif

}  // namespace

Input::~Input() {
  if (controller_) {
    SDL_GameControllerClose(controller_);
  }
}

void Input::beginFrame() {
  pressed_.fill(false);
  keys_.clear();
  text_.clear();
  taps_.clear();
}

void Input::handleEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_CONTROLLERDEVICEADDED:
      if (!controller_) {
        controller_ = SDL_GameControllerOpen(event.cdevice.which);
      }
      break;
    case SDL_CONTROLLERDEVICEREMOVED:
      if (controller_ &&
          SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_)) == event.cdevice.which) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
      }
      break;
    case SDL_CONTROLLERBUTTONDOWN:
      set(fromController(event.cbutton.button), true);
      break;
    case SDL_CONTROLLERBUTTONUP:
      set(fromController(event.cbutton.button), false);
      break;
    case SDL_KEYDOWN:
      keys_.push_back({event.key.keysym.sym, event.key.keysym.mod});
      if (textMode_) {
#ifdef __vita__
        text_ += typedText(event.key.keysym);
#endif
      } else if (!event.key.repeat) {
        set(fromKey(event.key.keysym.sym), true);
      }
      break;
    case SDL_KEYUP:
      set(fromKey(event.key.keysym.sym), false);
      break;
#ifndef __vita__
    case SDL_TEXTINPUT:
      if (textMode_) {
        text_ += event.text.text;
      }
      break;
#endif
    case SDL_FINGERDOWN:
      // the Vita's rear touchpad is an indirect device, so only the screen counts.
      if (SDL_GetTouchDeviceType(event.tfinger.touchId) == SDL_TOUCH_DEVICE_DIRECT) {
        taps_.push_back({event.tfinger.x * platform::kScreenWidth,
                         event.tfinger.y * platform::kScreenHeight});
      }
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (event.button.which != SDL_TOUCH_MOUSEID && event.button.button == SDL_BUTTON_LEFT) {
        taps_.push_back({static_cast<float>(event.button.x), static_cast<float>(event.button.y)});
      }
      break;
    default:
      break;
  }
}

bool Input::anyPressed() const {
  for (bool p : pressed_) {
    if (p) {
      return true;
    }
  }
  return !keys_.empty() || !taps_.empty();
}

void Input::update(float dt) {
  for (int i = 0; i < kCount; ++i) {
    repeated_[i] = pressed_[i];
    if (pressed_[i]) {
      heldTime_[i] = 0.f;
      nextRepeat_[i] = kRepeatDelay;
    } else if (held_[i]) {
      heldTime_[i] += dt;
      if (heldTime_[i] >= nextRepeat_[i]) {
        repeated_[i] = true;
        nextRepeat_[i] += kRepeatRate;
      }
    }
  }
}

void Input::set(int button, bool down) {
  if (button < 0) {
    return;
  }
  if (down && !held_[button]) {
    pressed_[button] = true;
  }
  held_[button] = down;
}

}  // namespace sienna
