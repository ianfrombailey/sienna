#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/fs.hpp"

namespace sienna::notes {

// the notes folder on disk plus the recently opened list.
class NotesStore {
public:
  static constexpr std::size_t kMaxRecent = 5;

  struct TreeRow {
    int depth;
    std::string name;
  };

  NotesStore();

  const std::string& root() const { return root_; }
  // folders first, then .md and .txt files, each sorted by name.
  std::vector<fs::Entry> list(const std::string& dir) const;
  // every folder under the root, depth first, up to maxRows.
  std::vector<TreeRow> folderTree(std::size_t maxRows) const;
  // path relative to the notes root, starting with a slash.
  std::string relative(const std::string& path) const;

  static bool isNote(const std::string& name);

  const std::vector<std::string>& recent() const { return recent_; }
  void touch(const std::string& path);
  void renamed(const std::string& from, const std::string& to);
  void removed(const std::string& path);

private:
  void saveRecent() const;

  std::string root_;
  std::string recentFile_;
  std::vector<std::string> recent_;
};

}  // namespace sienna::notes
