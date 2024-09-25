include(FetchContent)

find_package(Threads REQUIRED)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio
    GIT_TAG        asio-1-30-2
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    FetchContent_Populate(asio)
    add_library(asio::asio INTERFACE IMPORTED)
    target_compile_definitions(asio::asio INTERFACE ASIO_HAS_CO_AWAIT)
    target_include_directories(asio::asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
    target_link_libraries(asio::asio INTERFACE Threads::Threads)
endif()

set(asio_FOUND TRUE)