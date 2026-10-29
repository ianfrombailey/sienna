#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "modules/music/art_cache.hpp"
#include "modules/music/library.hpp"
#include "modules/music/player.hpp"
#include "modules/music/playlists.hpp"
#include "ui/section.hpp"
#include "ui/widgets/menu.hpp"
#include "ui/widgets/prompt.hpp"

namespace sienna::music {

class MusicSection : public Section {
public:
  MusicSection() = default;

  const char* title() const override { return "Music"; }
  bool showsNowPlayingBar() const override { return false; }
  void onOpen() override;
  void tick(float dt) override;
  void update(Input& input, float dt) override;
  void draw(gfx::Gfx& g, const SDL_FRect& area, bool fullscreen) override;

private:
  enum class Tab { Artists, Albums, Folders, Playlists };
  enum class RowKind { Artist, Album, Folder, Track, Playlist, NewPlaylist };
  enum class ViewKind { Root, Artist, Album, Folder, Playlist };
  enum class Option {
    Play,
    Shuffle,
    AddToPlaylist,
    RemoveFromPlaylist,
    MoveUp,
    MoveDown,
    Rename,
    Delete,
    Rescan
  };
  enum class Pending { None, NewPlaylist, NewPlaylistForAdd, RenamePlaylist };

  struct Row {
    RowKind kind;
    std::string label;
    std::string sub;
    std::string key;  // artist name, album key, folder path, playlist name or track path
  };
  struct View {
    ViewKind kind;
    std::string title;
    std::string key;
    std::vector<Row> rows{};
    std::size_t selected = 0;
    float scroll = 0.f;
  };

  void openTab(Tab tab);
  void buildRows(View& view);
  void refreshViews();
  View& view() { return stack_.back(); }
  const Row* selectedRow() const;

  std::vector<Track> tracksOf(const Row& row) const;
  void playRow(const Row& row, bool shuffle);
  void activate();
  void openOptions();
  void runOption(Option option);
  void addToPlaylist(const std::string& name, const std::vector<Track>& tracks);
  void editPlaylist(int moveBy, bool remove);
  void finishPrompt(const std::string& text);

  void updateLibrary(Input& input);
  void updateNowPlaying(Input& input);

  void drawLibrary(gfx::Gfx& g, const SDL_FRect& area);
  void drawNowPlaying(gfx::Gfx& g, const SDL_FRect& area);
  void drawOverview(gfx::Gfx& g, const SDL_FRect& area);
  void drawArt(gfx::Gfx& g, const std::string& trackPath, const SDL_FRect& rect);

  Library library_;
  Playlists playlists_;
  Player player_;
  ArtCache art_;

  Tab tab_ = Tab::Artists;
  std::vector<View> stack_;
  bool nowPlayingOpen_ = false;

  Menu options_;
  std::vector<Option> optionList_;
  Menu picker_;
  std::vector<std::string> pickerNames_;
  std::vector<Track> pendingTracks_;
  Menu confirm_;
  Menu playMenu_;
  TextPrompt prompt_;
  Pending pending_ = Pending::None;
  std::string pendingName_;
};

}  // namespace sienna::music
