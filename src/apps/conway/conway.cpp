#include "conway.hpp"
#include "Hub75.hpp"
#include "ControlServer.hpp"
#include "util/Netconn.hpp"

#include <vector>
#include <utility>
#include <string>

void step(std::vector<uint8_t>& state, std::vector<uint8_t>& next_state, uint width, uint height) {
  for(uint i = 0; i < state.size() * 8; ++i) {
	uint8_t neighbor_count = 0;

	const uint i_col = i % width;
	const uint i_row = i / width;

	uint start_col = (i_col == 0 ? i_col : i_col - 1), // Check if first i_col
	end_col = (i_col >= width - 1 ? i_col : i_col + 1), // Check if last i_col
	start_row = (i_row == 0 ? i_row : i_row - 1), // Check if first i_row
	end_row = (i_row >= height - 1 ? i_row : i_row + 1); // Check if last i_row

	for (uint row = start_row; row <= end_row; ++row) {
	  for (uint col = start_col; col <= end_col; ++col) {
		const uint index = row * width + col;
		if (i != index) {
		  neighbor_count += state[index / 8] >> index % 8 & 1;
		}
	  }
	}

	if (neighbor_count < 2 || neighbor_count > 3) {
	  next_state[i/8] &= ~((uint8_t)1 << (i % 8)); // set bit to 0
	} else if (neighbor_count == 3) {
	  next_state[i/8] |= (uint8_t)1 << (i % 8); // set bit to 1
	} else if (state[i / 8] >> i % 8 & 1) { // If 1, set to 1
	  next_state[i/8] |= (uint8_t)1 << (i % 8);
	} else {
	  next_state[i/8] &= ~((uint8_t)1 << (i % 8));
	}
  }
}

void conway_task(TaskHandle_t task) {
  // Create window
  Hub75& matrix = Hub75::instance();
  ControlServer& server = ControlServer::instance();
  const uint width = 64, height = 64;
  Window& window = matrix.create_window(0, 0, width, height);

  // TODO Populate window from netconn data
  // Each bit represents a cell, big endian left->right top->bottom
  std::vector<uint8_t> state1, state2;
  state1.resize(width * height / 8);
  state2.resize(width * height / 8);

  std::vector<uint8_t>* state = &state1;
  std::vector<uint8_t>* next_state = &state2;

  *state = {
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00,
0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x88,
0x30, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x03, 0x04, 0x31, 0x00, 0x00, 0x00, 0x00,
0x00, 0x03, 0x44, 0x43, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x01, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  state->resize(width * height / 8); // Make sure its the right size even though the provided sample isn't

  uint delay = 100; // Delay in milliseconds
  uint8_t fade_rate = 0x50;

  bool paused = false;
  // Main loop
  while(true) {

	Netconn* conn_ptr = server.receive_conn(pcTaskGetTaskName(nullptr), paused);
	if (conn_ptr != nullptr) {
	  Netconn& conn = *conn_ptr;
	  std::string cmd = conn.getline(" \r");

	  if (cmd == "quit") {
		break;
	  } else if (cmd == "delay") {
		delay = std::stoi(conn.getline());
		
	  } else if (cmd == "pause") {
		paused = true;
	  } else if (cmd == "resume") {
		paused = false;
	  } else if (cmd == "faderate") {
		fade_rate = std::stoi(conn.getline());
	  } else if (cmd == "setstate") {
		conn.println("Ready");
		conn.read_into(state->data(), state->size());
	  }
	  server.give_conn(pcTaskGetName(nullptr), conn_ptr);
	  continue;
	}

	step(*state, *next_state, width, height);
	std::swap(state, next_state);

	for(uint i = 0; i < width * height; ++i) {
	  if ((*state)[i/8] >> (i%8) & 1) {
		window.buffer[i] = 0xffffff;
	  } else {
		window.buffer[i] &= 0xff0000;
		window.buffer[i] = std::min(window.buffer[i], (uint32_t)0x880000);
		window.buffer[i] -= std::min(window.buffer[i], (uint32_t)fade_rate << 16);
	  }
	}
	matrix.request_update();
	vTaskDelay(pdMS_TO_TICKS(delay));

  }
  /* LOOP {

Simluation step, iterate through cell

Update window

Hub75::request_update()

 } */
}
