#include "Hub75.hpp"

#include <pico/stdlib.h>
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hub75.pio.h"

#define DATA_BASE_PIN 0
#define DATA_N_PINS 6
#define ROWSEL_BASE_PIN 6
#define ROWSEL_N_PINS 4
#define CLK_PIN 11
#define STROBE_PIN 12
#define OEN_PIN 13

Hub75::Hub75(uint panel_width, uint panel_height, uint panel_count, uint width, uint height)
  : panel_width(panel_width), panel_height(panel_height), panel_count(panel_count), width(width), height(height) {
  pio = pio0;
  sm_data = 0;
  sm_row = 1;
  data_prog_offs = pio_add_program(pio, &hub75_data_rgb888_program);
  row_prog_offs = pio_add_program(pio, &hub75_row_program);

  hub75_data_rgb888_program_init(pio, sm_data, data_prog_offs, DATA_BASE_PIN, CLK_PIN);
  hub75_row_program_init(pio, sm_row, row_prog_offs, ROWSEL_BASE_PIN, ROWSEL_N_PINS, STROBE_PIN);

  frame_buffer = new uint32_t[width * height];

  for(int i = 0; i < width * height; ++i) {
    frame_buffer[i] = 0x050000;
  }
}

Hub75::~Hub75() {
  delete[] frame_buffer;
}

void Hub75::render() {
  for (int panel_row = 0; panel_row < panel_height / 2; ++panel_row) {
        for (int bit = 0; bit < 8; ++bit) { // Something PWM/BCM related? Not sure why we need this loop
        hub75_data_rgb888_set_shift(pio, sm_data, data_prog_offs, bit);
            for (int panel = 0; panel < panel_count; ++panel) {
                for (int panel_col = 0; panel_col < panel_width; ++panel_col) {
                    if (panel % 2 == 1) {// ODD
                        int img_row = panel_col;
                        int img_col = (panel+1)*panel_height - 1 - panel_row;
                        pio_sm_put_blocking(pio, sm_data, frame_buffer[img_row * width + img_col]);
                        img_col -= panel_height/2;
                        pio_sm_put_blocking(pio, sm_data, frame_buffer[img_row * width + img_col]);
                    } else { // EVEN
                        int img_row = height - panel_col - 1;
                        int img_col = panel*panel_height + panel_row;
                        pio_sm_put_blocking(pio, sm_data, frame_buffer[img_row * width + img_col]);
                        img_col += panel_height/2;
                        pio_sm_put_blocking(pio, sm_data, frame_buffer[img_row * width + img_col]);
                    }
                }
            }
            // Dummy pixel per lane
            pio_sm_put_blocking(pio, sm_data, 0);
            pio_sm_put_blocking(pio, sm_data, 0);
            // SM is finished when it stalls on empty TX FIFO
            hub75_wait_tx_stall(pio, sm_data);
            // Also check that previous OEn pulse is finished, else things can get out of sequence
            hub75_wait_tx_stall(pio, sm_row);

            // Latch row data, pulse output enable for new row.
            pio_sm_put_blocking(pio, sm_row, panel_row | (100u * (1u << bit) << 5));
        }
    }
}

uint32_t* Hub75::get_frame() const {
  return frame_buffer;
}

void Hub75::set_frame(const uint32_t* img) {
  for(int i = 0; i < width * height; ++i) {
    frame_buffer[i] = img[i];
  }
}

void Hub75::overlayImage(uint32_t* data, uint x, uint y, uint src_width, uint src_height) {
  for(uint j = 0; j < this->width; ++j) {
    for(uint i = 0; i < this->height; ++i) {
      uint destX = x + i;
      uint destY = y + j;

      if (destX >= 0 && destX < this->width && destY >= 0 && destY < this->height) {
        this->frame_buffer[destY * this->width + destX] = data[j * src_width + i];
      }
    }
  }
}
