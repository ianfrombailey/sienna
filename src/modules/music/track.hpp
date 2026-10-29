#pragma once

#include <string>

namespace sienna::music {

struct Track {
  std::string path;
  std::string title;
  std::string artist;
  std::string album;
  std::string albumArtist;
  int track = 0;
  int disc = 0;
  float duration = 0.f;

  // falls back to the file name when the file has no title tag.
  std::string displayTitle() const;
  std::string displayArtist() const;
  std::string displayAlbum() const;
  // album artist when tagged, so compilations group under one name.
  std::string groupArtist() const;
};

// one tab-separated line, used by the library cache, resume file and playlists.
std::string serialize(const Track& t);
bool parse(const std::string& line, Track& t);

// "3:07" style.
std::string formatTime(double seconds);

}  // namespace sienna::music
