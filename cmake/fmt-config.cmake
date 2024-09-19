include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG        10.1.1
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
    FetchContent_Populate(fmt)
    set(FMT_INSTALL OFF CACHE BOOL "Disable install target for {fmt}")
    add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

set(fmt_FOUND TRUE)