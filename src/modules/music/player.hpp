#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "audio/decoder.hpp"
#include "modules/music/track.hpp"

namespace sienna::music {

// plays a queue of tracks. decoding happens on the audio thread, which also moves
// to the next track, so playback carries on while the main loop is slowed down.
class Player {
public:
  enum class Repeat { Off, All, One };

  Player();
  ~Player();
  Player(const Player&) = delete;
  Player& operator=(const Player&) = delete;

  void play(std::vector<Track> queue, std::size_t index, bool shuffle = false);
  void togglePause();
  void next();
  void previous();
  void seekBy(double seconds);

  void setShuffle(bool on);
  bool shuffle() const;
  void setRepeat(Repeat r);
  Repeat repeat() const;

  bool hasTrack() const;
  bool playing() const;
  Track current() const;
  double position() const;
  double duration() const;

  // main thread, every frame: updates the dashboard's now-playing info, the
  // recently played list and the resume file.
  void update(float dt);
  const std::vector<std::string>& recent() const { return recent_; }

private:
  void fill(std::int16_t* out, int frames);
  // these expect mutex_ to be held.
  bool openAt(std::size_t pos);
  void buildOrder(std::size_t first);
  bool advance(int step);
  double positionLocked() const;

  void save();
  void restore();

  mutable std::mutex mutex_;
  audio::Decoder decoder_;
  std::vector<Track> queue_;
  std::vector<std::size_t> order_;  // play order as indices into queue_
  std::size_t pos_ = 0;             // index into order_
  bool hasTrack_ = false;
  bool playing_ = false;
  bool shuffle_ = false;
  Repeat repeat_ = Repeat::Off;
  std::int64_t frames_ = 0;
  double base_ = 0.0;
  double duration_ = 0.0;
  std::size_t serial_ = 0;

  std::size_t seenSerial_ = 0;
  float saveTimer_ = 0.f;
  std::atomic<bool> stateDirty_{false};
  std::string stateFile_;
  std::string recentFile_;
  std::vector<std::string> recent_;
  bool outputOpen_ = false;
};

}  // namespace sienna::music
