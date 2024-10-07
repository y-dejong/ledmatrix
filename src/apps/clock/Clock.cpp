#include "Clock.hpp"
#include "font.hpp"

#include "Hub75.hpp"

#include "FreeRTOS.h"
#include "task.h"

// All in row/column format
constexpr static uint spriteDest[7][2] = {
	{1, 3}, // Hour, 10s digit
	{25, 3}, // Hour, 1s digit
	{1, 35}, // Minute, 10s digit
	{25, 35}, // Minute, 1s digit
	{46, 2}, // Month text
	{46, 10}, // Day text
	{46, 20} // Weather text
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
  // TODO: Get a time/date over TCP
  hour = 13;
  minute = 54;
  month = 3;
  day = 27;
  second = 0;
}

void Clock::drawDateTime() {
  uint32_t color = 0xffffff;

  drawLargeNumber5x7(hour/10, spriteDest[0][0], spriteDest[0][1], color, 0);
  drawLargeNumber5x7(hour%10, spriteDest[1][0], spriteDest[1][1], color, 0);
  drawLargeNumber5x7(minute/10, spriteDest[2][0], spriteDest[2][1], color, 0);
  drawLargeNumber5x7(minute%10, spriteDest[3][0], spriteDest[3][1], color, 0);

  drawAlphanumeric4x6(monthAbbrevs[month][0], spriteDest[4][0], spriteDest[4][1], color, 0);
  drawAlphanumeric4x6(monthAbbrevs[month][1], spriteDest[4][0] + 5, spriteDest[4][1], color, 0);
  drawAlphanumeric4x6(monthAbbrevs[month][2], spriteDest[4][0] + 10, spriteDest[4][1], color, 0);

  drawAlphanumeric4x6(day/10, spriteDest[5][0], spriteDest[5][1], color, 0);
  drawAlphanumeric4x6(day%10, spriteDest[5][0] + 5, spriteDest[5][1], color, 0);
}

void Clock::drawLargeNumber5x7(const uint number, uint x, uint y, const uint32_t color, const uint32_t bgcolor) {

  uint original_x = x;
  for (uint i = 0; i < 7; ++i) {
	x = original_x;
	for (uint j = 0; j < 5; ++j) {
	  uint currentPixel = i * 5 + j;
	  uint32_t currentColor = (numeric5x7_min[number][currentPixel / 8] >> currentPixel % 8 & 1) ? color : bgcolor;
	  matrix.set_pixel(x, y, currentColor);
	  matrix.set_pixel(x+1, y, currentColor);
	  matrix.set_pixel(x, y+1, currentColor);
	  matrix.set_pixel(x+1, y+1, currentColor);

	  // Clear space between pixels
	  matrix.set_pixel(x+2, y, bgcolor);
	  matrix.set_pixel(x+2, y+1, bgcolor);
	  matrix.set_pixel(x+2, y+2, bgcolor);
	  matrix.set_pixel(x, y+2, bgcolor);
	  matrix.set_pixel(x+1, y+2, bgcolor);
	  x += 3;
	}
	y += 3;
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

  // Get current time

  // LOOP
  // Show current time
  // delay 1 second
  // update sprite/blink, update time if needed

  initDateTime();

  for(uint i = 0; i < 64 * 64; ++i) {
	matrix.set_pixel(i % 64, i / 64, 0x000000);
  }

  while (1) {
	// Update current time
	bool needsRedraw = true;

	if(++second >= 60) {
	  second = 0;
	  ++minute;
	}

	if(minute >= 60) {
	  minute = 0;
	  ++hour;
	  needsRedraw = true;
	}

	if(hour >= 24) {
	  hour = 0;
	  ++day;
	  needsRedraw = true;
	}

	if(day >= 30) { // TODO variable length days
	  day = 1;
	  ++month;
	  needsRedraw = true;
	}

	if(month >= 12) {
	  month = 0;
	  needsRedraw = true;
	}

	drawDateTime();
	vTaskDelay(1000); // Delay for 1 second
  }

  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);


}

Clock::Clock(Hub75& matrix) : matrix(matrix) {
  
}
