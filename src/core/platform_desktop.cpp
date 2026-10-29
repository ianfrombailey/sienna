#include "core/platform.hpp"

#include <SDL.h>

#include <cstdlib>
#include <filesystem>

namespace sienna::platform {

namespace {
std::string gDataDir;
std::string gResDir;
}

void init() {
  // stand-in for ux0:data/sienna on the Mac.
  const char* home = std::getenv("HOME");
  std::filesystem::path dir = std::filesystem::path(home ? home : ".") / "SiennaData";
  std::filesystem::create_directories(dir);
  gDataDir = dir.string() + "/";

  // res/ is copied next to the executable at build time.
  char* base = SDL_GetBasePath();
  gResDir = std::string(base ? base : "./") + "res/";
  SDL_free(base);
}

const std::string& dataDir() {
  return gDataDir;
}

const std::string& resDir() {
  return gResDir;
}

bool openKeyboard(const std::string& title, const std::string& initial, bool multiline) {
  (void)title;
  (void)initial;
  (void)multiline;
  return false;
}

KeyboardStatus pollKeyboard(std::string& result) {
  (void)result;
  return KeyboardStatus::Cancelled;
}

}  // namespace sienna::platform
