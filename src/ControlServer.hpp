#pragma once

#include <string>

#include "lwip/api.h"

#include "Hub75.hpp"

class ControlServer {
private:
  netconn* conn;
  Hub75* matrix;

public:
  ControlServer();
  ~ControlServer();
  bool init(Hub75* matrix);
  void listen();
  void processCommand(std::string command, netconn* conn, netbuf* nbuf);
};
