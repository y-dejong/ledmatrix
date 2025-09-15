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

  // If we are part way through a netbuf
  /*if (this->offset != 0) {
	netbuf_data(this->nbuf, (void**)&data, &len);
	if ((cr_pos = std::string_view(static_cast<const char*>(data) + offset, len).find_first_of(delim)) != std::string::npos) {
	  // If we found a \r in the rest of the current netbuf, just return that and adjust the offset
	  buffer.append(data + offset, len - offset - cr_pos);
	  this->offset = offset + cr_pos + 1; // Dear god I hope this is correct
	  return buffer;
	} else {
	  buffer.append(data + offset, len - offset);
	  offset = len;
	}
  }*/

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
	  blink(1, 20);
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

  void* data = nullptr;

  while(len > 0 && (this->err_state = netconn_recv(this->conn, &this->nbuf)) == ERR_OK) {
	len -= netbuf_copy(this->nbuf, dest, len);
  }
  if (this->err_state != ERR_OK) {
	this->println("Error receiving netconn data");
	this->println(std::string("Remaining len: " + std::to_string(len)));
  }
  this->clear_buffer(); // Probably should try to remove this too
}

void Netconn::println(std::string_view msg) {
  netconn_write(this->conn, msg.data(), msg.size(), 0);
  netconn_write(this->conn, "\n", 1, 0);
}

bool Netconn::connected() {
  return this->conn != nullptr;
}
