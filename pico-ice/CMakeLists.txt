find_package(icestorm CONFIG)
find_package(nextpnr CONFIG)
find_package(yosys CONFIG)

if(yosys_FOUND AND icestorm_FOUND AND nextpnr_FOUND)
    set(FPGA_CONSTRAINTS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/pico-ice.pcf)
    set(FPGA_TARGET_FREQ 25)

    add_custom_target(
        pico-ice-synth
        DEPENDS pico-ice.bin
    )

    add_custom_target(
        pico-ice-program
        COMMAND ${ICEPROG_PATH} pico-ice.bin
        DEPENDS pico-ice.bin
    )

    add_custom_command(
        OUTPUT pico-ice.json
        COMMAND ${YOSYS_PATH} -q -p "synth_ice40 -device u -top top -spram -dsp -json pico-ice.json" -l pico-ice.log ${VERILOG_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/top.sv pico-ice-pll.v
        DEPENDS ${VERILOG_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/top.sv pico-ice-pll.v
    )

    add_custom_command(
        OUTPUT pico-ice-pll.v
        COMMAND ${ICEPLL_PATH} -q -i 12 -o ${FPGA_TARGET_FREQ} -p -m -f pico-ice-pll.v
    )

    add_custom_command(
        OUTPUT pico-ice.asc
        COMMAND ${NEXTPNR_ICE40_PATH} -q --up5k --package sg48 --freq ${FPGA_TARGET_FREQ} --asc pico-ice.asc --json pico-ice.json --pcf ${FPGA_CONSTRAINTS_FILE}
        DEPENDS ${FPGA_CONSTRAINTS_FILE} pico-ice.json
    )

    add_custom_command(
        OUTPUT pico-ice_timing.txt
        COMMAND ${ICETIME_PATH} -p ${FPGA_CONSTRAINTS_FILE} -r pico-ice_timing.txt -d up5k -c ${FPGA_TARGET_FREQ} pico-ice.asc
        DEPENDS pico-ice.asc
    )

    add_custom_command(
        OUTPUT pico-ice.bin
        COMMAND ${ICEPACK_PATH} pico-ice.asc pico-ice.bin
        DEPENDS pico-ice.asc pico-ice_timing.txt
    )
endif()