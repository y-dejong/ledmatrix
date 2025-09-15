#pragma once

#include "hardware/pio.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include <array>
#include <vector>
#include <memory>
#include <optional>

struct Window {
  uint x, y, width, height;
  std::vector<uint32_t> buffer;

  Window(uint x, uint y, uint width, uint height)
  : x(x), y(y), width(width), height(height), buffer() {
	buffer.resize(width*height);
  }
};

class Hub75 {
private:
  uint panel_width;
  uint panel_height;
  uint panel_count;

  std::vector<uint32_t> master_buffer;
  std::vector<Window> app_windows;
  SemaphoreHandle_t app_windows_mutex;
  bool needs_update;

  // Programmable IO
  PIO pio;
  uint sm_data;
  uint sm_row;
  uint data_prog_offs;
  uint row_prog_offs;

public:
  uint width;
  uint height;

public:
  Hub75(uint panel_width, uint panel_height, uint panel_count, uint width, uint height);

  void render();
  void update();
  void set_pixel(const uint x, const uint y, const uint32_t pixel);

  Window& create_window(uint x, uint y, uint width, uint height);
  void remove_window(const Window& win);
  void request_update();

  uint32_t gamma_correct_565_888(uint16_t pixel);
  uint32_t gamma_correct_888(uint32_t pixel, float gamma);
  void gamma_correct_frame();

  static std::optional<Hub75> instance_;
  static Hub75& initialize(uint panel_width, uint panel_height, uint panel_count, uint width, uint height);
  static Hub75& instance(); // can this be const?
};

inline void Hub75::set_pixel(const uint x, const uint y, const uint32_t pixel) {
  if (y * this->width + x < width * height)
    this->master_buffer[y * this->width + x] = pixel;
}


