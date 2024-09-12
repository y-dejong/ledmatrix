cmake_minimum_required(VERSION 3.20)
include(pico_sdk_import.cmake)

project(ledmatrix C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

set(PICO_BOARD "pico_w")
add_executable(${PROJECT_NAME})

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/hub75.pio)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE
	PICO_DEFAULT_UART_TX_PIN=28
	PICO_DEFAULT_UART_RX_PIN=29
)

target_link_libraries(${PROJECT_NAME} PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(${PROJECT_NAME})

