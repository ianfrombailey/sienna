#include "modules/notes/notes_store.hpp"

#include <algorithm>
#include <cctype>

#include "core/platform.hpp"

namespace sienna::notes {

namespace {

std::string lower(std::string s) {
  for (char& c : s) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return s;
}

bool startsWith(const std::string& s, const std::string& prefix) {
  return s.compare(0, prefix.size(), prefix) == 0;
}

void collectTree(const NotesStore& store, const std::string& dir, int depth, std::size_t maxRows,
                 std::vector<NotesStore::TreeRow>& out) {
  for (const fs::Entry& e : store.list(dir)) {
    if (out.size() >= maxRows) {
      return;
    }
    if (!e.isDir) {
      continue;
    }
    out.push_back({depth, e.name});
    collectTree(store, fs::join(dir, e.name), depth + 1, maxRows, out);
  }
}

}  // namespace

NotesStore::NotesStore() {
  root_ = platform::dataDir() + "notes";
  fs::makeDir(root_);
  fs::makeDir(platform::dataDir() + "config");
  recentFile_ = platform::dataDir() + "config/recent_notes.txt";

  std::string data;
  if (fs::readFile(recentFile_, data)) {
    std::size_t start = 0;
    while (start < data.size()) {
      std::size_t end = data.find('\n', start);
      if (end == std::string::npos) {
        end = data.size();
      }
      const std::string path = data.substr(start, end - start);
      if (!path.empty() && fs::exists(path) && recent_.size() < kMaxRecent) {
        recent_.push_back(path);
      }
      start = end + 1;
    }
  }
}

bool NotesStore::isNote(const std::string& name) {
  const std::string ext = fs::extension(name);
  return ext == "md" || ext == "txt";
}

std::vector<fs::Entry> NotesStore::list(const std::string& dir) const {
  std::vector<fs::Entry> entries;
  for (fs::Entry& e : fs::list(dir)) {
    if (e.name.empty() || e.name[0] == '.') {
      continue;
    }
    if (e.isDir || isNote(e.name)) {
      entries.push_back(std::move(e));
    }
  }
  std::sort(entries.begin(), entries.end(), [](const fs::Entry& a, const fs::Entry& b) {
    if (a.isDir != b.isDir) {
      return a.isDir;
    }
    return lower(a.name) < lower(b.name);
  });
  return entries;
}

std::vector<NotesStore::TreeRow> NotesStore::folderTree(std::size_t maxRows) const {
  std::vector<TreeRow> rows;
  collectTree(*this, root_, 0, maxRows, rows);
  return rows;
}

std::string NotesStore::relative(const std::string& path) const {
  if (startsWith(path, root_)) {
    const std::string rest = path.substr(root_.size());
    return rest.empty() ? "/" : rest;
  }
  return path;
}

void NotesStore::touch(const std::string& path) {
  recent_.erase(std::remove(recent_.begin(), recent_.end(), path), recent_.end());
  recent_.insert(recent_.begin(), path);
  if (recent_.size() > kMaxRecent) {
    recent_.resize(kMaxRecent);
  }
  saveRecent();
}

void NotesStore::renamed(const std::string& from, const std::string& to) {
  for (std::string& p : recent_) {
    if (p == from) {
      p = to;
    } else if (startsWith(p, from + "/")) {
      p = to + p.substr(from.size());
    }
  }
  saveRecent();
}

void NotesStore::removed(const std::string& path) {
  recent_.erase(std::remove_if(recent_.begin(), recent_.end(),
                               [&](const std::string& p) {
                                 return p == path || startsWith(p, path + "/");
                               }),
                recent_.end());
  saveRecent();
}

void NotesStore::saveRecent() const {
  std::string data;
  for (const std::string& p : recent_) {
    data += p + "\n";
  }
  fs::writeFile(recentFile_, data);
}

}  // namespace sienna::notes
