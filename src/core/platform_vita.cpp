#include "core/platform.hpp"

#include <psp2/io/stat.h>

namespace sienna::platform {

namespace {
const std::string kDataDir = "ux0:data/sienna/";
}

void init() {
  sceIoMkdir("ux0:data/sienna", 0777);
}

const std::string& dataDir() {
  return kDataDir;
}

}  // namespace sienna::platform
