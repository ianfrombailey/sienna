#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/input.hpp"
#include "gfx/gfx.hpp"

namespace sienna {

// modal list of choices. also used for yes/no confirmations.
class Menu {
public:
  void open(std::string title, std::vector<std::string> items);
  bool isOpen() const { return open_; }

  // returns the chosen index once, otherwise -1. closes on choose or cancel.
  int update(Input& input);
  void draw(gfx::Gfx& g);

private:
  bool open_ = false;
  std::string title_;
  std::vector<std::string> items_;
  std::size_t selected_ = 0;
};

}  // namespace sienna
