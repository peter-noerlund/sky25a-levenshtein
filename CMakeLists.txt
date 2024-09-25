cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR})

project(tt09-levenshtein LANGUAGES CXX)

find_package(asio CONFIG REQUIRED)
find_package(verilator REQUIRED)
find_package(Git REQUIRED)
find_package(Python COMPONENTS Interpreter REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(lyra CONFIG REQUIRED)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/user_config.json
    COMMAND . venv/bin/activate && tt/tt_tool.py --openlane2 --create-user-config
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/config.json ${CMAKE_CURRENT_SOURCE_DIR}/info.yaml ${CMAKE_CURRENT_SOURCE_DIR}/tt/tt_tool.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/tt/tt_tool.py
    COMMAND ${GIT_EXECUTABLE} clone -b tt09 https://github.com/TinyTapeout/tt-support-tools tt
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/venv/bin/python
    COMMAND ${Python_EXECUTABLE} -m venv venv
    COMMAND venv/bin/pip install --upgrade pip
    COMMAND venv/bin/pip install -r tt/requirements.txt
    COMMAND venv/bin/pip install openlane==2.1.7
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tt/tt_tool.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(
    harden
    COMMAND . venv/bin/activate && tt/tt_tool.py --openlane2 --harden
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/user_config.json ${CMAKE_CURRENT_SOURCE_DIR}/tt/tt_tool.py ${CMAKE_CURRENT_SOURCE_DIR}/venv/bin/python
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(
    create-png
    COMMAND . venv/bin/activate && tt/tt_tool.py --openlane2 --create-png
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tt/tt_tool.py ${CMAKE_CURRENT_SOURCE_DIR}/venv/bin/python
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(
    test
    COMMAND . ../venv/bin/activate && make -B
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tt/tt_tool.py ${CMAKE_CURRENT_SOURCE_DIR}/venv/bin/python
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/test/gate_level_netlist.v
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/runs/wokwi/final/pnl/tt_um_levenshtein.pnl.v
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/runs/wokwi/final/pnl/tt_um_levenshtein.pnl.v ${CMAKE_CURRENT_SOURCE_DIR}/test/gate_level_netlist.v
)

add_custom_target(
    test-gates
    COMMAND . ../venv/bin/activate && make -B GATES=yes
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tt/tt_tool.py ${CMAKE_CURRENT_SOURCE_DIR}/venv/bin/python ${CMAKE_CURRENT_SOURCE_DIR}/test/gate_level_netlist.v
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
)

add_executable(uart_wb tools/uart_wb.cpp)
target_compile_features(uart_wb PRIVATE cxx_std_20)
target_compile_options(uart_wb PRIVATE -Wall -W -Wextra -Wpedantic)
target_link_libraries(uart_wb PRIVATE fmt::fmt-header-only)

add_executable(client
    client/client.cpp
    client/levenshtein.cpp
    client/main.cpp
    client/real_context.cpp
    client/real_uart.cpp
    client/runner.cpp
    client/test_set.cpp
    client/uart_bus.cpp
    client/verilator_context.cpp
    client/verilator_uart.cpp
)
target_include_directories(client PRIVATE client)
target_compile_features(client PRIVATE cxx_std_20)
target_compile_options(client PRIVATE -Wall -W -Wextra -Wno-unused-parameter)
target_link_libraries(client PRIVATE asio::asio fmt::fmt-header-only lyra)
target_link_options(client PRIVATE $<$<CONFIG:Release>:-flto>)

verilate(client
    TOP_MODULE top
    TRACE
    OPT_FAST "-O3 -march=native -flto"
    VERILATOR_ARGS -Wall -I${CMAKE_CURRENT_SOURCE_DIR}/src
    SOURCES
    client/top.v
    test/qspi_sram.sv
    ${VERILOG_SOURCES}
)

add_subdirectory(fpga)