#include "modules/music/art_cache.hpp"

#include <SDL_image.h>

#include <algorithm>
#include <cctype>

#include "audio/decoder.hpp"
#include "core/fs.hpp"

namespace sienna::music {

namespace {

constexpr int kSize = 256;
constexpr std::size_t kMaxEntries = 48;

SDL_Surface* scaled(SDL_Surface* src) {
  if (!src) {
    return nullptr;
  }
  SDL_Surface* rgba = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(src);
  if (!rgba) {
    return nullptr;
  }
  // center-crop to a square, then scale down to keep memory small.
  const int side = std::min(rgba->w, rgba->h);
  SDL_Rect from{(rgba->w - side) / 2, (rgba->h - side) / 2, side, side};
  SDL_Surface* out = SDL_CreateRGBSurfaceWithFormat(0, kSize, kSize, 32, SDL_PIXELFORMAT_RGBA32);
  if (out) {
    SDL_BlitScaled(rgba, &from, out, nullptr);
  }
  SDL_FreeSurface(rgba);
  return out;
}

SDL_Surface* loadCover(const std::string& trackPath) {
  std::vector<std::uint8_t> bytes;
  if (audio::readCoverArt(trackPath, bytes)) {
    SDL_RWops* rw = SDL_RWFromConstMem(bytes.data(), static_cast<int>(bytes.size()));
    if (SDL_Surface* s = IMG_Load_RW(rw, 1)) {
      return scaled(s);
    }
  }
  static const char* kNames[] = {"cover", "folder", "front", "album"};
  const std::string dir = fs::parent(trackPath);
  for (const fs::Entry& e : fs::list(dir)) {
    std::string lowerName = e.name;
    for (char& c : lowerName) {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    const std::string ext = fs::extension(lowerName);
    if (ext != "jpg" && ext != "jpeg" && ext != "png") {
      continue;
    }
    const std::string stem = lowerName.substr(0, lowerName.find_last_of('.'));
    for (const char* name : kNames) {
      if (stem == name) {
        return scaled(IMG_Load(fs::join(dir, e.name).c_str()));
      }
    }
  }
  return nullptr;
}

}  // namespace

ArtCache::ArtCache() : thread_(&ArtCache::run, this) {}

ArtCache::~ArtCache() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stop_ = true;
  }
  wake_.notify_one();
  thread_.join();
  for (auto& [path, surface] : results_) {
    SDL_FreeSurface(surface);
  }
  for (auto& [path, e] : entries_) {
    if (e.texture) {
      SDL_DestroyTexture(e.texture);
    }
  }
}

void ArtCache::run() {
  while (true) {
    std::string path;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wake_.wait(lock, [this] { return stop_ || !requests_.empty(); });
      if (stop_) {
        return;
      }
      // newest first, so what's on screen now loads before what scrolled past.
      path = requests_.back();
      requests_.pop_back();
    }
    SDL_Surface* surface = loadCover(path);
    std::lock_guard<std::mutex> lock(mutex_);
    results_.emplace_back(path, surface);
  }
}

void ArtCache::collect(SDL_Renderer* renderer) {
  std::vector<std::pair<std::string, SDL_Surface*>> results;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    results.swap(results_);
  }
  for (auto& [path, surface] : results) {
    auto it = entries_.find(path);
    if (it == entries_.end()) {
      SDL_FreeSurface(surface);
      continue;
    }
    it->second.done = true;
    if (surface) {
      it->second.texture = SDL_CreateTextureFromSurface(renderer, surface);
      SDL_FreeSurface(surface);
    }
  }
}

void ArtCache::evict() {
  while (entries_.size() > kMaxEntries) {
    auto oldest = entries_.end();
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
      if (it->second.done && (oldest == entries_.end() || it->second.lastUsed < oldest->second.lastUsed)) {
        oldest = it;
      }
    }
    if (oldest == entries_.end()) {
      return;
    }
    if (oldest->second.texture) {
      SDL_DestroyTexture(oldest->second.texture);
    }
    entries_.erase(oldest);
  }
}

SDL_Texture* ArtCache::get(SDL_Renderer* renderer, const std::string& trackPath) {
  if (trackPath.empty()) {
    return nullptr;
  }
  collect(renderer);
  ++clock_;
  auto it = entries_.find(trackPath);
  if (it == entries_.end()) {
    entries_[trackPath].lastUsed = clock_;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      requests_.push_back(trackPath);
    }
    wake_.notify_one();
    evict();
    return nullptr;
  }
  it->second.lastUsed = clock_;
  return it->second.texture;
}

}  // namespace sienna::music
