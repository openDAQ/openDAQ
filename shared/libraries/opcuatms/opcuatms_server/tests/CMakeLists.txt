set(MODULE_NAME opcuatms_server)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_helpers.h
                 test_tms_device.cpp
                 test_tms_signal.cpp
                 test_tms_function_block.cpp
                 test_tms_channel.cpp
                 test_tms_input_port.cpp
                 test_tms_property_object.cpp
                 test_tms_server.cpp
                 tms_server_test.h
                 tms_server_test.cpp
)

add_executable(${TEST_APP} testapp.cpp
                           ${TEST_SOURCES}
)

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

target_link_libraries(${TEST_APP} PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          daq::opcuatms_test_utils
                                          daq::opcuaclient
                                          daq::opendaq_mocks
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE:${TEST_APP}>
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(opcuatmsservercoverage ${TEST_APP} opcuatmsservercoverage)
endif()
