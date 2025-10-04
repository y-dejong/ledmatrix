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
  Hub75& matrix = Hub75::instance();

  std::string cmd = conn.getline(" \r");

  if (cmd == "quit") {
	conn.println("Quitting picture frame");
  } else if (cmd == "add") {

	// Create new Hub75 window with raw data

	// TODO Make Netconn a stream that can << into numbers
	uint dimensions[4];
	std::string tmpint;
	for(int i = 0; i < 4; ++i) {
	  tmpint = conn.getline(" \r");
	  dimensions[i] = std::stoi(tmpint);
	}

	this->windows.emplace_back(dimensions[0], dimensions[1], dimensions[2], dimensions[3]);
	std::vector<uint16_t>& buffer = windows.back().buffer();

	// TODO populate window
	conn.println(std::string("Ready: ") + std::to_string(this->windows.size() - 1));
	conn.read_into(buffer.data(), buffer.size() * sizeof(uint16_t));
	conn.println("Finished, requesting update");

	conn.println(std::string("Window size: ") + std::to_string(buffer.size()));
	windows.back().paint();
  } else if (cmd == "setdata") {
	size_t index = std::stoi(conn.getline(" \r"));
	if (index >= this->windows.size()) {
	  conn.println("Window not found");
	  return;
	}
	Window& window = this->windows[index];
	std::vector<uint16_t>& buffer = window.buffer();
	conn.println("Ready");
	conn.read_into(buffer.data(), buffer.size() * sizeof(uint16_t));
	conn.println("Finished, requesting update");
	window.paint();
  } else if (cmd == "move") {
	size_t index = std::stoi(conn.getline(" \r"));
	if (index >= this->windows.size()) {
	  conn.println("Window not found");
	  return;
	}

	Window& window = this->windows[index];
	window.x() = std::stoi(conn.getline(" \r"));
	window.y() = std::stoi(conn.getline(" \r"));
	window.paint();
  } else if (cmd == "remove") {
	size_t index = std::stoi(conn.getline());
	this->windows.erase(this->windows.begin() + index);
	conn.println("Removed window");
	// TODO handle update on erase?
  }
}

PictureFrame::PictureFrame(TaskHandle_t handle)
: handle(handle) {}
