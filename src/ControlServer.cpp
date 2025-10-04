#include "ControlServer.hpp"

#include "FreeRTOSConfig.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <string>
#include <algorithm>
#include <tuple>

#include "lwip/api.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "util/Netconn.hpp"
#include "util.hpp"

#include "Apps.hpp"

// I don't like these two
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static void run_task(void* params) {
  std::tuple<AppFunction, const char *>* params_tuple = static_cast<std::tuple<AppFunction, const char *>*>(params);
  AppFunction task = std::get<AppFunction>(*params_tuple);
  // I currently don't do anything with the taskname
  
  TaskHandle_t handle = xTaskGetCurrentTaskHandle();
  delete[] params_tuple;

  task(handle);

  vTaskDelete(NULL);
}

bool ControlServer::init() {
  cyw43_arch_enable_sta_mode();

#if defined(WIFI_SSID) and defined(WIFI_PASSWORD)
  if(cyw43_arch_wifi_connect_timeout_ms(TOSTRING(WIFI_SSID), TOSTRING(WIFI_PASSWORD), CYW43_AUTH_WPA2_AES_PSK, 30000)) {
    return false;
  }

#else
#error "WIFI_SSID or WIFI_PASSWORD are not defined. Define it via environment variable and pass it to CMake."
#endif
  blink(3, 200);

  this->conn = netconn_new(NETCONN_TCP);
  if(this->conn == NULL) {
    return false;
  }

  if(netconn_bind(this->conn, IP4_ADDR_ANY, 2314) != ERR_OK) {
    netconn_delete(this->conn);
    return false;
  }

  this->control_server_handle = xTaskGetCurrentTaskHandle();

  //TODO start clock app by default

  return true;
}

void ControlServer::listen() {

  netconn_listen(this->conn);
  netconn* client_conn_ptr;
  while(1) {

    // Block until new client connection
    if(netconn_accept(this->conn, &client_conn_ptr) != ERR_OK) {
      blink(3, 1000);
      printf("Failed to accept connection\n");
      continue;
    }

	Netconn client_conn(client_conn_ptr);
	client_conn.println("LED Matrix OS v1.0");

	std::string command;
	while(client_conn.connected()) {
	  command = client_conn.getline(" \r"); // Gets 1 word
	  this->process_command(command, client_conn);
	}
  }
}

void ControlServer::process_command(const std::string_view command, Netconn& conn) {

  if (command == "start") {
	this->start_app(conn.getline(" \r"), conn);

  } else if (command == "forcequit") {
	this->force_quit_app(conn.getline(" \r"), conn);
  } else if (command == "msg") {
	// TODO send to app
	this->send_message(conn.getline(" \r"), conn);
  } else if (command == "listapps") {
	conn.println("Available apps:");
	for (const auto& pair : APPS) {
	  conn.println(pair.first);
	}
  } else if (command == "listrunning") {
	conn.println("Running apps:");
	for (const auto& pair : this->running_apps) {
	  conn.println(pair.first);
	}
  } else if (command == "ping") {
	conn.println("pong");
  } else {
	conn.println("Unknown command: ");
	conn.println(command);
	conn.clear_buffer(); // Discard rest of packet
  }
}

void ControlServer::start_app(std::string name, Netconn& conn) {
  const auto pair = APPS.find(name);
  if (pair == APPS.end()) {
	conn.println(std::string("start: Could not find app: ") + name);
	return;
  }

  // Generate unique task number
  uint16_t tasknum = 0;
  std::string taskname(pair->first + std::to_string(tasknum));
  while (this->running_apps.find(taskname) != this->running_apps.end()) taskname = pair->first + std::to_string((++tasknum));

  TaskHandle_t task_handle;

  std::tuple<AppFunction, const char*>* params = new std::tuple<AppFunction, const char *>{pair->second, taskname.c_str()}; // Dynamically allocated to transfer to new task

  this->running_apps[taskname] = AppInstance{nullptr, xQueueCreate(2, sizeof(Netconn*)), xQueueCreate(2, sizeof(Netconn*))};
  BaseType_t result = xTaskCreate(run_task, taskname.c_str(), 4096, params, tskIDLE_PRIORITY + 2UL, &this->running_apps[taskname].handle);
  if (result == pdPASS) {
	conn.println(std::string("Created task: ") + taskname);
  } else {
	conn.println("Unable to create task");
	this->running_apps.erase(taskname);
  }

  // TODO allow sending args
}

void ControlServer::force_quit_app(std::string app, Netconn& conn) {
  auto pair = this->running_apps.find(app);
  if (pair == this->running_apps.end()) {
	conn.println("send_message: Couldn't find app");
	return;
  }

  vTaskDelete(pair->second.handle);
  this->running_apps.erase(pair);
}

void ControlServer::send_message(std::string app, Netconn& conn) {

  auto pair = this->running_apps.find(app);
  if (pair == this->running_apps.end()) {
	conn.println("send_message: Couldn't find app");
	return;
  }

  auto& app_instance = pair->second;

  Netconn* item = &conn;

  if (xQueueSend(app_instance.conn_ready_queue, &item, 10000) != pdPASS) {
	conn.println("send_message: Failed to add conn to conn_ready_queue");
	return;
  }

  Netconn* queue_conn = nullptr;

  while(queue_conn != &conn) {
	if (xQueuePeek(app_instance.conn_finished_queue, &queue_conn, 10000) == errQUEUE_EMPTY) vTaskDelay(100);
  }
  xQueueReceive(app_instance.conn_finished_queue, nullptr, 1); // Wait 1 because it should be ready
}

// Meant to be called by App
Netconn* ControlServer::receive_conn(std::string_view taskname, bool block) {

  auto pair = this->running_apps.find(std::string(taskname));
  if (pair == this->running_apps.end()) return nullptr;


  Netconn* conn = nullptr;
  if (block) {
    while (xQueueReceive(pair->second.conn_ready_queue, &conn, portMAX_DELAY) != pdPASS);
  } else {
	xQueueReceive(pair->second.conn_ready_queue, &conn, 0);
  }
  return conn;
}

// Meant to be called by app
void ControlServer::give_conn(std::string_view taskname, Netconn* conn) {
  auto pair = this->running_apps.find(std::string(taskname));
  if (pair == this->running_apps.end()) {
	conn->println("give_conn: Couldn't find app");
    return;
  }

  if(xQueueSend(pair->second.conn_finished_queue, &conn, portMAX_DELAY) != pdPASS) {
	conn->println("Failed to give back semaphore");
  }
}

ControlServer::~ControlServer() {
  netconn_delete(this->conn);
}

ControlServer& ControlServer::instance() {
  static ControlServer instance;
  return instance;
}

