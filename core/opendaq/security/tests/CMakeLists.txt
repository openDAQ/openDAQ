opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME security)

set(TEST_SOURCES test_credential_field.cpp
                 test_authentication_method.cpp
                 test_authentication_config.cpp
                 test_credential_request.cpp
                 test_login_parameters.cpp
                 test_cmd_line_credential_provider.cpp
)

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_SOURCES}
)

target_link_libraries(${TEST_APP} PRIVATE daq::coreobjects
                                          daq::opendaq
                                          daq::opendaq_mocks
                                          daq::opendaq_gmocks
)

opendaq_target_pch_group(${TEST_APP} opendaq_tests <opendaq/opendaq.h> <gtest/gtest.h>)
opendaq_target_unity_tests(${TEST_APP} BATCH_SIZE 4)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (MSVC)
    target_compile_options(${TEST_APP} PRIVATE /bigobj)
endif()

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
