set(TEST_APP regression_tests)

set(TEST_SOURCES test_app.cpp
                 regression_test_device.cpp
                 regression_test_signal.cpp
                 
)

add_executable(${TEST_APP} ${TEST_SOURCES})

target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          daq::opendaq
                                          daq::opendaq_mocks
)
