#include "Hub75.hpp"

#include <pico/stdlib.h>
#include "hardware/pio.h"
#include "hub75.pio.h"

#include <math.h>

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

uint32_t Hub75::gamma_correct_565_888(uint16_t pixel) {
  uint8_t r5 = (pixel >> 11) & 0x1F;
  uint8_t g6 = (pixel >> 5) & 0x3F;
  uint8_t b5 = pixel & 0x1F;

  uint32_t r8 = (uint8_t)((r5 << 3) | (r5 >> 2));
  uint32_t g8 = (uint8_t)((g6 << 2) | (g6 >> 4));
  uint32_t b8 = (uint8_t)((b5 << 3) | (b5 >> 2));

  r8 = r8 * r8 / 255;
  r8 = r8 > 255 ? 255 : r8;

  g8 = g8 * g8 / 255;
  g8 = g8 > 255 ? 255 : g8;

  b8 = b8 * b8 / 255;
  b8 = b8 > 255 ? 255 : b8;

    // Pack the 8-bit RGB values into a 24-bit RGB888 format
    return (r8 << 16) | (g8 << 8) | (b8);
}

uint32_t Hub75::gamma_correct_888(uint32_t pixel, const float gamma) {
    uint8_t r_5bit = (pixel >> 11) & 0x1F;
    uint8_t g_6bit = (pixel >> 5) & 0x3F;
    uint8_t b_5bit = pixel & 0x1F;

	uint8_t r8 = (pixel >> 16) & 0xff;
	uint8_t g8 = (pixel >> 8) & 0xff;
	uint8_t b8 = pixel & 0xff;

    float r_corrected = powf((float)r8 / 255.0f, gamma);
    float g_corrected = powf((float)g8 / 255.0f, gamma);
    float b_corrected = powf((float)b8 / 255.0f, gamma);

    uint8_t r_8bit = (uint8_t)(r_corrected * 255.0f);
    uint8_t g_8bit = (uint8_t)(g_corrected * 255.0f);
    uint8_t b_8bit = (uint8_t)(b_corrected * 255.0f);

    // Pack the 8-bit RGB values into a 24-bit RGB888 format
    return (r_8bit << 16) | (g_8bit << 8) | (b_8bit);
}

void Hub75::gamma_correct_frame() {
  for(uint i = 0; i < width * height; ++i) {
	frame_buffer[i] = gamma_correct_888(frame_buffer[i], 2.2f);
  }
}
