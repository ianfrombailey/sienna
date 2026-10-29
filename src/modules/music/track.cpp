#include "modules/music/track.hpp"

#include <cstdlib>
#include <vector>

#include "core/fs.hpp"

namespace sienna::music {

namespace {

std::string clean(const std::string& s) {
  std::string out = s;
  for (char& c : out) {
    if (c == '\t' || c == '\n' || c == '\r') {
      c = ' ';
    }
  }
  return out;
}

std::vector<std::string> split(const std::string& line) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (true) {
    const std::size_t tab = line.find('\t', start);
    parts.push_back(line.substr(start, tab == std::string::npos ? std::string::npos : tab - start));
    if (tab == std::string::npos) {
      return parts;
    }
    start = tab + 1;
  }
}

}  // namespace

std::string Track::displayTitle() const {
  if (!title.empty()) {
    return title;
  }
  std::string name = fs::fileName(path);
  const std::size_t dot = name.find_last_of('.');
  return dot == std::string::npos || dot == 0 ? name : name.substr(0, dot);
}

std::string Track::displayArtist() const {
  return artist.empty() ? std::string("Unknown artist") : artist;
}

std::string Track::displayAlbum() const {
  return album.empty() ? std::string("Unknown album") : album;
}

std::string Track::groupArtist() const {
  if (!albumArtist.empty()) {
    return albumArtist;
  }
  return displayArtist();
}

std::string serialize(const Track& t) {
  return clean(t.path) + "\t" + clean(t.title) + "\t" + clean(t.artist) + "\t" + clean(t.album) +
         "\t" + clean(t.albumArtist) + "\t" + std::to_string(t.track) + "\t" +
         std::to_string(t.disc) + "\t" + std::to_string(t.duration);
}

bool parse(const std::string& line, Track& t) {
  const std::vector<std::string> p = split(line);
  if (p.size() < 8 || p[0].empty()) {
    return false;
  }
  t.path = p[0];
  t.title = p[1];
  t.artist = p[2];
  t.album = p[3];
  t.albumArtist = p[4];
  t.track = std::atoi(p[5].c_str());
  t.disc = std::atoi(p[6].c_str());
  t.duration = static_cast<float>(std::atof(p[7].c_str()));
  return true;
}

std::string formatTime(double seconds) {
  const int total = seconds > 0 ? static_cast<int>(seconds) : 0;
  const int s = total % 60;
  return std::to_string(total / 60) + ":" + (s < 10 ? "0" : "") + std::to_string(s);
}

}  // namespace sienna::music
