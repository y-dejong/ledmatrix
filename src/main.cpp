#include "ControlServer.hpp"
#include "hub75.hpp"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "FreeRTOS.h"
#include "task.h"

typedef struct FullState_struct {
  Hub75* matrix;
  ControlServer* server;
} FullState;

void controlServerTask(void* context) {
  FullState* state = (FullState*)context;
  ControlServer* server = state->server;

  cyw43_arch_init();
  
  if(!server->init()) {
    printf("Failed server init\n");
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    while(1);
    return;
  }
  server->listen();
}

void drawTask(void* context) {
  FullState* state = (FullState*) context;
  Hub75* matrix = state->matrix;

  while(1) {
    matrix->render();
    vTaskDelay(20);
  }
}

int main() {
  stdio_init_all();
  
  FullState state;
  Hub75 matrix(64, 32, 2, 64, 64);
  ControlServer server;
  state.matrix = &matrix;
  state.server = &server;
  
  TaskHandle_t controlServerHandle, drawHandle;
  xTaskCreate(controlServerTask, "ControlServerThread", 4096, &state, tskIDLE_PRIORITY + 1UL, &controlServerHandle);
  xTaskCreate(drawTask, "DrawThread", configMINIMAL_STACK_SIZE, &state, tskIDLE_PRIORITY + 5UL, &drawHandle);  

  vTaskStartScheduler();

  while(1);
}
