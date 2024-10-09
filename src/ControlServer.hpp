#pragma once

#include <string>

#include "lwip/api.h"
#include "FreeRTOS.h"
#include "task.h"


#include "Hub75.hpp"

class ControlServer {
private:
  netconn* conn;
  Hub75* matrix;
  TaskHandle_t currentAppHandle;

  void fullImg(netconn* conn, netbuf* nbuf);

public:
  ControlServer();
  ~ControlServer();
  bool init(Hub75* matrix);
  void listen();
  void processCommand(std::string command, netconn* conn, netbuf* nbuf);

  static void runTask(void* context);
};
