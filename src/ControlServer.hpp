#pragma once

#include <string>

#include "lwip/api.h"

class ControlServer {
private:
  netconn* conn;
  
public:
  ControlServer();
  ~ControlServer();
  bool init();
  void listen();
  std::string stringFromNetbuf(netbuf* buf);
};
