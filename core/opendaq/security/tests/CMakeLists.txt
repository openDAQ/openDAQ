set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME security)

set(TEST_SOURCES test_user.cpp
                 test_authentication_provider.cpp
                 test_permission_manager.cpp
                 test_permission_config_builder.cpp
                 test_access_control.cpp
)

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_SOURCES}
)

target_link_libraries(${TEST_APP} PRIVATE daq::coreobjects
                                          daq::opendaq_mocks
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
