#include "audio/output.hpp"

#include <SDL.h>

namespace sienna::audio {

namespace {

SDL_AudioDeviceID gDevice = 0;
FillFn gFill;

void callback(void*, Uint8* stream, int len) {
  gFill(reinterpret_cast<std::int16_t*>(stream), len / (kChannels * 2));
}

}  // namespace

bool openOutput(FillFn fill) {
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
    SDL_Log("SDL audio init failed: %s", SDL_GetError());
    return false;
  }
  gFill = std::move(fill);
  SDL_AudioSpec want{};
  want.freq = kSampleRate;
  want.format = AUDIO_S16SYS;
  want.channels = kChannels;
  want.samples = 1024;
  want.callback = callback;
  gDevice = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);
  if (gDevice == 0) {
    SDL_Log("SDL_OpenAudioDevice failed: %s", SDL_GetError());
    return false;
  }
  SDL_PauseAudioDevice(gDevice, 0);
  return true;
}

void closeOutput() {
  if (gDevice != 0) {
    SDL_CloseAudioDevice(gDevice);
    gDevice = 0;
  }
}

}  // namespace sienna::audio
