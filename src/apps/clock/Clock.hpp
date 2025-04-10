#include "Hub75.hpp"

#include "pico/cyw43_arch.h"

#include "FreeRTOS.h"
#include "task.h"

#include <ctime>

class Clock {

private:
  Hub75& matrix;
  std::time_t current_time;

  void initDateTime();
  void drawLargeNumber5x7(const uint number, uint x, uint y, const uint32_t color, const uint32_t bgcolor);
  void drawAlphanumeric4x6(const char c, uint x, uint y, const uint32_t color, const uint32_t bgcolor);
  void drawDateTime();
public:
  void run();

  Clock(Hub75& matrix);

  static void runTask(void* state) {

	Hub75& m = *(Hub75*)state;
	Clock clockapp(m);
	clockapp.run();
  }
};
