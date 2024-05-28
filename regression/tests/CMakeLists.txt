set(TEST_APP regression_tests)

set(TEST_SOURCES setup_regression.h
                 test_app.cpp
                 regression_test_device.cpp
                 regression_test_signal.cpp
                 regression_test_fb.cpp
                 regression_test_component.cpp
                 regression_test_input_port.cpp
                 regression_test_folder.cpp
                 regression_test_property.cpp
                 regression_test_channel.cpp
)

add_executable(${TEST_APP} ${TEST_SOURCES})

target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          daq::opendaq
                                          daq::opendaq_mocks
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)
