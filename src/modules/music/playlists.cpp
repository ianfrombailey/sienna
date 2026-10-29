#include "modules/music/playlists.hpp"

#include <algorithm>
#include <cctype>

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

}  // namespace

Playlists::Playlists() : dir_(platform::dataDir() + "playlists") {
  fs::makeDir(dir_);
}

std::string Playlists::pathFor(const std::string& name) const {
  return fs::join(dir_, name + ".m3u");
}

std::vector<std::string> Playlists::names() const {
  std::vector<std::string> out;
  for (const fs::Entry& e : fs::list(dir_)) {
    if (!e.isDir && fs::extension(e.name) == "m3u") {
      out.push_back(e.name.substr(0, e.name.size() - 4));
    }
  }
  std::sort(out.begin(), out.end(),
            [](const std::string& a, const std::string& b) { return lower(a) < lower(b); });
  return out;
}

std::vector<std::string> Playlists::load(const std::string& name) const {
  std::vector<std::string> paths;
  std::string data;
  if (!fs::readFile(pathFor(name), data)) {
    return paths;
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
    if (!line.empty() && line[0] != '#') {
      paths.push_back(line);
    }
    start = end + 1;
  }
  return paths;
}

void Playlists::save(const std::string& name, const std::vector<Track>& tracks) const {
  std::string data = "#EXTM3U\n";
  for (const Track& t : tracks) {
    data += "#EXTINF:" + std::to_string(static_cast<int>(t.duration)) + "," + t.displayArtist() +
            " - " + t.displayTitle() + "\n" + t.path + "\n";
  }
  fs::writeFile(pathFor(name), data);
}

bool Playlists::create(const std::string& name) const {
  if (name.empty() || fs::exists(pathFor(name))) {
    return false;
  }
  return fs::writeFile(pathFor(name), "#EXTM3U\n");
}

bool Playlists::rename(const std::string& from, const std::string& to) const {
  if (to.empty() || fs::exists(pathFor(to))) {
    return false;
  }
  return fs::move(pathFor(from), pathFor(to));
}

void Playlists::remove(const std::string& name) const {
  fs::removeAll(pathFor(name));
}

}  // namespace sienna::music
