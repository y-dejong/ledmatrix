#pragma once

#include "hardware/pio.h"

class Hub75 {
private:
  uint panel_width;
  uint panel_height;
  uint panel_count;

  uint32_t *frame_buffer;

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
  ~Hub75();

  void render();
  uint32_t* get_frame() const;
  void set_frame(const uint32_t* img);
  void set_pixel(const uint x, const uint y, const uint32_t pixel);
  void overlayImage(uint32_t* data, uint x, uint y, uint src_width, uint src_height);

  uint32_t gamma_correct_565_888(uint16_t pixel, float gamma);
  uint32_t gamma_correct_888(uint32_t pixel, float gamma);
  void gamma_correct_frame();

};

inline void Hub75::set_pixel(const uint x, const uint y, const uint32_t pixel) {
  if (y * this->width + x < width * height) this->frame_buffer[y * this->width + x] = pixel;
}
