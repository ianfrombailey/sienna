#pragma once

#include <string>
#include <vector>

#include "modules/music/track.hpp"

namespace sienna::music {

// .m3u playlists kept in the SIENNA data folder.
class Playlists {
public:
  Playlists();

  // sorted names, without the .m3u extension.
  std::vector<std::string> names() const;
  std::vector<std::string> load(const std::string& name) const;
  void save(const std::string& name, const std::vector<Track>& tracks) const;

  bool create(const std::string& name) const;
  bool rename(const std::string& from, const std::string& to) const;
  void remove(const std::string& name) const;

private:
  std::string pathFor(const std::string& name) const;

  std::string dir_;
};

}  // namespace sienna::music
