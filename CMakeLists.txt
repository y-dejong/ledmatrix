cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS yes) # Creates compile_commands.json for LSPs
set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/external/pico-sdk)
set(FREE_RTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/external/FreeRTOS-Kernel)
set(PICO_BOARD "pico_w")
include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)

project(ledmatrix C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

if(DEFINED ENV{WIFI_SSID})
    set(WIFI_SSID "$ENV{WIFI_SSID}")
else()
    message(FATAL_ERROR "WIFI_SSID not set in environment")
endif()

if(DEFINED ENV{WIFI_PASSWORD})
    set(WIFI_PASSWORD "$ENV{WIFI_PASSWORD}")
else()
    message(FATAL_ERROR "WIFI_PASSWORD not set in environment")
endif()

add_executable(${PROJECT_NAME})

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/hub75.pio)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "src/")

target_compile_definitions(${PROJECT_NAME} PRIVATE
	PICO_DEFAULT_UART_TX_PIN=28
	PICO_DEFAULT_UART_RX_PIN=29
	NO_SYS=0 # States that there is an RTOS for pico_cyw43_arch_lwip_sys_freertos to work
	LWIP_SOCKET=1
	WIFI_SSID=${WIFI_SSID}
	WIFI_PASSWORD=${WIFI_PASSWORD}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
pico_stdlib hardware_pio
pico_cyw43_arch_lwip_sys_freertos
FreeRTOS-Kernel-Heap4)
pico_add_extra_outputs(${PROJECT_NAME})

