#include "ControlServer.hpp"

#include "FreeRTOSConfig.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <string>

#include "lwip/api.h"

#include "FreeRTOS.h"
#include "task.h"

#include "util.hpp"
#include "apps/clock/Clock.hpp"
#include "apps/animation/Animation.hpp"
#include "apps/pictureframe/PictureFrame.hpp"

// I don't like these two
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static void netconn_write_str(netconn* conn, std::string str) {
  netconn_write(conn, str.c_str(), str.length(), 0);
}

/*inline void netconn_write_str(netconn* conn, const char* str) {
  netconn_write(conn, str, strnlen(str, 512), 0);
}*/

bool ControlServer::init(Hub75* matrix) {
  cyw43_arch_enable_sta_mode();

#if defined(WIFI_SSID) and defined(WIFI_PASSWORD)
    if(cyw43_arch_wifi_connect_timeout_ms(TOSTRING(WIFI_SSID), TOSTRING(WIFI_PASSWORD), CYW43_AUTH_WPA2_AES_PSK, 30000)) {
    return false;
  }

#else
#error "SSID or PASSWORD are not defined. Define it via environment variable and pass it to CMake."
#endif
  blink(3, 200);

  this->conn = netconn_new(NETCONN_TCP);
  if(this->conn == NULL) {
    return false;
  }

  if(netconn_bind(this->conn, IP4_ADDR_ANY, 2314) != ERR_OK) {
    netconn_delete(this->conn);
    return false;
  }

  this->matrix = matrix;

  // Clock app by default
  xTaskCreate(Clock::runTask, "ClockAppThread", 4096, matrix, tskIDLE_PRIORITY + 2UL, &currentAppHandle);
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

void ControlServer::processCommand(std::string command, netconn* conn, netbuf* nbuf) {

  if(command == "pictureframe") {
	if (currentAppHandle != NULL) vTaskDelete(currentAppHandle);
	netconn_write_str(conn, "Starting picture frame app");
    const void* params[] = { matrix, &app };
    xTaskCreate(PictureFrame::run_task, "PictureFrameAppThread", 4096, params, tskIDLE_PRIORITY + 2UL, &currentAppHandle);
  } else if (command == "draw") {
	if (strcmp(pcTaskGetName(currentAppHandle), "PictureFrameApp") == 0) {
	  PictureFrame* app = static_cast<PictureFrame*>(this->app);
	  app->set_picture(conn, nbuf);
	} else {
	  netconn_write_str(conn, "Picture frame app not running");
	}
  } else if(command == "clock") {
	netconn_write_str(conn, "Starting clock app");
	if (currentAppHandle != NULL) vTaskDelete(currentAppHandle);
	xTaskCreate(Clock::runTask, "ClockAppThread", 4096, matrix, tskIDLE_PRIORITY + 2UL, &currentAppHandle);
  } else if(command == "getclocktime") {
	if (strcmp(pcTaskGetName(currentAppHandle), "ClockAppThread") == 0) {
	  Clock* app = static_cast<Clock*>(this->app);
	  netconn_write_str(conn, "Insert time here");
	}
  } else if(command == "animate") {
	netconn_write_str(conn, "Animating");
	if (currentAppHandle != NULL) vTaskDelete(currentAppHandle);
	xTaskCreate(runAnimationTask, "AnimationAppThread", 4096, matrix, tskIDLE_PRIORITY + 2UL, &currentAppHandle);
  } else if(command == "getcurrentapp") {
	netconn_write_str(conn, pcTaskGetName(currentAppHandle));
  }
}

ControlServer::ControlServer() {
}

ControlServer::~ControlServer() {
  netconn_delete(this->conn);
}
