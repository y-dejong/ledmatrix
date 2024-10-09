#include "ControlServer.hpp"

#include "FreeRTOSConfig.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <string>

#include "lwip/api.h"

#include "FreeRTOS.h"
#include "task.h"

#include "apps/clock/Clock.hpp"

#define WIFI_SSID "Kingdom"
#define WIFI_PASSWORD "absolutewizardz"

static void blink(int count, int duration) {
  for (int i = 0; i < count; ++i) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    vTaskDelay(duration);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    vTaskDelay(duration);
  }
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}

static void netconn_write_str(netconn* conn, std::string str) {
  netconn_write(conn, str.c_str(), str.length(), 0);
}

bool ControlServer::init(Hub75* matrix) {
  cyw43_arch_enable_sta_mode();

  printf("Connecting to Wi-Fi...");
  if(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
    printf("Failed\n");
    return false;
  }
  printf("Connected\n");
  blink(3, 200);

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

  this->matrix = matrix;

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
    netbuf* nbuf;
    char* data;
    uint16_t len;
    while (netconn_recv(client_conn, &nbuf) == ERR_OK) {
      do {
		netbuf_data(nbuf, (void**)&data, &len);
		command.append(data, len);

		// If return carriage found, process the command
		size_t endPos = command.find('\r');
		if (endPos != std::string::npos) {
		  command.resize(endPos);

		  this->processCommand(command, client_conn, nbuf);
		  command.clear();
		  continue;
		}
      } while (netbuf_next(nbuf) >= 0);
      netbuf_delete(nbuf);
    }

    netconn_close(client_conn);
    netconn_delete(client_conn);
  }
}

void ControlServer::fullImg(netconn* conn, netbuf* nbuf) {
  uint32_t* frame_cursor = matrix->get_frame();

  char* data;
  uint16_t len;
  netbuf_data(nbuf, (void**)&data, &len);
  do {
    // Skip characters until past carriage return reached (end of command, start of data)
    ++data;
    --len;
  } while(*data != '\r');
  memcpy(frame_cursor, data, len);
  frame_cursor += len / 4;

  uint32_t total = 0;
  err_t current_err;
  netconn_write_str(conn, "Starting loop\n");
  while (total < matrix->width * matrix->height * 4 && (current_err = netconn_recv(conn, &nbuf)) == ERR_OK) {
    do {
	  netbuf_data(nbuf, (void**)&data, &len);
	  if (total + len > matrix->width * matrix->height * 4) {
		netconn_write_str(conn, "Limit safeguard triggered");
		len = matrix->width * matrix->height * 4 - total;
	  }

	  memcpy(frame_cursor, data, len);

	  total += len;
	  frame_cursor += len / 4;
	  netconn_write_str(conn, "Wrote packet buffer part: " + std::to_string(total) + " bytes");
    } while (netbuf_next(nbuf) >= 0);
    netconn_write_str(conn, "Finished a full netbuf?");
    netbuf_delete(nbuf);
  }
  netconn_write_str(conn, "Finished. " + std::to_string(total));
  netconn_write_str(conn, "Error with next recv: " + std::to_string(current_err));
  blink(4, 500);
}

void ControlServer::processCommand(std::string command, netconn* conn, netbuf* nbuf) {

  if(command == "fullimg") {
	fullImg(conn, nbuf);
  } else if(command == "overlayimg") {
	netconn_write_str(conn, "Drawing overlay image");

  } else if(command == "clock") {
	TaskHandle_t clockAppHandle;
	netconn_write_str(conn, "Starting clock app");
	blink(3,1000);
	xTaskCreate(Clock::runTask, "ClockAppThread", 4096, matrix, tskIDLE_PRIORITY + 2UL, &currentAppHandle);
  } else if(command == "getclocktime") {

  }
}

ControlServer::ControlServer() {
}

ControlServer::~ControlServer() {
  netconn_delete(this->conn);
}
