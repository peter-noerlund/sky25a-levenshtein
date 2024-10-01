if(NOT yosys_FOUND)
    set(yosys_FOUND OFF)

    find_program(YOSYS_PATH NAMES yosys HINTS ${YOSYS_PATH})
    if(NOT ${YOSYS_PATH} STREQUAL "YOSYS_PATH-NOTFOUND")
        message(STATUS "yosys path: ${YOSYS_PATH}")

        if(NOT DEFINED YOSYS_DATA_DIR)
            find_program(YOSYS_CONFIG_PATH NAME yosys-config HINTS ${YOSYS_CONFIG_PATH})
            if(NOT ${YOSYS_CONFIG_PATH} STREQUAL "YOSYS_CONFIG_PATH-NOTFOUND")
                message(STATUS "yosys-config path: ${YOSYS_CONFIG_PATH}")

                execute_process(
                    COMMAND ${YOSYS_CONFIG_PATH} --datdir
                    OUTPUT_VARIABLE YOSYS_DATA_DIR
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    RESULT_VARIABLE YOSYS_CONFIG_RETVAL
                )
                if (${YOSYS_CONFIG_RETVAL} EQUAL 0)
                    message(STATUS "yosys data directory: ${YOSYS_DATA_DIR}")
                    set(yosys_FOUND ON)
                endif()
            endif()
        else()
            message(STATUS "yosys data directory: ${YOSYS_DATA_DIR}")
            set(yosys_FOUND ON)
        endif()
    endif()
endif()