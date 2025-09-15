#include "PictureFrame.hpp"

#include "lwip/api.h"

#include <string>
#include <vector>
#include <charconv>

#include "util/Netconn.hpp"
#include "Hub75.hpp"
#include "ControlServer.hpp"

void PictureFrame::run_task(TaskHandle_t handle) {
  PictureFrame app(handle);

  ControlServer& server = ControlServer::instance();
  bool quit = false;

  while(!quit) {
	Netconn* conn_ptr = server.receive_conn(pcTaskGetTaskName(nullptr));
	if (conn_ptr == nullptr) continue;

	app.handle_message(*conn_ptr);
	server.give_conn(pcTaskGetName(nullptr), conn_ptr);
  }

  // TODO clear my windows
}

void PictureFrame::handle_message(Netconn& conn) {

  std::string cmd = conn.getline(" \r");

  if (cmd == "quit") {
	// TODO quit
	conn.println("Should quit");
  } else if (cmd == "add") {

	// Create new Hub75 window with raw data

	// TODO Make Netconn a stream that can << into numbers
	uint dimensions[4];
	std::string tmpint;
	for(int i = 0; i < 4; ++i) {
	  tmpint = conn.getline(" \r");
	  dimensions[i] = std::stoi(tmpint);
	}

	Window& window = Hub75::instance().create_window(dimensions[0], dimensions[1], dimensions[2], dimensions[3]);

	this->windows.push_back(window);

	// TODO populate window
	conn.println("Ready");
	conn.read_into(window.buffer.data(), window.buffer.size() * sizeof(uint32_t));
	conn.println("Finished, requesting update");

	conn.println(std::string("Window size: ") + std::to_string(window.buffer.size()));
	Hub75::instance().request_update();
  } else if (cmd == "remove") {
	size_t index = std::stoi(conn.getline());
	Hub75::instance().remove_window(this->windows[index]);
	conn.println("Removed window");
  }
}

PictureFrame::PictureFrame(TaskHandle_t handle)
: handle(handle) {}
