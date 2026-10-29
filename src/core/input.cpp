#include "core/input.hpp"

namespace sienna {

namespace {

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

}  // namespace

Input::~Input() {
  if (controller_) {
    SDL_GameControllerClose(controller_);
  }
}

void Input::beginFrame() {
  pressed_.fill(false);
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
      if (!event.key.repeat) {
        set(fromKey(event.key.keysym.sym), true);
      }
      break;
    case SDL_KEYUP:
      set(fromKey(event.key.keysym.sym), false);
      break;
    default:
      break;
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
