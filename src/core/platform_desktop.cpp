#include "core/platform.hpp"

#include <cstdlib>
#include <filesystem>

namespace sienna::platform {

namespace {
std::string gDataDir;
}

void init() {
  // stand-in for ux0:data/sienna on the mac.
  const char* home = std::getenv("HOME");
  std::filesystem::path dir = std::filesystem::path(home ? home : ".") / "SiennaData";
  std::filesystem::create_directories(dir);
  gDataDir = dir.string() + "/";
}

const std::string& dataDir() {
  return gDataDir;
}

}  // namespace sienna::platform
