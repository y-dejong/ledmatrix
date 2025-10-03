#include "Hub75.hpp"
#include "ControlServer.hpp"

#include "pico/cyw43_arch.h"

#include "FreeRTOS.h"
#include "task.h"

#include <ctime>

class Clock {

private:
  std::time_t current_time;
  bool quit = false;

  void initDateTime();
  void drawLargeNumber5x7(Window& window, const uint number, uint x, uint y, const uint32_t color);
  void drawAlphanumeric4x6(Window& window, const char c, uint x, uint y, const uint32_t color);
  void drawDateTime(Window& window);
  void handle_message(Netconn& conn);
public:
  void run();


  static void runTask(TaskHandle_t task) {
	Clock clockapp;
	clockapp.run();
  }
};
