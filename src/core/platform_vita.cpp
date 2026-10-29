#include "core/platform.hpp"

#include <psp2/io/stat.h>

namespace sienna::platform {

namespace {
const std::string kDataDir = "ux0:data/sienna/";
const std::string kResDir = "app0:res/";
}

void init() {
  sceIoMkdir("ux0:data/sienna", 0777);
}

const std::string& dataDir() {
  return kDataDir;
}

const std::string& resDir() {
  return kResDir;
}

}  // namespace sienna::platform
