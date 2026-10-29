#pragma once

#include <SDL.h>

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sienna::music {

// album covers, loaded and scaled on a worker thread. covers come from the file's
// embedded art or a cover/folder image next to it.
class ArtCache {
public:
  ArtCache();
  ~ArtCache();
  ArtCache(const ArtCache&) = delete;
  ArtCache& operator=(const ArtCache&) = delete;

  // the cover for a track, or nullptr while it loads or when there isn't one.
  SDL_Texture* get(SDL_Renderer* renderer, const std::string& trackPath);

private:
  struct Entry {
    SDL_Texture* texture = nullptr;
    bool done = false;
    std::uint64_t lastUsed = 0;
  };

  void run();
  void collect(SDL_Renderer* renderer);
  void evict();

  std::unordered_map<std::string, Entry> entries_;
  std::uint64_t clock_ = 0;

  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable wake_;
  bool stop_ = false;
  std::deque<std::string> requests_;
  std::vector<std::pair<std::string, SDL_Surface*>> results_;
};

}  // namespace sienna::music
