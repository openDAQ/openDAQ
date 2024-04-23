set(TEST_APP regression_tests)

set(TEST_SOURCES device.cpp
                 signal.cpp
                 test_app.cpp
)

add_executable(${TEST_APP} ${TEST_SOURCES})

target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          daq::opendaq
)
