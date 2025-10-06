#include "Window.hpp"

#include "util.hpp"

void Window::paint() {
  Hub75::instance().paint_window(this);
}

spin_lock_t* Window::registry_lock() {
  static spin_lock_t* lock = spin_lock_instance(spin_lock_claim_unused(true));
  return lock;
}

Window::Window(uint x, uint y, uint width, uint height)
: x(x), y(y), width(width), height(height) {
  buffer.resize(width * height);

  spin_lock_unsafe_blocking(Window::registry_lock());
  if(Window::registry_head == nullptr) Window::registry_head = this;
  if(Window::registry_tail) {
	Window::registry_tail->next = this;
  }
  Window::registry_tail = this;
  spin_unlock_unsafe(Window::registry_lock());
}

Window::~Window() {
  spin_lock_unsafe_blocking(Window::registry_lock());
  Window* old_prev = this->prev;
  if (old_prev) old_prev->next = this->next;
  if (this->next) this->next->prev = old_prev;
  if (Window::registry_head == this) Window::registry_head = this->next;
  if (Window::registry_tail == this) Window::registry_tail = this->prev;
  spin_unlock_unsafe(Window::registry_lock());
  Hub75::instance().update(true);
}

Window *Window::registry_head = nullptr, *Window::registry_tail = nullptr;
