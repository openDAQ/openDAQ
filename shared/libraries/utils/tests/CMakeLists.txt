set(MODULE_NAME opendaq_utils)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_finally.cpp
                 test_function_thread.cpp
                 test_thread_ex.cpp
                 test_timer_thread.cpp
)

add_executable(${TEST_APP} test_app.cpp
                           ${TEST_SOURCES}
)

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

target_link_libraries(${TEST_APP} PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          daq::test_utils
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME} ${TEST_APP} ${MODULE_NAME}coverage)
endif()
