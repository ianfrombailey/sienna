#pragma once

#include <string>

namespace sienna::platform {

// screen size of the Vita, used as the logical render size everywhere.
inline constexpr int kScreenWidth = 960;
inline constexpr int kScreenHeight = 544;

// one-time setup that must happen before SDL_Init.
void init();

// root folder for user data (books, music, notes, config), with a trailing slash.
const std::string& dataDir();

// read-only folder holding bundled resources (fonts, images), with a trailing slash.
const std::string& resDir();

}  // namespace sienna::platform
