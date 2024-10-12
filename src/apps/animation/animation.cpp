#include "Hub75.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include "images/img_vaporwave.h"

void runAnimationTask(void* state) {
  Hub75& matrix = *(Hub75*)state;
  uint frame_num = 0;
  while (1) {
	uint32_t* frame = matrix.get_frame();
	for (uint i = 0; i < 64 * 64; ++i) {
	  frame[i] = matrix.gamma_correct_565_888(img_vaporwave[frame_num][i], 2.2f);
	}
	++frame_num;
	if (frame_num == 90) frame_num = 0;
	vTaskDelay(100);
  }
}
