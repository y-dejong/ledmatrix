#include "Window.hpp"
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
  void drawDateTimeOld(Window& window);
  void drawDateTime(Window& window);
void handle_message(Netconn& conn);
public:
  void run();


  static void runTask(TaskHandle_t task) {
	Clock clockapp;
	clockapp.run();
  }
};
