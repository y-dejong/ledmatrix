#pragma once

#include "pico/cyw43_arch.h"

#include "FreeRTOS.h"
#include "task.h"

static void blink(int count, int duration) {
  for (int i = 0; i < count; ++i) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    vTaskDelay(duration);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    vTaskDelay(duration);
  }
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}

inline uint16_t to_rgba5551(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  // Downscale to 5 bits (shift right by 3)
  r >>= 3;
  g >>= 3;
  b >>= 3;
  return static_cast<uint16_t>((a << 15) | (b << 10) | (g << 5) | r);
}

inline uint16_t to_rgba5551(uint32_t rgb888) {
  return to_rgba5551(
					 rgb888 & 0xff,
					 (rgb888 >> 8) & 0xff,
					 (rgb888 >> 16) & 0xff,
					 (rgb888 >> 24) & 1);
}
