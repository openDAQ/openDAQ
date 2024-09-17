set(OPENDDS_ENABLE_TESTS OFF)

FetchContent_Declare(
    OpenDDS
    GIT_REPOSITORY https://github.com/OpenDDS/OpenDDS.git
    GIT_TAG        53414662d60f5e222aad9ca4383da0d856a577df  # Specific commit
    GIT_PROGRESS   ON
)

FetchContent_MakeAvailable(OpenDDS)

# Run the OpenDDS configure and setenv scripts (TBD: Find a better way to do this)
if (UNIX)
    if (EXISTS "${opendds_SOURCE_DIR}/configure")
        message(STATUS "Running OpenDDS configure script...")

        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env sh ${opendds_SOURCE_DIR}/configure
            WORKING_DIRECTORY ${opendds_SOURCE_DIR}
            RESULT_VARIABLE CONFIGURE_RESULT
            OUTPUT_VARIABLE CONFIGURE_OUTPUT
            ERROR_VARIABLE CONFIGURE_ERROR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if (CONFIGURE_RESULT)
            message(FATAL_ERROR "OpenDDS configure failed with error: ${CONFIGURE_ERROR}")
        else()
            message(STATUS "OpenDDS configure completed successfully.")
        endif()

    else()
        message(FATAL_ERROR "OpenDDS configure script not found!")
    endif()
endif()

if (EXISTS "${CMAKE_BINARY_DIR}/_deps/opendds-src/setenv.sh")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env sh ${CMAKE_BINARY_DIR}/_deps/opendds-src/setenv.sh
        OUTPUT_VARIABLE OPENDDS_ENV_SETUP_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    message(FATAL_ERROR "setenv.sh script not found. Make sure OpenDDS is properly configured.")
endif()
