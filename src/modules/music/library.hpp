#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "modules/music/track.hpp"

namespace sienna::music {

// every song in the music folders. loads from a cache at startup, then rescans in
// the background so new and changed files show up.
class Library {
public:
  struct Album {
    std::string title;
    std::string artist;
    std::vector<std::size_t> tracks;  // indices into tracks(), in play order
  };
  struct Artist {
    std::string name;
    std::vector<std::size_t> albums;  // indices into albums()
  };

  Library();
  ~Library();
  Library(const Library&) = delete;
  Library& operator=(const Library&) = delete;

  const std::vector<std::string>& folders() const { return folders_; }
  void rescan();
  bool scanning() const { return scanning_; }
  int scanCount() const { return scanCount_; }
  // call every frame. returns true when fresh scan results were applied.
  bool poll();

  const std::vector<Track>& tracks() const { return tracks_; }
  const std::vector<Album>& albums() const { return albums_; }
  const std::vector<Artist>& artists() const { return artists_; }
  const Track* find(const std::string& path) const;
  // a track from the library, or a bare one built from the file name.
  Track trackFor(const std::string& path) const;

  static bool isAudio(const std::string& name);

private:
  struct Cached {
    long long mtime;
    long long size;
    Track track;
  };

  void index();
  void scan(std::vector<std::string> folders, std::unordered_map<std::string, Cached> cache);
  void walk(const std::string& dir, std::unordered_map<std::string, Cached>& cache,
            std::vector<Cached>& out);
  std::unordered_map<std::string, Cached> loadCache() const;
  void saveCache(const std::vector<Cached>& entries) const;

  std::string cacheFile_;
  std::vector<std::string> folders_;
  std::vector<Track> tracks_;
  std::vector<Album> albums_;
  std::vector<Artist> artists_;
  std::unordered_map<std::string, std::size_t> byPath_;

  std::thread thread_;
  std::atomic<bool> scanning_{false};
  std::atomic<bool> stop_{false};
  std::atomic<int> scanCount_{0};
  std::mutex resultMutex_;
  bool resultReady_ = false;
  std::vector<Track> result_;
};

}  // namespace sienna::music
