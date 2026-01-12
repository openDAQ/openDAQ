function(opendaq_forward_include_headers LIB_NAME MODULES)
    foreach(MODULE_NAME ${${MODULES}})
        target_include_directories(
            ${LIB_NAME}
            INTERFACE $<TARGET_PROPERTY:${MODULE_NAME},INTERFACE_INCLUDE_DIRECTORIES>
        )
    endforeach()
endfunction()

function(opendaq_prepare_test_runner TEST_TARGET)
    set(options "")
    set(oneValueArgs FOR)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(RUNNER "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT DEFINED RUNNER_FOR)
        message(FATAL_ERROR "opendaq_prepare_test_runner() requires the module under test to be specified with FOR.")
    endif()

    if (NOT DEFINED RUNNER_SOURCES)
        message(FATAL_ERROR "No SOURCES given to opendaq_prepare_test_runner().")
    endif()

    set(TEST_RUNNER test_${RUNNER_FOR})
    add_executable(${TEST_RUNNER} ${RUNNER_SOURCES})

    set_target_properties(${TEST_RUNNER} PROPERTIES DEBUG_POSTFIX _debug)

    target_link_libraries(${TEST_RUNNER} PRIVATE ${MAIN_TARGET}
                                                 ${SDK_TARGET_NAMESPACE}::test_utils
    )

    if (MSVC)
        target_compile_options(${TEST_RUNNER} PRIVATE /wd4100)
    endif()

    set(${TEST_TARGET} ${TEST_RUNNER} PARENT_SCOPE)
endfunction()
