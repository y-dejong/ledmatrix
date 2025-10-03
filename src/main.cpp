#include "ControlServer.hpp"
#include "Hub75.hpp"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "FreeRTOS.h"
#include "task.h"

void controlServerTask(void* context) {
  ControlServer& server = ControlServer::instance();

  cyw43_arch_init();

  if(!server.init()) {
    printf("Failed server init\n");
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    return;
  }

  server.listen();
}

void drawTask(void* context) {
  Hub75& matrix = Hub75::instance();

  while(1) {
    matrix.render();
    vTaskDelay(1);
  }
}

int main() {
  stdio_init_all();

  Hub75::initialize(64, 32, 3, 96, 64);

  TaskHandle_t controlServerHandle, drawHandle;
  xTaskCreate(controlServerTask, "ControlServerThread", 4096, nullptr, tskIDLE_PRIORITY + 5UL, &controlServerHandle);
  xTaskCreate(drawTask, "DrawThread", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY + 4UL, &drawHandle);

  vTaskStartScheduler();

  while(1);
}
