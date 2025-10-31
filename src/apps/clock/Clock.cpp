#include "Clock.hpp"
#include "util/Text.hpp"

#include "ControlServer.hpp"
#include "Window.hpp"
#include "util.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/api.h"
#include <lwip/ip_addr.h>

#include <ctime>
#include <sstream>

#define TIMEZONE_OFFSET -25200

// All in row/column format
constexpr static uint spriteDest[8][2] = {
	{1, 3}, // Hour, 10s digit
	{25, 3}, // Hour, 1s digit
	{1, 35}, // Minute, 10s digit
	{25, 35}, // Minute, 1s digit
	{46, 2}, // Month text
	{46, 10}, // Day text
	{46, 20}, // Weather text
	{46, 56} // AM/PM
};

constexpr static const char* monthAbbrevs[] = {
  "jan",
  "feb",
  "mar",
  "apr",
  "may",
  "jun",
  "jul",
  "aug",
  "sep",
  "oct",
  "nov",
  "dec"
};

const uint32_t color_gradient[] = {0xbf6c1f, 0xba6c22, 0xb56c26, 0xb16c29, 0xac6c2d, 0xa86c30, 0xa36c34, 0x9f6d37, 0x9a6d3b,
0x966d3e, 0x916d42, 0x8d6d45, 0x886d49, 0x846e4c, 0x7f6e50, 0x7b6e53, 0x766e57, 0x726e5a, 0x6d6e5e, 0x696f61, 0x646f65,
0x606f68, 0x5b6f6c, 0x576f6f, 0x526f73, 0x4e7076, 0x49707a, 0x45707d, 0x407081, 0x3c7084, 0x377088, 0x33718c,
0x31718d, 0x2f718e, 0x2e718f, 0x2c7191, 0x2b7192, 0x297193, 0x277194, 0x267196, 0x247197, 0x237198, 0x21719a,
0x1f719b, 0x1e719c, 0x1c719d, 0x1b719f, 0x1972a0, 0x1772a1, 0x1672a3, 0x1472a4, 0x1372a5, 0x1172a6, 0x0f72a8,
0x0e72a9, 0x0c72aa, 0x0b72ac, 0x0972ad, 0x0772ae, 0x0672af, 0x0472b1, 0x0372b2, 0x0172b3, 0x0073b5};

void Clock::initDateTime() {

  // New netconn for UDP
  netconn* conn = netconn_new(NETCONN_UDP);
  if(conn == NULL) {
	blink(3, 500);
	return;
  }

  // Get time server IP address using DNS
  ip_addr_t ntp_addr;
  if(netconn_gethostbyname("time.cloudflare.com", &ntp_addr) != ERR_OK) {
	blink(3, 500);
	netconn_delete(conn);
	return;
  }

  // Connect to time server
  if (netconn_connect(conn, &ntp_addr, 123)) {
	blink(3, 500);
	netconn_delete(conn);
	return;
  }

  // Create network buffer for data
  netbuf* nbuf = netbuf_new();
  if (nbuf == NULL) {
	blink(3, 500);
	netconn_delete(conn);
	return;
  }

  // Populate network buffer
  unsigned char nbuf_data[48] = {0x1b};
  if (netbuf_ref(nbuf, nbuf_data, sizeof(nbuf_data)) != ERR_OK) {
	blink(3, 500);
	netbuf_delete(nbuf);
	netconn_delete(conn);
  }

  // Send NTP message
  if(netconn_send(conn, nbuf) != ERR_OK) {
	blink(3, 500);
	netbuf_delete(nbuf);
	netconn_delete(conn);
  }
  netbuf_delete(nbuf);

  // Wait for UDP reply
  if(netconn_recv(conn, &nbuf) == ERR_OK) {

	// Retrieve data
	unsigned char* data;
	unsigned short datalen;
	netbuf_data(nbuf, (void**)&data, &datalen);

	// Get timestamp and convert to unix time
	uint32_t timestamp = data[40] << 24 | data[41] << 16 | data[42] << 8 | data[43];
	timestamp -= 2208988800UL; // NTP Epoch offset
	timestamp += TIMEZONE_OFFSET;
	current_time = (std::time_t) timestamp;

	netbuf_delete(nbuf);

  } else {
	blink(4, 700);
  }

  netconn_close(conn);
  netconn_delete(conn);
}

void Clock::drawDateTime(Window& window) {
  std::tm* timeinfo = std::gmtime(&current_time);

  int hour = timeinfo->tm_hour%12;
  std::stringstream timestring;
  timestring << hour << " " << (timeinfo->tm_min < 10 ? "0" : "")
  << timeinfo->tm_min << (timeinfo->tm_hour / 12 ? " pm" : " am");

  Text::write_4x6(window, 4, 4, 1, to_rgba5551(200, 200, 200, 1), timestring.str());

  timestring.str(std::string()); // Clears it
  timestring << monthAbbrevs[timeinfo->tm_mon] << " " << timeinfo->tm_mday;
  Text::write_4x6(window, 4, 56, 1, to_rgba5551(200, 200, 200, 1), timestring.str());

  window.paint();
}

void Clock::drawDateTimeOld(Window& window) { // Old layout for date time, looks good on 64x64
  uint32_t color = 0xffffff;

  std::tm* timeinfo = std::gmtime(&current_time);

  for(auto& pixel : window.buffer) {
	pixel = 0;
  }

  int hour = timeinfo->tm_hour%12;
  hour = (hour == 0 ? 12 : hour);
  color = to_rgba5551(0x122a6f2);
  Text::draw_char_5x7(window, spriteDest[0][0], spriteDest[0][1], 3, color, hour/10 + '0');
  Text::draw_char_5x7(window, spriteDest[1][0], spriteDest[1][1], 3, color, hour%10 + '0');

  color = to_rgba5551(0x1bf6c1f);
  Text::draw_char_5x7(window, spriteDest[2][0], spriteDest[2][1], 3, color, timeinfo->tm_min/10 + '0');
  Text::draw_char_5x7(window, spriteDest[3][0], spriteDest[3][1], 3, color, timeinfo->tm_min%10 + '0');

  color = to_rgba5551(0x1bf6c1f);
  Text::draw_char_4x6(window, spriteDest[7][0], spriteDest[7][1], 1, color, (timeinfo->tm_hour / 12 ? 'p' : 'a'));
  Text::draw_char_4x6(window, spriteDest[7][0] + 5, spriteDest[7][1], 1, color, 'm');

  color = to_rgba5551(0x122a6f2);
  Text::draw_char_4x6(window, spriteDest[4][0], spriteDest[4][1], 1, color, monthAbbrevs[timeinfo->tm_mon][0]);
  Text::draw_char_4x6(window, spriteDest[4][0] + 5, spriteDest[4][1], 1, color, monthAbbrevs[timeinfo->tm_mon][1]);
  Text::draw_char_4x6(window, spriteDest[4][0] + 10, spriteDest[4][1], 1, color, monthAbbrevs[timeinfo->tm_mon][2]);

  Text::draw_char_4x6(window, spriteDest[5][0], spriteDest[5][1], 1, color, timeinfo->tm_mday/10 + '0');
  Text::draw_char_4x6(window, spriteDest[5][0] + 5, spriteDest[5][1], 1, color, timeinfo->tm_mday%10 + '0');

  window.paint();
  blink(5, 200);
}

void Clock::handle_message(Netconn& conn) {
  std::string cmd = conn.getline(" \r");
  if (cmd == "quit") {
	conn.println("Quitting clock");
	this->quit == true;
  } else if (cmd == "setgradient") {
	// TODO
  }
}

void Clock::run() {

  // Retrieve current time over internet
  initDateTime();

  Window window(0, 0, 96, 64);

  drawDateTime(window);
  while (!this->quit) {
	Netconn* conn_ptr = ControlServer::instance().receive_conn(pcTaskGetTaskName(nullptr), false);
	if (conn_ptr != nullptr) {
	  this->handle_message(*conn_ptr);
	  ControlServer::instance().give_conn(pcTaskGetTaskName(nullptr), conn_ptr);
	}
	// Update current time, and redraw if new minute
	if (++current_time % 60 == 0) drawDateTime(window);
	vTaskDelay(1000); // Delay for 1 second
  }
}
