#pragma once

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/api.h"

#include <vector>

#include "Hub75.hpp"
#include "util/Netconn.hpp"

class PictureFrame {
private:
  TaskHandle_t handle;
  std::vector<std::reference_wrapper<Window>> windows;

  void handle_message(Netconn& conn);

public:
  static void run_task(TaskHandle_t handle);

  PictureFrame(TaskHandle_t handle);
};
