#pragma once

#include <cstddef>
#include <string>

namespace sienna::utf8 {

inline bool isContinuation(char c) {
  return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

// byte index of the character before pos.
inline std::size_t prev(const std::string& s, std::size_t pos) {
  if (pos == 0) {
    return 0;
  }
  --pos;
  while (pos > 0 && isContinuation(s[pos])) {
    --pos;
  }
  return pos;
}

// byte index of the character after pos.
inline std::size_t next(const std::string& s, std::size_t pos) {
  if (pos >= s.size()) {
    return s.size();
  }
  ++pos;
  while (pos < s.size() && isContinuation(s[pos])) {
    ++pos;
  }
  return pos;
}

// moves pos back onto the start of a character, clamped to the string.
inline std::size_t clamp(const std::string& s, std::size_t pos) {
  if (pos >= s.size()) {
    return s.size();
  }
  while (pos > 0 && isContinuation(s[pos])) {
    --pos;
  }
  return pos;
}

}  // namespace sienna::utf8
