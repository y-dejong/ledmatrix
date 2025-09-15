#pragma once

#include <string>
#include <unordered_map>

#include "lwip/api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

#include "Hub75.hpp"
#include "util/Netconn.hpp"

typedef void (*AppFunction)(TaskHandle_t);

struct AppInstance {
  TaskHandle_t handle;
  QueueHandle_t conn_ready_queue;
  QueueHandle_t conn_finished_queue;
};

class ControlServer {
private:
  netconn* conn;
  std::unordered_map<std::string, AppInstance> running_apps;
  TaskHandle_t control_server_handle;

  void process_command(const std::string_view command, Netconn& conn);
  void start_app(std::string name, Netconn& conn);
  void force_quit_app(std::string name, Netconn& conn);
  void send_message(std::string app, Netconn& conn);

public:
  ~ControlServer();
  bool init();
  void listen();
  Netconn* receive_conn(std::string_view taskname, bool block = true);
  void give_conn(std::string_view taskname, Netconn* conn);

  static ControlServer& instance();
};
