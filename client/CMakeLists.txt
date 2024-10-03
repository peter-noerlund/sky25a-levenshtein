option(SPI_BUS_DEBUG "Debug SPI BUS" OFF)

find_package(asio CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(ICU REQUIRED COMPONENTS i18n uc data)
find_package(lyra CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(verilator REQUIRED)

pkg_check_modules(libftdi REQUIRED IMPORTED_TARGET libftdi)

add_executable(client
    basic_bus.cpp
    client.cpp
    levenshtein.cpp
    main.cpp
    icestick_spi.cpp
    real_context.cpp
    runner.cpp
    spi_bus.cpp
    test_set.cpp
    unicode.cpp
    verilator_context.cpp
    verilator_spi.cpp
)
target_include_directories(client PRIVATE client)
target_compile_features(client PRIVATE cxx_std_20)
target_compile_options(client PRIVATE -Wall -W -Wextra -Wno-unused-parameter)
target_link_libraries(client PRIVATE
    asio::asio
    fmt::fmt-header-only
    ICU::i18n
    ICU::uc
    ICU::data
    lyra
    PkgConfig::libftdi
)
target_link_options(client PRIVATE $<$<CONFIG:Release>:-flto>)
if(SPI_BUS_DEBUG)
    target_compile_definitions(client PRIVATE -DSPI_BUS_DEBUG)
endif()

verilate(client
    TOP_MODULE top
    TRACE
    OPT_FAST "-O3 -march=native -flto"
    VERILATOR_ARGS -Wall -I${CMAKE_CURRENT_SOURCE_DIR}/../src
    SOURCES
    top.v
    ../test/qspi_sram.sv
    ${VERILOG_SOURCES}
)