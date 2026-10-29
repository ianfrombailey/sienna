#include "core/fs.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cctype>
#include <cstdio>

namespace sienna::fs {

std::string join(const std::string& dir, const std::string& name) {
  if (dir.empty() || dir.back() == '/') {
    return dir + name;
  }
  return dir + "/" + name;
}

std::string fileName(const std::string& path) {
  const std::size_t slash = path.find_last_of('/');
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string parent(const std::string& path) {
  const std::size_t slash = path.find_last_of('/');
  return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

std::string extension(const std::string& name) {
  const std::size_t dot = name.find_last_of('.');
  if (dot == std::string::npos || dot == 0) {
    return {};
  }
  std::string ext = name.substr(dot + 1);
  for (char& c : ext) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return ext;
}

bool exists(const std::string& path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0;
}

bool isDir(const std::string& path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool stat(const std::string& path, long long& mtime, long long& size) {
  struct stat st;
  if (::stat(path.c_str(), &st) != 0) {
    return false;
  }
  mtime = static_cast<long long>(st.st_mtime);
  size = static_cast<long long>(st.st_size);
  return true;
}

std::vector<Entry> list(const std::string& dir) {
  std::vector<Entry> entries;
  DIR* d = opendir(dir.c_str());
  if (!d) {
    return entries;
  }
  while (dirent* e = readdir(d)) {
    const std::string name = e->d_name;
    if (name == "." || name == "..") {
      continue;
    }
    entries.push_back({name, isDir(join(dir, name))});
  }
  closedir(d);
  return entries;
}

bool makeDir(const std::string& path) {
  return mkdir(path.c_str(), 0777) == 0 || isDir(path);
}

bool removeAll(const std::string& path) {
  if (!isDir(path)) {
    return std::remove(path.c_str()) == 0;
  }
  bool ok = true;
  for (const Entry& e : list(path)) {
    ok = removeAll(join(path, e.name)) && ok;
  }
  return rmdir(path.c_str()) == 0 && ok;
}

bool move(const std::string& from, const std::string& to) {
  return std::rename(from.c_str(), to.c_str()) == 0;
}

bool readFile(const std::string& path, std::string& out) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) {
    return false;
  }
  out.clear();
  char buf[4096];
  std::size_t n;
  while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) {
    out.append(buf, n);
  }
  std::fclose(f);
  return true;
}

bool writeFile(const std::string& path, const std::string& data) {
  const std::string tmp = path + ".tmp";
  FILE* f = std::fopen(tmp.c_str(), "wb");
  if (!f) {
    return false;
  }
  bool ok = std::fwrite(data.data(), 1, data.size(), f) == data.size();
  ok = std::fclose(f) == 0 && ok;
  if (!ok) {
    std::remove(tmp.c_str());
    return false;
  }
  // the Vita won't rename over an existing file.
  std::remove(path.c_str());
  return std::rename(tmp.c_str(), path.c_str()) == 0;
}

}  // namespace sienna::fs
