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
