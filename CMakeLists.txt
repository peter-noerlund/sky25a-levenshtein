cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR})

project(tt09-levenshtein LANGUAGES CXX)

find_package(Git REQUIRED)
find_package(Python COMPONENTS Interpreter REQUIRED)

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
    create-pdf
    COMMAND . venv/bin/activate && tt/tt_tool.py --create-pdf
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tt/tt_tool.py ${CMAKE_CURRENT_SOURCE_DIR}/venv/bin/python
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(
    create-summary
    COMMAND . venv/bin/activate && tt/tt_tool.py --openlane2 --print-warnings > ${CMAKE_CURRENT_BINARY_DIR}/summary.md
    COMMAND . venv/bin/activate && tt/tt_tool.py --openlane2 --print-stats >> ${CMAKE_CURRENT_BINARY_DIR}/summary.md
    COMMAND scripts/clock_summary.py >> ${CMAKE_CURRENT_BINARY_DIR}/summary.md
    COMMAND . venv/bin/activate && tt/tt_tool.py --openlane2 --print-cell-summary >> ${CMAKE_CURRENT_BINARY_DIR}/summary.md
    COMMAND . venv/bin/activate && tt/tt_tool.py --openlane2 --print-cell-category >> ${CMAKE_CURRENT_BINARY_DIR}/summary.md
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

file(GLOB VERILOG_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.v ${CMAKE_CURRENT_SOURCE_DIR}/src/*.sv )

add_subdirectory(client)
add_subdirectory(icestick)
add_subdirectory(pico-ice)