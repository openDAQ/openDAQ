# Workaround script mode inability to pass-in lists
string(REPLACE "|" ";" PATCH_FILES ${PATCHES})

set(DO_PATCH OFF)
set(PATCH_FILE ${CMAKE_SOURCE_DIR}/.patched)

if (EXISTS "${PATCH_FILE}")
    file(STRINGS .patched PATCHED_VERSION)
    if (NOT PATCHED_VERSION STREQUAL REQUIRED_VERSION)
        set(DO_PATCH ON)

        message(WARNING "Already patched previous version: ${PATCHED_VERSION}. Cleaning up.")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} clean -d  -fx
            OUTPUT_VARIABLE CONSOLE_OUTPUT
            ERROR_VARIABLE CONSOLE_OUTPUT
        )

        message(STATUS "${CONSOLE_OUTPUT}")
    endif()
else()
    set(DO_PATCH ON)
endif()

if (NOT DO_PATCH)
    message(STATUS "Already patched!")
    return()
endif()

# execute_process(
    # COMMAND ${GIT_EXECUTABLE} status
    # OUTPUT_VARIABLE CONSOLE_OUTPUT
    # ERROR_VARIABLE CONSOLE_OUTPUT
# )
# message(STATUS "STATUS: ${CONSOLE_OUTPUT}")

execute_process(
    COMMAND ${GIT_EXECUTABLE} apply --ignore-whitespace --whitespace=fix
                                    ${PATCH_FILES}
    COMMAND_ECHO STDOUT
    OUTPUT_VARIABLE CONSOLE_OUTPUT
    ERROR_VARIABLE CONSOLE_OUTPUT
    RESULT_VARIABLE PATCHING_ERROR
)

if (PATCHING_ERROR)
    # reset working changes
    execute_process(
        COMMAND ${GIT_EXECUTABLE} reset --hard --quiet
        COMMAND_ECHO STDOUT
        OUTPUT_VARIABLE RESET_OUTPUT
        ERROR_VARIABLE RESET_ERROR
        RESULT_VARIABLE RESET_ERROR
    )

    message(FATAL_ERROR "Failed to patch files with error ${PATCHING_ERROR}: ${CONSOLE_OUTPUT}")
else()
    file(WRITE ${PATCH_FILE} "${REQUIRED_VERSION}")
endif()
