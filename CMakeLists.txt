cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR})

project(tt09-levenshtein LANGUAGES CXX)

find_package(verilator REQUIRED)
find_package(Git REQUIRED)
find_package(Python COMPONENTS Interpreter REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(tt09-levenshtein
    sim/levenshtein.cpp
    sim/main.cpp
    sim/test_set.cpp
)
target_include_directories(tt09-levenshtein PRIVATE sim)
target_compile_features(tt09-levenshtein PRIVATE cxx_std_20)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(tt09-levenshtein PRIVATE -Wall -W -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare $<$<CONFIG:Release>:-march=native -flto>)
endif()
target_link_libraries(tt09-levenshtein PRIVATE fmt::fmt-header-only)
target_link_options(tt09-levenshtein PRIVATE $<$<CONFIG:Release>:-flto>)

verilate(tt09-levenshtein
    TOP_MODULE top
    TRACE
    OPT_FAST "-O3 -march=native -flto"
    VERILATOR_ARGS -Wall -I${CMAKE_CURRENT_SOURCE_DIR}/src
    SOURCES
    sim/top.v
    src/tt_um_levenshtein.v
    src/levenshtein_controller.sv
    src/spi_controller.sv
    src/uart_wishbone_bridge.sv
    src/wb_arbiter.sv
    src/wb_interconnect.sv
    test/qspi_sram.sv
)

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
    fpga-bitstream
    COMMAND . venv/bin/activate && tt/tt_tool.py --create-fpga-bitstream
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tt/tt_tool.py ${CMAKE_CURRENT_SOURCE_DIR}/venv/bin/python
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