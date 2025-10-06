#pragma once

#include <memory>
#include <algorithm>
#include <vector>
#include <string>

#include <hardware/sync.h>

#include "Hub75.hpp"

class Window {
private:
  Window *prev = nullptr, *next = nullptr;
  static spin_lock_t* registry_lock();
  static Window* registry_head;
  static Window* registry_tail;

  Window() = default; // Empty window ctor should only be used by all_windows

public:
  std::vector<uint16_t> buffer;
  uint x, y, width, height;
  void dot(uint x, uint y, uint16_t value);
  void paint();

  friend void Hub75::update(bool full_repaint);

  Window(uint x, uint y, uint width, uint height);
  ~Window();
};

inline void Window::dot(uint x, uint y, uint16_t value) {
  this->buffer[y * this->width + x] = value;
}
