#pragma once

#include <cstdint>
#include <functional>

namespace sienna::audio {

inline constexpr int kSampleRate = 48000;
inline constexpr int kChannels = 2;

// fills interleaved stereo 16-bit frames. called from the audio thread.
using FillFn = std::function<void(std::int16_t* out, int frames)>;

// starts pulling audio from fill. on the Vita this uses the background music port.
bool openOutput(FillFn fill);
void closeOutput();

}  // namespace sienna::audio
