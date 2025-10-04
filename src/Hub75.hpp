#pragma once

#include "hardware/pio.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include <array>
#include <vector>
#include <memory>
#include <optional>

class Hub75 {
private:
  uint panel_width;
  uint panel_height;
  uint panel_count;

  std::vector<uint32_t> master_buffer;
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
  void set_pixel888(const uint x, const uint y, const uint32_t pixel);

  void request_update();

  uint32_t gamma_correct_565_888(uint16_t pixel);
  uint32_t gamma_correct_888(uint32_t pixel, float gamma);
  uint32_t gamma_correct_555_888(uint16_t pixel);
  void gamma_correct_frame();

  static std::optional<Hub75> instance_;
  static Hub75& initialize(uint panel_width, uint panel_height, uint panel_count, uint width, uint height);
  static Hub75& instance(); // can this be const?
};

inline void Hub75::set_pixel888(const uint x, const uint y, const uint32_t pixel) {
  if (y * this->width + x < width * height)
    this->master_buffer[y * this->width + x] = pixel;
}


