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
  uint32_t* get_frame();
  void set_frame(const uint32_t* img);
  constexpr void set_pixel(const uint x, const uint y, const uint32_t pixel);

};
