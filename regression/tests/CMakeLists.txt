set(TEST_APP regression_tests)

set(TEST_SOURCES setup_regression.h
                 test_app.cpp
                 reg_p_test_device.cpp
                 reg_p_test_signal.cpp
                 reg_p_test_fb.cpp
                 reg_p_test_component.cpp
                 reg_p_test_input_port.cpp
                 reg_p_test_folder.cpp
                 reg_p_test_property.cpp
                 reg_p_test_channel.cpp
)

add_executable(${TEST_APP} ${TEST_SOURCES})

target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          daq::opendaq
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)
