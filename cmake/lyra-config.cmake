

include(FetchContent)

FetchContent_Declare(
    lyra
    GIT_REPOSITORY https://github.com/bfgroup/Lyra.git
    GIT_TAG        1.6.1
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(lyra)
if(NOT lyra_POPULATED)
    FetchContent_Populate(lyra)
    add_library(lyra INTERFACE)
    target_include_directories(lyra INTERFACE ${lyra_SOURCE_DIR}/include)
endif()

set(lyra_FOUND TRUE)