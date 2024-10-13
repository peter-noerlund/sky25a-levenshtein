find_package(icestorm CONFIG)
find_package(nextpnr CONFIG)
find_package(yosys CONFIG)

if(yosys_FOUND AND icestorm_FOUND AND nextpnr_FOUND)
    set(FPGA_CONSTRAINTS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/icestick.pcf)
    set(FPGA_TARGET_FREQ 50)

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
        COMMAND ${YOSYS_PATH} -q -p "synth_ice40 -device hx -top top -json icestick.json" -l icestick.log ${VERILOG_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/top.sv icestick-pll.v
        DEPENDS ${VERILOG_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/top.sv icestick-pll.v
    )

    add_custom_command(
        OUTPUT icestick-pll.v
        COMMAND ${ICEPLL_PATH} -q -i 12 -o ${FPGA_TARGET_FREQ} -m -f icestick-pll.v
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
endif()