#include "audio/output.hpp"

#include <psp2/audioout.h>

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
  while (gRunning) {
    gFill(buffers[which].data(), kGrain);
    // blocks until the hardware wants more, which paces this thread.
    sceAudioOutOutput(port, buffers[which].data());
    which ^= 1;
  }
  sceAudioOutOutput(port, nullptr);
  sceAudioOutReleasePort(port);
}

}  // namespace

bool openOutput(FillFn fill) {
  const int port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, kGrain, kSampleRate,
                                       SCE_AUDIO_OUT_MODE_STEREO);
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
