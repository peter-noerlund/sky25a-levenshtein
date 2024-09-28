set(FPGA_CONSTRAINTS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/icestick.pcf)
set(FPGA_TARGET_FREQ 50)

find_program(YOSYS_PATH NAMES yosys HINTS ${YOSYS_PATH} REQUIRED)
message(STATUS "yosys path: ${YOSYS_PATH}")

if(NOT DEFINED YOSYS_DATA_DIR)
    find_program(YOSYS_CONFIG_PATH NAME yosys-config HINTS ${YOSYS_CONFIG_PATH} REQUIRED)
    message(STATUS "yosys-config path: ${YOSYS_CONFIG_PATH}")

    execute_process(
        COMMAND ${YOSYS_CONFIG_PATH} --datdir
        OUTPUT_VARIABLE YOSYS_DATA_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()
message(STATUS "yosys data directory: ${YOSYS_DATA_DIR}")

# Nextpnr
if(NOT DEFINED NEXTPNR_ICE40_PATH)
    find_program(NEXTPNR_ICE40_PATH NAMES nextpnr-ice40 REQUIRED)
endif()
message(STATUS "nextnpr-ice40 path: ${NEXTPNR_ICE40_PATH}")

# Icestorm
if(NOT DEFINED ICEPACK_PATH)
    find_program(ICEPACK_PATH NAMES icepack REQUIRED)
endif()
message(STATUS "icepack path: ${ICEPACK_PATH}")

if(NOT DEFINED ICEPROG_PATH)
    find_program(ICEPROG_PATH NAMES iceprog REQUIRED)
endif()
message(STATUS "iceprog path: ${ICEPROG_PATH}")

if(NOT DEFINED ICEPLL_PATH)
    find_program(ICEPLL_PATH NAMES icepll REQUIRED)
endif()
message(STATUS "icepll path: ${ICEPLL_PATH}")

if(NOT DEFINED ICETIME_PATH)
    find_program(ICETIME_PATH NAMES icetime REQUIRED)
endif()
message(STATUS "icetime path: ${ICETIME_PATH}")

add_custom_target(
    icestick-synth
    DEPENDS icestick.bin
)

add_custom_target(
    icestick-program
    COMMAND ${ICEPROG_PATH} icestick.bin
    DEPENDS icestick.bin
)

add_custom_command(
    OUTPUT icestick.json
    COMMAND ${YOSYS_PATH} -q -p "synth_ice40 -device hx -top top -json icestick.json" ${VERILOG_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/top.sv pll.v
    DEPENDS ${VERILOG_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/top.sv pll.v
)

add_custom_command(
    OUTPUT pll.v
    COMMAND ${ICEPLL_PATH} -q -i 12 -o ${FPGA_TARGET_FREQ} -m -f pll.v
)

add_custom_command(
    OUTPUT icestick.asc
    COMMAND ${NEXTPNR_ICE40_PATH} -q --hx1k --package tq144 --freq ${FPGA_TARGET_FREQ} --asc icestick.asc --json icestick.json --pcf ${FPGA_CONSTRAINTS_FILE}
    DEPENDS ${FPGA_CONSTRAINTS_FILE} icestick.json
)

add_custom_command(
    OUTPUT icestick_timing.txt
    COMMAND ${ICETIME_PATH} -p ${FPGA_CONSTRAINTS_FILE} -r icestick_timing.txt -d hx1k -c ${FPGA_TARGET_FREQ} icestick.asc
    DEPENDS icestick.asc
)

add_custom_command(
    OUTPUT icestick.bin
    COMMAND ${ICEPACK_PATH} icestick.asc icestick.bin
    DEPENDS icestick.asc icestick_timing.txt
)