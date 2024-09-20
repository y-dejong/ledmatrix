#include "hub75.hpp"

#include <stdio.h>
#include "pico/stdlib.h"

int main() {
  stdio_init_all();
  Hub75 matrix(64, 32, 2, 64, 64);
  
  while(1) {
    matrix.render();
  }
}
