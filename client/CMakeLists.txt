find_package(asio CONFIG REQUIRED)
find_package(verilator REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(lyra CONFIG REQUIRED)

add_executable(client
    client.cpp
    levenshtein.cpp
    main.cpp
    real_context.cpp
    real_uart.cpp
    runner.cpp
    test_set.cpp
    uart_bus.cpp
    verilator_context.cpp
    verilator_spi_bus.cpp
    verilator_uart.cpp
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
    VERILATOR_ARGS -Wall -I${CMAKE_CURRENT_SOURCE_DIR}/../src
    SOURCES
    top.v
    ../test/qspi_sram.sv
    ${VERILOG_SOURCES}
)