#pragma once

#include <cstddef>
#include <string>

#include "core/input.hpp"
#include "gfx/gfx.hpp"

namespace sienna {

// asks for a line of text. uses the system keyboard on the Vita and an inline
// field typed with the physical keyboard on the Mac.
class TextPrompt {
public:
  void open(std::string title, std::string initial, bool multiline);
  bool isOpen() const { return open_; }

  // returns true once when the text is confirmed; result() then holds it.
  bool update(Input& input, float dt);
  const std::string& result() const { return text_; }
  void draw(gfx::Gfx& g);

private:
  void close(Input& input);

  bool open_ = false;
  bool system_ = false;
  bool multiline_ = false;
  std::string title_;
  std::string text_;
  std::size_t caret_ = 0;
  float blink_ = 0.f;
};

}  // namespace sienna
