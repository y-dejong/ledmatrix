#include "util/Netconn.hpp"

#include "lwip/api.h"
#include "util.hpp"

Netconn::Netconn(netconn* conn)
: conn(conn), nbuf(nullptr), offset(0) {}

Netconn::~Netconn() {

  if (this->nbuf != nullptr) {
	netbuf_delete(this->nbuf);
  }

  if (this->conn != nullptr) {
	netconn_close(this->conn);
	netconn_delete(this->conn);
  }
}

void Netconn::clear_buffer() {
  netbuf_free(this->nbuf); // Free internal packet buffer, not free netbuf itself
  this->offset = 0;
}

std::string Netconn::getline(std::string_view delim) {

  std::string buffer;
  const char* data;
  uint16_t len;
  size_t cr_pos = std::string::npos;

  if (this->conn == nullptr) return std::string();

  if (this->nbuf == nullptr) netconn_recv(this->conn, &this->nbuf); // First recv by this Netconn

  do {
	do {
      if (netbuf_data(nbuf, (void **)&data, &len) != ERR_OK) {
        this->offset = 0;
        break;
      }

	  if (offset >= len) {
		this->offset = 0;
		continue;
	  }
	  //blink(1, 20);
	  cr_pos = std::string_view(data, len).find_first_of(delim, offset);

	  if (cr_pos != std::string::npos) {
		buffer.append(data + offset, cr_pos - offset);
		this->offset = cr_pos + 1;
		return buffer;
	  }

	  buffer.append(data + offset, len - offset);
	  this->offset = 0;

    } while(netbuf_next(this->nbuf) >= 0);
	netbuf_delete(this->nbuf);
  } while ((err_state = netconn_recv(this->conn, &this->nbuf)) == ERR_OK);

  // Client disconnected, should do something with err_state here probably
  netconn_delete(this->conn);
  this->conn = nullptr;
  return std::string();
}

// Throws out data in buffer first (data must be received after calling read_into)
void Netconn::read_into(void* dest, size_t len) {
  if (this->conn == nullptr) return;

  this->clear_buffer();

  void* data = nullptr; // should I be using std::bytes for pointer arithmetic?
  uint16_t offset = 0;

  while(len > 0 && (this->err_state = netconn_recv(this->conn, &this->nbuf)) == ERR_OK) {
	uint16_t copied = netbuf_copy(this->nbuf, dest + offset, len);
	len -= copied;
	offset += copied;
	if (len > 0) {
	  this->clear_buffer();
	  netbuf_delete(this->nbuf);
	}
  }
  if (this->err_state != ERR_OK) {
	this->print_err_state();
	this->println("Netconn::read_into: Error receiving netconn data");
	this->println(std::string("Remaining len: " + std::to_string(len)));
  }
}

void Netconn::println(std::string_view msg) {
  netconn_write(this->conn, msg.data(), msg.size(), NETCONN_MORE);
  netconn_write(this->conn, "\n", 1, 0);
}

bool Netconn::connected() {
  return this->conn != nullptr;
}


void Netconn::print_err_state() {
  switch(this->err_state) {
  case ERR_ARG:
	this->println("ERR_ARG");
	break;
  case ERR_MEM:
	this->println("ERR_MEM");
	break;
  case ERR_CONN:
	this->println("ERR_CONN");
	break;
  case ERR_WOULDBLOCK:
	this->println("ERR_WOULDBLOCK");
	break;
  case ERR_RST:
	this->println("ERR_RST");
	break;
  case ERR_CLSD:
	this->println("ERR_CLSD");
	break;
  case ERR_TIMEOUT:
	this->println("ERR_TIMEOUT");
  }
}
