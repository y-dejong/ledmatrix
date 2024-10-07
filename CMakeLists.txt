cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS yes)
set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/external/pico-sdk)
set(FREE_RTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/external/FreeRTOS-Kernel)
set(PICO_BOARD "pico_w")
include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)

project(ledmatrix C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(${PROJECT_NAME})

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/hub75.pio)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c" "src/apps/clock/*.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "src/")
target_include_directories(${PROJECT_NAME} PRIVATE "src/apps/clock/")


target_compile_definitions(${PROJECT_NAME} PRIVATE
	PICO_DEFAULT_UART_TX_PIN=28
	PICO_DEFAULT_UART_RX_PIN=29
	NO_SYS=0
	LWIP_SOCKET=1
)

target_link_libraries(${PROJECT_NAME} PRIVATE
pico_stdlib hardware_pio
pico_cyw43_arch_lwip_sys_freertos
FreeRTOS-Kernel-Heap4)
pico_add_extra_outputs(${PROJECT_NAME})

