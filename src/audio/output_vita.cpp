#include "audio/output.hpp"

#include <SDL.h>
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>

#include <atomic>
#include <thread>
#include <vector>

namespace sienna::audio {

namespace {

constexpr int kGrain = 1024;

std::thread gThread;
std::atomic<bool> gRunning{false};
FillFn gFill;

void run(int port) {
  std::vector<std::int16_t> buffers[2] = {std::vector<std::int16_t>(kGrain * kChannels),
                                          std::vector<std::int16_t>(kGrain * kChannels)};
  int which = 0;
  bool reported = false;
  while (gRunning) {
    gFill(buffers[which].data(), kGrain);
    // blocks until the hardware wants more, which paces this thread.
    const int r = sceAudioOutOutput(port, buffers[which].data());
    if (r < 0) {
      if (!reported) {
        SDL_Log("sceAudioOutOutput failed: 0x%08x", static_cast<unsigned>(r));
        reported = true;
      }
      // don't spin the CPU if the port stops accepting audio.
      sceKernelDelayThread(kGrain * 1000000 / kSampleRate);
    }
    which ^= 1;
  }
  sceAudioOutOutput(port, nullptr);
  sceAudioOutReleasePort(port);
}

int openPort(SceAudioOutPortType type, const char* name) {
  const int port = sceAudioOutOpenPort(type, kGrain, kSampleRate, SCE_AUDIO_OUT_MODE_STEREO);
  SDL_Log("audio: %s port open -> 0x%08x", name, static_cast<unsigned>(port));
  return port;
}

}  // namespace

bool openOutput(FillFn fill) {
#ifdef SIENNA_BACKGROUND
  // the BGM port keeps playing while SIENNA is in the background.
  int port = openPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, "bgm");
  if (port < 0) {
    port = openPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, "main");
  }
#else
  const int port = openPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, "main");
#endif
  if (port < 0) {
    return false;
  }
  int vol[2] = {SCE_AUDIO_OUT_MAX_VOL, SCE_AUDIO_OUT_MAX_VOL};
  sceAudioOutSetVolume(port,
                       static_cast<SceAudioOutChannelFlag>(SCE_AUDIO_VOLUME_FLAG_L_CH |
                                                           SCE_AUDIO_VOLUME_FLAG_R_CH),
                       vol);
  gFill = std::move(fill);
  gRunning = true;
  gThread = std::thread(run, port);
  return true;
}

void closeOutput() {
  if (gRunning) {
    gRunning = false;
    gThread.join();
  }
}

}  // namespace sienna::audio
