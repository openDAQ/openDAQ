set(MODULE_NAME native_streaming_protocol)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_types.cpp
                 test_base.h
                 test_config_packets.cpp
                 test_streaming_protocol.cpp
                 test_client_to_dev_streaming.cpp
)

add_executable(${TEST_APP} test_app.cpp
                           ${TEST_SOURCES}
)

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

target_link_libraries(${TEST_APP} PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          daq::config_protocol
                                          daq::test_utils
					  daq::opendaq
					  daq::opendaq_mocks
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}coverage ${TEST_APP} ${MODULE_NAME}coverage)
endif()
