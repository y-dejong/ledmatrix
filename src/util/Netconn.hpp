#pragma once

#include <string>

#include "lwip/api.h"

class Netconn {
private:
  netconn* conn;
  netbuf* nbuf;
  err_t err_state;
  uint16_t offset; // Offset inside the current pbuf segment, i.e. nbuf->ptr

  void read_nbuf_into_string(std::string& str);
  void print_err_state();

public:
  Netconn() = default;
  Netconn(netconn* conn);
  ~Netconn();

  std::string getline(std::string_view delim = "\r");
  void read_into(void* dest, size_t len);
  err_t get_err();
  void println(std::string_view msg);
  bool connected();
  void clear_buffer();
};
