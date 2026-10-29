#include "modules/music/player.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <random>

#include "audio/output.hpp"
#include "core/fs.hpp"
#include "core/platform.hpp"
#include "ui/now_playing.hpp"

namespace sienna::music {

namespace {

constexpr std::size_t kMaxRecent = 5;
constexpr float kSaveInterval = 10.f;

std::vector<std::string> splitLines(const std::string& data) {
  std::vector<std::string> lines;
  std::size_t start = 0;
  while (start < data.size()) {
    std::size_t end = data.find('\n', start);
    if (end == std::string::npos) {
      end = data.size();
    }
    lines.push_back(data.substr(start, end - start));
    start = end + 1;
  }
  return lines;
}

}  // namespace

Player::Player() {
  const std::string config = platform::dataDir() + "config/";
  fs::makeDir(config);
  stateFile_ = config + "music_state.txt";
  recentFile_ = config + "music_recent.txt";

  std::string data;
  if (fs::readFile(recentFile_, data)) {
    for (const std::string& line : splitLines(data)) {
      if (!line.empty() && recent_.size() < kMaxRecent) {
        recent_.push_back(line);
      }
    }
  }
  restore();
  outputOpen_ = audio::openOutput([this](std::int16_t* out, int frames) { fill(out, frames); });
}

Player::~Player() {
  if (outputOpen_) {
    audio::closeOutput();
  }
  save();
}

void Player::fill(std::int16_t* out, int frames) {
  std::lock_guard<std::mutex> lock(mutex_);
  int written = 0;
  // a few attempts so one unreadable file doesn't stall the queue.
  int attempts = static_cast<int>(queue_.size()) + 1;
  while (playing_ && written < frames && attempts > 0) {
    const int n = decoder_.read(out + static_cast<std::size_t>(written) * audio::kChannels,
                                frames - written);
    written += n;
    frames_ += n;
    if (written < frames) {
      --attempts;
      if (repeat_ == Repeat::One) {
        openAt(pos_);
      } else if (!advance(1)) {
        // end of the queue: stop on the first track, ready to play again.
        playing_ = false;
        openAt(0);
      }
    }
  }
  std::memset(out + static_cast<std::size_t>(written) * audio::kChannels, 0,
              static_cast<std::size_t>(frames - written) * audio::kChannels * sizeof(std::int16_t));
}

bool Player::openAt(std::size_t pos) {
  if (pos >= order_.size()) {
    return false;
  }
  pos_ = pos;
  frames_ = 0;
  base_ = 0.0;
  const Track& t = queue_[order_[pos_]];
  const bool ok = decoder_.open(t.path);
  duration_ = ok && decoder_.duration() > 0.0 ? decoder_.duration() : t.duration;
  ++serial_;
  stateDirty_ = true;
  return ok;
}

bool Player::advance(int step) {
  if (order_.empty()) {
    return false;
  }
  const long long next = static_cast<long long>(pos_) + step;
  if (next < 0 || next >= static_cast<long long>(order_.size())) {
    if (repeat_ != Repeat::All) {
      return false;
    }
    return openAt(next < 0 ? order_.size() - 1 : 0);
  }
  return openAt(static_cast<std::size_t>(next));
}

void Player::buildOrder(std::size_t first) {
  order_.resize(queue_.size());
  for (std::size_t i = 0; i < order_.size(); ++i) {
    order_[i] = i;
  }
  if (shuffle_ && !order_.empty()) {
    static std::mt19937 rng{std::random_device{}()};
    std::shuffle(order_.begin(), order_.end(), rng);
    // the chosen track plays first.
    auto it = std::find(order_.begin(), order_.end(), first);
    std::iter_swap(order_.begin(), it);
  }
}

void Player::play(std::vector<Track> queue, std::size_t index, bool shuffle) {
  std::lock_guard<std::mutex> lock(mutex_);
  queue_ = std::move(queue);
  if (queue_.empty()) {
    hasTrack_ = false;
    playing_ = false;
    decoder_.close();
    return;
  }
  shuffle_ = shuffle;
  index = std::min(index, queue_.size() - 1);
  buildOrder(index);
  const std::size_t start = static_cast<std::size_t>(
      std::find(order_.begin(), order_.end(), index) - order_.begin());
  hasTrack_ = openAt(start);
  playing_ = hasTrack_;
}

void Player::togglePause() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (hasTrack_) {
    playing_ = !playing_;
    stateDirty_ = true;
  }
}

void Player::next() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (hasTrack_ && !advance(1)) {
    openAt(0);
    playing_ = false;
  }
}

void Player::previous() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!hasTrack_) {
    return;
  }
  // like most players: restart the song unless it only just began.
  if (positionLocked() > 3.0 || !advance(-1)) {
    openAt(pos_);
  }
}

void Player::seekBy(double seconds) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!hasTrack_) {
    return;
  }
  double target = positionLocked() + seconds;
  target = std::clamp(target, 0.0, std::max(0.0, duration_ - 1.0));
  if (decoder_.seek(target)) {
    base_ = target;
    frames_ = 0;
    stateDirty_ = true;
  }
}

void Player::setShuffle(bool on) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (shuffle_ == on || order_.empty()) {
    shuffle_ = on;
    return;
  }
  shuffle_ = on;
  const std::size_t currentIndex = order_[pos_];
  buildOrder(currentIndex);
  pos_ = static_cast<std::size_t>(std::find(order_.begin(), order_.end(), currentIndex) -
                                  order_.begin());
  stateDirty_ = true;
}

bool Player::shuffle() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return shuffle_;
}

void Player::setRepeat(Repeat r) {
  std::lock_guard<std::mutex> lock(mutex_);
  repeat_ = r;
  stateDirty_ = true;
}

Player::Repeat Player::repeat() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return repeat_;
}

bool Player::hasTrack() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return hasTrack_;
}

bool Player::playing() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return playing_;
}

Track Player::current() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return hasTrack_ ? queue_[order_[pos_]] : Track{};
}

double Player::positionLocked() const {
  return base_ + static_cast<double>(frames_) / audio::kSampleRate;
}

double Player::position() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return positionLocked();
}

double Player::duration() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return duration_;
}

void Player::update(float dt) {
  std::size_t serial;
  Track track;
  bool has;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    serial = serial_;
    has = hasTrack_;
    if (has) {
      track = queue_[order_[pos_]];
    }
    NowPlaying& np = nowPlaying();
    np.active = has;
    np.playing = playing_;
    np.title = has ? track.displayTitle() : std::string();
    np.artist = has ? track.displayArtist() : std::string();
    np.progress = duration_ > 0.0 ? static_cast<float>(positionLocked() / duration_) : 0.f;
  }

  if (has && serial != seenSerial_) {
    seenSerial_ = serial;
    recent_.erase(std::remove(recent_.begin(), recent_.end(), track.path), recent_.end());
    recent_.insert(recent_.begin(), track.path);
    if (recent_.size() > kMaxRecent) {
      recent_.resize(kMaxRecent);
    }
    std::string data;
    for (const std::string& p : recent_) {
      data += p + "\n";
    }
    fs::writeFile(recentFile_, data);
  }

  saveTimer_ += dt;
  if (stateDirty_ || (playing() && saveTimer_ >= kSaveInterval)) {
    if (saveTimer_ >= 1.f) {
      save();
    }
  }
}

// resume file: index, position, shuffle, repeat, then one track per line.
void Player::save() {
  std::lock_guard<std::mutex> lock(mutex_);
  saveTimer_ = 0.f;
  stateDirty_ = false;
  if (!hasTrack_) {
    fs::removeAll(stateFile_);
    return;
  }
  std::string data = std::to_string(order_[pos_]) + "\n" + std::to_string(positionLocked()) + "\n" +
                     (shuffle_ ? "1" : "0") + "\n" + std::to_string(static_cast<int>(repeat_)) + "\n";
  for (const Track& t : queue_) {
    data += serialize(t) + "\n";
  }
  fs::writeFile(stateFile_, data);
}

void Player::restore() {
  std::string data;
  if (!fs::readFile(stateFile_, data)) {
    return;
  }
  const std::vector<std::string> lines = splitLines(data);
  if (lines.size() < 5) {
    return;
  }
  std::vector<Track> queue;
  for (std::size_t i = 4; i < lines.size(); ++i) {
    Track t;
    if (parse(lines[i], t)) {
      queue.push_back(std::move(t));
    }
  }
  const std::size_t index = static_cast<std::size_t>(std::atoll(lines[0].c_str()));
  if (queue.empty() || index >= queue.size()) {
    return;
  }
  const double position = std::atof(lines[1].c_str());

  std::lock_guard<std::mutex> lock(mutex_);
  queue_ = std::move(queue);
  shuffle_ = lines[2] == "1";
  repeat_ = static_cast<Repeat>(std::clamp(std::atoi(lines[3].c_str()), 0, 2));
  buildOrder(index);
  const std::size_t start = static_cast<std::size_t>(
      std::find(order_.begin(), order_.end(), index) - order_.begin());
  hasTrack_ = openAt(start);
  playing_ = false;
  if (hasTrack_ && position > 0.0 && decoder_.seek(position)) {
    base_ = position;
  }
  // restoring shouldn't count as playing it again.
  seenSerial_ = serial_;
  stateDirty_ = false;
}

}  // namespace sienna::music
