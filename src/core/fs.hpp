#pragma once

#include <string>
#include <vector>

// small POSIX file helpers that work the same on the Vita and the Mac.
namespace sienna::fs {

struct Entry {
  std::string name;
  bool isDir;
};

std::string join(const std::string& dir, const std::string& name);
std::string fileName(const std::string& path);
std::string parent(const std::string& path);
std::string extension(const std::string& name);  // lowercase, without the dot

bool exists(const std::string& path);
bool isDir(const std::string& path);
// skips . and .. and returns entries in no particular order.
std::vector<Entry> list(const std::string& dir);

bool makeDir(const std::string& path);
bool removeAll(const std::string& path);
bool move(const std::string& from, const std::string& to);

bool readFile(const std::string& path, std::string& out);
// writes to a temp file first so a failed write doesn't wipe the original.
bool writeFile(const std::string& path, const std::string& data);

}  // namespace sienna::fs
