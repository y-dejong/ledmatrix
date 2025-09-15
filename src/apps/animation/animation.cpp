#include "Hub75.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include "images/img_vaporwave.h"

void runAnimationTask(TaskHandle_t task) {
  Hub75& matrix = Hub75::instance();

  Window& win = matrix.create_window(0, 0, 64, 64);
  uint frame_num = 0;
  while (1) {
	for (uint i = 0; i < 64 * 64; ++i) {
	  win.buffer[i] = matrix.gamma_correct_565_888(img_vaporwave[frame_num][i]);
	}
	++frame_num;
	if (frame_num == 151) frame_num = 0;
	vTaskDelay(80);
  }
}
