#include "ControlServer.hpp"

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <string>

#include "lwip/api.h"
#include "lwip/priv/api_msg.h"

#include "FreeRTOS.h"
#include "task.h"

#define WIFI_SSID "Kingdom"
#define WIFI_PASSWORD "absolutewizardz"

void blink(int count, int duration) {
  for (int i = 0; i < count; ++i) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    vTaskDelay(duration);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    vTaskDelay(duration);
  }
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}

bool ControlServer::init() {
  cyw43_arch_enable_sta_mode();
  
  printf("Connecting to Wi-Fi...");
  if(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
    printf("Failed\n");
    return false;
  }
  printf("Connected\n");

  this->conn = netconn_new(NETCONN_TCP);
  if(this->conn == NULL) {
    printf("Failed to create netconn\n");
    return false;
  }
  
  if(netconn_bind(this->conn, IP4_ADDR_ANY, 2314) != ERR_OK) {
    printf("Failed to bind netconn\n");
    netconn_delete(this->conn);
    return false;
  }

  return true;
}

void ControlServer::listen() {

  netconn_listen(this->conn);
  netconn* client_conn;
  while(1) {

    // Block until new client connection
    if(netconn_accept(this->conn, &client_conn) != ERR_OK) {
      blink(3, 1000);
      printf("Failed to accept connection\n");
      continue;
    }
    blink(10, 100);

    // Parse command from incoming data
    std::string command;
    netbuf* current_buf;
    char* databuf;
    uint16_t len;
    while (netconn_recv(client_conn, current_buf) == ERR_OK) {
      do {
	netbuf_data(buf, &databuf, &len);
	command.append(databuf, len);

	size_t endPos = command.find('\r');
	if (endPos != std::string::npos) {
	  command.resize(pos);
	  this->processCommand(command);
	  command.clear();
	}
      } while (netbuf_next(databuf) >= 0);
    }

    std::string command = stringFromNetbuf(current_buf);
    
    netbuf_delete(current_buf);
    netconn_close(client_conn);
    netconn_delete(client_conn);
  }
}

void ControlServer::processCommand(std::string command, std::string partial, netbuf* nbuf) {
  if(command == "F
}

std::string ControlServer::stringFromNetbuf(netbuf* buf) {
  void* dataptr;
  uint16_t length;
  std::string result;
  
  netbuf_data(buf, &dataptr, &length);
  result.assign((char*)dataptr, length);
  return result;
}

ControlServer::ControlServer() {
}

ControlServer::~ControlServer() {
  netconn_delete(this->conn);
}
