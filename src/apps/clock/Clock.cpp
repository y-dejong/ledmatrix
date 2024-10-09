#include "Clock.hpp"
#include "font.hpp"

#include "Hub75.hpp"
#include "util.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/api.h"
#include <lwip/ip_addr.h>

#include <ctime>

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
	{1, 58} // AM/PM
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

void Clock::drawDateTime() {
  uint32_t color = 0xffffff;

  std::tm* timeinfo = std::gmtime(&current_time);
  
  drawLargeNumber5x7(timeinfo->tm_hour%12/10, spriteDest[0][0], spriteDest[0][1], color, 0);
  drawLargeNumber5x7(timeinfo->tm_hour%12%10, spriteDest[1][0], spriteDest[1][1], color, 0);
  drawLargeNumber5x7(timeinfo->tm_min/10, spriteDest[2][0], spriteDest[2][1], color, 0);
  drawLargeNumber5x7(timeinfo->tm_min%10, spriteDest[3][0], spriteDest[3][1], color, 0);
  drawAlphanumeric4x6((timeinfo->tm_hour / 12 ? 'p' : 'a'), spriteDest[6][0], spriteDest[6][1], color, 0);
  drawAlphanumeric4x6('m', spriteDest[6][0] + 5, spriteDest[6][1], color, 0);

  drawAlphanumeric4x6(monthAbbrevs[timeinfo->tm_mon][0], spriteDest[4][0], spriteDest[4][1], color, 0);
  drawAlphanumeric4x6(monthAbbrevs[timeinfo->tm_mon][1], spriteDest[4][0] + 5, spriteDest[4][1], color, 0);
  drawAlphanumeric4x6(monthAbbrevs[timeinfo->tm_mon][2], spriteDest[4][0] + 10, spriteDest[4][1], color, 0);

  drawAlphanumeric4x6(timeinfo->tm_mday/10, spriteDest[5][0], spriteDest[5][1], color, 0);
  drawAlphanumeric4x6(timeinfo->tm_mday%10, spriteDest[5][0] + 5, spriteDest[5][1], color, 0);
}

void Clock::drawLargeNumber5x7(const uint number, uint x, uint y, const uint32_t color, const uint32_t bgcolor) {

  const uint original_x = x;
  for (uint i = 0; i < 7; ++i) {
	x = original_x;
	for (uint j = 0; j < 5; ++j) {
	  uint currentPixel = i * 5 + j;
	  uint32_t currentColor = (numeric5x7_min[number][currentPixel / 8] >> currentPixel % 8 & 1) ? color : bgcolor;
	  matrix.set_pixel(x, y, currentColor);
	  matrix.set_pixel(x+1, y, currentColor);
	  matrix.set_pixel(x+2, y, currentColor);
	  matrix.set_pixel(x, y+1, currentColor);
	  matrix.set_pixel(x+1, y+1, currentColor);
	  matrix.set_pixel(x+2, y+1, currentColor);
	  matrix.set_pixel(x, y+2, currentColor);
	  matrix.set_pixel(x+1, y+2, currentColor);
	  matrix.set_pixel(x+2, y+2, currentColor);

	  // Clear space between pixels
	  matrix.set_pixel(x+3, y, bgcolor);
	  matrix.set_pixel(x+3, y+1, bgcolor);
	  matrix.set_pixel(x+3, y+2, bgcolor);
	  matrix.set_pixel(x, y+3, bgcolor);
	  matrix.set_pixel(x+1, y+3, bgcolor);
	  matrix.set_pixel(x+2, y+3, bgcolor);
	  x += 4;
	}
	y += 4;
  }
}

void Clock::drawAlphanumeric4x6(const char c, uint x, uint y, const uint32_t color, const uint32_t bgcolor) {

  // Get index in the font array from the char
  uint fontIndex;
  if (c >= 'a' && c <= 'z') {
    fontIndex = c - 'a' + 10;
  } else {
	fontIndex = c;
  }

  for (uint i = 0; i < 6; ++i) {
	for (uint j = 0; j < 4; ++j) {
	  uint currentPixel = i * 4 + j;
	  uint32_t currentColor = (alphanumeric4x6_min[fontIndex][currentPixel / 8] >> currentPixel % 8 & 1) ? color : bgcolor;
	  matrix.set_pixel(x + j, y + i, currentColor);
	}
  }
}

void Clock::run() {

  // Retrieve current time over internet
  initDateTime();

  // Set screen to black
  for(uint i = 0; i < 64 * 64; ++i) {
	matrix.set_pixel(i % 64, i / 64, 0x000000);
  }

  drawDateTime();
  bool needs_redraw;
  while (1) {
	// Update current time, and redraw if new minute
	if (++current_time % 60 == 0) drawDateTime();
	vTaskDelay(1000); // Delay for 1 second
  }
}

Clock::Clock(Hub75& matrix) : matrix(matrix) {
  
}
