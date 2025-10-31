#include "Window.hpp"

namespace Text {
  void write_4x6(Window& window, uint x, uint y, uint pitch, uint16_t color, std::string text);
  void draw_char_4x6(Window& window, uint x, uint y, uint pitch, uint16_t color, char c);
  void write_5x7(Window& window, uint x, uint y, uint pitch, uint16_t color, std::string text);
  void draw_char_5x7(Window& window, uint x, uint y, uint pitch, uint16_t color, char c);
}
