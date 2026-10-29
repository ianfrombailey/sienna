#include "modules/music/music_section.hpp"

#include <algorithm>
#include <random>

#include "core/fs.hpp"
#include "core/platform.hpp"
#include "gfx/wrap.hpp"
#include "ui/theme.hpp"
#include "ui/widgets/hints.hpp"
#include "ui/widgets/panel.hpp"

namespace sienna::music {

namespace {

using Font = gfx::Text::Font;
using Align = gfx::Text::Align;

constexpr float kRow = 52.f;
constexpr float kListTop = 78.f;
constexpr float kListBottom = 48.f;
constexpr const char* kTabNames[] = {"Artists", "Albums", "Folders", "Playlists"};

std::string albumKey(const Library::Album& a) {
  return a.title + "\x1f" + a.artist;
}

std::string plural(std::size_t n, const char* one, const char* many) {
  return std::to_string(n) + " " + (n == 1 ? one : many);
}

std::string cleanName(const std::string& s) {
  std::string out;
  for (char c : s) {
    if (c != '/' && c != '\\' && c != ':' && c != '\n' && c != '\r') {
      out += c;
    }
  }
  const std::size_t a = out.find_first_not_of(' ');
  if (a == std::string::npos) {
    return {};
  }
  return out.substr(a, out.find_last_not_of(' ') - a + 1);
}

void drawFolderIcon(SDL_Renderer* r, float x, float cy, gfx::Color c) {
  gfx::fillRect(r, {x, cy - 7.f, 8.f, 3.f}, c);
  gfx::fillRect(r, {x, cy - 5.f, 22.f, 13.f}, c);
}

void drawListIcon(SDL_Renderer* r, float x, float cy, gfx::Color c) {
  for (int i = -1; i <= 1; ++i) {
    gfx::fillRect(r, {x, cy + i * 6.f - 1.f, 3.f, 2.f}, c);
    gfx::fillRect(r, {x + 6.f, cy + i * 6.f - 1.f, 16.f, 2.f}, c);
  }
}

void drawNote(SDL_Renderer* r, float cx, float cy, float s, gfx::Color c) {
  gfx::ring(r, cx - s * 0.25f, cy + s * 0.3f, s * 0.14f, s * 0.12f, c);
  gfx::line(r, cx - s * 0.12f, cy + s * 0.3f, cx - s * 0.12f, cy - s * 0.35f, s * 0.06f, c);
  gfx::line(r, cx - s * 0.12f, cy - s * 0.35f, cx + s * 0.22f, cy - s * 0.25f, s * 0.06f, c);
}

}  // namespace

void MusicSection::onOpen() {
  if (stack_.empty()) {
    openTab(tab_);
  } else {
    refreshViews();
  }
}

void MusicSection::tick(float dt) {
  if (library_.poll() && !stack_.empty()) {
    refreshViews();
  }
  player_.update(dt);
}

void MusicSection::openTab(Tab tab) {
  tab_ = tab;
  stack_.clear();
  stack_.push_back({ViewKind::Root, kTabNames[static_cast<int>(tab)], ""});
  buildRows(stack_.back());
}

void MusicSection::refreshViews() {
  for (View& v : stack_) {
    buildRows(v);
  }
}

const MusicSection::Row* MusicSection::selectedRow() const {
  if (stack_.empty() || stack_.back().rows.empty()) {
    return nullptr;
  }
  const View& v = stack_.back();
  return &v.rows[std::min(v.selected, v.rows.size() - 1)];
}

void MusicSection::buildRows(View& v) {
  v.rows.clear();
  const auto& albums = library_.albums();
  const auto& tracks = library_.tracks();

  auto trackRow = [&](const Track& t, bool showArtist) {
    return Row{RowKind::Track, t.displayTitle(),
               showArtist ? t.displayArtist() : formatTime(t.duration), t.path};
  };

  switch (v.kind) {
    case ViewKind::Root:
      switch (tab_) {
        case Tab::Artists:
          for (const Library::Artist& a : library_.artists()) {
            v.rows.push_back({RowKind::Artist, a.name, plural(a.albums.size(), "album", "albums"), a.name});
          }
          break;
        case Tab::Albums:
          for (const Library::Album& a : albums) {
            v.rows.push_back({RowKind::Album, a.title, a.artist, albumKey(a)});
          }
          break;
        case Tab::Folders:
          for (const std::string& f : library_.folders()) {
            if (fs::isDir(f)) {
              v.rows.push_back({RowKind::Folder, f, "", f});
            }
          }
          break;
        case Tab::Playlists:
          v.rows.push_back({RowKind::NewPlaylist, "New playlist", "", ""});
          for (const std::string& name : playlists_.names()) {
            v.rows.push_back({RowKind::Playlist, name,
                              plural(playlists_.load(name).size(), "song", "songs"), name});
          }
          break;
      }
      break;
    case ViewKind::Artist:
      for (const Library::Artist& a : library_.artists()) {
        if (a.name != v.key) {
          continue;
        }
        for (std::size_t i : a.albums) {
          v.rows.push_back({RowKind::Album, albums[i].title,
                            plural(albums[i].tracks.size(), "song", "songs"), albumKey(albums[i])});
        }
      }
      break;
    case ViewKind::Album:
      for (const Library::Album& a : albums) {
        if (albumKey(a) != v.key) {
          continue;
        }
        for (std::size_t i : a.tracks) {
          v.rows.push_back(trackRow(tracks[i], false));
        }
      }
      break;
    case ViewKind::Folder: {
      std::vector<fs::Entry> entries = fs::list(v.key);
      std::sort(entries.begin(), entries.end(), [](const fs::Entry& a, const fs::Entry& b) {
        if (a.isDir != b.isDir) {
          return a.isDir;
        }
        return a.name < b.name;
      });
      for (const fs::Entry& e : entries) {
        if (e.name.empty() || e.name[0] == '.') {
          continue;
        }
        const std::string path = fs::join(v.key, e.name);
        if (e.isDir) {
          v.rows.push_back({RowKind::Folder, e.name, "", path});
        } else if (Library::isAudio(e.name)) {
          v.rows.push_back(trackRow(library_.trackFor(path), true));
        }
      }
      break;
    }
    case ViewKind::Playlist:
      for (const std::string& path : playlists_.load(v.key)) {
        v.rows.push_back(trackRow(library_.trackFor(path), true));
      }
      break;
  }
  if (!v.rows.empty()) {
    v.selected = std::min(v.selected, v.rows.size() - 1);
  } else {
    v.selected = 0;
  }
}

std::vector<Track> MusicSection::tracksOf(const Row& row) const {
  std::vector<Track> out;
  const auto& albums = library_.albums();
  const auto& tracks = library_.tracks();
  switch (row.kind) {
    case RowKind::Artist:
      for (const Library::Artist& a : library_.artists()) {
        if (a.name == row.key) {
          for (std::size_t ai : a.albums) {
            for (std::size_t ti : albums[ai].tracks) {
              out.push_back(tracks[ti]);
            }
          }
        }
      }
      break;
    case RowKind::Album:
      for (const Library::Album& a : albums) {
        if (albumKey(a) == row.key) {
          for (std::size_t ti : a.tracks) {
            out.push_back(tracks[ti]);
          }
        }
      }
      break;
    case RowKind::Folder: {
      const std::string prefix = row.key + "/";
      for (const Track& t : tracks) {
        if (t.path.compare(0, prefix.size(), prefix) == 0) {
          out.push_back(t);
        }
      }
      std::sort(out.begin(), out.end(), [](const Track& a, const Track& b) { return a.path < b.path; });
      break;
    }
    case RowKind::Playlist:
      for (const std::string& path : playlists_.load(row.key)) {
        out.push_back(library_.trackFor(path));
      }
      break;
    case RowKind::Track:
      out.push_back(library_.trackFor(row.key));
      break;
    case RowKind::NewPlaylist:
      break;
  }
  return out;
}

void MusicSection::playRow(const Row& row, bool shuffle) {
  std::vector<Track> tracks = tracksOf(row);
  if (tracks.empty()) {
    return;
  }
  std::size_t start = 0;
  if (shuffle) {
    static std::mt19937 rng{std::random_device{}()};
    start = std::uniform_int_distribution<std::size_t>(0, tracks.size() - 1)(rng);
  }
  player_.play(std::move(tracks), start, shuffle);
  nowPlayingOpen_ = true;
}

void MusicSection::activate() {
  const Row* row = selectedRow();
  if (!row) {
    return;
  }
  const Row r = *row;
  switch (r.kind) {
    case RowKind::Artist:
      stack_.push_back({ViewKind::Artist, r.label, r.key});
      break;
    case RowKind::Album:
      stack_.push_back({ViewKind::Album, r.label, r.key});
      break;
    case RowKind::Folder:
      stack_.push_back({ViewKind::Folder, fs::fileName(r.key), r.key});
      break;
    case RowKind::Playlist:
      stack_.push_back({ViewKind::Playlist, r.label, r.key});
      break;
    case RowKind::NewPlaylist:
      pending_ = Pending::NewPlaylist;
      prompt_.open("New playlist", "", false);
      return;
    case RowKind::Track: {
      // the whole list plays, starting from the chosen song.
      std::vector<Track> tracks;
      std::size_t start = 0;
      const std::vector<Row>& rows = view().rows;
      for (std::size_t j = 0; j < rows.size(); ++j) {
        if (rows[j].kind != RowKind::Track) {
          continue;
        }
        if (j == view().selected) {
          start = tracks.size();
        }
        tracks.push_back(library_.trackFor(rows[j].key));
      }
      player_.play(std::move(tracks), start, player_.shuffle());
      nowPlayingOpen_ = true;
      return;
    }
  }
  buildRows(stack_.back());
}

void MusicSection::openOptions() {
  optionList_.clear();
  std::vector<std::string> labels;
  auto add = [&](Option o, const char* label) {
    optionList_.push_back(o);
    labels.push_back(label);
  };
  if (const Row* row = selectedRow()) {
    switch (row->kind) {
      case RowKind::Track:
        add(Option::AddToPlaylist, "Add to playlist");
        if (view().kind == ViewKind::Playlist) {
          add(Option::RemoveFromPlaylist, "Remove from playlist");
          add(Option::MoveUp, "Move up");
          add(Option::MoveDown, "Move down");
        }
        break;
      case RowKind::Artist:
      case RowKind::Album:
      case RowKind::Folder:
        add(Option::Play, "Play");
        add(Option::Shuffle, "Shuffle");
        add(Option::AddToPlaylist, "Add to playlist");
        break;
      case RowKind::Playlist:
        add(Option::Play, "Play");
        add(Option::Shuffle, "Shuffle");
        add(Option::Rename, "Rename");
        add(Option::Delete, "Delete");
        break;
      case RowKind::NewPlaylist:
        break;
    }
  }
  add(Option::Rescan, "Rescan library");
  options_.open("Options", labels);
}

void MusicSection::runOption(Option option) {
  const Row* row = selectedRow();
  switch (option) {
    case Option::Play:
    case Option::Shuffle:
      if (row) {
        playRow(*row, option == Option::Shuffle);
      }
      break;
    case Option::AddToPlaylist: {
      if (!row) {
        break;
      }
      pendingTracks_ = tracksOf(*row);
      pickerNames_ = playlists_.names();
      std::vector<std::string> labels = {"New playlist"};
      labels.insert(labels.end(), pickerNames_.begin(), pickerNames_.end());
      picker_.open("Add to playlist", labels);
      break;
    }
    case Option::RemoveFromPlaylist:
      editPlaylist(0, true);
      break;
    case Option::MoveUp:
      editPlaylist(-1, false);
      break;
    case Option::MoveDown:
      editPlaylist(1, false);
      break;
    case Option::Rename:
      if (row) {
        pending_ = Pending::RenamePlaylist;
        pendingName_ = row->key;
        prompt_.open("Rename playlist", row->key, false);
      }
      break;
    case Option::Delete:
      if (row) {
        pendingName_ = row->key;
        confirm_.open("Delete playlist \"" + row->key + "\"?", {"Delete", "Cancel"});
      }
      break;
    case Option::Rescan:
      library_.rescan();
      break;
  }
}

void MusicSection::addToPlaylist(const std::string& name, const std::vector<Track>& tracks) {
  std::vector<Track> all;
  for (const std::string& path : playlists_.load(name)) {
    all.push_back(library_.trackFor(path));
  }
  all.insert(all.end(), tracks.begin(), tracks.end());
  playlists_.save(name, all);
  refreshViews();
}

void MusicSection::editPlaylist(int moveBy, bool remove) {
  View& v = view();
  if (v.kind != ViewKind::Playlist || v.rows.empty()) {
    return;
  }
  std::vector<std::string> paths = playlists_.load(v.key);
  const std::size_t i = v.selected;
  if (i >= paths.size()) {
    return;
  }
  if (remove) {
    paths.erase(paths.begin() + static_cast<std::ptrdiff_t>(i));
  } else {
    const long long j = static_cast<long long>(i) + moveBy;
    if (j < 0 || j >= static_cast<long long>(paths.size())) {
      return;
    }
    std::swap(paths[i], paths[static_cast<std::size_t>(j)]);
    v.selected = static_cast<std::size_t>(j);
  }
  std::vector<Track> tracks;
  for (const std::string& p : paths) {
    tracks.push_back(library_.trackFor(p));
  }
  playlists_.save(v.key, tracks);
  refreshViews();
}

void MusicSection::finishPrompt(const std::string& text) {
  const Pending pending = pending_;
  pending_ = Pending::None;
  const std::string name = cleanName(text);
  if (name.empty()) {
    return;
  }
  switch (pending) {
    case Pending::NewPlaylist:
      playlists_.create(name);
      break;
    case Pending::NewPlaylistForAdd:
      playlists_.create(name);
      addToPlaylist(name, pendingTracks_);
      break;
    case Pending::RenamePlaylist:
      playlists_.rename(pendingName_, name);
      break;
    case Pending::None:
      break;
  }
  refreshViews();
}

void MusicSection::update(Input& input, float dt) {
  input.setTextMode(false);
  if (stack_.empty()) {
    openTab(tab_);
  }

  if (prompt_.isOpen()) {
    if (prompt_.update(input, dt)) {
      finishPrompt(prompt_.result());
    } else if (!prompt_.isOpen()) {
      pending_ = Pending::None;
    }
    return;
  }
  if (confirm_.isOpen()) {
    if (confirm_.update(input) == 0) {
      playlists_.remove(pendingName_);
      refreshViews();
    }
    return;
  }
  if (picker_.isOpen()) {
    const int choice = picker_.update(input);
    if (choice == 0) {
      pending_ = Pending::NewPlaylistForAdd;
      prompt_.open("New playlist", "", false);
    } else if (choice > 0) {
      addToPlaylist(pickerNames_[static_cast<std::size_t>(choice - 1)], pendingTracks_);
    }
    return;
  }
  if (options_.isOpen()) {
    const int choice = options_.update(input);
    if (choice >= 0) {
      runOption(optionList_[static_cast<std::size_t>(choice)]);
    }
    return;
  }
  if (playMenu_.isOpen()) {
    const int choice = playMenu_.update(input);
    if (choice == 0) {
      player_.setShuffle(!player_.shuffle());
    } else if (choice == 1) {
      const auto r = player_.repeat();
      player_.setRepeat(r == Player::Repeat::Off   ? Player::Repeat::All
                        : r == Player::Repeat::All ? Player::Repeat::One
                                                   : Player::Repeat::Off);
    }
    return;
  }

  if (nowPlayingOpen_) {
    updateNowPlaying(input);
  } else {
    updateLibrary(input);
  }
}

void MusicSection::updateLibrary(Input& input) {
  View& v = view();
  const std::size_t n = v.rows.size();
  if (n > 0) {
    if (input.repeated(Button::Up) && v.selected > 0) {
      --v.selected;
    }
    if (input.repeated(Button::Down) && v.selected + 1 < n) {
      ++v.selected;
    }
  }
  if (input.pressed(Button::L) && tab_ != Tab::Artists) {
    openTab(static_cast<Tab>(static_cast<int>(tab_) - 1));
    return;
  }
  if (input.pressed(Button::R) && tab_ != Tab::Playlists) {
    openTab(static_cast<Tab>(static_cast<int>(tab_) + 1));
    return;
  }
  if (input.pressed(Button::Cross)) {
    activate();
  } else if (input.pressed(Button::Circle) && stack_.size() > 1) {
    stack_.pop_back();
    buildRows(stack_.back());
  } else if (input.pressed(Button::Square)) {
    openOptions();
  } else if (input.pressed(Button::Triangle) && player_.hasTrack()) {
    nowPlayingOpen_ = true;
  }
}

void MusicSection::updateNowPlaying(Input& input) {
  if (input.pressed(Button::Cross)) {
    player_.togglePause();
  }
  if (input.pressed(Button::L)) {
    player_.previous();
  }
  if (input.pressed(Button::R)) {
    player_.next();
  }
  if (input.repeated(Button::Left)) {
    player_.seekBy(-5.0);
  }
  if (input.repeated(Button::Right)) {
    player_.seekBy(5.0);
  }
  if (input.pressed(Button::Triangle)) {
    const auto r = player_.repeat();
    const char* repeatLabel = r == Player::Repeat::Off   ? "Repeat: off"
                              : r == Player::Repeat::All ? "Repeat: all"
                                                         : "Repeat: one";
    playMenu_.open("Playback", {player_.shuffle() ? "Shuffle: on" : "Shuffle: off", repeatLabel});
  }
  if (input.pressed(Button::Select)) {
    platform::setScreenOn(false);
  }
  if (input.pressed(Button::Circle)) {
    nowPlayingOpen_ = false;
  }
}

void MusicSection::draw(gfx::Gfx& g, const SDL_FRect& area, bool fullscreen) {
  if (!fullscreen) {
    drawOverview(g, area);
    return;
  }
  if (stack_.empty()) {
    openTab(tab_);
  }
  if (nowPlayingOpen_) {
    drawNowPlaying(g, area);
  } else {
    drawLibrary(g, area);
  }
  options_.draw(g);
  picker_.draw(g);
  confirm_.draw(g);
  playMenu_.draw(g);
  prompt_.draw(g);
}

void MusicSection::drawArt(gfx::Gfx& g, const std::string& trackPath, const SDL_FRect& r) {
  if (SDL_Texture* tex = art_.get(g.renderer, trackPath)) {
    SDL_RenderCopyF(g.renderer, tex, nullptr, &r);
  } else {
    gfx::vGradient(g.renderer, r, {58, 62, 72}, {30, 32, 38});
    drawNote(g.renderer, r.x + r.w / 2.f, r.y + r.h / 2.f, r.w * 0.5f, theme::kTextSecondary);
  }
  gfx::outlineRect(g.renderer, r, 1.f, {255, 255, 255, 30});
}

void MusicSection::drawLibrary(gfx::Gfx& g, const SDL_FRect& area) {
  const float x = area.x + 36.f;
  g.text.draw("Music", 26, Font::SemiBold, x, area.y + 24.f, theme::kTextPrimary);
  if (stack_.size() > 1) {
    const float w = g.text.width("Music", 26, Font::SemiBold);
    const std::string crumb = gfx::ellipsize(g.text, "/  " + view().title, 18, Font::Regular, 300.f);
    g.text.draw(crumb, 18, Font::Regular, x + w + 14.f, area.y + 31.f, theme::kTextSecondary);
  }

  // tabs, right-aligned, with L and R either side.
  float tx = area.x + area.w - 36.f;
  drawButtonGlyph(g, Button::R, tx - 6.f, area.y + 42.f);
  tx -= 26.f;
  for (int i = 3; i >= 0; --i) {
    const bool active = static_cast<int>(tab_) == i;
    const float w = g.text.width(kTabNames[i], 15, Font::SemiBold);
    tx -= w;
    g.text.draw(kTabNames[i], 15, Font::SemiBold, tx, area.y + 32.f,
                active ? theme::kTextPrimary : theme::kTextSecondary);
    if (active) {
      gfx::fillRect(g.renderer, {tx, area.y + 54.f, w, 2.f}, theme::kAccent);
    }
    tx -= 20.f;
  }
  drawButtonGlyph(g, Button::L, tx + 4.f, area.y + 42.f);
  gfx::fillRect(g.renderer, {x, area.y + 66.f, area.w - 72.f, 1.f}, {255, 255, 255, 30});

  View& v = view();
  const float top = area.y + kListTop;
  const float bottom = area.y + area.h - kListBottom;
  const float viewH = bottom - top;

  if (v.rows.empty()) {
    const char* msg = "Nothing here yet.";
    if (v.kind == ViewKind::Root && tab_ != Tab::Playlists) {
      msg = library_.scanning() ? "Scanning your music..." : "No music found.";
    }
    g.text.draw(msg, 17, Font::Regular, x, top + 16.f, theme::kTextSecondary);
    if (v.kind == ViewKind::Root && tab_ != Tab::Playlists && !library_.scanning()) {
      float y = top + 46.f;
      g.text.draw("Put music in one of these folders:", 15, Font::Regular, x, y, theme::kTextSecondary);
      for (const std::string& f : library_.folders()) {
        y += 24.f;
        g.text.draw(f, 15, Font::Mono, x + 12.f, y, theme::kTextPrimary);
      }
    }
  } else {
    const float selTop = kRow * static_cast<float>(v.selected);
    if (selTop < v.scroll) {
      v.scroll = selTop;
    } else if (selTop + kRow > v.scroll + viewH) {
      v.scroll = selTop + kRow - viewH;
    }

    const SDL_Rect clip{static_cast<int>(area.x), static_cast<int>(top), static_cast<int>(area.w),
                        static_cast<int>(viewH)};
    SDL_RenderSetClipRect(g.renderer, &clip);
    for (std::size_t i = 0; i < v.rows.size(); ++i) {
      const float y = top + kRow * static_cast<float>(i) - v.scroll;
      if (y + kRow < top || y > bottom) {
        continue;
      }
      const Row& row = v.rows[i];
      const bool sel = i == v.selected;
      const SDL_FRect rect{area.x + 24.f, y, area.w - 48.f, kRow};
      if (sel) {
        drawSelection(g, rect);
      }
      const gfx::Color iconColor = sel ? theme::kAccent : theme::kTextSecondary;
      const SDL_FRect art{rect.x + 14.f, y + 6.f, 40.f, 40.f};
      const float cy = y + kRow / 2.f;
      switch (row.kind) {
        case RowKind::Album: {
          for (const Library::Album& a : library_.albums()) {
            if (albumKey(a) == row.key && !a.tracks.empty()) {
              drawArt(g, library_.tracks()[a.tracks.front()].path, art);
              break;
            }
          }
          break;
        }
        case RowKind::Track:
          if (v.kind == ViewKind::Album) {
            g.text.draw(std::to_string(i + 1), 15, Font::Mono, art.x + art.w / 2.f, cy - 9.f,
                        iconColor, Align::Center);
          } else {
            drawArt(g, row.key, art);
          }
          break;
        case RowKind::Artist: {
          gfx::ring(g.renderer, art.x + 20.f, cy, 17.f, 2.f, iconColor);
          const std::string initial = row.label.substr(0, 1);
          g.text.draw(initial, 16, Font::SemiBold, art.x + 20.f, cy - 10.f, iconColor, Align::Center);
          break;
        }
        case RowKind::Folder:
          drawFolderIcon(g.renderer, art.x + 9.f, cy, iconColor);
          break;
        case RowKind::Playlist:
          drawListIcon(g.renderer, art.x + 9.f, cy, iconColor);
          break;
        case RowKind::NewPlaylist:
          gfx::fillRect(g.renderer, {art.x + 11.f, cy - 1.f, 18.f, 2.f}, iconColor);
          gfx::fillRect(g.renderer, {art.x + 19.f, cy - 9.f, 2.f, 18.f}, iconColor);
          break;
      }

      const float textX = art.x + art.w + 16.f;
      const float textW = rect.x + rect.w - textX - 16.f;
      const gfx::Color main = sel ? theme::kTextPrimary : gfx::fade(theme::kTextPrimary, 0.8f);
      if (row.sub.empty()) {
        g.text.draw(gfx::ellipsize(g.text, row.label, 17, Font::Regular, textW), 17, Font::Regular,
                    textX, cy - 11.f, main);
      } else {
        g.text.draw(gfx::ellipsize(g.text, row.label, 17, Font::Regular, textW), 17, Font::Regular,
                    textX, y + 6.f, main);
        g.text.draw(gfx::ellipsize(g.text, row.sub, 13, Font::Regular, textW), 13, Font::Regular,
                    textX, y + 28.f, theme::kTextSecondary);
      }
    }
    SDL_RenderSetClipRect(g.renderer, nullptr);
  }

  if (library_.scanning()) {
    g.text.draw("Scanning... " + std::to_string(library_.scanCount()), 13, Font::Regular, x,
                area.y + area.h - 30.f, theme::kTextSecondary);
  }

  if (player_.hasTrack()) {
    drawHints(g, area,
              {{Button::Cross, "Open"},
               {Button::Circle, "Back"},
               {Button::Square, "Options"},
               {Button::Triangle, "Now playing"}});
  } else {
    drawHints(g, area, {{Button::Cross, "Open"}, {Button::Circle, "Back"}, {Button::Square, "Options"}});
  }
}

void MusicSection::drawNowPlaying(gfx::Gfx& g, const SDL_FRect& area) {
  const Track t = player_.current();
  const double pos = player_.position();
  const double dur = player_.duration();

  const float x = area.x + 48.f;
  const SDL_FRect art{x, area.y + 70.f, 260.f, 260.f};
  gfx::fillRect(g.renderer, {art.x + 6.f, art.y + 8.f, art.w, art.h}, {0, 0, 0, 90});
  drawArt(g, t.path, art);

  const float ix = art.x + art.w + 40.f;
  const float iw = area.x + area.w - 48.f - ix;
  float y = art.y + 6.f;
  g.text.draw(player_.playing() ? "NOW PLAYING" : "PAUSED", 12, Font::SemiBold, ix, y, theme::kAccent);
  y += 26.f;
  const std::string title = t.displayTitle();
  const auto segs = gfx::wrap(g.text, title, 28, Font::SemiBold, iw);
  for (std::size_t i = 0; i < segs.size() && i < 2; ++i) {
    std::string line = title.substr(segs[i].start, segs[i].length);
    if (i == 1 && segs.size() > 2) {
      line = gfx::ellipsize(g.text, title.substr(segs[i].start), 28, Font::SemiBold, iw);
    }
    g.text.draw(line, 28, Font::SemiBold, ix, y, theme::kTextPrimary);
    y += 36.f;
  }
  y += 4.f;
  g.text.draw(gfx::ellipsize(g.text, t.displayArtist(), 19, Font::Regular, iw), 19, Font::Regular, ix,
              y, theme::kTextPrimary);
  y += 28.f;
  g.text.draw(gfx::ellipsize(g.text, t.displayAlbum(), 15, Font::Regular, iw), 15, Font::Regular, ix,
              y, theme::kTextSecondary);

  // progress.
  const float barY = art.y + art.h - 52.f;
  const float progress = dur > 0.0 ? static_cast<float>(std::clamp(pos / dur, 0.0, 1.0)) : 0.f;
  gfx::fillRect(g.renderer, {ix, barY, iw, 4.f}, {255, 255, 255, 36});
  gfx::fillRect(g.renderer, {ix, barY, iw * progress, 4.f}, theme::kAccent);
  gfx::fillRect(g.renderer, {ix + iw * progress - 5.f, barY - 4.f, 10.f, 12.f}, theme::kTextPrimary);
  g.text.draw(formatTime(pos), 13, Font::Regular, ix, barY + 14.f, theme::kTextSecondary);
  g.text.draw(formatTime(dur), 13, Font::Regular, ix + iw, barY + 14.f, theme::kTextSecondary,
              Align::Right);

  // shuffle and repeat state.
  const auto repeat = player_.repeat();
  const bool shuffle = player_.shuffle();
  const char* repeatText = repeat == Player::Repeat::One ? "REPEAT ONE" : "REPEAT";
  g.text.draw("SHUFFLE", 12, Font::SemiBold, ix, art.y + art.h + 18.f,
              shuffle ? theme::kAccent : gfx::fade(theme::kTextSecondary, 0.5f));
  g.text.draw(repeatText, 12, Font::SemiBold, ix + 84.f, art.y + art.h + 18.f,
              repeat != Player::Repeat::Off ? theme::kAccent : gfx::fade(theme::kTextSecondary, 0.5f));

  drawHints(g, area,
            {{Button::Cross, player_.playing() ? "Pause" : "Play"},
             {Button::L, "Prev"},
             {Button::R, "Next"},
             {Button::Triangle, "Options"},
             {Button::Select, "Screen off"},
             {Button::Circle, "Library"}});
}

void MusicSection::drawOverview(gfx::Gfx& g, const SDL_FRect& area) {
  const float x = area.x + 36.f;
  const float w = area.w - 72.f;
  g.text.draw("Music", 44, Font::SemiBold, x, area.y + 34.f, theme::kTextPrimary);
  gfx::fillRect(g.renderer, {x + 2.f, area.y + 94.f, 56.f, 3.f}, theme::kAccent);

  float y = area.y + 118.f;
  if (player_.hasTrack()) {
    const Track t = player_.current();
    const SDL_FRect art{x, y, 120.f, 120.f};
    drawArt(g, t.path, art);
    const float ix = x + 140.f;
    const float iw = w - 140.f;
    g.text.draw(player_.playing() ? "NOW PLAYING" : "PAUSED", 12, Font::SemiBold, ix, y + 2.f,
                theme::kAccent);
    g.text.draw(gfx::ellipsize(g.text, t.displayTitle(), 21, Font::SemiBold, iw), 21, Font::SemiBold,
                ix, y + 20.f, theme::kTextPrimary);
    g.text.draw(gfx::ellipsize(g.text, t.displayArtist(), 16, Font::Regular, iw), 16, Font::Regular,
                ix, y + 50.f, theme::kTextPrimary);
    g.text.draw(gfx::ellipsize(g.text, t.displayAlbum(), 14, Font::Regular, iw), 14, Font::Regular, ix,
                y + 72.f, theme::kTextSecondary);
    const double dur = player_.duration();
    const float p = dur > 0.0 ? static_cast<float>(std::clamp(player_.position() / dur, 0.0, 1.0)) : 0.f;
    gfx::fillRect(g.renderer, {ix, y + 104.f, iw, 3.f}, {255, 255, 255, 36});
    gfx::fillRect(g.renderer, {ix, y + 104.f, iw * p, 3.f}, theme::kAccent);
    y += 146.f;
  } else {
    g.text.draw("Nothing playing", 17, Font::Regular, x, y, theme::kTextSecondary);
    g.text.draw(plural(library_.tracks().size(), "song", "songs") + " in your library", 14,
                Font::Regular, x, y + 26.f, gfx::fade(theme::kTextSecondary, 0.8f));
    y += 66.f;
  }

  g.text.draw("RECENTLY PLAYED", 12, Font::SemiBold, x, y, theme::kTextSecondary);
  y += 24.f;
  if (player_.recent().empty()) {
    g.text.draw("Nothing yet", 16, Font::Regular, x, y, gfx::fade(theme::kTextSecondary, 0.7f));
  }
  for (const std::string& path : player_.recent()) {
    if (y + 26.f > area.y + area.h - 16.f) {
      break;
    }
    const Track t = library_.trackFor(path);
    const float artistW = std::min(w * 0.4f, g.text.width(t.displayArtist(), 13, Font::Regular));
    g.text.draw(gfx::ellipsize(g.text, t.displayTitle(), 16, Font::Regular, w - artistW - 20.f), 16,
                Font::Regular, x, y, theme::kTextPrimary);
    g.text.draw(gfx::ellipsize(g.text, t.displayArtist(), 13, Font::Regular, artistW), 13,
                Font::Regular, x + w, y + 3.f, theme::kTextSecondary, Align::Right);
    y += 26.f;
  }
}

}  // namespace sienna::music
