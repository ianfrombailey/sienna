#include "modules/music/library.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>

#include "audio/decoder.hpp"
#include "core/fs.hpp"
#include "core/platform.hpp"

namespace sienna::music {

namespace {

std::string lower(std::string s) {
  for (char& c : s) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return s;
}

std::vector<std::string> readLines(const std::string& path) {
  std::vector<std::string> lines;
  std::string data;
  if (!fs::readFile(path, data)) {
    return lines;
  }
  std::size_t start = 0;
  while (start < data.size()) {
    std::size_t end = data.find('\n', start);
    if (end == std::string::npos) {
      end = data.size();
    }
    std::string line = data.substr(start, end - start);
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (!line.empty()) {
      lines.push_back(line);
    }
    start = end + 1;
  }
  return lines;
}

}  // namespace

Library::Library() {
  const std::string config = platform::dataDir() + "config/";
  fs::makeDir(config);
  cacheFile_ = config + "music_library.tsv";

  // a custom folder list overrides the defaults. one folder per line.
  folders_ = readLines(config + "music_folders.txt");
  if (folders_.empty()) {
    folders_ = platform::defaultMusicFolders();
  }

  for (auto& [path, entry] : loadCache()) {
    tracks_.push_back(entry.track);
  }
  index();
  rescan();
}

Library::~Library() {
  stop_ = true;
  if (thread_.joinable()) {
    thread_.join();
  }
}

bool Library::isAudio(const std::string& name) {
  static const char* kExts[] = {"mp3", "flac", "ogg", "oga", "opus", "wav", "aif", "aiff",
                                "ape", "wv",   "tta", "ac3", "aac", "m4a", "mka"};
  const std::string ext = fs::extension(name);
  for (const char* e : kExts) {
    if (ext == e) {
      return true;
    }
  }
  return false;
}

const Track* Library::find(const std::string& path) const {
  auto it = byPath_.find(path);
  return it == byPath_.end() ? nullptr : &tracks_[it->second];
}

Track Library::trackFor(const std::string& path) const {
  if (const Track* t = find(path)) {
    return *t;
  }
  Track t;
  t.path = path;
  return t;
}

void Library::rescan() {
  if (scanning_) {
    return;
  }
  if (thread_.joinable()) {
    thread_.join();
  }
  scanning_ = true;
  scanCount_ = 0;
  thread_ = std::thread(&Library::scan, this, folders_, loadCache());
}

bool Library::poll() {
  std::lock_guard<std::mutex> lock(resultMutex_);
  if (!resultReady_) {
    return false;
  }
  resultReady_ = false;
  tracks_ = std::move(result_);
  result_.clear();
  index();
  return true;
}

void Library::scan(std::vector<std::string> folders, std::unordered_map<std::string, Cached> cache) {
  std::vector<Cached> found;
  for (const std::string& folder : folders) {
    walk(folder, cache, found);
    if (stop_) {
      break;
    }
  }
  if (!stop_) {
    saveCache(found);
    std::lock_guard<std::mutex> lock(resultMutex_);
    result_.clear();
    for (Cached& c : found) {
      result_.push_back(std::move(c.track));
    }
    resultReady_ = true;
  }
  scanning_ = false;
}

void Library::walk(const std::string& dir, std::unordered_map<std::string, Cached>& cache,
                   std::vector<Cached>& out) {
  for (const fs::Entry& e : fs::list(dir)) {
    if (stop_) {
      return;
    }
    if (e.name.empty() || e.name[0] == '.') {
      continue;
    }
    const std::string path = fs::join(dir, e.name);
    if (e.isDir) {
      walk(path, cache, out);
      continue;
    }
    if (!isAudio(e.name)) {
      continue;
    }
    long long mtime = 0;
    long long size = 0;
    fs::stat(path, mtime, size);
    auto it = cache.find(path);
    if (it != cache.end() && it->second.mtime == mtime && it->second.size == size) {
      out.push_back(it->second);
    } else {
      audio::TrackInfo info;
      Cached c{mtime, size, Track{}};
      c.track.path = path;
      if (audio::readInfo(path, info)) {
        c.track.title = info.title;
        c.track.artist = info.artist;
        c.track.album = info.album;
        c.track.albumArtist = info.albumArtist;
        c.track.track = info.track;
        c.track.disc = info.disc;
        c.track.duration = info.duration;
      }
      out.push_back(std::move(c));
    }
    ++scanCount_;
  }
}

std::unordered_map<std::string, Library::Cached> Library::loadCache() const {
  std::unordered_map<std::string, Cached> cache;
  for (const std::string& line : readLines(cacheFile_)) {
    // mtime, size, then the serialized track.
    const std::size_t a = line.find('\t');
    const std::size_t b = a == std::string::npos ? a : line.find('\t', a + 1);
    if (b == std::string::npos) {
      continue;
    }
    Cached c{std::atoll(line.substr(0, a).c_str()), std::atoll(line.substr(a + 1, b - a - 1).c_str()),
             Track{}};
    if (parse(line.substr(b + 1), c.track)) {
      cache[c.track.path] = std::move(c);
    }
  }
  return cache;
}

void Library::saveCache(const std::vector<Cached>& entries) const {
  std::string data;
  for (const Cached& c : entries) {
    data += std::to_string(c.mtime) + "\t" + std::to_string(c.size) + "\t" + serialize(c.track) + "\n";
  }
  fs::writeFile(cacheFile_, data);
}

void Library::index() {
  byPath_.clear();
  albums_.clear();
  artists_.clear();
  for (std::size_t i = 0; i < tracks_.size(); ++i) {
    byPath_[tracks_[i].path] = i;
  }

  std::map<std::string, std::size_t> albumKeys;
  for (std::size_t i = 0; i < tracks_.size(); ++i) {
    const Track& t = tracks_[i];
    const std::string key = lower(t.groupArtist()) + "\x1f" + lower(t.displayAlbum());
    auto it = albumKeys.find(key);
    if (it == albumKeys.end()) {
      it = albumKeys.emplace(key, albums_.size()).first;
      albums_.push_back({t.displayAlbum(), t.groupArtist(), {}});
    }
    albums_[it->second].tracks.push_back(i);
  }

  for (Album& a : albums_) {
    std::sort(a.tracks.begin(), a.tracks.end(), [&](std::size_t x, std::size_t y) {
      const Track& p = tracks_[x];
      const Track& q = tracks_[y];
      if (p.disc != q.disc) {
        return p.disc < q.disc;
      }
      if (p.track != q.track) {
        return p.track < q.track;
      }
      return lower(p.displayTitle()) < lower(q.displayTitle());
    });
  }
  std::sort(albums_.begin(), albums_.end(), [](const Album& a, const Album& b) {
    return lower(a.title) < lower(b.title);
  });

  std::map<std::string, std::size_t> artistKeys;
  for (std::size_t i = 0; i < albums_.size(); ++i) {
    const std::string key = lower(albums_[i].artist);
    auto it = artistKeys.find(key);
    if (it == artistKeys.end()) {
      it = artistKeys.emplace(key, artists_.size()).first;
      artists_.push_back({albums_[i].artist, {}});
    }
    artists_[it->second].albums.push_back(i);
  }
  std::sort(artists_.begin(), artists_.end(), [](const Artist& a, const Artist& b) {
    return lower(a.name) < lower(b.name);
  });
}

}  // namespace sienna::music
