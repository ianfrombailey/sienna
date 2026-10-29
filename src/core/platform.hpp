#pragma once

#include <string>
#include <vector>

namespace sienna::platform {

// screen size of the Vita, used as the logical render size everywhere.
inline constexpr int kScreenWidth = 960;
inline constexpr int kScreenHeight = 544;

// one-time setup that must happen before SDL_Init.
void init();
void shutdown();

// root folder for user data (books, music, notes, config), with a trailing slash.
const std::string& dataDir();

// read-only folder holding bundled resources (fonts, images), with a trailing slash.
const std::string& resDir();

enum class KeyboardStatus { Running, Accepted, Cancelled };

// opens the system on-screen keyboard. returns false where there isn't one (the Mac).
bool openKeyboard(const std::string& title, const std::string& initial, bool multiline);
// call every frame while the keyboard is open. on Accepted, result holds the text.
KeyboardStatus pollKeyboard(std::string& result);

// folders scanned for music when the user hasn't picked their own.
std::vector<std::string> defaultMusicFolders();

// on the Vita SIENNA keeps running after the user leaves it, so music can play on.
// call once per frame: busy keeps the console awake, input resets the idle timer.
void tick(bool busy, bool input);
// the system asked SIENNA to close (from LiveArea).
bool quitRequested();
// SIENNA is in front with the screen on, so drawing is worth doing.
bool visible();
void setScreenOn(bool on);
bool screenOn();

}  // namespace sienna::platform
