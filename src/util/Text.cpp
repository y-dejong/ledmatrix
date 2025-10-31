#include "Text.hpp"
#include "util/textfont.hpp"
#include "util.hpp"

void Text::draw_char_5x7(Window& window, uint x, uint y, uint pitch, uint16_t color, char c) {

  uint fontIndex;
  // Get character map
  if (c >= 'a' && c <= 'z') {
	fontIndex = c - 'a' + 10;
  } else if (c >= '0' && c <= '9'){
	fontIndex = c - '0';
  } else if (c == ' ') {
	return;
  } else {
	// PANIC
	// do something for space
	blink(20,30);
  }

  const uint original_x = x;

  for (uint i = 0; i < 7; ++i) {
	x = original_x;
	for (uint j = 0; j < 5; ++j) {
	  const uint currentPixel = i * 5 + j;
	  const uint8_t shouldPaint = numeric5x7_min[fontIndex][currentPixel / 8] >> currentPixel % 8 & 1;
	  for (uint u = y; u < y + pitch; ++u) {
		for (uint v = x; v < x + pitch; ++v) {
		  if (shouldPaint) window.dot(v, u, color);
		}
	  }
	  x += (pitch == 1 ? 1 : pitch + 1);
	}
	y += (pitch == 1 ? 1 : pitch + 1);
  }
}

void Text::write_5x7(Window& window, uint x, uint y, uint pitch, uint16_t color, std::string text) {
  for (const auto& c : text) {
	draw_char_5x7(window, x, y, pitch, color, c);
	x += 6 * (pitch == 1 ? 1 : pitch + 1);
  }
}


void Text::draw_char_4x6(Window& window, uint x, uint y, uint pitch, uint16_t color, char c) {

  size_t fontIndex;
  // Get character map
  if (c >= 'a' && c <= 'z') {
	fontIndex = c - 'a' + 10;
  } else if (c >= '0' && c <= '9'){
	fontIndex = c - '0';
  } else if (c == ' ') {
	return;
  } else {
	// PANIC
	// do something for space
	blink(20,30);
  }

  const uint original_x = x;

  for (uint i = 0; i < 6; ++i) {
	x = original_x;
	for (uint j = 0; j < 4; ++j) {
	  const uint currentPixel = i * 4 + j;
	  const uint8_t shouldPaint = alphanumeric4x6_min[fontIndex][currentPixel / 8] >> currentPixel % 8 & 1;
	  for (uint u = y; u < y + pitch; ++u) {
		for (uint v = x; v < x + pitch; ++v) {
		  if (shouldPaint) window.dot(v, u, color);
		}
	  }
	  x += (pitch == 1 ? 1 : pitch + 1);
	}
	y += (pitch == 1 ? 1 : pitch + 1);
  }
}

void Text::write_4x6(Window& window, uint x, uint y, uint pitch, uint16_t color, std::string text) {
  for (const auto& c : text) {
	draw_char_4x6(window, x, y, pitch, color, c);
	x += 5 * (pitch == 1 ? 1 : pitch + 1);
  }
}
