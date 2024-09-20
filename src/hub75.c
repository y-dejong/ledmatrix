/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hub75.pio.h"

#include "eevee.h"

#define DATA_BASE_PIN 0
#define DATA_N_PINS 6
#define ROWSEL_BASE_PIN 6
#define ROWSEL_N_PINS 4
#define CLK_PIN 11
#define STROBE_PIN 12
#define OEN_PIN 13

#define WIDTH 64
#define HEIGHT 64

#define PANEL_HEIGHT 32
#define PANEL_WIDTH 64
#define PANEL_COUNT 2

struct PIO_Data {
    PIO pio;
    uint sm_data;
    uint sm_row;
    uint data_prog_offs;
    uint row_prog_offs;
};

uint32_t framebuffer[WIDTH*HEIGHT];

uint32_t color_correct(uint32_t pixel) {
    uint32_t r = (pixel >> 16) & 0xFF; // Extract red component
    uint32_t g = (pixel >> 8) & 0xFF;  // Extract green component
    uint32_t b = (pixel >> 0) & 0xFF;  // Extract blue component
    
    r = r*r/255;
    g = g*g/255;
    b = b*b/255;
    return (r << 16) | (g << 8) | b;
}

void render(const uint32_t *image, struct PIO_Data* pio_data) {
    for (int panel_row = 0; panel_row < PANEL_HEIGHT / 2; ++panel_row) {
        for (int bit = 0; bit < 8; ++bit) { // Something PWM/BCM related? Not sure why we need this loop
        hub75_data_rgb888_set_shift(pio_data->pio, pio_data->sm_data, pio_data->data_prog_offs, bit);
            for (int panel = 0; panel < PANEL_COUNT; ++panel) {
                for (int panel_col = 0; panel_col < PANEL_WIDTH; ++panel_col) {
                    if (panel % 2 == 1) {// ODD
                        int img_row = panel_col;
                        int img_col = (panel+1)*PANEL_HEIGHT - 1 - panel_row;
                        pio_sm_put_blocking(pio_data->pio, pio_data->sm_data, image[img_row * WIDTH + img_col]);
                        img_col -= PANEL_HEIGHT/2;
                        pio_sm_put_blocking(pio_data->pio, pio_data->sm_data, image[img_row * WIDTH + img_col]);
                    } else { // EVEN
                        int img_row = HEIGHT - panel_col - 1;
                        int img_col = panel*PANEL_HEIGHT + panel_row;
                        pio_sm_put_blocking(pio_data->pio, pio_data->sm_data, image[img_row * WIDTH + img_col]);
                        img_col += PANEL_HEIGHT/2;
                        pio_sm_put_blocking(pio_data->pio, pio_data->sm_data, image[img_row * WIDTH + img_col]);
                    }
                }
            }
            // Dummy pixel per lane
            pio_sm_put_blocking(pio_data->pio, pio_data->sm_data, 0);
            pio_sm_put_blocking(pio_data->pio, pio_data->sm_data, 0);
            // SM is finished when it stalls on empty TX FIFO
            hub75_wait_tx_stall(pio_data->pio, pio_data->sm_data);
            // Also check that previous OEn pulse is finished, else things can get out of sequence
            hub75_wait_tx_stall(pio_data->pio, pio_data->sm_row);

            // Latch row data, pulse output enable for new row.
            pio_sm_put_blocking(pio_data->pio, pio_data->sm_row, panel_row | (100u * (1u << bit) << 5));
        }
    }
}

static inline uint32_t gamma_correct_565_888(uint16_t pix) {
    uint32_t r_gamma = pix & 0xf800u;
    r_gamma *= r_gamma;
    uint32_t g_gamma = pix & 0x07e0u;
    g_gamma *= g_gamma;
    uint32_t b_gamma = pix & 0x001fu;
    b_gamma *= b_gamma;
    return (b_gamma >> 2 << 16) | (g_gamma >> 14 << 8) | (r_gamma >> 24 << 0);
}

int main1() {
    stdio_init_all();

    struct PIO_Data pio_data;
    pio_data.pio = pio0;
    pio_data.sm_data = 0;
    pio_data.sm_row = 1;
    pio_data.data_prog_offs = pio_add_program(pio_data.pio, &hub75_data_rgb888_program);
    pio_data.row_prog_offs = pio_add_program(pio_data.pio, &hub75_row_program); 

    hub75_data_rgb888_program_init(pio_data.pio, pio_data.sm_data, pio_data.data_prog_offs, DATA_BASE_PIN, CLK_PIN);
    hub75_row_program_init(pio_data.pio, pio_data.sm_row, pio_data.row_prog_offs, ROWSEL_BASE_PIN, ROWSEL_N_PINS, STROBE_PIN);
    
    const uint32_t *img = image_rgb888;
    
    while(1) {
        for(int i = 0; i < WIDTH * HEIGHT; ++i) {
            framebuffer[i] = color_correct(img[i]);
        }
        render(framebuffer, &pio_data);
    }
}
